#include "ah_common/settings.hpp"

#include "ah_common/string_util.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>

namespace ah {
namespace {

struct DefaultEntry {
  const char* path;  // '/' path from root, e.g. "JSBSim/airport/magvar"
  const char* value;
};

// Built-in defaults written to aerohub_settings.ini when the file is missing (CWD).
constexpr DefaultEntry DefaultSettings[] = {
    {.path = "ROS/domain_id", .value = "42"},
    {.path = "ROS/namespace", .value = ""},
    {.path = "ROS/rmw_implementation", .value = "rmw_fastrtps_cpp"},
    // Relative to process CWD (aero-hub/run/); real tree is aero-hub/yolo-models/.
    // Published as AERO_HUB_YOLO_MODELS.
    {.path = "ROS/yolo_models_dir", .value = "../yolo-models"},

    {.path = "Camera/video_source", .value = "synthetic"},
    {.path = "Camera/device_id", .value = "-1"},
    {.path = "Camera/device_path", .value = "synthetic"},
    {.path = "Camera/backend", .value = ""},

    {.path = "JSBSim/command_line/executable", .value = ""},
    {.path = "JSBSim/command_line/realtime", .value = "--realtime"},
    {.path = "JSBSim/command_line/aircraft", .value = "--aircraft=737"},
    {.path = "JSBSim/command_line/initfile", .value = "--initfile=reset00"},
    {.path = "JSBSim/command_line/arg1", .value = "--property=propulsion/engine[0]/set-running=1"},
    {.path = "JSBSim/command_line/arg2", .value = "--property=propulsion/engine[1]/set-running=1"},
    {.path = "JSBSim/command_line/arg3", .value = "--suspend"},
    {.path = "JSBSim/command_line/arg4", .value = ""},
    {.path = "JSBSim/command_line/arg5", .value = ""},
    {.path = "JSBSim/command_line/arg6", .value = ""},

    {.path = "JSBSim/ports/input", .value = "5138"},
    {.path = "JSBSim/ports/output", .value = "5139"},
    {.path = "JSBSim/ports/telnet", .value = "5137"},
    {.path = "JSBSim/ports/flightgear", .value = "5508"},

    {.path = "JSBSim/rates/UI", .value = "100"},
    {.path = "JSBSim/rates/output", .value = "10"},
    {.path = "JSBSim/rates/FlightGear", .value = "30"},

    {.path = "JSBSim/aircraft/pitch-trim", .value = "-0.32"},
    {.path = "JSBSim/aircraft/pitch-trim-rate", .value = "0.2"},

    {.path = "JSBSim/joystick/elevator-axis", .value = "1"},
    {.path = "JSBSim/joystick/aileron-axis", .value = "0"},
    {.path = "JSBSim/joystick/rudder-axis", .value = "2"},
    {.path = "JSBSim/joystick/throttle-axis", .value = "3"},
    {.path = "JSBSim/joystick/aileron-trim-axis", .value = "4"},
    {.path = "JSBSim/joystick/elevator-trim-axis", .value = "5"},
    {.path = "JSBSim/joystick/axis-0-deadband", .value = "0.04"},
    {.path = "JSBSim/joystick/axis-1-deadband", .value = "0.04"},
    {.path = "JSBSim/joystick/axis-2-deadband", .value = "0.07"},

    {.path = "JSBSim/airport/magvar", .value = "12.0"},
    {.path = "JSBSim/airport/runway-length-ft", .value = "11095"},
    {.path = "JSBSim/airport/ILS-runway-near-latitude", .value = "33.937363033"},
    {.path = "JSBSim/airport/ILS-runway-near-longitude", .value = "-118.382713917"},
    {.path = "JSBSim/airport/ILS-runway-far-latitude", .value = "33.933649383"},
    {.path = "JSBSim/airport/ILS-runway-far-longitude", .value = "-118.419018333"},
    {.path = "JSBSim/airport/ILS-frequency", .value = "109.9"},
    {.path = "JSBSim/airport/ILS-course-mag", .value = "251.0"},
    {.path = "JSBSim/airport/ILS-GS", .value = "3.0"},
    {.path = "JSBSim/airport/ILS-TDZE", .value = "97.8"},
};

}  // namespace

std::vector<std::string> Settings::SplitPath(std::string_view path) {
  std::vector<std::string> parts;
  std::string cur;
  for (const char CharacterInPath : path) {
    if (CharacterInPath == '/' || CharacterInPath == '\\') {
      if (!cur.empty()) {
        parts.push_back(std::move(cur));
        cur.clear();
      }
    } else {
      cur.push_back(CharacterInPath);
    }
  }
  if (!cur.empty()) {
    parts.push_back(std::move(cur));
  }
  return parts;
}

void Settings::FlattenNode(const Node& node, const std::string& prefix,
                           std::vector<std::pair<std::string, std::string>>& out) {
  for (const auto& [leaf_key, value] : node.values) {
    std::string key = prefix;
    if (!key.empty()) {
      key += '/';
    }
    key += leaf_key;
    out.emplace_back(std::move(key), value);
  }
  for (const auto& [subgroup_name, child_node] : node.children) {
    std::string next = prefix;
    if (!next.empty()) {
      next += '/';
    }
    next += subgroup_name;
    FlattenNode(child_node, next, out);
  }
}

Settings::Node* Settings::EnsurePath(const std::vector<std::string>& path_to_group) {
  Node* cur = &root_;
  for (const auto& seg : path_to_group) {
    cur = &cur->children[seg];
  }
  return cur;
}

const Settings::Node* Settings::FindNode(const std::vector<std::string>& path_to_group) const {
  const Node* current_node = &root_;
  for (const auto& seg : path_to_group) {
    const auto ChildIterator = current_node->children.find(seg);
    if (ChildIterator == current_node->children.end()) {
      return nullptr;
    }
    current_node = &ChildIterator->second;
  }
  return current_node;
}

const Settings::Node* Settings::Section(std::string_view name) const {
  return FindNode({std::string(name)});
}

Settings::Node* Settings::Section(std::string_view name) {
  return EnsurePath({std::string(name)});
}

std::string Settings::Get(std::vector<std::string_view> path, std::string_view fallback) const {
  if (path.empty()) {
    return std::string(fallback);
  }
  std::vector<std::string> group;
  group.reserve(!path.empty() ? path.size() - 1 : 0);
  for (size_t i = 0; i + 1 < path.size(); ++i) {
    group.emplace_back(path[i]);
  }
  const std::string Leaf(path.back());
  const Node* node = FindNode(group);
  if (!node) {
    return std::string(fallback);
  }
  const auto It = node->values.find(Leaf);
  if (It == node->values.end()) {
    return std::string(fallback);
  }
  return It->second;
}

void Settings::Set(std::vector<std::string_view> path, std::string value) {
  if (path.empty()) {
    return;
  }
  std::vector<std::string> group;
  for (size_t i = 0; i + 1 < path.size(); ++i) {
    group.emplace_back(path[i]);
  }
  const std::string Leaf(path.back());
  EnsurePath(group)->values[Leaf] = std::move(value);
}

std::string Settings::GetPath(std::string_view path, std::string_view fallback) const {
  const auto Parts = SplitPath(path);
  if (Parts.empty()) {
    return std::string(fallback);
  }
  std::vector<std::string_view> views;
  views.reserve(Parts.size());
  for (const auto& p : Parts) {
    views.push_back(p);
  }
  return Get(views, fallback);
}

void Settings::SetPath(std::string_view path, std::string value) {
  const auto Parts = SplitPath(path);
  if (Parts.empty()) {
    return;
  }
  std::vector<std::string_view> views;
  views.reserve(Parts.size());
  for (const auto& p : Parts) {
    views.push_back(p);
  }
  Set(views, std::move(value));
}

std::vector<std::pair<std::string, std::string>> Settings::EntriesUnder(std::string_view top_section) const {
  std::vector<std::pair<std::string, std::string>> out;
  const Node* sec = Section(top_section);
  if (!sec) {
    return out;
  }
  FlattenNode(*sec, "", out);
  return out;
}

Settings Settings::Load() {
  return LoadFromPath(ResolvePath());
}

Settings Settings::LoadFromPath(std::string path) {
  Settings s;
  s.path_ = std::move(path);
  s.PopulateDefaults();

  std::ifstream probe(s.path_);
  s.was_file_loaded_ = static_cast<bool>(probe);
  probe.close();

  if (s.was_file_loaded_) {
    s.OverlayFromFile(s.path_);
  } else {
    // Fail loudly: only the process working directory is searched (no parent-path hunt).
    std::cerr << "AhCommon Settings: warning: \"" << s.path_ << "\" not found in the current working directory. "
              << "Using built-in defaults (and writing a new file if possible). "
              << "Operational CWD is typically aero-hub/run/.\n";
  }

  // Runtime env (domain, RMW, models path) comes only from the settings tree.
  s.PublishRuntimeEnvFromSettings();

  if (!s.was_file_loaded_) {
    if (std::string error_out; !s.Save(error_out)) {
      std::cerr << "AhCommon Settings: warning: could not write defaults: " << error_out << '\n';
    }
  }

  return s;
}

std::string Settings::ResolvePath() {
  // Only the working directory — never search parents or alternate roots.
  // A missing file is reported as a warning in LoadFromPath.
  return std::string(SettingsFileName);
}

void Settings::PopulateDefaults() {
  root_ = Node{};
  for (const auto& d : DefaultSettings) {
    SetPath(d.path, d.value);
  }
}

void Settings::OverlayFromFile(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    return;
  }
  std::string line;
  std::string section;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const std::string TrimmedLine = Trim(line, TrimIniChars);
    if (TrimmedLine.empty() || TrimmedLine[0] == ';' || TrimmedLine[0] == '#') {
      continue;
    }
    if (TrimmedLine.front() == '[') {
      const auto Close = TrimmedLine.find(']');
      if (Close != std::string::npos && Close > 1) {
        section = TrimmedLine.substr(1, Close - 1);
      }
      continue;
    }
    const auto EqualSignPosition = TrimmedLine.find('=');
    if (EqualSignPosition == std::string::npos || section.empty()) {
      continue;
    }
    const std::string Key = Trim(TrimmedLine.substr(0, EqualSignPosition), TrimIniChars);
    const std::string Value = Trim(TrimmedLine.substr(EqualSignPosition + 1), TrimIniChars);
    if (Key.empty()) {
      continue;
    }
    // Path = section + nested key; accept '/' or '\' in both.
    std::string full = section;
    full.push_back('/');
    full += Key;
    SetPath(full, Value);
  }
}

void Settings::PublishRuntimeEnvFromSettings() const {
  // rmw/rcl and lab tools still read process environment variables. Always
  // overwrite from the settings tree so the INI is the single source of truth
  // (no competing shell defaults for domain / RMW / models).
  const std::string RosDomainId = Get({"ROS", "domain_id"}, "42");
  if (!RosDomainId.empty()) {
    setenv("ROS_DOMAIN_ID", RosDomainId.c_str(), /*overwrite=*/1);
  }

  const std::string Rmw = Ros().RmwImplementation();
  if (!Rmw.empty()) {
    setenv("RMW_IMPLEMENTATION", Rmw.c_str(), /*overwrite=*/1);
  }

  const std::string YoloModelsDir = Ros().YoloModelsDir();
  if (!YoloModelsDir.empty()) {
    setenv("AERO_HUB_YOLO_MODELS", YoloModelsDir.c_str(), /*overwrite=*/1);
  }
}

bool Settings::Save() const {
  std::string ignored;
  return Save(ignored);
}

bool Settings::Save(std::string& error_out) const {
  error_out.clear();
  if (path_.empty()) {
    error_out = "empty settings path";
    return false;
  }

  // QSettings IniFormat: each root child is [Section]; deeper path uses '\'.
  std::vector<std::string> section_order;
  auto push_if = [&](const std::string& name) {
    if (root_.children.contains(name)) {
      section_order.push_back(name);
    }
  };
  push_if("ROS");
  push_if("Camera");
  for (const auto& subgroup_name : root_.children | std::views::keys) {
    if (subgroup_name == "ROS" || subgroup_name == "Camera") {
      continue;
    }
    section_order.push_back(subgroup_name);
  }

  std::ofstream out(path_);
  if (!out) {
    error_out = "cannot write " + path_;
    return false;
  }

  out << "; AeroHub settings (Qt QSettings IniFormat-compatible).\n"
      << "; In-memory model is a tree; on disk nested groups use '\\'.\n\n";

  bool first = true;
  for (const auto& sec_name : section_order) {
    const Node& sec = root_.children.at(sec_name);
    if (!first) {
      out << '\n';
    }
    first = false;
    out << '[' << sec_name << "]\n";

    // Leaves directly under the top-level section.
    for (const auto& kv : sec.values) {
      out << kv.first << '=' << kv.second << '\n';
    }

    // Nested groups → key path with '\'.
    std::vector<std::pair<std::string, std::string>> nested;
    for (const auto& ch : sec.children) {
      FlattenNode(ch.second, ch.first, nested);
    }
    for (auto& kv : nested) {
      for (char& c : kv.first) {
        if (c == '/') {
          c = '\\';
        }
      }
      out << kv.first << '=' << kv.second << '\n';
    }
  }
  return true;
}

bool Settings::PersistCamera(const CameraSelection& camera) const {
  std::string ignored;
  return PersistCamera(camera, ignored);
}

bool Settings::PersistCamera(const CameraSelection& camera, std::string& error_out) const {
  CameraSelection sel = camera;
  if (sel.video_source == "synthetic" || sel.device_path == "synthetic") {
    sel.video_source = "synthetic";
    sel.device_id = -1;
    if (sel.device_path.empty()) {
      sel.device_path = "synthetic";
    }
  }
  this->Camera().SetSelection(sel);
  return Save(error_out);
}

// --- RosSection ---

std::uint8_t Settings::RosSection::DomainId() const {
  const std::string DomainIdString = owner_->Get({"ROS", "domain_id"}, "42");
  try {
    const int IntValue = std::stoi(DomainIdString);
    if (IntValue < 0 || IntValue > 255) {
      return DefaultRosDomainId;
    }
    return static_cast<std::uint8_t>(IntValue);
  } catch (...) {
    return DefaultRosDomainId;
  }
}

std::string Settings::RosSection::NamespaceName() const {
  return SanitizeNamespace(owner_->Get({"ROS", "namespace"}, ""));
}

std::string Settings::RosSection::RmwImplementation() const {
  return owner_->Get({"ROS", "rmw_implementation"}, "rmw_fastrtps_cpp");
}

std::string Settings::RosSection::YoloModelsDir() const {
  const std::string Dir = owner_->Get({"ROS", "yolo_models_dir"}, "../yolo-models");
  if (Dir.empty()) {
    return {};
  }
  namespace fs = std::filesystem;
  fs::path path(Dir);
  if (!path.is_absolute()) {
    path = fs::current_path() / path;
  }
  return path.lexically_normal().string();
}

void Settings::RosSection::SetDomainId(const std::uint8_t value) const {
  MutableOwner()->Set({"ROS", "domain_id"}, std::to_string(static_cast<int>(value)));
}

void Settings::RosSection::SetNamespaceName(const std::string& value) const {
  MutableOwner()->Set({"ROS", "namespace"}, SanitizeNamespace(value));
}

void Settings::RosSection::SetRmwImplementation(const std::string& value) const {
  MutableOwner()->Set({"ROS", "rmw_implementation"}, value);
}

void Settings::RosSection::SetYoloModelsDir(const std::string& value) const {
  MutableOwner()->Set({"ROS", "yolo_models_dir"}, value);
}

// --- CameraSection ---

std::string Settings::CameraSection::VideoSource() const {
  return owner_->Get({"Camera", "video_source"}, "synthetic");
}

int Settings::CameraSection::DeviceId() const {
  try {
    return std::stoi(owner_->Get({"Camera", "device_id"}, "-1"));
  } catch (...) {
    return -1;
  }
}

std::string Settings::CameraSection::DevicePath() const {
  return owner_->Get({"Camera", "device_path"}, "synthetic");
}

std::string Settings::CameraSection::Backend() const {
  return owner_->Get({"Camera", "backend"}, "");
}

CameraSelection Settings::CameraSection::Selection() const {
  CameraSelection s;
  s.video_source = VideoSource();
  s.device_id = DeviceId();
  s.device_path = DevicePath();
  s.backend = Backend();
  if (s.video_source == "synthetic" || s.device_path == "synthetic") {
    s.video_source = "synthetic";
    s.device_id = -1;
    if (s.device_path.empty()) {
      s.device_path = "synthetic";
    }
  }
  return s;
}

void Settings::CameraSection::SetSelection(const CameraSelection& sel) const {
  MutableOwner()->Set({"Camera", "video_source"}, sel.video_source);
  MutableOwner()->Set({"Camera", "device_id"}, std::to_string(sel.device_id));
  MutableOwner()->Set({"Camera", "device_path"}, sel.device_path);
  MutableOwner()->Set({"Camera", "backend"}, sel.backend);
}

// --- JsbSimSection ---

std::string Settings::JsbSimSection::Get(const std::string_view relative_key) const {
  const auto Parts = Settings::SplitPath(relative_key);
  std::vector<std::string_view> path;
  path.emplace_back("JSBSim");
  for (const auto& p : Parts) {
    path.push_back(p);
  }
  return owner_->Get(path, "");
}

void Settings::JsbSimSection::Set(const std::string_view relative_key, std::string value) const {
  auto parts = Settings::SplitPath(relative_key);
  std::vector<std::string_view> path;
  path.emplace_back("JSBSim");
  for (const auto& p : parts) {
    path.push_back(p);
  }
  MutableOwner()->Set(path, std::move(value));
}

const Settings::Node* Settings::JsbSimSection::Tree() const {
  return owner_->Section("JSBSim");
}

}  // namespace ah
