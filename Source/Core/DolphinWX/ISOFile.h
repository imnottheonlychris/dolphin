// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Common/Common.h"
#include "DiscIO/Volume.h"

#if defined(HAVE_WX) && HAVE_WX
#include <wx/image.h>
#include <wx/bitmap.h>
#endif

class PointerWrap;
class GameListItem
{
public:
	GameListItem(const std::string& _rFileName, const std::unordered_map<std::string, std::string>& custom_titles);
	~GameListItem();

	bool IsValid() const {return m_Valid;}
	const std::string& GetFileName() const {return m_FileName;}
	std::string GetName(DiscIO::IVolume::ELanguage language) const;
	std::string GetName() const;
	std::string GetDescription(DiscIO::IVolume::ELanguage language) const;
	std::string GetDescription() const;
	std::vector<DiscIO::IVolume::ELanguage> GetLanguages() const;
	std::string GetCompany() const { return m_company; }
	u16 GetRevision() const { return m_Revision; }
	const std::string& GetUniqueID() const {return m_UniqueID;}
	const std::string GetWiiFSPath() const;
	DiscIO::IVolume::ECountry GetCountry() const {return m_Country;}
	DiscIO::IVolume::EPlatform GetPlatform() const { return m_Platform; }
	const std::string& GetIssues() const { return m_issues; }
	const std::string& GetVRIssues() const { return m_vr_issues; }
	int GetEmuState() const { return m_emu_state; }
	int GetVRState() const { return m_vr_state; }
	bool IsCompressed() const { return m_BlobCompressed; }
	u64 GetFileSize() const {return m_FileSize;}
	u64 GetVolumeSize() const {return m_VolumeSize;}
	// 0 is the first disc, 1 is the second disc
	u8 GetDiscNumber() const { return m_disc_number; }

#if defined(HAVE_WX) && HAVE_WX
	const wxBitmap& GetBitmap() const {return m_Bitmap;}
#endif

	void DoState(PointerWrap &p);

private:
	std::string m_FileName;

	std::map<DiscIO::IVolume::ELanguage, std::string> m_names;
	std::map<DiscIO::IVolume::ELanguage, std::string> m_descriptions;
	std::string m_company;

	std::string m_UniqueID;

	std::string m_issues, m_vr_issues;
	int m_emu_state, m_vr_state;

	u64 m_FileSize;
	u64 m_VolumeSize;

	DiscIO::IVolume::ECountry m_Country;
	DiscIO::IVolume::EPlatform m_Platform;
	u16 m_Revision;

#if defined(HAVE_WX) && HAVE_WX
	wxBitmap m_Bitmap;
#endif
	bool m_Valid;
	bool m_BlobCompressed;
	std::vector<u8> m_pImage;
	int m_ImageWidth, m_ImageHeight;
	u8 m_disc_number;

	std::string m_custom_name;
	bool m_has_custom_name;

	bool LoadFromCache();
	void SaveToCache();

	bool IsElfOrDol() const;
	std::string CreateCacheFilename() const;

	// Outputs to m_pImage
	void ReadVolumeBanner(const DiscIO::IVolume& volume);
	// Outputs to m_Bitmap
	bool ReadPNGBanner(const std::string& path);

	static wxBitmap ScaleBanner(wxImage* image);
};
