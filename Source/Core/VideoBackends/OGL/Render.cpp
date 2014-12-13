// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <SOIL/SOIL.h>

#include "Common/Atomic.h"
#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Common/Profiler.h"
#include "Common/StringUtil.h"
#include "Common/Thread.h"
#include "Common/Timer.h"

#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/Movie.h"

#include "VideoBackends/OGL/BoundingBox.h"
#include "VideoBackends/OGL/FramebufferManager.h"
#include "VideoBackends/OGL/GLInterfaceBase.h"
#include "VideoBackends/OGL/GLUtil.h"
#include "VideoBackends/OGL/main.h"
#include "VideoBackends/OGL/PostProcessing.h"
#include "VideoBackends/OGL/ProgramShaderCache.h"
#include "VideoBackends/OGL/RasterFont.h"
#include "VideoBackends/OGL/Render.h"
#include "VideoBackends/OGL/SamplerCache.h"
#include "VideoBackends/OGL/StreamBuffer.h"
#include "VideoBackends/OGL/TextureCache.h"
#include "VideoBackends/OGL/TextureConverter.h"
#include "VideoBackends/OGL/VertexManager.h"

#include "VideoCommon/BPFunctions.h"
#include "VideoCommon/BPStructs.h"
#include "VideoCommon/DriverDetails.h"
#include "VideoCommon/Fifo.h"
#include "VideoCommon/FPSCounter.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/OpcodeDecoding.h"
#include "VideoCommon/PixelEngine.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VertexLoader.h"
#include "VideoCommon/VertexLoaderManager.h"
#include "VideoCommon/VertexShaderGen.h"
#include "VideoCommon/VertexShaderManager.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/VR.h"

#if defined _WIN32 || defined HAVE_LIBAV
#include "VideoCommon/AVIDump.h"
#endif

static bool g_first_rift_frame = true;
static GLsync eyesFence = 0;

void VideoConfig::UpdateProjectionHack()
{
	::UpdateProjectionHack(g_Config.iPhackvalue, g_Config.sPhackvalue);
}

static int OSDInternalW, OSDInternalH;

namespace OGL
{

enum MultisampleMode
{
	MULTISAMPLE_OFF,
	MULTISAMPLE_2X,
	MULTISAMPLE_4X,
	MULTISAMPLE_8X,
	MULTISAMPLE_SSAA_4X,
};


VideoConfig g_ogl_config;

// Declarations and definitions
// ----------------------------
static GLuint s_ShowEFBCopyRegions_VBO = 0;
static GLuint s_ShowEFBCopyRegions_VAO = 0;
static SHADER s_ShowEFBCopyRegions;

static GLuint g_man_texture = 0;

static RasterFont* s_pfont = nullptr;

// 1 for no MSAA. Use s_MSAASamples > 1 to check for MSAA.
static int s_MSAASamples = 1;
static int s_LastMultisampleMode = 0;

static bool s_LastStereo = false;

static u32 s_blendMode;

static bool s_vsync;

// EFB cache related
static const u32 EFB_CACHE_RECT_SIZE = 64; // Cache 64x64 blocks.
static const u32 EFB_CACHE_WIDTH = (EFB_WIDTH + EFB_CACHE_RECT_SIZE - 1) / EFB_CACHE_RECT_SIZE; // round up
static const u32 EFB_CACHE_HEIGHT = (EFB_HEIGHT + EFB_CACHE_RECT_SIZE - 1) / EFB_CACHE_RECT_SIZE;
static bool s_efbCacheValid[2][EFB_CACHE_WIDTH * EFB_CACHE_HEIGHT];
static bool s_efbCacheIsCleared = false;
static std::vector<u32> s_efbCache[2][EFB_CACHE_WIDTH * EFB_CACHE_HEIGHT]; // 2 for PEEK_Z and PEEK_COLOR

static int GetNumMSAASamples(int MSAAMode)
{
	int samples;
	switch (MSAAMode)
	{
		case MULTISAMPLE_OFF:
			samples = 1;
			break;

		case MULTISAMPLE_2X:
			samples = 2;
			break;

		case MULTISAMPLE_4X:
		case MULTISAMPLE_SSAA_4X:
			samples = 4;
			break;

		case MULTISAMPLE_8X:
			samples = 8;
			break;

		default:
			samples = 1;
	}

	if (samples <= g_ogl_config.max_samples)
		return samples;

	// TODO: move this to InitBackendInfo
	OSD::AddMessage(StringFromFormat("%d Anti Aliasing samples selected, but only %d supported by your GPU.", samples, g_ogl_config.max_samples), 10000);
	return g_ogl_config.max_samples;
}

static void ApplySSAASettings()
{
	// GLES3 doesn't support SSAA
	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
	{
		if (g_ActiveConfig.iMultisampleMode == MULTISAMPLE_SSAA_4X)
		{
			if (g_ogl_config.bSupportSampleShading)
			{
				glEnable(GL_SAMPLE_SHADING_ARB);
				GLfloat min_sample_shading_value = static_cast<GLfloat>(s_MSAASamples);
				glMinSampleShadingARB(min_sample_shading_value);
			}
			else
			{
				// TODO: move this to InitBackendInfo
				OSD::AddMessage("SSAA Anti Aliasing isn't supported by your GPU.", 10000);
			}
		}
		else if (g_ogl_config.bSupportSampleShading)
		{
			glDisable(GL_SAMPLE_SHADING_ARB);
		}
	}
}

static void GLAPIENTRY ErrorCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	const char *s_source;
	const char *s_type;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API_ARB:             s_source = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   s_source = "Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: s_source = "Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     s_source = "Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB:     s_source = "Application"; break;
		case GL_DEBUG_SOURCE_OTHER_ARB:           s_source = "Other"; break;
		default:                                  s_source = "Unknown"; break;
	}
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR_ARB:               s_type = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: s_type = "Deprecated"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  s_type = "Undefined"; break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:         s_type = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:         s_type = "Performance"; break;
		case GL_DEBUG_TYPE_OTHER_ARB:               s_type = "Other"; break;
		default:                                    s_type = "Unknown"; break;
	}
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB:   ERROR_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB: WARN_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
		case GL_DEBUG_SEVERITY_LOW_ARB:    WARN_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
		default:                           ERROR_LOG(VIDEO, "id: %x, source: %s, type: %s - %s", id, s_source, s_type, message); break;
	}
}

// Two small Fallbacks to avoid GL_ARB_ES2_compatibility
static void GLAPIENTRY DepthRangef(GLfloat neardepth, GLfloat fardepth)
{
	glDepthRange(neardepth, fardepth);
}
static void GLAPIENTRY ClearDepthf(GLfloat depthval)
{
	glClearDepth(depthval);
}

static void InitDriverInfo()
{
	std::string svendor = std::string(g_ogl_config.gl_vendor);
	std::string srenderer = std::string(g_ogl_config.gl_renderer);
	std::string sversion = std::string(g_ogl_config.gl_version);
	DriverDetails::Vendor vendor = DriverDetails::VENDOR_UNKNOWN;
	DriverDetails::Driver driver = DriverDetails::DRIVER_UNKNOWN;
	double version = 0.0;
	u32 family = 0;

	// Get the vendor first
	if (svendor == "NVIDIA Corporation" && srenderer != "NVIDIA Tegra")
	{
		vendor = DriverDetails::VENDOR_NVIDIA;
	}
	else if (svendor == "ATI Technologies Inc." || svendor == "Advanced Micro Devices, Inc.")
	{
		vendor = DriverDetails::VENDOR_ATI;
	}
	else if (std::string::npos != sversion.find("Mesa"))
	{
		vendor = DriverDetails::VENDOR_MESA;
	}
	else if (std::string::npos != svendor.find("Intel"))
	{
		vendor = DriverDetails::VENDOR_INTEL;
	}
	else if (svendor == "ARM")
	{
		vendor = DriverDetails::VENDOR_ARM;
	}
	else if (svendor == "http://limadriver.org/")
	{
		vendor = DriverDetails::VENDOR_ARM;
		driver = DriverDetails::DRIVER_LIMA;
	}
	else if (svendor == "Qualcomm")
	{
		vendor = DriverDetails::VENDOR_QUALCOMM;
	}
	else if (svendor == "Imagination Technologies")
	{
		vendor = DriverDetails::VENDOR_IMGTEC;
	}
	else if (svendor == "NVIDIA Corporation" && srenderer == "NVIDIA Tegra")
	{
		vendor = DriverDetails::VENDOR_TEGRA;
	}
	else if (svendor == "Vivante Corporation")
	{
		vendor = DriverDetails::VENDOR_VIVANTE;
	}

	// Get device family and driver version...if we care about it
	switch (vendor)
	{
		case DriverDetails::VENDOR_QUALCOMM:
		{
			if (std::string::npos != srenderer.find("Adreno (TM) 3"))
				driver = DriverDetails::DRIVER_QUALCOMM_3XX;
			else
				driver = DriverDetails::DRIVER_QUALCOMM_2XX;
			double glVersion;
			sscanf(g_ogl_config.gl_version, "OpenGL ES %lg V@%lg", &glVersion, &version);
		}
		break;
		case DriverDetails::VENDOR_ARM:
			// Currently the Mali-T line has two families in it.
			// Mali-T6xx and Mali-T7xx
			// These two families are similar enough that they share bugs in their drivers.
			if (std::string::npos != srenderer.find("Mali-T"))
			{
				driver = DriverDetails::DRIVER_ARM_MIDGARD;
				// Mali drivers provide no way to explicitly find out what video driver is running.
				// This is similar to how we can't find the Nvidia driver version in Windows.
				// Good thing is that ARM introduces a new video driver about once every two years so we can
				// find the driver version by the features it exposes.
				// r2p0 - No OpenGL ES 3.0 support (We don't support this)
				// r3p0 - OpenGL ES 3.0 support
				// r4p0 - Supports 'GL_EXT_shader_pixel_local_storage' extension.

				if (GLExtensions::Supports("GL_EXT_shader_pixel_local_storage"))
					version = 400;
				else
					version = 300;
			}
			else if (std::string::npos != srenderer.find("Mali-4") ||
			         std::string::npos != srenderer.find("Mali-3") ||
			         std::string::npos != srenderer.find("Mali-2"))
			{
				driver = DriverDetails::DRIVER_ARM_UTGARD;
			}
		break;
		case DriverDetails::VENDOR_MESA:
		{
			if (svendor == "nouveau")
				driver = DriverDetails::DRIVER_NOUVEAU;
			else if (svendor == "Intel Open Source Technology Center")
				driver = DriverDetails::DRIVER_I965;
			else if (std::string::npos != srenderer.find("AMD") || std::string::npos != srenderer.find("ATI"))
				driver = DriverDetails::DRIVER_R600;

			int major = 0;
			int minor = 0;
			int release = 0;
			sscanf(g_ogl_config.gl_version, "%*s Mesa %d.%d.%d", &major, &minor, &release);
			version = 100*major + 10*minor + release;
		}
		break;
		case DriverDetails::VENDOR_INTEL: // Happens in OS X/Windows
		{
			sscanf(g_ogl_config.gl_renderer, "Intel HD Graphics %d", &family);
#ifdef _WIN32
			int glmajor = 0;
			int glminor = 0;
			int major = 0;
			int minor = 0;
			int release = 0;
			int revision = 0;
			// Example version string: '4.3.0 - Build 10.18.10.3907'
			sscanf(g_ogl_config.gl_version, "%d.%d.0 - Build %d.%d.%d.%d", &glmajor, &glminor, &major, &minor, &release, &revision);
			version = 100000000 * major + 1000000 * minor + 10000 * release + revision;
			version /= 10000;
#endif
		}
		break;
		case DriverDetails::VENDOR_NVIDIA:
		{
			int glmajor = 0;
			int glminor = 0;
			int glrelease = 0;
			int major = 0;
			int minor = 0;
			// TODO: this is known to be broken on windows
			// nvidia seems to have removed their driver version from this string, so we can't get it.
			// hopefully we'll never have to workaround nvidia bugs
			sscanf(g_ogl_config.gl_version, "%d.%d.%d NVIDIA %d.%d", &glmajor, &glminor, &glrelease, &major, &minor);
			version = 100*major + minor;
		}
		break;
		// We don't care about these
		default:
		break;
	}
	DriverDetails::Init(vendor, driver, version, family);
}

// Init functions
Renderer::Renderer()
{
	g_first_rift_frame = true;
	OSDInternalW = 0;
	OSDInternalH = 0;

	s_ShowEFBCopyRegions_VBO = 0;
	s_blendMode = 0;

	bool bSuccess = true;

	// Init extension support.
	if (!GLExtensions::Init())
	{
		// OpenGL 2.0 is required for all shader based drawings. There is no way to get this by extensions
		PanicAlert("GPU: OGL ERROR: Does your video card support OpenGL 2.0?");
		bSuccess = false;
	}

	g_ogl_config.gl_vendor = (const char*)glGetString(GL_VENDOR);
	g_ogl_config.gl_renderer = (const char*)glGetString(GL_RENDERER);
	g_ogl_config.gl_version = (const char*)glGetString(GL_VERSION);
	g_ogl_config.glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	InitDriverInfo();

	// check for the max vertex attributes
	GLint numvertexattribs = 0;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &numvertexattribs);
	if (numvertexattribs < 16)
	{
		PanicAlert("GPU: OGL ERROR: Number of attributes %d not enough.\n"
				"GPU: Does your video card support OpenGL 2.x?",
				numvertexattribs);
		bSuccess = false;
	}

	// check the max texture width and height
	GLint max_texture_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&max_texture_size);
	if (max_texture_size < 1024)
	{
		PanicAlert("GL_MAX_TEXTURE_SIZE too small at %i - must be at least 1024.",
				max_texture_size);
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_framebuffer_object"))
	{
		// We want the ogl3 framebuffer instead of the ogl2 one for better blitting support.
		// It's also compatible with the gles3 one.
		PanicAlert("GPU: ERROR: Need GL_ARB_framebuffer_object for multiple render targets.\n"
				"GPU: Does your video card support OpenGL 3.0?");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_vertex_array_object"))
	{
		// This extension is used to replace lots of pointer setting function.
		// Also gles3 requires to use it.
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_vertex_array_object.\n"
				"GPU: Does your video card support OpenGL 3.0?");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_map_buffer_range"))
	{
		// ogl3 buffer mapping for better streaming support.
		// The ogl2 one also isn't in gles3.
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_map_buffer_range.\n"
				"GPU: Does your video card support OpenGL 3.0?");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_uniform_buffer_object"))
	{
		// ubo allow us to keep the current constants on shader switches
		// we also can stream them much nicer and pack into it whatever we want to
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_uniform_buffer_object.\n"
				"GPU: Does your video card support OpenGL 3.1?");
		bSuccess = false;
	}
	else if (DriverDetails::HasBug(DriverDetails::BUG_BROKENUBO))
	{
		PanicAlert("Buggy GPU driver detected.\n"
				"Please either install the closed-source GPU driver or update your Mesa 3D version.");
		bSuccess = false;
	}

	if (!GLExtensions::Supports("GL_ARB_sampler_objects"))
	{
		// Our sampler cache uses this extension. It could easyly be workaround and it's by far the
		// highest requirement, but it seems that no driver lacks support for it.
		PanicAlert("GPU: OGL ERROR: Need GL_ARB_sampler_objects.\n"
				"GPU: Does your video card support OpenGL 3.3?");
		bSuccess = false;
	}

	if (GLExtensions::Version() < 300)
	{
		// integer vertex attributes require a gl3 only function
		PanicAlert("GPU: OGL ERROR: Need OpenGL version 3.\n"
				"GPU: Does your video card support OpenGL 3?");
		bSuccess = false;
	}

	// OpenGL 3 doesn't provide GLES like float functions for depth.
	// They are in core in OpenGL 4.1, so almost every driver should support them.
	// But for the oldest ones, we provide fallbacks to the old double functions.
	if (!GLExtensions::Supports("GL_ARB_ES2_compatibility") && GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
	{
		glDepthRangef = DepthRangef;
		glClearDepthf = ClearDepthf;
	}

	g_Config.backend_info.bSupportsDualSourceBlend = GLExtensions::Supports("GL_ARB_blend_func_extended");
	g_Config.backend_info.bSupportsPrimitiveRestart = !DriverDetails::HasBug(DriverDetails::BUG_PRIMITIVERESTART) &&
				((GLExtensions::Version() >= 310) || GLExtensions::Supports("GL_NV_primitive_restart"));
	g_Config.backend_info.bSupportsEarlyZ = GLExtensions::Supports("GL_ARB_shader_image_load_store");
	g_Config.backend_info.bSupportsBBox = GLExtensions::Supports("GL_ARB_shader_storage_buffer_object");
	g_Config.backend_info.bSupportsGSInstancing = GLExtensions::Supports("GL_ARB_gpu_shader5");

	// Desktop OpenGL supports the binding layout if it supports 420pack
	// OpenGL ES 3.1 supports it implicitly without an extension
	g_Config.backend_info.bSupportsBindingLayout = GLExtensions::Supports("GL_ARB_shading_language_420pack");

	g_ogl_config.bSupportsGLSLCache = GLExtensions::Supports("GL_ARB_get_program_binary");
	g_ogl_config.bSupportsGLPinnedMemory = GLExtensions::Supports("GL_AMD_pinned_memory");
	g_ogl_config.bSupportsGLSync = GLExtensions::Supports("GL_ARB_sync");
	g_ogl_config.bSupportsGLBaseVertex = GLExtensions::Supports("GL_ARB_draw_elements_base_vertex") ||
	                                     GLExtensions::Supports("GL_EXT_draw_elements_base_vertex");
	g_ogl_config.bSupportsGLBufferStorage = GLExtensions::Supports("GL_ARB_buffer_storage");
	g_ogl_config.bSupportsMSAA = GLExtensions::Supports("GL_ARB_texture_multisample");
	g_ogl_config.bSupportSampleShading = GLExtensions::Supports("GL_ARB_sample_shading");
	g_ogl_config.bSupportOGL31 = GLExtensions::Version() >= 310;
	g_ogl_config.bSupportViewportFloat = GLExtensions::Supports("GL_ARB_viewport_array");

	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGLES3)
	{
		if (strstr(g_ogl_config.glsl_version, "3.0"))
		{
			g_ogl_config.eSupportedGLSLVersion = GLSLES_300;
			g_ogl_config.bSupportsAEP = false;
			g_Config.backend_info.bSupportsStereoscopy = false;
		}
		else
		{
			g_ogl_config.eSupportedGLSLVersion = GLSLES_310;
			g_ogl_config.bSupportsAEP = GLExtensions::Supports("GL_ANDROID_extension_pack_es31a");
			g_Config.backend_info.bSupportsBindingLayout = true;
			g_Config.backend_info.bSupportsEarlyZ = true;
			g_Config.backend_info.bSupportsStereoscopy = g_ogl_config.bSupportsAEP;
		}
	}
	else
	{
		if (strstr(g_ogl_config.glsl_version, "1.00") || strstr(g_ogl_config.glsl_version, "1.10") || strstr(g_ogl_config.glsl_version, "1.20"))
		{
			PanicAlert("GPU: OGL ERROR: Need at least GLSL 1.30\n"
					"GPU: Does your video card support OpenGL 3.0?\n"
					"GPU: Your driver supports GLSL %s", g_ogl_config.glsl_version);
			bSuccess = false;
		}
		else if (strstr(g_ogl_config.glsl_version, "1.30"))
		{
			g_ogl_config.eSupportedGLSLVersion = GLSL_130;
			g_Config.backend_info.bSupportsEarlyZ = false; // layout keyword is only supported on glsl150+
			g_Config.backend_info.bSupportsStereoscopy = false; // geometry shaders are only supported on glsl150+
		}
		else if (strstr(g_ogl_config.glsl_version, "1.40"))
		{
			g_ogl_config.eSupportedGLSLVersion = GLSL_140;
			g_Config.backend_info.bSupportsEarlyZ = false; // layout keyword is only supported on glsl150+
			g_Config.backend_info.bSupportsStereoscopy = false; // geometry shaders are only supported on glsl150+
		}
		else
		{
			g_ogl_config.eSupportedGLSLVersion = GLSL_150;
		}

		// Desktop OpenGL can't have the Android Extension Pack
		g_ogl_config.bSupportsAEP = false;
	}

	if (GLExtensions::Supports("GL_KHR_debug"))
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
		glDebugMessageCallback( ErrorCallback, nullptr );
		glEnable( GL_DEBUG_OUTPUT );
	}
	else if (GLExtensions::Supports("GL_ARB_debug_output"))
	{
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
		glDebugMessageCallbackARB( ErrorCallback, nullptr );
		glEnable( GL_DEBUG_OUTPUT );
	}

	int samples;
	glGetIntegerv(GL_SAMPLES, &samples);
	if (samples > 1)
	{
		// MSAA on default framebuffer isn't working because of glBlitFramebuffer.
		// It also isn't useful as we don't render anything to the default framebuffer.
		// We also try to get a non-msaa fb, so this only happens when forced by the driver.
		PanicAlert("MSAA on default framebuffer isn't supported.\n"
			"Please avoid forcing dolphin to use MSAA by the driver.\n"
			"%d samples on default framebuffer found.", samples);
		bSuccess = false;
	}

	if (g_Config.iStereoMode > 0 && !g_Config.backend_info.bSupportsStereoscopy)
		OSD::AddMessage("Stereoscopic 3D isn't supported by your GPU, support for OpenGL 3.2 is required.", 10000);

	if (!bSuccess)
	{
		// Not all needed extensions are supported, so we have to stop here.
		// Else some of the next calls might crash.
		return;
	}

	glGetIntegerv(GL_MAX_SAMPLES, &g_ogl_config.max_samples);
	if (g_ogl_config.max_samples < 1 || !g_ogl_config.bSupportsMSAA)
		g_ogl_config.max_samples = 1;

	g_Config.VerifyValidity();
	UpdateActiveConfig();

	OSD::AddMessage(StringFromFormat("Video Info: %s, %s, %s",
				g_ogl_config.gl_vendor,
				g_ogl_config.gl_renderer,
				g_ogl_config.gl_version), 5000);

	WARN_LOG(VIDEO,"Missing OGL Extensions: %s%s%s%s%s%s%s%s%s%s%s",
			g_ActiveConfig.backend_info.bSupportsDualSourceBlend ? "" : "DualSourceBlend ",
			g_ActiveConfig.backend_info.bSupportsPrimitiveRestart ? "" : "PrimitiveRestart ",
			g_ActiveConfig.backend_info.bSupportsEarlyZ ? "" : "EarlyZ ",
			g_ogl_config.bSupportsGLPinnedMemory ? "" : "PinnedMemory ",
			g_ogl_config.bSupportsGLSLCache ? "" : "ShaderCache ",
			g_ogl_config.bSupportsGLBaseVertex ? "" : "BaseVertex ",
			g_ogl_config.bSupportsGLBufferStorage ? "" : "BufferStorage ",
			g_ogl_config.bSupportsGLSync ? "" : "Sync ",
			g_ogl_config.bSupportsMSAA ? "" : "MSAA ",
			g_ogl_config.bSupportSampleShading ? "" : "SSAA ",
			g_ActiveConfig.backend_info.bSupportsGSInstancing ? "" : "GSInstancing "
			);

	s_LastMultisampleMode = g_ActiveConfig.iMultisampleMode;
	s_MSAASamples = GetNumMSAASamples(s_LastMultisampleMode);
	ApplySSAASettings();
	s_LastStereo = g_ActiveConfig.iStereoMode > 0;

	// Decide framebuffer size
	s_backbuffer_width = (int)GLInterface->GetBackBufferWidth();
	s_backbuffer_height = (int)GLInterface->GetBackBufferHeight();

	// Handle VSync on/off
	s_vsync = g_ActiveConfig.IsVSync();
	GLInterface->SwapInterval(s_vsync);

	// TODO: Move these somewhere else?
	FramebufferManagerBase::SetLastXfbWidth(MAX_XFB_WIDTH);
	FramebufferManagerBase::SetLastXfbHeight(MAX_XFB_HEIGHT);

	UpdateDrawRectangle(s_backbuffer_width, s_backbuffer_height);

	s_LastEFBScale = g_ActiveConfig.iEFBScale;
	CalculateTargetSize(s_backbuffer_width, s_backbuffer_height);

	// Because of the fixed framebuffer size we need to disable the resolution
	// options while running
	g_Config.bRunning = true;

	glStencilFunc(GL_ALWAYS, 0, 0);
	glBlendFunc(GL_ONE, GL_ONE);

	glViewport(0, 0, GetTargetWidth(), GetTargetHeight()); // Reset The Current Viewport

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepthf(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);  // 4-byte pixel alignment

	glDisable(GL_STENCIL_TEST);
	glEnable(GL_SCISSOR_TEST);

	glScissor(0, 0, GetTargetWidth(), GetTargetHeight());
	glBlendColor(0, 0, 0, 0.5f);
	glClearDepthf(1.0f);

	if (g_ActiveConfig.backend_info.bSupportsPrimitiveRestart)
	{
		if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGLES3)
		{
			glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		}
		else
		{
			if (g_ogl_config.bSupportOGL31)
			{
				glEnable(GL_PRIMITIVE_RESTART);
				glPrimitiveRestartIndex(65535);
			}
			else
			{
				glEnableClientState(GL_PRIMITIVE_RESTART_NV);
				glPrimitiveRestartIndexNV(65535);
			}
		}
	}
	UpdateActiveConfig();
	ClearEFBCache();

	{
		/* load an image file directly as a new OpenGL texture */
		unsigned char* img;
		int width, height, channels;
		/*	try to load the image	*/
		img = SOIL_load_image((File::GetSysDirectory() + "man.png").c_str(), &width, &height, &channels, SOIL_LOAD_AUTO);
		glGenTextures(1, &g_man_texture);
		glActiveTexture(GL_TEXTURE0 + 9);
		glBindTexture(GL_TEXTURE_2D, g_man_texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
		SOIL_free_image_data(img);
	}
}

Renderer::~Renderer()
{
}

void Renderer::Shutdown()
{
	static int ElementArrayBufferBinding, ArrayBufferBinding;
#ifdef HAVE_OCULUSSDK
	if (g_has_rift && !g_first_rift_frame && g_ActiveConfig.bEnableVR && !g_ActiveConfig.bAsynchronousTimewarp)
	{
		//TargetRectangle targetRc = ConvertEFBRectangle(rc);

		// for msaa mode, we must resolve the efb content to non-msaa
		//FramebufferManager::ResolveAndGetRenderTarget(rc, 0);
		//FramebufferManager::ResolveAndGetRenderTarget(rc, 1);

		// Render to the real/postprocessing buffer now. (resolve have changed this in msaa mode)
		//m_post_processor->BindTargetFramebuffer();

		//ovrHmd_EndEyeRender(hmd, ovrEye_Left, g_left_eye_pose, &FramebufferManager::m_eye_texture[ovrEye_Left].Texture);
		//ovrHmd_EndEyeRender(hmd, ovrEye_Right, g_right_eye_pose, &FramebufferManager::m_eye_texture[ovrEye_Right].Texture);

		// Let OVR do distortion rendering, Present and flush/sync.
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ElementArrayBufferBinding);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &ArrayBufferBinding);
		ovrHmd_EndFrame(hmd, g_eye_poses, &FramebufferManager::m_eye_texture[0].Texture);
		Common::AtomicIncrement(g_drawn_vr);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementArrayBufferBinding);
		glBindBuffer(GL_ARRAY_BUFFER, ArrayBufferBinding);
	}
#endif
	g_first_rift_frame = true;

	delete g_framebuffer_manager;

	g_Config.bRunning = false;
	UpdateActiveConfig();

	glDeleteBuffers(1, &s_ShowEFBCopyRegions_VBO);
	glDeleteVertexArrays(1, &s_ShowEFBCopyRegions_VAO);
	s_ShowEFBCopyRegions_VBO = 0;

	delete s_pfont;
	s_pfont = nullptr;
	s_ShowEFBCopyRegions.Destroy();

	delete m_post_processor;
	m_post_processor = nullptr;
}

void Renderer::Init()
{
	// Initialize the FramebufferManager
	g_framebuffer_manager = new FramebufferManager(s_target_width, s_target_height,
			s_MSAASamples);

	m_post_processor = new OpenGLPostProcessing();

	s_pfont = new RasterFont();

	ProgramShaderCache::CompileShader(s_ShowEFBCopyRegions,
		"in vec2 rawpos;\n"
		"in vec3 color0;\n"
		"out vec4 c;\n"
		"void main(void) {\n"
		"	gl_Position = vec4(rawpos, 0.0, 1.0);\n"
		"	c = vec4(color0, 1.0);\n"
		"}\n",
		"in vec4 c;\n"
		"out vec4 ocol0;\n"
		"void main(void) {\n"
		"	ocol0 = c;\n"
		"}\n");

	// creating buffers
	glGenBuffers(1, &s_ShowEFBCopyRegions_VBO);
	glGenVertexArrays(1, &s_ShowEFBCopyRegions_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, s_ShowEFBCopyRegions_VBO);
	glBindVertexArray( s_ShowEFBCopyRegions_VAO );
	glEnableVertexAttribArray(SHADER_POSITION_ATTRIB);
	glVertexAttribPointer(SHADER_POSITION_ATTRIB, 2, GL_FLOAT, 0, sizeof(GLfloat)*5, nullptr);
	glEnableVertexAttribArray(SHADER_COLOR0_ATTRIB);
	glVertexAttribPointer(SHADER_COLOR0_ATTRIB, 3, GL_FLOAT, 0, sizeof(GLfloat)*5, (GLfloat*)nullptr+2);
}

// Create On-Screen-Messages
void Renderer::DrawDebugInfo()
{
	// Reset viewport for drawing text
	glViewport(0, 0, GLInterface->GetBackBufferWidth(), GLInterface->GetBackBufferHeight());

	// Draw various messages on the screen, like FPS, statistics, etc.
	std::string debug_info;

	if (g_ActiveConfig.bShowFPS || SConfig::GetInstance().m_ShowFrameCount)
	{
		std::string fps = "";
		if (g_ActiveConfig.bShowFPS)
			debug_info += StringFromFormat("FPS: %d", m_fps_counter.m_fps);

		if (g_ActiveConfig.bShowFPS && SConfig::GetInstance().m_ShowFrameCount)
			debug_info += " - ";
		if (SConfig::GetInstance().m_ShowFrameCount)
		{
			debug_info += StringFromFormat("Frame: %llu", (unsigned long long) Movie::g_currentFrame);
			if (Movie::IsPlayingInput())
				debug_info += StringFromFormat(" / %llu", (unsigned long long) Movie::g_totalFrames);
		}

		debug_info += "\n";
	}

	if (SConfig::GetInstance().m_ShowLag)
		debug_info += StringFromFormat("Lag: %" PRIu64 "\n", Movie::g_currentLagCount);

	if (SConfig::GetInstance().m_ShowInputDisplay)
		debug_info += Movie::GetInputDisplay();

	debug_info += Profiler::ToString();

	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL && g_ActiveConfig.bShowEFBCopyRegions)
	{
		// Set Line Size
		glLineWidth(3.0f);

		// 2*Coords + 3*Color
		GLsizeiptr length = stats.efb_regions.size() * sizeof(GLfloat) * (2 + 3) * 2 * 6;
		glBindBuffer(GL_ARRAY_BUFFER, s_ShowEFBCopyRegions_VBO);
		glBufferData(GL_ARRAY_BUFFER, length, nullptr, GL_STREAM_DRAW);
		GLfloat *Vertices = (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, length, GL_MAP_WRITE_BIT);

		// Draw EFB copy regions rectangles
		int a = 0;
		GLfloat color[3] = {0.0f, 1.0f, 1.0f};

		for (const EFBRectangle& rect : stats.efb_regions)
		{
			GLfloat halfWidth = EFB_WIDTH / 2.0f;
			GLfloat halfHeight = EFB_HEIGHT / 2.0f;
			GLfloat x =  (GLfloat) -1.0f + ((GLfloat)rect.left / halfWidth);
			GLfloat y =  (GLfloat) 1.0f - ((GLfloat)rect.top / halfHeight);
			GLfloat x2 = (GLfloat) -1.0f + ((GLfloat)rect.right / halfWidth);
			GLfloat y2 = (GLfloat) 1.0f - ((GLfloat)rect.bottom / halfHeight);

			Vertices[a++] = x;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x2;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x2;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x2;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x2;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x2;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];


			Vertices[a++] = x2;
			Vertices[a++] = y;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			Vertices[a++] = x;
			Vertices[a++] = y2;
			Vertices[a++] = color[0];
			Vertices[a++] = color[1];
			Vertices[a++] = color[2];

			// TO DO: build something nicer here
			GLfloat temp = color[0];
			color[0] = color[1];
			color[1] = color[2];
			color[2] = temp;
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);

		s_ShowEFBCopyRegions.Bind();
		glBindVertexArray(s_ShowEFBCopyRegions_VAO);
		GLsizei count = static_cast<GLsizei>(stats.efb_regions.size() * 2*6);
		glDrawArrays(GL_LINES, 0, count);

		// Restore Line Size
		SetLineWidth();

		// Clear stored regions
		stats.efb_regions.clear();
	}

	if (g_ActiveConfig.bOverlayStats)
		debug_info += Statistics::ToString();

	if (g_ActiveConfig.bOverlayProjStats)
		debug_info += Statistics::ToStringProj();

	if (!debug_info.empty())
	{
		// Render a shadow, and then the text.
		Renderer::RenderText(debug_info, 21, 21, 0xDD000000);
		Renderer::RenderText(debug_info, 20, 20, 0xFF00FFFF);
	}
}

void Renderer::RenderText(const std::string& text, int left, int top, u32 color)
{
	const int nBackbufferWidth = (int)GLInterface->GetBackBufferWidth();
	const int nBackbufferHeight = (int)GLInterface->GetBackBufferHeight();

	s_pfont->printMultilineText(text,
		left * 2.0f / (float)nBackbufferWidth - 1,
		1 - top * 2.0f / (float)nBackbufferHeight,
		0, nBackbufferWidth, nBackbufferHeight, color);
}

TargetRectangle Renderer::ConvertEFBRectangle(const EFBRectangle& rc)
{
	TargetRectangle result;
	result.left   = EFBToScaledX(rc.left);
	result.top    = EFBToScaledY(EFB_HEIGHT - rc.top);
	result.right  = EFBToScaledX(rc.right);
	result.bottom = EFBToScaledY(EFB_HEIGHT - rc.bottom);
	return result;
}

// Function: This function handles the OpenGL glScissor() function
// ----------------------------
// Call browser: OpcodeDecoding.cpp ExecuteDisplayList > Decode() > LoadBPReg()
//		case 0x52 > SetScissorRect()
// ----------------------------
// bpmem.scissorTL.x, y = 342x342
// bpmem.scissorBR.x, y = 981x821
// Renderer::GetTargetHeight() = the fixed ini file setting
// donkopunchstania - it appears scissorBR is the bottom right pixel inside the scissor box
// therefore the width and height are (scissorBR + 1) - scissorTL
void Renderer::SetScissorRect(const EFBRectangle& rc)
{
	TargetRectangle trc;
	// In VR we use the whole EFB instead of just the bpmem.copyTexSrc rectangle passed to this function. 
	if (g_has_hmd)
	{
		EFBRectangle sourceRc;
		sourceRc.left = 0;
		sourceRc.right = EFB_WIDTH-1;
		sourceRc.top = 0;
		sourceRc.bottom = EFB_HEIGHT-1;
		trc = g_renderer->ConvertEFBRectangle(sourceRc);
		glScissor(0, 0, GetTargetWidth(), GetTargetHeight());
		glDisable(GL_SCISSOR_TEST);
	}
	else
	{
		trc = g_renderer->ConvertEFBRectangle(rc);
		glScissor(trc.left, trc.bottom, trc.GetWidth(), trc.GetHeight());
	}
}

void Renderer::SetColorMask()
{
	// Only enable alpha channel if it's supported by the current EFB format
	GLenum ColorMask = GL_FALSE, AlphaMask = GL_FALSE;
	if (bpmem.alpha_test.TestResult() != AlphaTest::FAIL)
	{
		if (bpmem.blendmode.colorupdate)
			ColorMask = GL_TRUE;
		if (bpmem.blendmode.alphaupdate && (bpmem.zcontrol.pixel_format == PEControl::RGBA6_Z24))
			AlphaMask = GL_TRUE;
	}
	glColorMask(ColorMask,  ColorMask,  ColorMask,  AlphaMask);
}

void ClearEFBCache()
{
	if (!s_efbCacheIsCleared)
	{
		s_efbCacheIsCleared = true;
		memset(s_efbCacheValid, 0, sizeof(s_efbCacheValid));
	}
}

void Renderer::UpdateEFBCache(EFBAccessType type, u32 cacheRectIdx, const EFBRectangle& efbPixelRc, const TargetRectangle& targetPixelRc, const u32* data)
{
	u32 cacheType = (type == PEEK_Z ? 0 : 1);

	if (!s_efbCache[cacheType][cacheRectIdx].size())
		s_efbCache[cacheType][cacheRectIdx].resize(EFB_CACHE_RECT_SIZE * EFB_CACHE_RECT_SIZE);

	u32 targetPixelRcWidth = targetPixelRc.right - targetPixelRc.left;
	u32 efbPixelRcHeight = efbPixelRc.bottom - efbPixelRc.top;
	u32 efbPixelRcWidth = efbPixelRc.right - efbPixelRc.left;

	for (u32 yCache = 0; yCache < efbPixelRcHeight; ++yCache)
	{
		u32 yEFB = efbPixelRc.top + yCache;
		u32 yPixel = (EFBToScaledY(EFB_HEIGHT - yEFB) + EFBToScaledY(EFB_HEIGHT - yEFB - 1)) / 2;
		u32 yData = yPixel - targetPixelRc.bottom;

		for (u32 xCache = 0; xCache < efbPixelRcWidth; ++xCache)
		{
			u32 xEFB = efbPixelRc.left + xCache;
			u32 xPixel = (EFBToScaledX(xEFB) + EFBToScaledX(xEFB + 1)) / 2;
			u32 xData = xPixel - targetPixelRc.left;
			s_efbCache[cacheType][cacheRectIdx][yCache * EFB_CACHE_RECT_SIZE + xCache] = data[yData * targetPixelRcWidth + xData];
		}
	}

	s_efbCacheValid[cacheType][cacheRectIdx] = true;
	s_efbCacheIsCleared = false;
}

// This function allows the CPU to directly access the EFB.
// There are EFB peeks (which will read the color or depth of a pixel)
// and EFB pokes (which will change the color or depth of a pixel).
//
// The behavior of EFB peeks can only be modified by:
// - GX_PokeAlphaRead
// The behavior of EFB pokes can be modified by:
// - GX_PokeAlphaMode (TODO)
// - GX_PokeAlphaUpdate (TODO)
// - GX_PokeBlendMode (TODO)
// - GX_PokeColorUpdate (TODO)
// - GX_PokeDither (TODO)
// - GX_PokeDstAlpha (TODO)
// - GX_PokeZMode (TODO)
u32 Renderer::AccessEFB(EFBAccessType type, u32 x, u32 y, u32 poke_data)
{
	u32 cacheRectIdx = (y / EFB_CACHE_RECT_SIZE) * EFB_CACHE_WIDTH
	                 + (x / EFB_CACHE_RECT_SIZE);

	// Get the rectangular target region containing the EFB pixel
	EFBRectangle efbPixelRc;
	efbPixelRc.left = (x / EFB_CACHE_RECT_SIZE) * EFB_CACHE_RECT_SIZE;
	efbPixelRc.top = (y / EFB_CACHE_RECT_SIZE) * EFB_CACHE_RECT_SIZE;
	efbPixelRc.right = std::min(efbPixelRc.left + EFB_CACHE_RECT_SIZE, (u32)EFB_WIDTH);
	efbPixelRc.bottom = std::min(efbPixelRc.top + EFB_CACHE_RECT_SIZE, (u32)EFB_HEIGHT);

	TargetRectangle targetPixelRc = ConvertEFBRectangle(efbPixelRc);
	u32 targetPixelRcWidth = targetPixelRc.right - targetPixelRc.left;
	u32 targetPixelRcHeight = targetPixelRc.top - targetPixelRc.bottom;

	// TODO (FIX) : currently, AA path is broken/offset and doesn't return the correct pixel
	switch (type)
	{
	case PEEK_Z:
		{
			u32 z;

			if (!s_efbCacheValid[0][cacheRectIdx])
			{
				if (s_MSAASamples > 1)
				{
					g_renderer->ResetAPIState();

					// Resolve our rectangle.
					FramebufferManager::GetEFBDepthTexture(efbPixelRc);
					glBindFramebuffer(GL_READ_FRAMEBUFFER, FramebufferManager::GetResolvedFramebuffer());

					g_renderer->RestoreAPIState();
				}

				u32* depthMap = new u32[targetPixelRcWidth * targetPixelRcHeight];

				glReadPixels(targetPixelRc.left, targetPixelRc.bottom, targetPixelRcWidth, targetPixelRcHeight,
				             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, depthMap);

				UpdateEFBCache(type, cacheRectIdx, efbPixelRc, targetPixelRc, depthMap);

				delete[] depthMap;
			}

			u32 xRect = x % EFB_CACHE_RECT_SIZE;
			u32 yRect = y % EFB_CACHE_RECT_SIZE;
			z = s_efbCache[0][cacheRectIdx][yRect * EFB_CACHE_RECT_SIZE + xRect];

			// Scale the 32-bit value returned by glReadPixels to a 24-bit
			// value (GC uses a 24-bit Z-buffer).
			// TODO: in RE0 this value is often off by one, which causes lighting to disappear
			if (bpmem.zcontrol.pixel_format == PEControl::RGB565_Z16)
			{
				// if Z is in 16 bit format you must return a 16 bit integer
				z = z >> 16;
			}
			else
			{
				z = z >> 8;
			}
			return z;
		}

	case PEEK_COLOR: // GXPeekARGB
		{
			// Although it may sound strange, this really is A8R8G8B8 and not RGBA or 24-bit...

			// Tested in Killer 7, the first 8bits represent the alpha value which is used to
			// determine if we're aiming at an enemy (0x80 / 0x88) or not (0x70)
			// Wind Waker is also using it for the pictograph to determine the color of each pixel

			u32 color;

			if (!s_efbCacheValid[1][cacheRectIdx])
			{
				if (s_MSAASamples > 1)
				{
					g_renderer->ResetAPIState();

					// Resolve our rectangle.
					FramebufferManager::GetEFBColorTexture(efbPixelRc);
					glBindFramebuffer(GL_READ_FRAMEBUFFER, FramebufferManager::GetResolvedFramebuffer());

					g_renderer->RestoreAPIState();
				}

				u32* colorMap = new u32[targetPixelRcWidth * targetPixelRcHeight];

				if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGLES3)
				// XXX: Swap colours
					glReadPixels(targetPixelRc.left, targetPixelRc.bottom, targetPixelRcWidth, targetPixelRcHeight,
						     GL_RGBA, GL_UNSIGNED_BYTE, colorMap);
				else
					glReadPixels(targetPixelRc.left, targetPixelRc.bottom, targetPixelRcWidth, targetPixelRcHeight,
						     GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, colorMap);

				UpdateEFBCache(type, cacheRectIdx, efbPixelRc, targetPixelRc, colorMap);

				delete[] colorMap;
			}

			u32 xRect = x % EFB_CACHE_RECT_SIZE;
			u32 yRect = y % EFB_CACHE_RECT_SIZE;
			color = s_efbCache[1][cacheRectIdx][yRect * EFB_CACHE_RECT_SIZE + xRect];

			// check what to do with the alpha channel (GX_PokeAlphaRead)
			PixelEngine::UPEAlphaReadReg alpha_read_mode = PixelEngine::GetAlphaReadMode();

			if (bpmem.zcontrol.pixel_format == PEControl::RGBA6_Z24)
			{
				color = RGBA8ToRGBA6ToRGBA8(color);
			}
			else if (bpmem.zcontrol.pixel_format == PEControl::RGB565_Z16)
			{
				color = RGBA8ToRGB565ToRGBA8(color);
			}
			if (bpmem.zcontrol.pixel_format != PEControl::RGBA6_Z24)
			{
				color |= 0xFF000000;
			}
			if (alpha_read_mode.ReadMode == 2)
			{
				// GX_READ_NONE
				return color;
			}
			else if (alpha_read_mode.ReadMode == 1)
			{
				// GX_READ_FF
				return (color | 0xFF000000);
			}
			else /*if(alpha_read_mode.ReadMode == 0)*/
			{
				// GX_READ_00
				return (color & 0x00FFFFFF);
			}
		}

	case POKE_COLOR:
	{
		ResetAPIState();

		glClearColor(float((poke_data >> 16) & 0xFF) / 255.0f,
		             float((poke_data >>  8) & 0xFF) / 255.0f,
		             float((poke_data >>  0) & 0xFF) / 255.0f,
		             float((poke_data >> 24) & 0xFF) / 255.0f);

		glEnable(GL_SCISSOR_TEST);
		glScissor(targetPixelRc.left, targetPixelRc.bottom, targetPixelRc.GetWidth(), targetPixelRc.GetHeight());

		glClear(GL_COLOR_BUFFER_BIT);

		RestoreAPIState();

		// TODO: Could just update the EFB cache with the new value
		ClearEFBCache();

		break;
	}

	case POKE_Z:
	{
		ResetAPIState();

		glDepthMask(GL_TRUE);
		glClearDepthf(float(poke_data & 0xFFFFFF) / float(0xFFFFFF));

		glEnable(GL_SCISSOR_TEST);
		glScissor(targetPixelRc.left, targetPixelRc.bottom, targetPixelRc.GetWidth(), targetPixelRc.GetHeight());

		glClear(GL_DEPTH_BUFFER_BIT);

		RestoreAPIState();

		// TODO: Could just update the EFB cache with the new value
		ClearEFBCache();

		break;
	}

	default:
		break;
	}

	return 0;
}

u16 Renderer::BBoxRead(int index)
{
	int swapped_index = index;
	if (index >= 2)
		swapped_index ^= 1; // swap 2 and 3 for top/bottom

	// Here we get the min/max value of the truncated position of the upscaled and swapped framebuffer.
	// So we have to correct them to the unscaled EFB sizes.
	int value = BoundingBox::Get(swapped_index);

	if (index < 2)
	{
		// left/right
		value = value * EFB_WIDTH / s_target_width;
	}
	else
	{
		// up/down -- we have to swap up and down
		value = value * EFB_HEIGHT / s_target_height;
		value = EFB_HEIGHT - value - 1;
	}
	if (index & 1)
		value++; // fix max values to describe the outer border

	return value;
}

void Renderer::BBoxWrite(int index, u16 _value)
{
	int value = _value; // u16 isn't enough to multiply by the efb width
	if (index & 1)
		value--;
	if (index < 2)
	{
		value = value * s_target_width / EFB_WIDTH;
	}
	else
	{
		index ^= 1; // swap 2 and 3 for top/bottom
		value = EFB_HEIGHT - value - 1;
		value = value * s_target_height / EFB_HEIGHT;
	}

	BoundingBox::Set(index, value);
}

void Renderer::SetViewport()
{
	// reversed gxsetviewport(xorig, yorig, width, height, nearz, farz)
	// [0] = width/2
	// [1] = height/2
	// [2] = 16777215 * (farz - nearz)
	// [3] = xorig + width/2 + 342
	// [4] = yorig + height/2 + 342
	// [5] = 16777215 * farz

	int scissorXOff = bpmem.scissorOffset.x * 2;
	int scissorYOff = bpmem.scissorOffset.y * 2;

	// TODO: ceil, floor or just cast to int?
	float X, Y, Width, Height;
	if (!g_has_hmd)
	{
		X = EFBToScaledXf(xfmem.viewport.xOrig - xfmem.viewport.wd - (float)scissorXOff);
		Y = EFBToScaledYf((float)EFB_HEIGHT - xfmem.viewport.yOrig + xfmem.viewport.ht + (float)scissorYOff);
		Width = EFBToScaledXf(2.0f * xfmem.viewport.wd);
		Height = EFBToScaledYf(-2.0f * xfmem.viewport.ht);
	}
	else
	{
		// In VR we must use the entire EFB, not just the copyTexSrc area that is normally used.
		// So scale from copyTexSrc to entire EFB, and we won't use copyTexSrc during rendering.
		//X = (xfmem.viewport.xOrig - xfmem.viewport.wd - bpmem.copyTexSrcXY.x - (float)scissorXOff) * (float)GetTargetWidth() / (float)bpmem.copyTexSrcWH.x;
		//Y = (float)GetTargetHeight() - (xfmem.viewport.yOrig - xfmem.viewport.ht - bpmem.copyTexSrcXY.y - (float)scissorYOff) * (float)GetTargetHeight() / (float)bpmem.copyTexSrcWH.y;
		//Width = (2.0f * xfmem.viewport.wd) * (float)GetTargetWidth() / (float)bpmem.copyTexSrcWH.x;
		//Height = (-2.0f * xfmem.viewport.ht) * (float)GetTargetHeight() / (float)bpmem.copyTexSrcWH.y;
		X = 0.0f; Y = 0.0f; Width = (float)GetTargetWidth(); Height = (float)GetTargetHeight();
	}

	float GLNear = (xfmem.viewport.farZ - xfmem.viewport.zRange) / 16777216.0f;
	float GLFar = xfmem.viewport.farZ / 16777216.0f;
	if (Width < 0)
	{
		X += Width;
		Width *= -1;
	}
	if (Height < 0)
	{
		Y += Height;
		Height *= -1;
	}

	// Update the view port
	if (g_ogl_config.bSupportViewportFloat)
	{
		glViewportIndexedf(0, X, Y, Width, Height);
		//NOTICE_LOG(VR, "glViewportIndexedf(0,   %f, %f,   %f, %f) TargetSize=%d, %d", X, Y, Width, Height, GetTargetWidth(), GetTargetHeight());
	}
	else
	{
		auto iceilf = [](float f)
		{
			return static_cast<GLint>(ceilf(f));
		};
		glViewport(iceilf(X), iceilf(Y), iceilf(Width), iceilf(Height));
		NOTICE_LOG(VR, "glViewport(%d, %d,   %d, %d)", ceil(X), ceil(Y), ceil(Width), ceil(Height));
	}
	glDepthRangef(GLNear, GLFar);
	//NOTICE_LOG(VR, "gDepthRangef(%f, %f)", GLNear, GLFar);
}

void Renderer::ClearScreen(const EFBRectangle& rc, bool colorEnable, bool alphaEnable, bool zEnable, u32 color, u32 z)
{
	ResetAPIState();

	// color
	GLboolean const
		color_mask = colorEnable ? GL_TRUE : GL_FALSE,
		alpha_mask = alphaEnable ? GL_TRUE : GL_FALSE;
	glColorMask(color_mask, color_mask, color_mask, alpha_mask);

	glClearColor(
		float((color >> 16) & 0xFF) / 255.0f,
		float((color >> 8) & 0xFF) / 255.0f,
		float((color >> 0) & 0xFF) / 255.0f,
		float((color >> 24) & 0xFF) / 255.0f);

	// depth
	glDepthMask(zEnable ? GL_TRUE : GL_FALSE);

	glClearDepthf(float(z & 0xFFFFFF) / float(0xFFFFFF));

	// Update rect for clearing the picture
	// TODO fix this properly by setting the scissor rectangle
	if (g_has_hmd)
		glDisable(GL_SCISSOR_TEST);
	else
		glEnable(GL_SCISSOR_TEST);

	TargetRectangle const targetRc = ConvertEFBRectangle(rc);
	glScissor(targetRc.left, targetRc.bottom, targetRc.GetWidth(), targetRc.GetHeight());

	// glColorMask/glDepthMask/glScissor affect glClear (glViewport does not)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RestoreAPIState();

	ClearEFBCache();
}

void Renderer::SkipClearScreen(bool colorEnable, bool alphaEnable, bool zEnable)
{
	ResetAPIState();

	// color
	GLboolean const
		color_mask = colorEnable ? GL_TRUE : GL_FALSE,
		alpha_mask = alphaEnable ? GL_TRUE : GL_FALSE;
	glColorMask(color_mask, color_mask, color_mask, alpha_mask);

	// depth
	glDepthMask(zEnable ? GL_TRUE : GL_FALSE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RestoreAPIState();

	ClearEFBCache();
}

void Renderer::ReinterpretPixelData(unsigned int convtype)
{
	if (convtype == 0 || convtype == 2)
	{
		// Reinterpretting pixel data crashes OpenGL at the next glFinish
		//	FramebufferManager::ReinterpretPixelData(convtype);
		if (!g_has_hmd)
			FramebufferManager::ReinterpretPixelData(convtype);
	}
	else
	{
		ERROR_LOG(VIDEO, "Trying to reinterpret pixel data with unsupported conversion type %d", convtype);
	}
}

void Renderer::SetBlendMode(bool forceUpdate)
{
	// Our render target always uses an alpha channel, so we need to override the blend functions to assume a destination alpha of 1 if the render target isn't supposed to have an alpha channel
	// Example: D3DBLEND_DESTALPHA needs to be D3DBLEND_ONE since the result without an alpha channel is assumed to always be 1.
	bool target_has_alpha = bpmem.zcontrol.pixel_format == PEControl::RGBA6_Z24;

	bool useDstAlpha = !g_ActiveConfig.bDstAlphaPass && bpmem.dstalpha.enable && bpmem.blendmode.alphaupdate && target_has_alpha;
	bool useDualSource = useDstAlpha && g_ActiveConfig.backend_info.bSupportsDualSourceBlend;

	const GLenum glSrcFactors[8] =
	{
		GL_ZERO,
		GL_ONE,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		(useDualSource)  ? GL_SRC1_ALPHA : (GLenum)GL_SRC_ALPHA,
		(useDualSource)  ? GL_ONE_MINUS_SRC1_ALPHA : (GLenum)GL_ONE_MINUS_SRC_ALPHA,
		(target_has_alpha) ? GL_DST_ALPHA : (GLenum)GL_ONE,
		(target_has_alpha) ? GL_ONE_MINUS_DST_ALPHA : (GLenum)GL_ZERO
	};
	const GLenum glDestFactors[8] =
	{
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		(useDualSource)  ? GL_SRC1_ALPHA : (GLenum)GL_SRC_ALPHA,
		(useDualSource)  ? GL_ONE_MINUS_SRC1_ALPHA : (GLenum)GL_ONE_MINUS_SRC_ALPHA,
		(target_has_alpha) ? GL_DST_ALPHA : (GLenum)GL_ONE,
		(target_has_alpha) ? GL_ONE_MINUS_DST_ALPHA : (GLenum)GL_ZERO
	};

	// blend mode bit mask
	// 0 - blend enable
	// 1 - dst alpha enabled
	// 2 - reverse subtract enable (else add)
	// 3-5 - srcRGB function
	// 6-8 - dstRGB function

	u32 newval = useDualSource << 1;
	newval |= bpmem.blendmode.subtract << 2;

	if (bpmem.blendmode.subtract)
	{
		newval |= 0x0049;   // enable blending src 1 dst 1
	}
	else if (bpmem.blendmode.blendenable)
	{
		newval |= 1;    // enable blending
		newval |= bpmem.blendmode.srcfactor << 3;
		newval |= bpmem.blendmode.dstfactor << 6;
	}

	u32 changes = forceUpdate ? 0xFFFFFFFF : newval ^ s_blendMode;

	if (changes & 1)
	{
		// blend enable change
		(newval & 1) ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
	}

	if (changes & 4)
	{
		// subtract enable change
		GLenum equation = newval & 4 ? GL_FUNC_REVERSE_SUBTRACT : GL_FUNC_ADD;
		GLenum equationAlpha = useDualSource ? GL_FUNC_ADD : equation;

		glBlendEquationSeparate(equation, equationAlpha);
	}

	if (changes & 0x1FA)
	{
		u32 srcidx = (newval >> 3) & 7;
		u32 dstidx = (newval >> 6) & 7;
		GLenum srcFactor = glSrcFactors[srcidx];
		GLenum dstFactor = glDestFactors[dstidx];

		// adjust alpha factors
		if (useDualSource)
		{
			srcidx = BlendMode::ONE;
			dstidx = BlendMode::ZERO;
		}
		else
		{
			// we can't use GL_DST_COLOR or GL_ONE_MINUS_DST_COLOR for source in alpha channel so use their alpha equivalent instead
			if (srcidx == BlendMode::DSTCLR)
				srcidx = BlendMode::DSTALPHA;
			else if (srcidx == BlendMode::INVDSTCLR)
				srcidx = BlendMode::INVDSTALPHA;

			// we can't use GL_SRC_COLOR or GL_ONE_MINUS_SRC_COLOR for destination in alpha channel so use their alpha equivalent instead
			if (dstidx == BlendMode::SRCCLR)
				dstidx = BlendMode::SRCALPHA;
			else if (dstidx == BlendMode::INVSRCCLR)
				dstidx = BlendMode::INVSRCALPHA;
		}
		GLenum srcFactorAlpha = glSrcFactors[srcidx];
		GLenum dstFactorAlpha = glDestFactors[dstidx];
		// blend RGB change
		glBlendFuncSeparate(srcFactor, dstFactor, srcFactorAlpha, dstFactorAlpha);
	}
	s_blendMode = newval;
}

static void DumpFrame(const std::vector<u8>& data, int w, int h)
{
#if defined(HAVE_LIBAV) || defined(_WIN32)
	if (SConfig::GetInstance().m_DumpFrames && !data.empty())
	{
		AVIDump::AddFrame(&data[0], w, h);
	}
#endif
}

void Renderer::AsyncTimewarpDraw()
{
#ifdef HAVE_OCULUSSDK
	auto frameTime = ovrHmd_BeginFrame(hmd, ++g_ovr_frameindex);
	g_ovr_lock.unlock();

	if (0 == frameTime.TimewarpPointSeconds) {
		ovr_WaitTillTime(frameTime.TimewarpPointSeconds - 0.002);
	}
	else {
		ovr_WaitTillTime(frameTime.NextFrameSeconds - 0.008);
	}

	g_ovr_lock.lock();
	// Grab the most recent textures
	for (int eye = 0; eye < 2; eye++)
	{
		((ovrGLTexture&)(FramebufferManager::m_eye_texture[eye])).OGL.TexId = FramebufferManager::m_frontBuffer[eye];
	}
#ifdef _WIN32
	//HANDLE thread_handle = g_video_backend->m_video_thread->native_handle();
	//SuspendThread(thread_handle);
#endif
	ovrHmd_EndFrame(hmd, g_front_eye_poses, &FramebufferManager::m_eye_texture[0].Texture);
	Core::ShouldAddTimewarpFrame();
#ifdef _WIN32
	//ResumeThread(thread_handle);
#endif

	static int w = 0, h = 0;
	// Save screenshot
	if (s_bScreenshot)
	{
		TargetRectangle flipped_trc = GetTargetRectangle();
		// Flip top and bottom for some reason; TODO: Fix the code to suck less?
		std::swap(flipped_trc.top, flipped_trc.bottom);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		std::lock_guard<std::mutex> lk(s_criticalScreenshot);
		SaveScreenshot(s_sScreenshotName, flipped_trc);
		// Reset settings
		s_sScreenshotName.clear();
		s_bScreenshot = false;
	}

	// Frame dumps are handled a little differently in Windows
	// Frame dumping disabled entirely on GLES3
	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
	{
#if defined _WIN32 || defined HAVE_LIBAV
		if (SConfig::GetInstance().m_DumpFrames)
		{
			TargetRectangle flipped_trc = GetTargetRectangle();
			// Flip top and bottom for some reason; TODO: Fix the code to suck less?
			std::swap(flipped_trc.top, flipped_trc.bottom);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			std::lock_guard<std::mutex> lk(s_criticalScreenshot);
			if (frame_data.empty() || w != flipped_trc.GetWidth() ||
				h != flipped_trc.GetHeight())
			{
				w = flipped_trc.GetWidth();
				h = flipped_trc.GetHeight();
				frame_data.resize(3 * w * h);
			}
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(flipped_trc.left, flipped_trc.bottom, w, h, GL_BGR, GL_UNSIGNED_BYTE, &frame_data[0]);
			if (w > 0 && h > 0)
			{
				if (!bLastFrameDumped)
				{
#ifdef _WIN32
					bAVIDumping = AVIDump::Start(nullptr, w, h);
#else
					bAVIDumping = AVIDump::Start(w, h);
#endif
					if (!bAVIDumping)
					{
						OSD::AddMessage("AVIDump Start failed", 2000);
					}
					else
					{
						OSD::AddMessage(StringFromFormat(
							"Dumping Frames to \"%sframedump0.avi\" (%dx%d RGB24)",
							File::GetUserPath(D_DUMPFRAMES_IDX).c_str(), w, h), 2000);
					}
				}
				if (bAVIDumping)
				{
#ifndef _WIN32
					FlipImageData(&frame_data[0], w, h);
#endif

					AVIDump::AddFrame(&frame_data[0], w, h);
				}

				bLastFrameDumped = true;
			}
			else
			{
				NOTICE_LOG(VIDEO, "Error reading framebuffer");
			}
		}
		else
		{
			if (bLastFrameDumped && bAVIDumping)
			{
				std::vector<u8>().swap(frame_data);
				w = h = 0;
				AVIDump::Stop();
				bAVIDumping = false;
				OSD::AddMessage("Stop dumping frames", 2000);
			}
			bLastFrameDumped = false;
		}
#else
		if (g_ActiveConfig.bDumpFrames)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			std::lock_guard<std::mutex> lk(s_criticalScreenshot);
			std::string movie_file_name;
			w = GetTargetRectangle().GetWidth();
			h = GetTargetRectangle().GetHeight();
			frame_data.resize(3 * w * h);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(GetTargetRectangle().left, GetTargetRectangle().bottom, w, h, GL_BGR, GL_UNSIGNED_BYTE, &frame_data[0]);
			if (GL_REPORT_ERROR() == GL_NO_ERROR)
			{
				if (!bLastFrameDumped)
				{
					movie_file_name = File::GetUserPath(D_DUMPFRAMES_IDX) + "framedump.raw";
					pFrameDump.Open(movie_file_name, "wb");
					if (!pFrameDump)
					{
						OSD::AddMessage("Error opening framedump.raw for writing.", 2000);
					}
					else
					{
						OSD::AddMessage(StringFromFormat("Dumping Frames to \"%s\" (%dx%d RGB24)", movie_file_name.c_str(), w, h), 2000);
					}
				}
				if (pFrameDump)
				{
					FlipImageData(&frame_data[0], w, h);
					pFrameDump.WriteBytes(&frame_data[0], w * 3 * h);
					pFrameDump.Flush();
				}
				bLastFrameDumped = true;
			}
		}
		else
		{
			if (bLastFrameDumped)
				pFrameDump.Close();
			bLastFrameDumped = false;
		}
#endif
	}
	// end of frame dumping code
#endif
}

// This function has the final picture. We adjust the aspect ratio here.
void Renderer::SwapImpl(u32 xfbAddr, u32 fbWidth, u32 fbStride, u32 fbHeight, const EFBRectangle& rc, float Gamma)
{
#ifdef HAVE_OCULUSSDK
	if (g_first_rift_frame && g_has_rift && g_ActiveConfig.bEnableVR)
	{
		if (!g_ActiveConfig.bAsynchronousTimewarp)
		{
			g_rift_frame_timing = ovrHmd_BeginFrame(hmd, 0);
#ifdef OCULUSSDK042
			g_eye_poses[ovrEye_Left] = ovrHmd_GetEyePose(hmd, ovrEye_Left);
			g_eye_poses[ovrEye_Right] = ovrHmd_GetEyePose(hmd, ovrEye_Right);
#else
			ovrVector3f useHmdToEyeViewOffset[2] = { g_eye_render_desc[0].HmdToEyeViewOffset, g_eye_render_desc[1].HmdToEyeViewOffset };
			ovrHmd_GetEyePoses(hmd, g_ovr_frameindex, useHmdToEyeViewOffset, g_eye_poses, nullptr);
#endif
		}
		g_first_rift_frame = false;

		int cap = 0;
		if (g_ActiveConfig.bOrientationTracking)
			cap |= ovrTrackingCap_Orientation;
		if (g_ActiveConfig.bMagYawCorrection)
			cap |= ovrTrackingCap_MagYawCorrection;
		if (g_ActiveConfig.bPositionTracking)
			cap |= ovrTrackingCap_Position;
		ovrHmd_ConfigureTracking(hmd, cap, 0);
	}
#endif

	static int w = 0, h = 0;
	if (g_bSkipCurrentFrame || (!XFBWrited && !g_ActiveConfig.RealXFBEnabled()) || !fbWidth || !fbHeight)
	{
		DumpFrame(frame_data, w, h);
		Core::Callback_VideoCopiedToXFB(false);
		return;
	}

	u32 xfbCount = 0;
	const XFBSourceBase* const* xfbSourceList = FramebufferManager::GetXFBSource(xfbAddr, fbStride, fbHeight, &xfbCount);
	if (g_ActiveConfig.VirtualXFBEnabled() && (!xfbSourceList || xfbCount == 0))
	{
		DumpFrame(frame_data, w, h);
		Core::Callback_VideoCopiedToXFB(false);
		return;
	}

	eyesFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	ResetAPIState();

	UpdateDrawRectangle(s_backbuffer_width, s_backbuffer_height);
	TargetRectangle flipped_trc = GetTargetRectangle();

		// Flip top and bottom for some reason; TODO: Fix the code to suck less?
		std::swap(flipped_trc.top, flipped_trc.bottom);

	// Copy the framebuffer to screen.
	const XFBSource* xfbSource = nullptr;

	if (g_ActiveConfig.bUseXFB)
	{
		// draw each xfb source
		for (u32 i = 0; i < xfbCount; ++i)
		{
			xfbSource = (const XFBSource*) xfbSourceList[i];

			TargetRectangle drawRc;

			if (g_ActiveConfig.bUseRealXFB)
			{
				drawRc = flipped_trc;
			}
			else
			{
				// use virtual xfb with offset
				int xfbHeight = xfbSource->srcHeight;
				int xfbWidth = xfbSource->srcWidth;
				int hOffset = ((s32)xfbSource->srcAddr - (s32)xfbAddr) / ((s32)fbStride * 2);

				drawRc.top = flipped_trc.top - hOffset * flipped_trc.GetHeight() / fbHeight;
				drawRc.bottom = flipped_trc.top - (hOffset + xfbHeight) * flipped_trc.GetHeight() / fbHeight;
				drawRc.left = flipped_trc.left + (flipped_trc.GetWidth() - xfbWidth * flipped_trc.GetWidth() / fbStride) / 2;
				drawRc.right = flipped_trc.left + (flipped_trc.GetWidth() + xfbWidth * flipped_trc.GetWidth() / fbStride) / 2;

				// The following code disables auto stretch.  Kept for reference.
				// scale draw area for a 1 to 1 pixel mapping with the draw target
				//float vScale = (float)fbHeight / (float)flipped_trc.GetHeight();
				//float hScale = (float)fbWidth / (float)flipped_trc.GetWidth();
				//drawRc.top *= vScale;
				//drawRc.bottom *= vScale;
				//drawRc.left *= hScale;
				//drawRc.right *= hScale;
			}
			// Tell the OSD Menu about the current internal resolution
			OSDInternalW = xfbSource->sourceRc.GetWidth(); OSDInternalH = xfbSource->sourceRc.GetHeight();

			TargetRectangle sourceRc;
			sourceRc.left = xfbSource->sourceRc.left;
			sourceRc.right = xfbSource->sourceRc.right;
			sourceRc.top = xfbSource->sourceRc.top;
			sourceRc.bottom = xfbSource->sourceRc.bottom;

			sourceRc.right -= fbStride - fbWidth;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			// TODO: Virtual XFB stereoscopic 3D support.
			m_post_processor->BlitFromTexture(sourceRc, drawRc, xfbSource->texture, xfbSource->texWidth, xfbSource->texHeight, 1);
		}
	}
#ifdef HAVE_OCULUSSDK
	else if (g_has_rift && g_ActiveConfig.bEnableVR)
	{
		EFBRectangle sourceRc;
		// In VR we use the whole EFB instead of just the bpmem.copyTexSrc rectangle passed to this function. 
		sourceRc.left = 0;
		sourceRc.right = EFB_WIDTH;
		sourceRc.top = 0;
		sourceRc.bottom = EFB_HEIGHT;

		TargetRectangle targetRc = ConvertEFBRectangle(sourceRc);

		// for msaa mode, we must resolve the efb content to non-msaa
		GLuint tex = FramebufferManager::ResolveAndGetRenderTarget(sourceRc);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FramebufferManager::m_eyeFramebuffer[0]);
		if (g_ActiveConfig.iStereoMode == STEREO_OCULUS)
		{
			m_post_processor->BlitFromTexture(targetRc, targetRc, tex, s_target_width, s_target_height, 0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FramebufferManager::m_eyeFramebuffer[1]);
			m_post_processor->BlitFromTexture(targetRc, targetRc, tex, s_target_width, s_target_height, 1);
		}
		else
		{
			m_post_processor->BlitFromTexture(targetRc, flipped_trc, tex, s_target_width, s_target_height);
		}

		//ovrHmd_EndEyeRender(hmd, ovrEye_Left, g_left_eye_pose, &FramebufferManager::m_eye_texture[ovrEye_Left].Texture);
		//ovrHmd_EndEyeRender(hmd, ovrEye_Right, g_right_eye_pose, &FramebufferManager::m_eye_texture[ovrEye_Right].Texture);

		if (!g_ActiveConfig.bAsynchronousTimewarp)
		{
			// Let OVR do distortion rendering, Present and flush/sync.
			static int ElementArrayBufferBinding, ArrayBufferBinding, VertexArrayBinding;
			glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ElementArrayBufferBinding);
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &ArrayBufferBinding);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &VertexArrayBinding);
			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			ovrHmd_EndFrame(hmd, g_eye_poses, &FramebufferManager::m_eye_texture[0].Texture);
			for (int i = 0; i < (int)g_ActiveConfig.iExtraFrames; ++i)
			{
				ovrFrameTiming frameTime = ovrHmd_BeginFrame(hmd, ++g_ovr_frameindex);

				ovr_WaitTillTime(frameTime.NextFrameSeconds - g_ActiveConfig.fTimeWarpTweak);

				ovrHmd_EndFrame(hmd, g_eye_poses, &FramebufferManager::m_eye_texture[0].Texture);
			}

			//glBindVertexArray(VertexArrayBinding);
			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementArrayBufferBinding);
			glBindBuffer(GL_ARRAY_BUFFER, ArrayBufferBinding);
		}
		else
		{
			// Wait for OpenGL to finish drawing the commands we have given it,
			// and when finished, swap the back buffer textures to the front buffer textures
			do
			{
				eyesFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
				if (eyesFence != 0) {
					GLenum result = glClientWaitSync(eyesFence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
					switch (result)
					{
					case GL_ALREADY_SIGNALED:
					case GL_CONDITION_SATISFIED:
						eyesFence = 0;
						g_ovr_lock.lock();
						FramebufferManager::SwapAsyncFrontBuffers();
						g_front_eye_poses[0] = g_eye_poses[0];
						g_front_eye_poses[1] = g_eye_poses[1];
						//glFinish();
						g_ovr_lock.unlock();
						break;
					}
				}
			} while (eyesFence != 0);
		}
	}
#endif
	else
	{
		EFBRectangle sourceRc;
			sourceRc = rc;
		TargetRectangle targetRc = ConvertEFBRectangle(sourceRc);

		// for msaa mode, we must resolve the efb content to non-msaa
		GLuint tex = FramebufferManager::ResolveAndGetRenderTarget(sourceRc);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		if (g_ActiveConfig.iStereoMode == STEREO_SBS || g_ActiveConfig.iStereoMode == STEREO_TAB)
		{
			TargetRectangle leftRc, rightRc;
			ConvertStereoRectangle(flipped_trc, leftRc, rightRc);

			m_post_processor->BlitFromTexture(targetRc, leftRc, tex, s_target_width, s_target_height, 0);
			m_post_processor->BlitFromTexture(targetRc, rightRc, tex, s_target_width, s_target_height, 1);
		}
		else
		{
			m_post_processor->BlitFromTexture(targetRc, flipped_trc, tex, s_target_width, s_target_height);
		}
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	// Save screenshot
	if (s_bScreenshot && !g_ActiveConfig.bAsynchronousTimewarp)
	{
		std::lock_guard<std::mutex> lk(s_criticalScreenshot);
		SaveScreenshot(s_sScreenshotName, flipped_trc);
		// Reset settings
		s_sScreenshotName.clear();
		s_bScreenshot = false;
	}

	// Frame dumps are handled a little differently in Windows
	// Frame dumping disabled entirely on GLES3, or handled elsewhere for Oculus Rift's Asynchronous Timewarp
	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL && !g_ActiveConfig.bAsynchronousTimewarp)
	{
#if defined _WIN32 || defined HAVE_LIBAV
		if (SConfig::GetInstance().m_DumpFrames)
		{
			std::lock_guard<std::mutex> lk(s_criticalScreenshot);
			if (frame_data.empty() || w != flipped_trc.GetWidth() ||
				     h != flipped_trc.GetHeight())
			{
				w = flipped_trc.GetWidth();
				h = flipped_trc.GetHeight();
				frame_data.resize(3 * w * h);
			}
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(flipped_trc.left, flipped_trc.bottom, w, h, GL_BGR, GL_UNSIGNED_BYTE, &frame_data[0]);
			if (w > 0 && h > 0)
			{
				if (!bLastFrameDumped)
				{
					#ifdef _WIN32
						bAVIDumping = AVIDump::Start(nullptr, w, h);
					#else
						bAVIDumping = AVIDump::Start(w, h);
					#endif
					if (!bAVIDumping)
					{
						OSD::AddMessage("AVIDump Start failed", 2000);
					}
					else
					{
						OSD::AddMessage(StringFromFormat(
									"Dumping Frames to \"%sframedump0.avi\" (%dx%d RGB24)",
									File::GetUserPath(D_DUMPFRAMES_IDX).c_str(), w, h), 2000);
					}
				}
				if (bAVIDumping)
				{
					#ifndef _WIN32
						FlipImageData(&frame_data[0], w, h);
					#endif

						AVIDump::AddFrame(&frame_data[0], w, h);
				}

				bLastFrameDumped = true;
			}
			else
			{
				NOTICE_LOG(VIDEO, "Error reading framebuffer");
			}
		}
		else
		{
			if (bLastFrameDumped && bAVIDumping)
			{
				std::vector<u8>().swap(frame_data);
				w = h = 0;
				AVIDump::Stop();
				bAVIDumping = false;
				OSD::AddMessage("Stop dumping frames", 2000);
			}
			bLastFrameDumped = false;
		}
#else
		if (SConfig::GetInstance().m_DumpFrames)
		{
			std::lock_guard<std::mutex> lk(s_criticalScreenshot);
			std::string movie_file_name;
			w = GetTargetRectangle().GetWidth();
			h = GetTargetRectangle().GetHeight();
			frame_data.resize(3 * w * h);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(GetTargetRectangle().left, GetTargetRectangle().bottom, w, h, GL_BGR, GL_UNSIGNED_BYTE, &frame_data[0]);

				if (!bLastFrameDumped)
				{
					movie_file_name = File::GetUserPath(D_DUMPFRAMES_IDX) + "framedump.raw";
				File::CreateFullPath(movie_file_name);
					pFrameDump.Open(movie_file_name, "wb");
					if (!pFrameDump)
					{
						OSD::AddMessage("Error opening framedump.raw for writing.", 2000);
					}
					else
					{
						OSD::AddMessage(StringFromFormat("Dumping Frames to \"%s\" (%dx%d RGB24)", movie_file_name.c_str(), w, h), 2000);
					}
				}
				if (pFrameDump)
				{
					FlipImageData(&frame_data[0], w, h);
					pFrameDump.WriteBytes(&frame_data[0], w * 3 * h);
					pFrameDump.Flush();
				}
				bLastFrameDumped = true;
			}
		else
		{
			if (bLastFrameDumped)
				pFrameDump.Close();
			bLastFrameDumped = false;
		}
#endif
	}
	// Finish up the current frame, print some stats

	SetWindowSize(fbStride, fbHeight);

	GLInterface->Update(); // just updates the render window position and the backbuffer size

	bool xfbchanged = false;

	if (FramebufferManagerBase::LastXfbWidth() != fbStride || FramebufferManagerBase::LastXfbHeight() != fbHeight)
	{
		xfbchanged = true;
		unsigned int const last_w = (fbStride < 1 || fbStride > MAX_XFB_WIDTH) ? MAX_XFB_WIDTH : fbStride;
		unsigned int const last_h = (fbHeight < 1 || fbHeight > MAX_XFB_HEIGHT) ? MAX_XFB_HEIGHT : fbHeight;
		FramebufferManagerBase::SetLastXfbWidth(last_w);
		FramebufferManagerBase::SetLastXfbHeight(last_h);
	}

	bool WindowResized = false;
	int W = (int)GLInterface->GetBackBufferWidth();
	int H = (int)GLInterface->GetBackBufferHeight();
	if (W != s_backbuffer_width || H != s_backbuffer_height || s_LastEFBScale != g_ActiveConfig.iEFBScale)
	{
		WindowResized = true;
		s_backbuffer_width = W;
		s_backbuffer_height = H;
		s_LastEFBScale = g_ActiveConfig.iEFBScale;
	}

	if (xfbchanged || WindowResized || (s_LastMultisampleMode != g_ActiveConfig.iMultisampleMode) || (s_LastStereo != (g_ActiveConfig.iStereoMode > 0)))
	{
		UpdateDrawRectangle(s_backbuffer_width, s_backbuffer_height);

		if (CalculateTargetSize(s_backbuffer_width, s_backbuffer_height) || s_LastMultisampleMode != g_ActiveConfig.iMultisampleMode || s_LastStereo != (g_ActiveConfig.iStereoMode > 0))
		{
			s_LastMultisampleMode = g_ActiveConfig.iMultisampleMode;
			s_MSAASamples = GetNumMSAASamples(s_LastMultisampleMode);
			ApplySSAASettings();
			s_LastStereo = g_ActiveConfig.iStereoMode > 0;

			if (g_ActiveConfig.bAsynchronousTimewarp)
				g_ovr_lock.lock();
			delete g_framebuffer_manager;
			g_framebuffer_manager = new FramebufferManager(s_target_width, s_target_height,
				s_MSAASamples);
			glFinish();
			if (g_ActiveConfig.bAsynchronousTimewarp)
				g_ovr_lock.unlock();
		}
	}

	// ---------------------------------------------------------------------
	if (!DriverDetails::HasBug(DriverDetails::BUG_BROKENSWAP) && !(g_has_rift && g_ActiveConfig.bEnableVR))
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		DrawDebugInfo();
		DrawDebugText();

		// Do our OSD callbacks
		OSD::DoCallbacks(OSD::OSD_ONFRAME);
		OSD::DrawMessages();
	}
	// Copy the rendered frame to the real window
	if (!(g_has_rift && g_ActiveConfig.bEnableVR))
		GLInterface->Swap();

	NewVRFrame();

	// Clear framebuffer
	if (!DriverDetails::HasBug(DriverDetails::BUG_BROKENSWAP))
	{
		glClearColor(0, 0, 0, 0);
		// glClearColor(0.f, 0.f, 0.f, 1.f);
		glClearDepth(1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
	// VR
	//g_texture_cache->ClearRenderTargets();

	if (s_vsync != g_ActiveConfig.IsVSync())
	{
		s_vsync = g_ActiveConfig.IsVSync();
		GLInterface->SwapInterval(s_vsync);
	}

	// Clean out old stuff from caches. It's not worth it to clean out the shader caches.
	TextureCache::Cleanup();

#ifdef HAVE_OCULUSSDK
	if (g_has_rift)
	{
		if (g_Config.bLowPersistence != g_ActiveConfig.bLowPersistence ||
			g_Config.bDynamicPrediction != g_ActiveConfig.bDynamicPrediction)
		{
			int caps = ovrHmd_GetEnabledCaps(hmd) & ~(ovrHmdCap_DynamicPrediction | ovrHmdCap_LowPersistence);
			if (g_Config.bLowPersistence)
				caps |= ovrHmdCap_LowPersistence;
			if (g_Config.bDynamicPrediction)
				caps |= ovrHmdCap_DynamicPrediction;

			ovrHmd_SetEnabledCaps(hmd, caps);
		}

		if (g_Config.bOrientationTracking != g_ActiveConfig.bOrientationTracking ||
			g_Config.bMagYawCorrection != g_ActiveConfig.bMagYawCorrection ||
			g_Config.bPositionTracking != g_ActiveConfig.bPositionTracking)
		{
			int cap = 0;
			if (g_ActiveConfig.bOrientationTracking)
				cap |= ovrTrackingCap_Orientation;
			if (g_ActiveConfig.bMagYawCorrection)
				cap |= ovrTrackingCap_MagYawCorrection;
			if (g_ActiveConfig.bPositionTracking)
				cap |= ovrTrackingCap_Position;
			ovrHmd_ConfigureTracking(hmd, cap, 0);
		}

		if (g_Config.bChromatic != g_ActiveConfig.bChromatic ||
			g_Config.bTimewarp != g_ActiveConfig.bTimewarp ||
			g_Config.bVignette != g_ActiveConfig.bVignette ||
			g_Config.bNoRestore != g_ActiveConfig.bNoRestore ||
			g_Config.bFlipVertical != g_ActiveConfig.bFlipVertical ||
			g_Config.bSRGB != g_ActiveConfig.bSRGB ||
			g_Config.bOverdrive != g_ActiveConfig.bOverdrive ||
			g_Config.bHqDistortion != g_ActiveConfig.bHqDistortion)
		{
			FramebufferManager::ConfigureRift();
		}

		//To do: Probably not the right place for these.  Why do they update for D3D automatically, but not for OpenGL?
		g_ActiveConfig.iExtraFrames = g_Config.iExtraFrames;
		g_ActiveConfig.iExtraVideoLoops = g_Config.iExtraVideoLoops;
		g_ActiveConfig.iExtraVideoLoopsDivider = g_Config.iExtraVideoLoopsDivider;
		g_ActiveConfig.fTimeWarpTweak = g_Config.fTimeWarpTweak;
	}
#endif

	// Render to the framebuffer.
	FramebufferManager::SetFramebuffer(0);

	RestoreAPIState();

	g_Config.iSaveTargetId = 0;
	
	// VR layer debugging, sometimes layers need to flash.
	g_Config.iFlashState++;
	if (g_Config.iFlashState >= 10)
		g_Config.iFlashState = 0;

	UpdateActiveConfig();
	// VR XFB isn't implemented yet, so always disable it for VR
	if (g_has_hmd && g_ActiveConfig.bEnableVR)
	{
		g_ActiveConfig.bUseXFB = false;
		// always stretch to fit
		g_ActiveConfig.iAspectRatio = 3; 
	}
	TextureCache::OnConfigChanged(g_ActiveConfig);
#ifdef HAVE_OCULUSSDK
	if (g_has_rift && g_ActiveConfig.bEnableVR && !g_ActiveConfig.bAsynchronousTimewarp)
	{
		g_rift_frame_timing = ovrHmd_BeginFrame(hmd, 0);
	}
#endif

	// For testing zbuffer targets.
	// Renderer::SetZBufferRender();
	// SaveTexture("tex.png", GL_TEXTURE_2D, s_FakeZTarget,
	//	      GetTargetWidth(), GetTargetHeight());

	// Invalidate EFB cache
	ClearEFBCache();

	// Opcode Replay Buffer Code.  This enables the capture of all the Video Opcodes that occur during a frame,
	// and then plays them back with new headtracking information.  Allows ways to easily set headtracking at 75fps
	// for various games.  In Alpha right now, will crash many games/cause corruption.
	static int extra_video_loops_count = 0;
	static int real_frame_count = 0;
	if (g_ActiveConfig.iExtraVideoLoops)
	{
		if (g_ActiveConfig.bPullUp20fps)
		{
			if (real_frame_count % 4 == 1)
			{
				g_ActiveConfig.iExtraVideoLoops = 2;
			}
			else
			{
				g_ActiveConfig.iExtraVideoLoops = 3;
			}
		}

		if (g_ActiveConfig.bPullUp30fps)
		{
			if (real_frame_count % 2 == 1)
			{
				g_ActiveConfig.iExtraVideoLoops = 1;
			}
			else
			{
				g_ActiveConfig.iExtraVideoLoops = 2;
			}
		}

		if (g_ActiveConfig.bPullUp60fps)
		{
			g_ActiveConfig.iExtraVideoLoops = 1;
			g_ActiveConfig.iExtraVideoLoopsDivider = 3;
		}

		if ((g_timewarped_frame && (extra_video_loops_count >= (int)g_ActiveConfig.iExtraVideoLoops)))
		{
			g_timewarped_frame = false;
			++real_frame_count;
			extra_video_loops_count = 0;
		}
		else
		{
			if (skipped_opcode_replay_count >= (int)g_ActiveConfig.iExtraVideoLoopsDivider)
			{
				g_timewarped_frame = true;
				++extra_video_loops_count;
				skipped_opcode_replay_count = 0;

				for (int i = 0; i < timewarp_log.size(); ++i)
				{
					if (!cached_ram_location.at(i))
					{
						OpcodeDecoder_Run(timewarp_log.at(i), nullptr, display_list_log.at(i));
					}
				}
			}
			else
			{
				++skipped_opcode_replay_count;
			}
			timewarp_log.clear();
			timewarp_log.resize(0);
			display_list_log.clear();
			display_list_log.resize(0);
			cached_ram_location.clear();
			cached_ram_location.resize(0);
		}
	}
	else
	{
		g_timewarped_frame = true; //Don't log frames
	}
}

// ALWAYS call RestoreAPIState for each ResetAPIState call you're doing
void Renderer::ResetAPIState()
{
	// Gets us to a reasonably sane state where it's possible to do things like
	// image copies with textured quads, etc.
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
	{
		glDisable(GL_COLOR_LOGIC_OP);
		if (g_ActiveConfig.bWireFrame)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	glDepthMask(GL_FALSE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Renderer::RestoreAPIState()
{
	// Gets us back into a more game-like state.
	if (g_has_hmd)
		glDisable(GL_SCISSOR_TEST);
	else
		glEnable(GL_SCISSOR_TEST);
	SetGenerationMode();
	BPFunctions::SetScissor();
	SetColorMask();
	SetDepthMode();
	SetBlendMode(true);
	SetLogicOpMode();
	SetViewport();

	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL)
		glPolygonMode(GL_FRONT_AND_BACK, g_ActiveConfig.bWireFrame ? GL_LINE : GL_FILL);

	VertexManager *vm = (OGL::VertexManager*)g_vertex_manager;
	glBindBuffer(GL_ARRAY_BUFFER, vm->m_vertex_buffers);
	if (vm->m_last_vao)
		glBindVertexArray(vm->m_last_vao);

	TextureCache::SetStage();
}

void Renderer::SetGenerationMode()
{
	// none, ccw, cw, ccw
	if (bpmem.genMode.cullmode > 0)
	{
		// TODO: GX_CULL_ALL not supported, yet!
		glEnable(GL_CULL_FACE);
		glFrontFace(bpmem.genMode.cullmode == 2 ? GL_CCW : GL_CW);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}
}

void Renderer::SetDepthMode()
{
	const GLenum glCmpFuncs[8] =
	{
		GL_NEVER,
		GL_LESS,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS
	};

	if (bpmem.zmode.testenable)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthMask(bpmem.zmode.updateenable ? GL_TRUE : GL_FALSE);
		glDepthFunc(glCmpFuncs[bpmem.zmode.func]);
	}
	else
	{
		// if the test is disabled write is disabled too
		// TODO: When PE performance metrics are being emulated via occlusion queries, we should (probably?) enable depth test with depth function ALWAYS here
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}
}

void Renderer::SetLogicOpMode()
{
	if (GLInterface->GetMode() != GLInterfaceMode::MODE_OPENGL)
		return;
	// Logic ops aren't available in GLES3/GLES2
	const GLenum glLogicOpCodes[16] =
	{
		GL_CLEAR,
		GL_AND,
		GL_AND_REVERSE,
		GL_COPY,
		GL_AND_INVERTED,
		GL_NOOP,
		GL_XOR,
		GL_OR,
		GL_NOR,
		GL_EQUIV,
		GL_INVERT,
		GL_OR_REVERSE,
		GL_COPY_INVERTED,
		GL_OR_INVERTED,
		GL_NAND,
		GL_SET
	};

	if (bpmem.blendmode.logicopenable)
	{
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(glLogicOpCodes[bpmem.blendmode.logicmode]);
	}
	else
	{
		glDisable(GL_COLOR_LOGIC_OP);
	}
}

void Renderer::SetDitherMode()
{
	if (bpmem.blendmode.dither)
		glEnable(GL_DITHER);
	else
		glDisable(GL_DITHER);
}

void Renderer::SetLineWidth()
{
	float fratio = xfmem.viewport.wd != 0 ?
		((float)Renderer::GetTargetWidth() / EFB_WIDTH) : 1.0f;
	if (bpmem.lineptwidth.linesize > 0)
		// scale by ratio of widths
		glLineWidth((float)bpmem.lineptwidth.linesize * fratio / 6.0f);
	if (GLInterface->GetMode() == GLInterfaceMode::MODE_OPENGL && bpmem.lineptwidth.pointsize > 0)
		glPointSize((float)bpmem.lineptwidth.pointsize * fratio / 6.0f);
}

void Renderer::SetSamplerState(int stage, int texindex)
{
	auto const& tex = bpmem.tex[texindex];
	auto const& tm0 = tex.texMode0[stage];
	auto const& tm1 = tex.texMode1[stage];

	g_sampler_cache->SetSamplerState((texindex * 4) + stage, tm0, tm1);
}

void Renderer::SetInterlacingMode()
{
	// TODO
}

void Renderer::FlipImageData(u8 *data, int w, int h, int pixel_width)
{
	// Flip image upside down. Damn OpenGL.
	for (int y = 0; y < h / 2; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			for (int delta = 0; delta < pixel_width; ++delta)
				std::swap(data[(y * w + x) * pixel_width + delta], data[((h - 1 - y) * w + x) * pixel_width + delta]);
		}
	}
}

}

namespace OGL
{

bool Renderer::SaveScreenshot(const std::string &filename, const TargetRectangle &back_rc)
{
	u32 W = back_rc.GetWidth();
	u32 H = back_rc.GetHeight();
	std::unique_ptr<u8[]> data(new u8[W * 4 * H]);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glReadPixels(back_rc.left, back_rc.bottom, W, H, GL_RGBA, GL_UNSIGNED_BYTE, data.get());

	// Turn image upside down
	FlipImageData(data.get(), W, H, 4);

	return TextureToPng(data.get(), W * 4, filename, W, H, false);

}

int Renderer::GetMaxTextureSize()
{
	int max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	return max_size;
}

}
