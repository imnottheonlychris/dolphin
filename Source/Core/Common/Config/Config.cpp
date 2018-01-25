// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>
#include <list>
#include <map>
#include <tuple>

#include "Common/Assert.h"
#include "Common/Config/Config.h"

namespace Config
{
static Layers s_layers;
static std::list<ConfigChangedCallback> s_callbacks;

void InvokeConfigChangedCallbacks();

Section* GetOrCreateSection(System system, const std::string& section_name)
{
  return s_layers[LayerType::Meta]->GetOrCreateSection(system, section_name);
}

Layers* GetLayers()
{
  return &s_layers;
}

void AddLayer(std::unique_ptr<Layer> layer)
{
  s_layers[layer->GetLayer()] = std::move(layer);
  InvokeConfigChangedCallbacks();
}

void AddLayer(std::unique_ptr<ConfigLayerLoader> loader)
{
  AddLayer(std::make_unique<Layer>(std::move(loader)));
}

Layer* GetLayer(LayerType layer)
{
  if (!LayerExists(layer))
    return nullptr;
  return s_layers[layer].get();
}

void RemoveLayer(LayerType layer)
{
  s_layers.erase(layer);
  InvokeConfigChangedCallbacks();
}
bool LayerExists(LayerType layer)
{
  return s_layers.find(layer) != s_layers.end();
}

void AddConfigChangedCallback(ConfigChangedCallback func)
{
  s_callbacks.emplace_back(func);
}

void InvokeConfigChangedCallbacks()
{
  for (const auto& callback : s_callbacks)
    callback();
}

// Explicit load and save of layers
void Load()
{
  for (auto& layer : s_layers)
    layer.second->Load();
}

void Save()
{
  for (auto& layer : s_layers)
    layer.second->Save();
}

void Init()
{
  // These layers contain temporary values
  ClearCurrentRunLayer();
  // This layer always has to exist
  s_layers[LayerType::Meta] = std::make_unique<RecursiveLayer>();
}

void Shutdown()
{
  s_layers.clear();
  s_callbacks.clear();
}

void ClearCurrentRunLayer()
{
  s_layers[LayerType::CurrentRun] = std::make_unique<Layer>(LayerType::CurrentRun);
}

void CreateVRGameLayer()
{
  s_layers[LayerType::VRGame] = std::make_unique<Layer>(LayerType::VRGame);
}

bool OverrideSectionWithSection(const std::string& sectionName, const std::string& sectionName2)
{
  Section* section_default = s_layers[LayerType::GlobalGame] ? (s_layers[LayerType::GlobalGame]->GetOrCreateSection(Config::System::GFX, sectionName2)) : nullptr;
  Config::SectionValueMap section_values_default;
  if (section_default)
	  section_values_default = section_default->GetValues();
  Section* section_local = s_layers[LayerType::LocalGame] ? (s_layers[LayerType::LocalGame]->GetOrCreateSection(Config::System::GFX, sectionName2)) : nullptr;
  Config::SectionValueMap section_values_local;
  if (section_local)
	  section_values_local = section_local->GetValues();
  if (!section_values_default.size() && !section_values_local.size())
    return false;
  Section* section =
      s_layers[LayerType::VRGame]->GetOrCreateSection(Config::System::GFX, sectionName);
  for (const auto& value : section_values_default)
  {
    NOTICE_LOG(VR, "Override [%s] %s with game VR default [%s] %s =\"%s\"", sectionName.c_str(),
               value.first.c_str(), sectionName2.c_str(), value.first.c_str(),
               value.second.c_str());
    section->Set(value.first, value.second);
  }
  for (const auto& value : section_values_local)
  {
	  NOTICE_LOG(VR, "Override [%s] %s with game VR user setting [%s] %s =\"%s\"", sectionName.c_str(),
		  value.first.c_str(), sectionName2.c_str(), value.first.c_str(),
		  value.second.c_str());
	  section->Set(value.first, value.second);
  }
  return true;
}

static const std::map<System, std::string> system_to_name = {
    {System::Main, "Dolphin"},          {System::GCPad, "GCPad"},  {System::WiiPad, "Wiimote"},
    {System::GCKeyboard, "GCKeyboard"}, {System::GFX, "Graphics"}, {System::Logger, "Logger"},
    {System::Debugger, "Debugger"},     {System::UI, "UI"},
};

const std::string& GetSystemName(System system)
{
  return system_to_name.at(system);
}

System GetSystemFromName(const std::string& name)
{
  const auto system = std::find_if(system_to_name.begin(), system_to_name.end(),
                                   [&name](const auto& entry) { return entry.second == name; });
  if (system != system_to_name.end())
    return system->first;

  _assert_msg_(COMMON, false, "Programming error! Couldn't convert '%s' to system!", name.c_str());
  return System::Main;
}

const std::string& GetLayerName(LayerType layer)
{
  static const std::map<LayerType, std::string> layer_to_name = {
      {LayerType::Base, "Base"},
      {LayerType::GlobalGame, "Global GameINI"},
      {LayerType::LocalGame, "Local GameINI"},
      {LayerType::VRGame, "VR GameINI"},
      {LayerType::Netplay, "Netplay"},
      {LayerType::Movie, "Movie"},
      {LayerType::CommandLine, "Command Line"},
      {LayerType::CurrentRun, "Current Run"},
      {LayerType::Meta, "Top"},
  };
  return layer_to_name.at(layer);
}

bool ConfigLocation::operator==(const ConfigLocation& other) const
{
  return std::tie(system, section, key) == std::tie(other.system, other.section, other.key);
}

bool ConfigLocation::operator!=(const ConfigLocation& other) const
{
  return !(*this == other);
}

bool ConfigLocation::operator<(const ConfigLocation& other) const
{
  return std::tie(system, section, key) < std::tie(other.system, other.section, other.key);
}

LayerType GetActiveLayerForConfig(const ConfigLocation& config)
{
  for (auto layer : SEARCH_ORDER)
  {
    if (!LayerExists(layer))
      continue;

    if (GetLayer(layer)->Exists(config.system, config.section, config.key))
      return layer;
  }

  // If config is not present in any layer, base layer is considered active.
  return LayerType::Base;
}
}
