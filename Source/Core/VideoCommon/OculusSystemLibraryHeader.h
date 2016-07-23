// Licensed under GPL2+
// Copyright Carl Kenner 2016

// This file should be included either after the Oculus SDK CAPI and version file with
// HAVE_OCULUSSDK defined,
// or without the Oculus SDK and HAVE_OCULUSSDK and OVR_MAJOR_VERSION not defined.
// It adds all the constants, types, and functions (in my own words) from all the Oculus SDK
// versions 0.5 to 1.5.
// The functions will be loaded dynamically at runtime.
// You should only use #ifdefs in your code around features from Oculus SDK 0.4 and below.
// Use normal if (g_libovr_version > libovr_060) etc. statements to check which functions and types
// to use.

#pragma once
#include <stdint.h>

#define OCULUSSYSTEMLIBRARYHEADER

#if defined(OVR_MAJOR_VERSION) && !defined(OVR_PRODUCT_VERSION)
#define OVR_PRODUCT_VERSION 0
#endif

#define ALIGN_TO_FOUR_BYTE_BOUNDARY __declspec(align(4))
#define ALIGN_TO_EIGHT_BYTE_BOUNDARY __declspec(align(8))
#ifdef _WIN64
#define ALIGN_TO_POINTER_BOUNDARY __declspec(align(8))
#else
#define ALIGN_TO_POINTER_BOUNDARY __declspec(align(4))
#endif

enum TLibOvrVersion
{
  libovr_none = 0,
  libovr_013,
  libovr_014,
  libovr_015,
  libovr_021,
  libovr_022,
  libovr_023,
  libovr_024,
  libovr_025,
  libovr_031,
  libovr_032,
  libovr_033,
  libovr_040,
  libovr_041,
  libovr_042,
  libovr_043,
  libovr_044,
  libovr_050,
  libovr_060,
  libovr_070,
  libovr_080,
  libovr_130
};

extern TLibOvrVersion g_libovr_version;

#if !defined(HAVE_OCULUSSDK)
#define ovrHmd_None 0
#define ovrHmd_DK1 3
#define ovrHmd_DKHD 4
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION >= 5) || (OVR_PRODUCT_VERSION >= 1)
#define ovrHmd_CrystalCoveProto 5
#endif
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrHmd_DK2 6
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION != 5 || OVR_PRODUCT_VERSION != 0)
#define ovrHmd_BlackStar 7
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION < 5 && OVR_PRODUCT_VERSION < 1)
#define ovrHmd_CB 8
#endif
#if !defined(HAVE_OCULUSSDK)
#define ovrHmd_Other 9
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION < 7 && OVR_PRODUCT_VERSION < 1)
#define ovrHmd_E3_2015 10
#define ovrHmd_ES06 11
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION < 8 && OVR_PRODUCT_VERSION < 1)
#define ovrHmd_ES09 12
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION < 8 && OVR_PRODUCT_VERSION < 1)
#define ovrHmd_ES09 12
#endif
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION < 1
#define ovrHmd_ES11 13
#define ovrHmd_CV1 14
#endif

// SDK 0.3 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrEye_Left 0
#define ovrEye_Right 1
#define ovrEye_Count 2
#endif

// SDK 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION < 7 && OVR_PRODUCT_VERSION < 1)
#define ovrHand_Left 0
#define ovrHand_Right 1
#endif

// SDK 0.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrHmdCap_Present 0x0001
#define ovrHmdCap_Available 0x0002
#endif
// SDK 0.4 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrHmdCap_Captured 0x0004
#endif
// SDK 0.4 to 0.6 (commented out in 0.3, in unused file in SDK 0.7 and above)
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 6 ||                \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrHmdCap_ExtendDesktop 0x0008
#endif
// SDK 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrHmdCap_DebugDevice 0x0010
#endif
// SDK 0.4 to SDK 0.5 (commented out in 0.3)
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrHmdCap_DisplayOff 0x0040
#endif
// SDK 0.3 to 0.6
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 6 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrHmdCap_LowPersistence 0x0080
#define ovrHmdCap_DynamicPrediction 0x0200
#endif
// SDK 0.4.4 only
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION != 0 ||               \
    OVR_MINOR_VERSION != 4 || OVR_BUILD_VERSION < 4
#define ovrHmdCap_DirectPentile 0x0400
#endif
// SDK 0.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrHmdCap_NoVSync 0x1000
#endif
// SDK 0.4 to SDK 0.5 (commented out in 0.3)
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrHmdCap_NoMirrorToWindow 0x2000
#endif
// SDK 0.4 to 0.8 (but set to zero on 0.7 and 0.8)
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 ||                                         \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrHmdCap_Service_Mask 0x22C0
#endif
// SDK 0.3 to 0.8 (but set to zero on 0.7 and 0.8)
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 ||                                         \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrHmdCap_Writable_Mask 0x32C0
#endif

// SDK 0.4 and above
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrTrackingCap_Orientation 0x010
#define ovrTrackingCap_MagYawCorrection 0x020
#define ovrTrackingCap_Position 0x040
#endif
// SDK 0.4 to 0.7
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 7 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrTrackingCap_Idle 0x100
#endif

// SDK 0.3 and above
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrStatus_OrientationTracked 0x0001
#define ovrStatus_PositionTracked 0x0002
#endif
// SDK 0.4 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 ||                                         \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrStatus_CameraPoseTracked 0x0004
#endif
// SDK 0.3 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 ||                                         \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrStatus_PositionConnected 0x0020
#define ovrStatus_HmdConnected 0x0080
#endif

// 0.3 to 0.4
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 0 ||                \
    OVR_MINOR_VERSION < 3
#define ovrDistortionCap_Chromatic 0x00001
#endif

// 0.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrDistortionCap_TimeWarp 0x00002
#define ovrDistortionCap_Vignette 0x00008
#endif
// 0.4.0 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
#define ovrDistortionCap_NoRestore 0x00010
#define ovrDistortionCap_FlipInput 0x00020
#define ovrDistortionCap_SRGB 0x00040
#define ovrDistortionCap_Overdrive 0x00080
#endif
// 0.4.2 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4) ||                                           \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION == 4 && OVR_BUILD_VERSION < 2)
#define ovrDistortionCap_HqDistortion 0x00100
#endif
// 0.4.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4) ||                                           \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION == 4 && OVR_BUILD_VERSION < 3)
#define ovrDistortionCap_LinuxDevFullscreen 0x00200
#endif
// 0.4.4 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4) ||                                           \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION == 4 && OVR_BUILD_VERSION < 4)
#define ovrDistortionCap_ComputeShader 0x00400
#endif

// 0.5 only
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION != 5
#define ovrDistortionCap_NoTimewarpJit 0x00800
#define ovrDistortionCap_TimewarpJitDelay 0x01000
#define ovrDistortionCap_ProfileNoSpinWaits 0x10000
#endif

// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrLogLevel_Debug 0
#define ovrLogLevel_Info 1
#define ovrLogLevel_Error 2
#endif

// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrInit_Debug 1
#endif
// 0.5 to 0.7
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 7 ||                \
    OVR_MAJOR_VERSION < 5
#define ovrInit_ServerOptional 2
#endif
// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrInit_RequestVersion 4
#endif
// 0.5 to 0.6
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 6 ||                \
    OVR_MAJOR_VERSION < 5
#define ovrInit_ForceNoDebug 8
#endif

// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrFalse 0
#define ovrTrue 1
#endif

// 0.3 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 ||                                         \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrRenderAPI_None 0
#define ovrRenderAPI_OpenGL 1
#define ovrRenderAPI_Android_GLES 2
#endif
// 0.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrRenderAPI_D3D9 3
#define ovrRenderAPI_D3D10 4
#endif
// 0.6 only
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION != 6
#define ovrRenderAPI_D3D9_Obsolete 3
#define ovrRenderAPI_D3D10_Obsolete 4
#endif
// 0.3 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 ||                                         \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#define ovrRenderAPI_D3D11 5
// 0.3 to 0.8 (but has different values)
#define ovrRenderAPI_Count 6
#endif

// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrProjection_None 0
#endif
// 0.5 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 5
#define ovrProjection_RightHanded 1
#endif
// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrProjection_LeftHanded 1
#endif
// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrProjection_FarLessThanNear 2
#define ovrProjection_FarClipAtInfinity 4
#define ovrProjection_ClipRangeOpenGL 8
#endif

// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrSuccess 0
#endif
// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define ovrSuccess_NotVisible 1000
#define ovrSuccess_HMDFirmwareMismatch 4100
#define ovrSuccess_TrackerFirmwareMismatch 4101
#endif
// 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7)
#define ovrSuccess_ControllerFirmwareMismatch 4104
#endif
// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrSuccess_TrackerDriverNotFound 4105
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define ovrError_MemoryAllocationFailure -1000
#define ovrError_SocketCreationFailure -1001
#endif
// 0.6 to 0.7
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 7 ||                \
    OVR_MAJOR_VERSION < 6
#define ovrError_InvalidHmd -1002
#endif
// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrError_InvalidSession -1002
#endif
// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define ovrError_Timeout -1003
#define ovrError_NotInitialized -1004
#define ovrError_InvalidParameter -1005
#define ovrError_ServiceError -1006
#define ovrError_NoHmd -1007
#endif

// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrError_Unsupported -1009
#define ovrError_DeviceUnavailable -1010
#define ovrError_InvalidHeadsetOrientation -1011
#define ovrError_ClientSkippedDestroy -1012
#define ovrError_ClientSkippedShutdown -1013
#endif
// 1.4 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0 ||                                        \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION <= 1 && OVR_MINOR_VERSION < 4)
#define ovrError_ServiceDeadlockDetected -1014
#endif

// 0.6 to 1.4
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6) ||             \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION == 1 && OVR_MINOR_VERSION > 4) ||               \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION > 1) || (OVR_PRODUCT_VERSION > 1)
#define ovrError_AudioReservedBegin -2000
#define ovrError_AudioReservedEnd -2999
#endif
// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrError_AudioDeviceNotFound -2001
#define ovrError_AudioComError -2002
#endif

// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
#define ovrError_Initialize -3000
#define ovrError_LibLoad -3001
#define ovrError_LibVersion -3002
#define ovrError_ServiceConnection -3003
#define ovrError_ServiceVersion -3004
#define ovrError_IncompatibleOS -3005
#define ovrError_DisplayInit -3006
#define ovrError_ServerStart -3007
#define ovrError_Reinitialization -3008
#endif
// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define ovrError_MismatchedAdapters -3009
#endif
// 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7)
#define ovrError_LeakingResources -3010
#define ovrError_ClientVersion -3011
#endif
// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrError_OutOfDateOS -3012
#define ovrError_OutOfDateGfxDriver -3013
#define ovrError_IncompatibleGPU -3014
#define ovrError_NoValidVRDisplaySystem -3015
#endif
// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrError_Obsolete -3016
#define ovrError_DisabledOrDefaultAdapter -3017
#define ovrError_HybridGraphicsNotSupported -3018
#define ovrError_DisplayManagerInit -3019
#define ovrError_TrackerDriverInit -3020
#endif

// 0.6 to 1.4
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6) ||             \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION == 1 && OVR_MINOR_VERSION > 4) ||               \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION > 1) || (OVR_PRODUCT_VERSION > 1)
#define ovrError_InvalidBundleAdjustment -4000
#define ovrError_USBBandwidth -4001
#define ovrError_USBEnumeratedSpeed -4002
#define ovrError_ImageSensorCommError -4003
#define ovrError_GeneralTrackerFailure -4004
#define ovrError_ExcessiveFrameTruncation -4005
#define ovrError_ExcessiveFrameSkipping -4006
#define ovrError_SyncDisconnected -4007
#endif
// 0.7 to 1.4
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7) ||             \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION == 1 && OVR_MINOR_VERSION > 4) ||               \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION > 1) || (OVR_PRODUCT_VERSION > 1)
#define ovrError_TrackerMemoryReadFailure -4008
#define ovrError_TrackerMemoryWriteFailure -4009
#define ovrError_TrackerFrameTimeout -4010
#define ovrError_TrackerTruncatedFrame -4011
#endif
// 1.3 to 1.4
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION != 1 || OVR_MAJOR_VERSION != 1 ||              \
    OVR_MINOR_VERSION > 4
#define ovrError_TrackerDriverFailure = -4012
#define ovrError_TrackerNRFFailure = -4013
#define ovrError_HardwareGone = -4014
#define ovrError_NordicEnabledNoSync = -4015
#define ovrError_NordicSyncNoFrames = -4016
#define ovrError_CatastrophicFailure = -4017
#endif
// 1.4 only
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION != 1 || OVR_MAJOR_VERSION != 1 ||              \
    OVR_MINOR_VERSION != 4
#define ovrError_CatastrophicTimeout -4018
#define ovrError_RepeatCatastrophicFail -4019
#define ovrError_USBOpenDeviceFailure -4020
#define ovrError_HMDGeneralFailure -4021
#endif

// 0.6 to 1.4
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6) ||             \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION == 1 && OVR_MINOR_VERSION > 4) ||               \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION > 1) || (OVR_PRODUCT_VERSION > 1)
#define ovrError_HMDFirmwareMismatch -4100
#define ovrError_TrackerFirmwareMismatch -4101
#endif
// 0.7 to 1.4
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7) ||             \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION == 1 && OVR_MINOR_VERSION > 4) ||               \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION > 1) || (OVR_PRODUCT_VERSION > 1)
#define ovrError_BootloaderDeviceDetected -4102
#define ovrError_TrackerCalibrationError -4103
#define ovrError_ControllerFirmwareMismatch -4104
#endif

// 1.5 only
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION != 1 || OVR_MAJOR_VERSION != 1 ||              \
    OVR_MINOR_VERSION != 5
#define ovrError_DevManDeviceDetected -4105
#define ovrError_RebootedBootloaderDevice -4106
#define ovrError_FailedRebootBootloaderDev -4107
#endif

// 1.3 to 1.4
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION != 1 || OVR_MAJOR_VERSION != 1 ||              \
    OVR_MINOR_VERSION > 4
#define ovrError_IMUTooManyLostSamples -4200
#define ovrError_IMURateError -4201
#define ovrError_FeatureReportFailure -4202
#endif
// 1.4 only
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION != 1 || OVR_MAJOR_VERSION != 1 ||              \
    OVR_MINOR_VERSION != 4
#define ovrError_HMDWirelessTimeout -4203
#define ovrError_BootloaderAssertLog -4300
#define ovrError_AppAssertLog -4301
#endif

// 0.6 to 1.4
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6) ||             \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION == 1 && OVR_MINOR_VERSION > 4) ||               \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION > 1) || (OVR_PRODUCT_VERSION > 1)
#define ovrError_Incomplete -5000
#define ovrError_Abandoned -5001
#endif

// 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7)
#define ovrError_DisplayLost -6000
#endif
// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrError_TextureSwapChainFull -6001
#define ovrError_TextureSwapChainInvalid -6002
#endif
// 1.4 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0 ||                                        \
    (OVR_PRODUCT_VERSION == 1 && OVR_MAJOR_VERSION <= 1 && OVR_MINOR_VERSION < 4)
#define ovrError_GraphicsDeviceReset -6003
#define ovrError_DisplayRemoved -6004
#define ovrError_ContentProtectionNotAvailable -6005
#define ovrError_ApplicationInvisible -6006
#define ovrError_Disallowed -6007
#define ovrError_DisplayPluggedIncorrectly -6008
#endif

// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrError_RuntimeException -7000
#endif

// 1.3 to 1.4
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION != 1 || OVR_MAJOR_VERSION != 1 ||              \
    OVR_MINOR_VERSION > 4
#define ovrError_MetricsUnknownApp -90000,
#define ovrError_MetricsDuplicateApp -90001,
#define ovrError_MetricsNoEvents -90002,
#define ovrError_MetricsRuntime -90003,
#define ovrError_MetricsFile -90004,
#define ovrError_MetricsNoClientInfo -90005,
#define ovrError_MetricsNoAppMetaData -90006,
#define ovrError_MetricsNoApp -90007,
#define ovrError_MetricsOafFailure -90008,
#define ovrError_MetricsSessionAlreadyActive -90009,
#define ovrError_MetricsSessionNotActive -90010,
#endif

// 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7)
#define ovrButton_A 0x00000001
#define ovrTouch_A ovrButton_A
#define ovrButton_B 0x00000002
#define ovrTouch_B ovrButton_B
#define ovrButton_RThumb 0x00000004
#define ovrTouch_RThumb ovrButton_RThumb
#define ovrButton_RShoulder 0x00000008
#define ovrTouch_RIndexTrigger 0x00000010
#define ovrTouch_RIndexPointing 0x00000020
#define ovrTouch_RThumbUp 0x00000040
#define ovrButton_X 0x00000100
#define ovrTouch_X ovrButton_X
#define ovrButton_Y 0x00000200
#define ovrTouch_Y ovrButton_Y
#define ovrButton_LThumb 0x00000400
#define ovrTouch_LThumb ovrButton_LThumb
#define ovrButton_LShoulder 0x00000800
#define ovrTouch_LIndexTrigger 0x00001000
#define ovrTouch_LIndexPointing 0x00002000
#define ovrTouch_LThumbUp 0x00004000
#define ovrButton_Up 0x00010000
#define ovrButton_Down 0x00020000
#define ovrButton_Left 0x00040000
#define ovrButton_Right 0x00080000
#define ovrButton_Enter 0x00100000
#define ovrButton_Back 0x00200000
#endif
// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrButton_Private 0x01C00000
#endif
// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrButton_RMask 0x0000000F
#define ovrTouch_RButtonMask 0x0000001F
#define ovrTouch_RPoseMask 0x00000060
#define ovrButton_LMask 0x00000F00
#define ovrTouch_LButtonMask 0x00001F00
#define ovrTouch_LPoseMask 0x00006000
#define ovrButton_VolUp 0x00400000
#define ovrButton_VolDown 0x00800000
#define ovrButton_Home 0x01000000
#endif
// 1.5 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION < 1 || OVR_MAJOR_VERSION < 1 ||                \
    OVR_MINOR_VERSION < 5
#define ovrTouch_RThumbRest 0x00000008
#define ovrTouch_LThumbRest 0x00000800
#endif

// 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7)
#define ovrControllerType_LTouch 0x01
#define ovrControllerType_RTouch 0x02
#define ovrControllerType_Touch 0x03
#endif
// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrControllerType_Remote 0x04
#endif
// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrControllerType_XBox 0x10
#endif
// 0.7 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 7
#define ovrControllerType_All 0xff
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define ovrLayerType_Disabled 0
#define ovrLayerType_EyeFov 1
#endif
// 0.6 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6
#define ovrLayerType_EyeFovDepth 2
#endif
// 0.6 to 0.7
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6 ||                \
    OVR_MAJOR_VERSION > 7
#define ovrLayerType_QuadInWorld 3
#define ovrLayerType_QuadHeadLocked 4
#endif
// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrLayerType_Quad 3
#define ovrLayerType_EyeMatrix 5
#endif
// 0.6 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6
#define ovrLayerType_Direct 6
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define ovrLayerFlag_HighQuality 1
#define ovrLayerFlag_TextureOriginAtBottomLeft 2
#endif
// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrLayerFlag_HeadLocked 4
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define ovrPerfHud_Off 0
// 0.6 and above but different values
#define ovrPerfHud_LatencyTiming 1
#endif
// 0.6 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6
#define ovrPerfHud_RenderTiming 2
#endif
// 0.7 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 7
#define ovrPerfHud_PerfHeadroom 3
#endif
// 0.7 and above but different values
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7)
#define ovrPerfHud_VersionInfo 4
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
// 0.6 and above but different values
#define ovrPerfHud_Count 5
#endif

// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
#define ovrPerfHud_PerfSummary 1
#define ovrPerfHud_AppRenderTiming 3
#define ovrPerfHud_CompRenderTiming 4
#endif

// we named these ourselves
#define ovrPerfHud6_Off 0
#define ovrPerfHud6_LatencyTiming 1
#define ovrPerfHud6_RenderTiming 2
#define ovrPerfHud6_PerfHeadroom 3
#define ovrPerfHud6_VersionInfo 4
#define ovrPerfHud6_Count 5
#define ovrPerfHud13_Off 0
#define ovrPerfHud13_PerfSummary 1
#define ovrPerfHud13_LatencyTiming 2
#define ovrPerfHud13_AppRenderTiming 3
#define ovrPerfHud13_CompRenderTiming 4
#define ovrPerfHud13_VersionInfo 5
#define ovrPerfHud13_Count 6

// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
#define ovrLayerHud_Off 0
#define ovrLayerHud_Info 1

// 0.8 and above (but different value on 0.8)
#define ovrMaxLayerCount 16
#endif

// 0.7 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 7
#define ovrSwapTextureSetD3D11_Typeless 1
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
#define OVR_SUCCESS(x) (x >= 0)
#define OVR_FAILURE(x) (x < 0)

typedef int ovrResult;
#endif

// we named these ourselves
typedef float ovrScalar;
typedef void(__cdecl* ovrLogCallback5)(int level, const char* message);
typedef void(__cdecl* ovrLogCallback7)(uintptr_t userData, int level, const char* message);

// 0.5 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 5)
typedef void* ovrLogCallback;
#endif

// 0.3 and above
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
typedef char ovrBool;
typedef int ovrEyeType;
typedef struct
{
  int w, h;
} ovrSizei;
typedef struct
{
  int x, y;
} ovrVector2i;
typedef struct
{
  ovrVector2i Pos;
  ovrSizei Size;
} ovrRecti;
typedef struct
{
  ovrScalar x, y;
} ovrVector2f;
typedef struct
{
  ovrScalar x, y, z;
} ovrVector3f;
typedef struct
{
  ovrScalar x, y, z, w;
} ovrQuatf;

typedef struct
{
  ovrScalar UpTan, DownTan, LeftTan, RightTan;
} ovrFovPort;

#endif

// It's only valid to access the contents of this struct on 0.5
typedef struct
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
    // 0.3 and above
    ovrHmdStruct
#endif
{
  void* internal;
  int Type;
  const char *ProductName, *Manufacturer;
  short VendorId, ProductId;
  char SerialNumber[24];
  short FirmwareMajor, FirmwareMinor;
  ovrScalar CameraFrustumHFovInRadians, CameraFrustumVFovInRadians, CameraFrustumNearZInMeters,
      CameraFrustumFarZInMeters;
  unsigned int HmdCaps, TrackingCaps, DistortionCaps;
  ovrFovPort DefaultEyeFov[2], MaxEyeFov[2];
  int EyeRenderOrder[2];
  ovrSizei Resolution;
  ovrVector2i WindowsPos;
  char* DisplayDeviceName;
  int DisplayId;
} ovrHmdDesc5
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
    // 0.3 to 0.8, but typedefed on 1.3 and above
    ,
    *ovrHmd
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
    // 0.8 and above
    ,
    *ovrSession
#endif
    ;

typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  void* internal;
  int Type;
#ifdef _WIN64
  int padding;
#endif
  const char *ProductName, *Manufacturer;
  short VendorId, ProductId;
  char SerialNumber[24];
  short FirmwareMajor, FirmwareMinor;
  ovrScalar CameraFrustumHFovInRadians, CameraFrustumVFovInRadians, CameraFrustumNearZInMeters,
      CameraFrustumFarZInMeters;
  unsigned int HmdCaps, TrackingCaps;
  ovrFovPort DefaultEyeFov[2], MaxEyeFov[2];
  int EyeRenderOrder[2];
  ovrSizei Resolution;
} ovrHmdDesc6;

typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  int Type;
#ifdef _WIN64
  int padding;
#endif
  char ProductName[64], Manufacturer[64];
  short VendorId, ProductId;
  char SerialNumber[24];
  short FirmwareMajor, FirmwareMinor;
  ovrScalar CameraFrustumHFovInRadians, CameraFrustumVFovInRadians, CameraFrustumNearZInMeters,
      CameraFrustumFarZInMeters;
  unsigned int AvailableHmdCaps, DefaultHmdCaps, AvailableTrackingCaps, DefaultTrackingCaps;
  ovrFovPort DefaultEyeFov[2], MaxEyeFov[2];
  ovrSizei Resolution;
  ovrScalar DisplayRefreshRate;
#ifdef _WIN64
  char padding2[4];
#endif
} ovrHmdDesc7;

typedef struct
{
  void* internal;
  int Type;
  char ProductName[64], Manufacturer[64];
  short VendorId, ProductId;
  char SerialNumber[24];
  short FirmwareMajor, FirmwareMinor;
  ovrScalar CameraFrustumHFovInRadians, CameraFrustumVFovInRadians, CameraFrustumNearZInMeters,
      CameraFrustumFarZInMeters;
  unsigned int AvailableHmdCaps, DefaultHmdCaps, AvailableTrackingCaps, DefaultTrackingCaps,
      HmdCaps, TrackingCaps, DistortionCaps;
  ovrFovPort DefaultEyeFov[2], MaxEyeFov[2];
  int EyeRenderOrder[2];
  ovrSizei Resolution;
  ovrScalar DisplayRefreshRate;
  ovrVector2i WindowsPos;
  char DisplayDeviceName[128];
  int DisplayId;
}
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
// 0.3 and above
ovrHmdDesc,
#endif
    ovrHmdDescComplete;

// 0.3 and above
#if !defined(HAVE_OCULUSSDK) ||                                                                    \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
typedef struct ALIGN_TO_FOUR_BYTE_BOUNDARY
{
  int Eye;
  ovrFovPort Fov;
  ovrRecti DistortedViewport;
  ovrVector2f PixelsPerTanAngleAtCenter;
  union {
    ovrVector3f HmdToEyeViewOffset;
    ovrVector3f HmdToEyeOffset;
    ovrVector3f ViewAdjust;
  };
} ovrEyeRenderDesc;
#endif

// 0.3 to 0.5
#if !defined(OVR_MAJOR_VERSION) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||             \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  ovrScalar DeltaSeconds, unused;
  double ThisFrameSeconds, TimewarpPointSeconds, NextFrameSeconds, ScanoutMidpointSeconds,
      EyeScanoutSeconds[2];
} ovrFrameTiming5;
#else
#define ovrFrameTiming5 ovrFrameTiming
#endif

// 0.6 to 0.7 - there is no ovrFrameTiming in SDK 0.8 and above
#if !defined(OVR_MAJOR_VERSION) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6 ||             \
    OVR_MAJOR_VERSION > 7
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  double DisplayMidpointSeconds, FrameIntervalSeconds;
  unsigned AppFrameIndex, DisplayFrameIndex;
} ovrFrameTiming6;
#else
#define ovrFrameTiming6 ovrFrameTiming
#endif

// 0.3 and above
#if !defined(OVR_MAJOR_VERSION) ||                                                                 \
    (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
typedef struct
{
  ovrQuatf Orientation;
  ovrVector3f Position;
} ovrPosef;

typedef struct
{
  ovrScalar M[4][4];
} ovrMatrix4f;
#endif

// 0.4 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  char Displayed, unused[7];
  double StartTime, DismissibleTime;
} ovrHSWDisplayState;
#endif

// 0.5 to 0.6
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 6 ||                \
    OVR_MAJOR_VERSION < 5
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  unsigned Flags, RequestedMinorVersion;
  ovrLogCallback LogCallback;
  unsigned ConnectionTimeoutMS;
  unsigned padding[3];
} ovrInitParams5;
#else
#define ovrInitParams5 ovrInitParams
#endif

#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 7
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  unsigned Flags, RequestedMinorVersion;
  ovrLogCallback7 LogCallback;
  uintptr_t UserData;
  unsigned ConnectionTimeoutMS;
  unsigned padding[1];
} ovrInitParams7;
#else
#define ovrInitParams7 ovrInitParams
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
typedef struct
{
  ovrResult Result;
  char ErrorString[512];
} ovrErrorInfo;
#endif

// 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_MAJOR_VERSION > 5 || OVR_PRODUCT_VERSION > 0

typedef struct
{
  int API;
  union {
    ovrSizei BackBufferSize;
    ovrSizei RTSize;
  };
  int Multisample;
} ovrRenderAPIConfigHeader;

typedef struct
{
  ovrRenderAPIConfigHeader Header;
  void* PlatformData[8];
} ovrRenderAPIConfig;

typedef struct
{
  int API;
  ovrSizei TextureSize;
  ovrRecti RenderViewport;
} ovrTextureHeader5;

typedef struct
{
  ovrTextureHeader5 Header;
  void* PlatformData[8];
} ovrTexture5;

#else

typedef ovrTextureHeader ovrTextureHeader5;
typedef ovrTexture ovrTexture5;

#endif

#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6

typedef struct
{
  int API;
  ovrSizei TextureSize;
} ovrTextureHeader6;

typedef struct
{
  ovrTextureHeader6 Header;
#ifdef _WIN64
  unsigned padding;
#endif
  void* PlatformData[8];
} ovrTexture6;

typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrTexture6* Textures;
  int TextureCount, CurrentIndex;
} ovrSwapTextureSet;

#else

typedef ovrTextureHeader ovrTextureHeader6;
typedef ovrTexture ovrTexture6;

#endif

// 0.4 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 ||                                         \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
typedef struct
{
  ovrVector3f Accelerometer, Gyro, Magnetometer;
  ovrScalar Temperature, TimeInSeconds;
} ovrSensorData;
#endif

// unchanged from 0.5 to 1.5
#ifndef HAVE_OCULUSSDK
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  ovrPosef ThePose;
  ovrVector3f AngularVelocity, LinearVelocity, AngularAcceleration, LinearAcceleration;
  ovrScalar unused;
  double TimeInSeconds;
} ovrPoseStatef;
#endif

// 0.5 (or below) to 0.6
#if !defined(HAVE_OCULUSSDK) || OVR_MAJOR_VERSION > 6 || OVR_PRODUCT_VERSION > 0
typedef struct
{
  ovrPoseStatef HeadPose;
  ovrPosef CameraPose, LeveledCameraPose;
  ovrSensorData RawSensorData;
  unsigned StatusFlags, LastCameraFrameCounter, unusued;
} ovrTrackingState5;
#else
typedef ovrTrackingState ovrTrackingState5;
#endif

#if !defined(HAVE_OCULUSSDK) || OVR_MAJOR_VERSION != 7 || OVR_PRODUCT_VERSION > 0
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  ovrPoseStatef HeadPose;
  ovrPosef CameraPose, LeveledCameraPose;
  ovrPoseStatef HandPoses[2];
  ovrSensorData RawSensorData;
  unsigned StatusFlags, LastCameraFrameCounter, unusued;
} ovrTrackingState7;
#else
typedef ovrTrackingState ovrTrackingState7;
#endif

#if !defined(HAVE_OCULUSSDK) || OVR_MAJOR_VERSION < 8 || OVR_PRODUCT_VERSION > 0
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  ovrPoseStatef HeadPose;
  ovrPosef CameraPose, LeveledCameraPose;
  ovrPoseStatef HandPoses[2];
  ovrSensorData RawSensorData;
  unsigned StatusFlags, HandStatusFlags[2], LastCameraFrameCounter, unusued;
} ovrTrackingState8;
#else
typedef ovrTrackingState ovrTrackingState8;
#endif

#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
typedef struct ALIGN_TO_EIGHT_BYTE_BOUNDARY
{
  ovrPoseStatef HeadPose;
  unsigned StatusFlags;
  ovrPoseStatef HandPoses[2];
  ovrSensorData RawSensorData;
  unsigned HandStatusFlags[2];
  ovrPosef CalibratedOrigin;
} ovrTrackingState13;
#else
typedef ovrTrackingState ovrTrackingState13;
#endif

// 0.7 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 7
typedef struct
{
  double TimeInSeconds;
  unsigned ConnectedControllerTypes, Buttons, Touches;
  ovrScalar IndexTrigger[2], HandTrigger[2];
  ovrVector2f Thumbstick[2];
} ovrInputState7;
#else
typedef ovrInputState ovrInputState7;
#endif

// 1.3 and above
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
typedef struct
{
  double TimeInSeconds;
  unsigned Buttons, Touches;
  ovrScalar IndexTrigger[2], HandTrigger[2];
  ovrVector2f Thumbstick[2];
} ovrInputState13;
#else
typedef ovrInputState ovrInputState13;
#endif

// runtime 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 8
typedef struct
{
  char HasVrFocus, HmdPresent;
} ovrSessionStatus8;
#else
typedef ovrSessionStatus ovrSessionStatus8;
#endif

// runtime 1.3, but backwards compatible with runtime 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION == 0
typedef struct
{
  char HasVrFocus, HmdPresent, HmdMounted, DisplayLost, ShouldQuit, ShouldRecenter;
} ovrSessionStatus13;
#else
typedef ovrSessionStatus ovrSessionStatus13;
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
typedef ovrSessionStatus13 ovrSessionStatus;
#endif

// 0.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
typedef struct
{
  ovrVector2f ScreenPosNDC;
  ovrScalar TimeWarpFactor, VignetteFactor;
  ovrVector2f TanEyeAnglesR, TanEyeAnglesG, TanEyeAnglesB;
} ovrDistortionVertex;

typedef struct
{
  ovrDistortionVertex* pVertexData;
  unsigned short* pIndexData;
  unsigned VertexCount, IndexCount;
} ovrDistortionMesh;
#endif

// 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION < 7 && OVR_PRODUCT_VERSION == 0)

typedef struct
{
  unsigned char platform_dependent[8];
} ovrGraphicsLuid;

#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
typedef struct ALIGN_TO_FOUR_BYTE_BOUNDARY
{
  ovrScalar Projection22, Projection23, Projection32;
} ovrTimewarpProjectionDesc;

typedef struct ALIGN_TO_FOUR_BYTE_BOUNDARY
{
  ovrVector3f HmdToEyeViewOffset[2];
  ovrScalar HmdSpaceToWorldScaleInMeters;
} ovrViewScaleDesc;

typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  unsigned Type, Flags;
} ovrLayerHeader;

#endif

#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION != 6
typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrLayerHeader Header;
  ovrSwapTextureSet* ColorTexture[2];
  ovrRecti Viewport[2];
  ovrFovPort Fov[2];
  ovrPosef RenderPose[2];
} ovrLayerEyeFov6;

typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrLayerHeader Header;
  ovrSwapTextureSet* ColorTexture[2];
  ovrRecti Viewport[2];
  ovrFovPort Fov[2];
  ovrPosef RenderPose[2];
  ovrSwapTextureSet* DepthTexture[2];
  ovrTimewarpProjectionDesc ProjectionDesc;
} ovrLayerEyeFovDepth6;

#else
typedef ovrLayerEyeFov ovrLayerEyeFov6;
typedef ovrLayerEyeFovDepth ovrLayerEyeFovDepth6;
#endif

#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 8
// runtime 0.8 but backwards compatible
typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrLayerHeader Header;
  ovrSwapTextureSet* ColorTexture[2];
  ovrRecti Viewport[2];
  ovrFovPort Fov[2];
  ovrPosef RenderPose[2];
  double SensorSampleTime;
} ovrLayerEyeFov8;

// not backwards compatible
typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrLayerHeader Header;
  ovrSwapTextureSet* ColorTexture[2];
  ovrRecti Viewport[2];
  ovrFovPort Fov[2];
  ovrPosef RenderPose[2];
  double SensorSampleTime;
  ovrSwapTextureSet* DepthTexture[2];
  ovrTimewarpProjectionDesc ProjectionDesc;
} ovrLayerEyeFovDepth8;
#else
typedef ovrLayerEyeFov ovrLayerEyeFov8;
typedef ovrLayerEyeFovDepth ovrLayerEyeFovDepth8;
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
typedef ovrLayerEyeFov8 ovrLayerEyeFov;
#endif

// 0.8 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 8)
typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrLayerHeader Header;
  ovrSwapTextureSet* ColorTexture[2];
  ovrRecti Viewport[2];
  ovrPosef RenderPose[2];
  ovrMatrix4f Matrix[2];
  double SensorSampleTime;
} ovrLayerEyeMatrix;
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrLayerHeader Header;
  ovrSwapTextureSet* ColorTexture;
  ovrRecti Viewport;
  ovrPosef QuadPoseCenter;
  ovrVector2f QuadSize;
} ovrLayerQuad;
#endif

// 0.6 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6
typedef struct ALIGN_TO_POINTER_BOUNDARY
{
  ovrLayerHeader Header;
  ovrSwapTextureSet* ColorTexture[2];
  ovrRecti Viewport[2];
} ovrLayerDirect;
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 6)
typedef union {
  ovrLayerHeader Header;
  ovrLayerEyeFov EyeFov;
  ovrLayerEyeFovDepth6 EyeFovDepth;
  ovrLayerEyeFovDepth8 EyeFovDepth8;
  ovrLayerQuad Quad;
  ovrLayerDirect Direct;
} ovrLayer_Union;
#endif

// 0.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
#ifdef _WIN32

typedef struct
{
  ovrRenderAPIConfigHeader Header;
  HWND Window;
  HDC DC;
} ovrGLConfigData;

#elif defined(__linux__)

typedef struct
{
  ovrRenderAPIConfigHeader Header;
  struct _XDisplay* Disp;
} ovrGLConfigData;

#else

typedef struct
{
  ovrRenderAPIConfigHeader Header;
} ovrGLConfigData;

#endif

typedef union {
  ovrGLConfigData OGL;
  ovrRenderAPIConfig Config;
} ovrGLConfig;

#endif

#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5

typedef struct
{
  ovrTextureHeader5 Header;
  unsigned TexId;
} ovrGLTextureData5;

typedef union {
  ovrGLTextureData5 OGL;
  ovrTexture5 Texture;
} ovrGLTexture5;

#else

#define ovrGLTextureData5 ovrGLTextureData
#define ovrGLTexture5 ovrGLTexture

#endif

// 0.6 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6

typedef struct
{
  ovrTextureHeader6 Header;
  unsigned TexId;
} ovrGLTextureData6;

typedef union {
  ovrGLTextureData6 OGL;
  ovrTexture6 Texture;
} ovrGLTexture6;

#else

#define ovrGLTextureData6 ovrGLTextureData
#define ovrGLTexture6 ovrGLTexture

#endif

typedef ovrResult(__cdecl* PFUNC_INIT)(void* InitParams);
typedef void(__cdecl* PFUNC_VOID)(void);
typedef ovrResult(__cdecl* PFUNC_CHAR)(void);
typedef ovrResult(__cdecl* PFUNC_CHAR_INT)(int);
typedef char*(__cdecl* PFUNC_PCHAR)(void);
typedef int(__cdecl* PFUNC_INT)(void);
typedef ovrHmd(__cdecl* PFUNC_HMD_INT)(int number);
typedef ovrHmd(__cdecl* PFUNC_HMD_INT32)(int version_ovrHmd);
typedef void(__cdecl* PFUNC_VOID_HMD)(ovrHmd hmd);
typedef const char*(__cdecl* PFUNC_PCHAR_HMD)(ovrHmd hmd);
typedef char(__cdecl* PFUNC_ATTACH)(ovrHmd hmd, void* hwnd, ovrRecti* dest, ovrRecti* source);
typedef unsigned(__cdecl* PFUNC_UINT_HMD)(ovrHmd hmd);
typedef void(__cdecl* PFUNC_HMD_UINT)(ovrHmd hmd, unsigned);
typedef ovrResult(__cdecl* PFUNC_HMD_INT_INT)(ovrHmd hmd, unsigned supported_ovrTrackingCap,
                                              unsigned required_ovrTrackingCap);
typedef ovrTrackingState5(__cdecl* PFUNC_TRACKING_STATE5)(ovrHmd hmd, double time);
typedef ovrSizei(__cdecl* PFUNC_FOV)(ovrHmd hmd, int eye, ovrFovPort tan_of_fovs,
                                     ovrScalar pixel_ratio);
typedef char(__cdecl* PFUNC_CONFIG)(ovrHmd hmd, const ovrRenderAPIConfig* address_of_cfg_dot_Config,
                                    unsigned flags_ovrDistortionCap,
                                    const ovrFovPort* tan_of_fovs_array,
                                    ovrEyeRenderDesc* result_array);
typedef ovrFrameTiming5(__cdecl* PFUNC_BEGIN)(ovrHmd hmd, unsigned frame_number);
typedef ovrFrameTiming6(__cdecl* PFUNC_FRAMETIMING)(ovrHmd hmd, unsigned frame_number);
typedef void(__cdecl* PFUNC_END)(ovrHmd hmd, const ovrPosef* render_pose_array,
                                 const ovrTexture5* eye_texture_array);
typedef void(__cdecl* PFUNC_EYEPOSES)(ovrHmd hmd, unsigned frame_number,
                                      const ovrVector3f* eye_offset_array_result,
                                      ovrPosef* eye_pose_array_result,
                                      ovrTrackingState7* tracking_state_result);
typedef ovrPosef(__cdecl* PFUNC_EYEPOSE)(ovrHmd hmd, int eye);
typedef ovrEyeRenderDesc(__cdecl* PFUNC_RENDERDESC)(ovrHmd hmd, int eye, ovrFovPort tan_of_fovs);
typedef char(__cdecl* PFUNC_DISTORTIONMESH)(ovrHmd hmd, int eye, ovrFovPort tan_of_fovs,
                                            unsigned flags_ovrDistortionCap,
                                            ovrDistortionMesh* mesh_result);
typedef char(__cdecl* PFUNC_DISTORTIONMESHDEBUG)(ovrHmd hmd, int eye, ovrFovPort tan_of_fovs,
                                                 unsigned flags_ovrDistortionCap,
                                                 ovrDistortionMesh* mesh_result,
                                                 ovrScalar eye_relief_in_metres);
typedef void(__cdecl* PFUNC_DESTROYMESH)(ovrDistortionMesh* mesh);
typedef void(__cdecl* PFUNC_SCALEOFFSET)(ovrFovPort tan_of_fovs, ovrSizei size_of_texture,
                                         ovrRecti viewport,
                                         ovrVector2f* scale_and_offset_array_result);
typedef double(__cdecl* PFUNC_DOUBLE)(void);
typedef double(__cdecl* PFUNC_TIMEWARP)(ovrHmd hmd, int eye, ovrPosef render_pose,
                                        ovrMatrix4f* timewarp_matrices_array_result);
typedef double(__cdecl* PFUNC_TIMEWARPDEBUG)(ovrHmd hmd, int eye, ovrPosef render_pose,
                                             ovrQuatf playerTorsoMotion,
                                             ovrMatrix4f* timewarp_matrices_array_result,
                                             double seconds_of_debug_offset);
typedef void(__cdecl* PFUNC_HMD_HSW)(ovrHmd hmd, ovrHSWDisplayState* hsw_state_result);
typedef char(__cdecl* PFUNC_CHAR_HMD)(ovrHmd hmd);
typedef char(__cdecl* PFUNC_LATENCY)(ovrHmd hmd, unsigned char* colour_rgb_array);
typedef double(__cdecl* PFUNC_DOUBLE_DOUBLE)(double time);
typedef ovrMatrix4f(__cdecl* PFUNC_PROJECTION)(ovrFovPort tan_of_fovs, ovrScalar near_z,
                                               ovrScalar far_z, unsigned flags_ovrProjection);
typedef ovrMatrix4f(__cdecl* PFUNC_ORTHO)(ovrMatrix4f projection, ovrScalar ortho_scale,
                                          ovrScalar ortho_distance,
                                          ovrScalar hmd_to_eye_horizontal_offset);
typedef ovrTimewarpProjectionDesc(__cdecl* PFUNC_TIMEWARPPROJ)(ovrMatrix4f projection,
                                                               unsigned projection_mod_flags);
typedef char(__cdecl* PFUNC_GETBOOL)(ovrHmd hmd, const char* property, char default);
typedef int(__cdecl* PFUNC_GETINT)(ovrHmd hmd, const char* property, int default);
typedef ovrScalar(__cdecl* PFUNC_GETFLOAT)(ovrHmd hmd, const char* property, float default);
typedef unsigned(__cdecl* PFUNC_GETFLOATARRAY)(ovrHmd hmd, const char* property,
                                               ovrScalar* result_array, unsigned result_array_size);
typedef const char*(__cdecl* PFUNC_GETSTRING)(ovrHmd hmd, const char* property,
                                              const char* default);
typedef char(__cdecl* PFUNC_SETBOOL)(ovrHmd hmd, const char* property, char new_value);
typedef char(__cdecl* PFUNC_SETINT)(ovrHmd hmd, const char* property, int new_value);
typedef char(__cdecl* PFUNC_SETFLOAT)(ovrHmd hmd, const char* property, ovrScalar new_value);
typedef char(__cdecl* PFUNC_SETFLOATARRAY)(ovrHmd hmd, const char* property,
                                           const ovrScalar* new_value_array,
                                           unsigned new_value_array_size);
typedef char(__cdecl* PFUNC_SETSTRING)(ovrHmd hmd, const char* property, const char* new_value);
typedef int(__cdecl* PFUNC_TRACE)(int ovrloglevel, const char* message);

typedef ovrResult(__cdecl* PFUNC_CREATE6)(int number, ovrHmd* hmd_pointer);
typedef ovrResult(__cdecl* PFUNC_CREATEMIRRORD3D116)(ovrHmd hmd, void* id3d11device,
                                                     const void* d3d11_texture2d_desc,
                                                     ovrTexture6** mirror_texture_result_pointer);
typedef ovrResult(__cdecl* PFUNC_CREATESWAPD3D116)(
    ovrHmd hmd, void* id3d11device, const void* d3d11_texture2d_desc,
    ovrSwapTextureSet** swap_texture_set_result_pointer);
typedef ovrResult(__cdecl* PFUNC_CREATEMIRRORGL)(ovrHmd hmd, unsigned gl_colour_format, int width,
                                                 int height,
                                                 ovrTexture6** mirror_texture_result_pointer);
typedef ovrResult(__cdecl* PFUNC_CREATESWAPGL)(ovrHmd hmd, unsigned gl_colour_format, int width,
                                               int height,
                                               ovrSwapTextureSet** swap_texture_set_result_pointer);
typedef ovrResult(__cdecl* PFUNC_DESTROYMIRROR)(ovrHmd hmd, ovrTexture6* mirror_texture);
typedef ovrResult(__cdecl* PFUNC_DESTROYSWAP)(ovrHmd hmd, ovrSwapTextureSet* swap_texture_set);
typedef ovrResult(__cdecl* PFUNC_CALC)(ovrPosef head_pose,
                                       const ovrVector3f hmd_to_eye_view_offset[2],
                                       ovrPosef out_eye_poses[2]);
typedef void(__cdecl* PFUNC_ERRORINFO)(ovrErrorInfo* error_info_result);
typedef ovrResult(__cdecl* PFUNC_SUBMIT)(ovrHmd hmd, unsigned frame_number,
                                         const ovrViewScaleDesc* view_scale_desc,
                                         ovrLayerHeader const* const* layer_ptr_list,
                                         unsigned layer_count);

typedef ovrHmdDesc7(__cdecl* PFUNC_HMDDESC)(ovrHmd hmd);
typedef ovrResult(__cdecl* PFUNC_CREATE7)(ovrHmd* hmd_pointer, ovrGraphicsLuid* LUID_pointer);
typedef ovrResult(__cdecl* PFUNC_CREATEMIRRORD3D117)(ovrHmd hmd, void* id3d11device,
                                                     const void* d3d11_texture2d_desc,
                                                     unsigned ovrswaptexturesetd3d11_typeless_flag,
                                                     ovrTexture6** mirror_texture_result_pointer);
typedef ovrResult(__cdecl* PFUNC_CREATESWAPD3D117)(
    ovrHmd hmd, void* id3d11device, const void* d3d11_texture2d_desc,
    unsigned ovrswaptexturesetd3d11_typeless_flag,
    ovrSwapTextureSet** swap_texture_set_result_pointer);
typedef ovrResult(__cdecl* PFUNC_INPUT)(ovrHmd hmd, unsigned controller_type_mask,
                                        ovrInputState7* input_state_result);
typedef ovrResult(__cdecl* PFUNC_INPUT13)(ovrHmd hmd, unsigned controller_type_mask,
                                          ovrInputState13* input_state_result);
typedef ovrResult(__cdecl* PFUNC_LOOKUP)(const char* name, void** data_result_pointer);
typedef ovrResult(__cdecl* PFUNC_VIBE)(ovrHmd hmd, unsigned controller_types_mask,
                                       ovrScalar frequency_zero_half_or_one,
                                       ovrScalar amplitude_zero_to_one);
typedef ovrTrackingState7(__cdecl* PFUNC_TRACKING_STATE7)(ovrHmd hmd, double time);
typedef ovrTrackingState8(__cdecl* PFUNC_TRACKING_STATE8)(ovrHmd hmd, double time,
                                                          bool latency_marker);
typedef void(__cdecl* PFUNC_EYEPOSES7)(ovrHmd hmd, unsigned frame_number,
                                       const ovrVector3f* eye_offset_array_result,
                                       ovrPosef* eye_pose_array_result,
                                       ovrTrackingState7* tracking_state_result);
typedef ovrResult(__cdecl* PFUNC_SESSIONSTATUS)(ovrHmd hmd, ovrSessionStatus* session_status);
typedef ovrResult(__cdecl* PFUNC_SUBMIT8)(ovrHmd hmd, long long frame_number,
                                          const ovrViewScaleDesc* view_scale_desc,
                                          ovrLayerHeader const* const* layer_ptr_list,
                                          unsigned layer_count);
typedef double(__cdecl* PFUNC_DISPLAYTIME)(ovrHmd hmd, long long frame_number);

#ifndef HAVE_OCULUSSDK

// These functions will work without the DLL
bool ovr_InitializeRenderingShimVersion(int MinorVersion);
bool ovr_InitializeRenderingShim(void);
ovrResult ovr_Initialize(void* initParams = nullptr);
void ovr_Shutdown(void);
ovrHmd ovrHmd_Create(int number);
ovrHmd ovrHmd_CreateDebug(int version_ovrHmd);
ovrResult ovrHmd_Create(int number, ovrHmd* hmd_pointer);
ovrResult ovrHmd_CreateDebug(int version_ovrHmd, ovrHmd* hmd_pointer);
ovrResult ovr_Create(ovrHmd* hmd_pointer, ovrGraphicsLuid* LUID_pointer);
int ovrHmd_Detect(void);
const char* ovr_GetVersionString(void);
ovrHmdDescComplete ovr_GetHmdDesc(ovrHmd hmd);
ovrResult ovrHmd_ConfigureTracking(ovrHmd hmd, unsigned supported_ovrTrackingCap,
                                   unsigned required_ovrTrackingCap);
ovrResult ovrHmd_DismissHSWDisplay(ovrHmd hmd);

// These functions will crash unless ovr_Initialize returned successfully
extern PFUNC_VOID_HMD ovrHmd_RecenterPose, ovrHmd_EndFrameTiming, ovrHmd_Destroy,
    ovr_ResetBackOfHeadTracking, ovr_ResetMulticameraTracking;
extern PFUNC_PCHAR_HMD ovrHmd_GetLastError, ovrHmd_GetLatencyTestResult;
extern PFUNC_UINT_HMD ovrHmd_GetEnabledCaps;
extern PFUNC_HMD_UINT ovrHmd_SetEnabledCaps, ovrHmd_ResetFrameTiming;
extern PFUNC_TRACKING_STATE5 ovrHmd_GetTrackingState;
extern PFUNC_FOV ovrHmd_GetFovTextureSize;
extern PFUNC_BEGIN ovrHmd_BeginFrameTiming;
extern PFUNC_EYEPOSES ovrHmd_GetEyePoses;
extern PFUNC_EYEPOSE ovrHmd_GetHmdPosePerEye;
extern PFUNC_RENDERDESC ovrHmd_GetRenderDesc;
extern PFUNC_DISTORTIONMESH ovrHmd_CreateDistortionMesh;
extern PFUNC_DISTORTIONMESHDEBUG ovrHmd_CreateDistortionMeshDebug;
extern PFUNC_DESTROYMESH ovrHmd_DestroyDistortionMesh;
extern PFUNC_SCALEOFFSET ovrHmd_GetRenderScaleAndOffset;
extern PFUNC_TIMEWARP ovrHmd_GetEyeTimewarpMatrices;
extern PFUNC_TIMEWARPDEBUG ovrHmd_GetEyeTimewarpMatricesDebug;
extern PFUNC_DOUBLE ovr_GetTimeInSeconds;
extern PFUNC_HMD_HSW ovrHmd_GetHSWDisplayState;
extern PFUNC_LATENCY ovrHmd_ProcessLatencyTest, ovrHmd_GetLatencyTest2DrawColor;
extern PFUNC_PROJECTION ovrMatrix4f_Projection;
extern PFUNC_GETBOOL ovrHmd_GetBool;
extern PFUNC_GETFLOAT ovrHmd_GetFloat;
extern PFUNC_GETFLOATARRAY ovrHmd_GetFloatArray;
extern PFUNC_GETINT ovrHmd_GetInt;
extern PFUNC_GETSTRING ovrHmd_GetString;
extern PFUNC_SETBOOL ovrHmd_SetBool;
extern PFUNC_SETFLOAT ovrHmd_SetFloat;
extern PFUNC_SETFLOATARRAY ovrHmd_SetFloatArray;
extern PFUNC_SETINT ovrHmd_SetInt;
extern PFUNC_SETSTRING ovrHmd_SetString;
extern PFUNC_TRACE ovr_TraceMessage;
extern PFUNC_ORTHO ovrMatrix4f_OrthoSubProjection;
extern PFUNC_TIMEWARPPROJ ovrTimewarpProjectionDesc_FromProjection;
extern PFUNC_CREATEMIRRORGL ovrHmd_CreateMirrorTextureGL;

#endif

// 0.3 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
extern PFUNC_CONFIG ovrHmd_ConfigureRendering;
extern PFUNC_BEGIN ovrHmd_BeginFrame;
extern PFUNC_END ovrHmd_EndFrame;
#endif

// 0.3 to 0.6, but 0.6 version is different
#if !defined(OVR_MAJOR_VERSION) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 6 ||             \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
extern PFUNC_BEGIN ovrHmd_GetFrameTiming;
#endif

// 0.3 to 0.6
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 6 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 3)
extern PFUNC_DOUBLE_DOUBLE ovr_WaitTillTime;
#endif

// 0.4 only
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 0 ||                \
    OVR_MINOR_VERSION < 4
void ovrhmd_EnableHSWDisplaySDKRender(ovrHmd hmd, char enabled);
#endif

// 0.4 to 0.5
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION > 5 ||                \
    (OVR_MAJOR_VERSION == 0 && OVR_MINOR_VERSION < 4)
extern PFUNC_ATTACH ovrHmd_AttachToWindow;
#endif

#if !defined(HAVE_OCULUSSDK) || OVR_MAJOR_VERSION != 6 || OVR_PRODUCT_VERSION > 0
extern PFUNC_CREATEMIRRORD3D116 ovrHmd_CreateMirrorTextureD3D11;
extern PFUNC_CREATESWAPD3D116 ovrHmd_CreateSwapTextureSetD3D11;
#endif

// 0.6 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_MAJOR_VERSION < 6 && OVR_PRODUCT_VERSION == 0)
extern PFUNC_DESTROYMIRROR ovrHmd_DestroyMirrorTexture;
extern PFUNC_DESTROYSWAP ovrHmd_DestroySwapTextureSet;
extern PFUNC_SUBMIT ovrHmd_SubmitFrame;
#endif

// 0.6 to 0.7
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6 ||                \
    OVR_MAJOR_VERSION > 7
extern PFUNC_FRAMETIMING ovr_GetFrameTiming;
#endif

// no SDK, or SDK < 6 or SDK > 8: our ovrHmd_CreateSwapTextureSetGL and #defined
// ovr_CreateSwapTextureSetGL
// SDK 0.6: #defined ovr_CreateSwapTextureSetGL
// SDK 0.7, 0.8: #defined ovrHmd_CreateSwapTextureSetGL
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 6
extern PFUNC_CREATESWAPGL ovrHmd_CreateSwapTextureSetGL;
#endif
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 6)
#define ovr_CreateSwapTextureSetGL ovrHmd_CreateSwapTextureSetGL
#endif
#if defined(HAVE_OCULUSSDK) && OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION >= 7
#define ovrHmd_CreateSwapTextureSetGL ovr_CreateSwapTextureSetGL
#endif

// 0.6 only
#if defined(HAVE_OCULUSSDK) && OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION == 6
#define ovr_GetFrameTiming ovrHmd_GetFrameTiming
#endif

// 0.7 and above
#if !defined(HAVE_OCULUSSDK) || (OVR_PRODUCT_VERSION == 0 && OVR_MAJOR_VERSION < 7)
extern PFUNC_CREATESWAPD3D117 ovr_CreateSwapTextureSetD3D11;
extern PFUNC_CREATEMIRRORD3D117 ovr_CreateMirrorTextureD3D11;
#endif

#ifndef HAVE_OCULUSSDK

extern PFUNC_CALC ovr_CalcEyePoses;
extern PFUNC_ERRORINFO ovr_GetLastErrorInfo;

extern PFUNC_INPUT ovr_GetInputState;
extern PFUNC_LOOKUP ovr_Lookup;
extern PFUNC_VIBE ovr_SetControllerVibration;
extern PFUNC_TRACKING_STATE7 ovr_GetTrackingState;

extern PFUNC_SESSIONSTATUS ovr_GetSessionStatus;
extern PFUNC_UINT_HMD ovr_GetTrackingCaps;
extern PFUNC_TRACKING_STATE8 ovr_GetTrackingState8;
extern PFUNC_SUBMIT8 ovr_SubmitFrame8;
extern PFUNC_DISPLAYTIME ovr_GetPredictedDisplayTime;

#define ovr_Destroy ovrHmd_Destroy
#define ovr_ConfigureTracking ovrHmd_ConfigureTracking
#define ovr_DestroyMirrorTexture ovrHmd_DestroyMirrorTexture
#define ovr_DestroySwapTextureSet ovrHmd_DestroySwapTextureSet
#define ovr_CreateMirrorTextureGL ovrHmd_CreateMirrorTextureGL
#define ovr_GetBool ovrHmd_GetBool
#define ovr_GetFloat ovrHmd_GetFloat
#define ovr_GetFloatArray ovrHmd_GetFloatArray
#define ovr_GetInt ovrHmd_GetInt
#define ovr_GetString ovrHmd_GetString
#define ovr_SetBool ovrHmd_SetBool
#define ovr_SetFloat ovrHmd_SetFloat
#define ovr_SetFloatArray ovrHmd_SetFloatArray
#define ovr_SetInt ovrHmd_SetInt
#define ovr_SetString ovrHmd_SetString
#define ovr_GetEnabledCaps ovrHmd_GetEnabledCaps
#define ovr_SetEnabledCaps ovrHmd_SetEnabledCaps
#define ovr_GetFovTextureSize ovrHmd_GetFovTextureSize
#define ovr_RecenterPose ovrHmd_RecenterPose
#define ovr_SubmitFrame ovrHmd_SubmitFrame
#define ovr_GetRenderDesc ovrHmd_GetRenderDesc
#define ovrHmd_GetFrameTiming6 ovr_GetFrameTiming

#endif

// 0.7 to 0.8
#if !defined(HAVE_OCULUSSDK) || OVR_PRODUCT_VERSION > 0 || OVR_MAJOR_VERSION < 7
#define ovr_CreateSwapTextureSetGL ovrHmd_CreateSwapTextureSetGL
#endif

// 0.13 and above
#ifndef HAVE_OCULUSSDK

namespace OVR
{
typedef ovrSizei Sizei;
enum Axis
{
  Axis_X,
  Axis_Y,
  Axis_Z
};
class Quatf
{
public:
  float x, y, z, w;

  Quatf() : x(0), y(0), z(0), w(1) {}
  Quatf(const ovrQuatf& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
  template <Axis AXIS1, Axis AXIS2, Axis AXIS3>
  void GetEulerAngles(float* axis1_result, float* axis2_result, float* axis3_result) const
  {
    float sign = ((AXIS2 == (AXIS1 + 1) % 3) && (AXIS3 == (AXIS2 + 1) % 3)) ? 1 : -1;
    float Q[3] = {x, y, z};
    float sin_axis_2 = sign * 2 * (Q[AXIS1] * Q[AXIS3] + sign * w * Q[AXIS2]);
    float epsilon = 1e-7f;
    if (sin_axis_2 > 1.0f - epsilon)
    {
      *axis1_result = 0.0f;
      *axis2_result = (float)(3.14159265358979323846 / 2);
      *axis3_result =
          atan2(2 * (sign * Q[AXIS1] * Q[AXIS2] + w * Q[AXIS3]),
                w * w + Q[AXIS2] * Q[AXIS2] - Q[AXIS1] * Q[AXIS1] - Q[AXIS3] * Q[AXIS3]);
    }
    else if (sin_axis_2 < -1.0f + epsilon)
    {
      *axis1_result = 0.0f;
      *axis2_result = (float)(-3.14159265358979323846 / 2);
      *axis3_result =
          atan2(2 * (sign * Q[AXIS1] * Q[AXIS2] + w * Q[AXIS3]),
                w * w + Q[AXIS2] * Q[AXIS2] - Q[AXIS1] * Q[AXIS1] - Q[AXIS3] * Q[AXIS3]);
    }
    else
    {
      *axis1_result =
          -atan2(-2 * (w * Q[AXIS1] - sign * Q[AXIS2] * Q[AXIS3]),
                 w * w + Q[AXIS3] * Q[AXIS3] - Q[AXIS1] * Q[AXIS1] - Q[AXIS2] * Q[AXIS2]);
      *axis2_result = asin(sin_axis_2);
      *axis3_result =
          atan2(2 * (w * Q[AXIS3] - sign * Q[AXIS1] * Q[AXIS2]),
                w * w + Q[AXIS1] * Q[AXIS1] - Q[AXIS2] * Q[AXIS2] - Q[AXIS3] * Q[AXIS3]);
    }
  }
};

class Posef
{
public:
  Posef() {}
  Posef(const Posef& other) : Rotation(other.Rotation), Translation(other.Translation) {}
  Posef(const ovrPosef& other) : Rotation(other.Orientation), Translation(other.Position) {}
  Quatf Rotation;
  ovrVector3f Translation;
};
}

#define OVR_PRODUCT_VERSION 0
#define OVR_MAJOR_VERSION 5
#define OVR_MINOR_VERSION 0
#define OVR_BUILD_VERSION 0
#define OVR_PATCH_VERSION 1
#define OVR_BUILD_NUMBER 0
#define OVR_VERSION_STRING "0.5.0.1"

#endif