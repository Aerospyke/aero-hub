#include "ah_common/settings.hpp"

#include "ah_common/string_util.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

namespace ah {
namespace {

struct DefaultEntry {
  const char* path;  // '/' path from root, e.g. "JSBSim/airport/magvar"
  const char* value;
};

// Keep in sync with aerohub_settings_template.ini
constexpr DefaultEntry kDefaults[] = {
    {.path = "ROS/domain_id", .value = "42"},
    {.path = "ROS/namespace", .value = ""},

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
  for (char c : path) {
    if (c == '/' || c == '\\') {
      if (!cur.empty()) {
        parts.push_back(std::move(cur));
        cur.clear();
      }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) {
    parts.push_back(std::move(cur));
  }
  return parts;
}

void Settings::FlattenNode(const Node& node, const std::string& prefix,
                           std::vector<std::pair<std::string, std::string>>& out) {
  for (const auto& kv : node.values) {
    const std::string key = prefix.empty() ? kv.first : prefix + "/" + kv.first;
    out.emplace_back(key, kv.second);
  }
  for (const auto& ch : node.children) {
    const std::string next = prefix.empty() ? ch.first : prefix + "/" + ch.first;
    FlattenNode(ch.second, next, out);
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
  const Node* cur = &root_;
  for (const auto& seg : path_to_group) {
    const auto it = cur->children.find(seg);
    if (it == cur->children.end()) {
      return nullptr;
    }
    cur = &it->second;
  }
  return cur;
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
  group.reserve(path.size() > 0 ? path.size() - 1 : 0);
  for (size_t i = 0; i + 1 < path.size(); ++i) {
    group.emplace_back(path[i]);
  }
  const std::string leaf(path.back());
  const Node* node = FindNode(group);
  if (!node) {
    return std::string(fallback);
  }
  const auto it = node->values.find(leaf);
  if (it == node->values.end()) {
    return std::string(fallback);
  }
  return it->second;
}

void Settings::Set(std::vector<std::string_view> path, std::string value) {
  if (path.empty()) {
    return;
  }
  std::vector<std::string> group;
  for (size_t i = 0; i + 1 < path.size(); ++i) {
    group.emplace_back(path[i]);
  }
  const std::string leaf(path.back());
  EnsurePath(group)->values[leaf] = std::move(value);
}

std::string Settings::GetPath(std::string_view path, std::string_view fallback) const {
  const auto parts = SplitPath(path);
  if (parts.empty()) {
    return std::string(fallback);
  }
  std::vector<std::string_view> views;
  views.reserve(parts.size());
  for (const auto& p : parts) {
    views.push_back(p);
  }
  return Get(views, fallback);
}

void Settings::SetPath(std::string_view path, std::string value) {
  const auto parts = SplitPath(path);
  if (parts.empty()) {
    return;
  }
  std::vector<std::string_view> views;
  views.reserve(parts.size());
  for (const auto& p : parts) {
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
  }

  s.ApplyProcessEnvOverrides();
  s.ExportDomainIdToEnvIfUnset();

  if (!s.was_file_loaded_) {
    if (std::string error_out; !s.Save(error_out)) {
      std::cerr << error_out << std::endl;
    }
  }

  return s;
}

std::string Settings::ResolvePath() {
  const char* env = std::getenv("AERO_HUB_SETTINGS");
  if (env != nullptr && env[0] != '\0') {
    return std::string(env);
  }
  const char* candidates[] = {
      SettingsFileName,
      "../aerohub_settings.ini",
      "../../aerohub_settings.ini",
      "/aero-hub/aerohub_settings.ini",
  };
  for (const char* c : candidates) {
    std::ifstream in(c);
    if (in) {
      return std::string(c);
    }
  }
  return std::string(SettingsFileName);
}

void Settings::PopulateDefaults() {
  root_ = Node{};
  for (const auto& d : kDefaults) {
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
    const std::string trimmed_line = Trim(line, TrimIniChars);
    if (trimmed_line.empty() || trimmed_line[0] == ';' || trimmed_line[0] == '#') {
      continue;
    }
    if (trimmed_line.front() == '[') {
      const auto close = trimmed_line.find(']');
      if (close != std::string::npos && close > 1) {
        section = trimmed_line.substr(1, close - 1);
      }
      continue;
    }
    const auto eq = trimmed_line.find('=');
    if (eq == std::string::npos || section.empty()) {
      continue;
    }
    std::string key = Trim(trimmed_line.substr(0, eq), TrimIniChars);
    const std::string val = Trim(trimmed_line.substr(eq + 1), TrimIniChars);
    if (key.empty()) {
      continue;
    }
    // Path = section + nested key; accept '/' or '\' in both.
    std::string full = section;
    full.push_back('/');
    full += key;
    SetPath(full, val);
  }
}

void Settings::ApplyProcessEnvOverrides() {
  const char* ns_env = std::getenv("AERO_HUB_ROS_NAMESPACE");
  if (ns_env != nullptr) {
    Set({"ROS", "namespace"}, SanitizeNamespace(ns_env));
  }
}

void Settings::ExportDomainIdToEnvIfUnset() const {
  const char* domain_env = std::getenv("ROS_DOMAIN_ID");
  if (domain_env != nullptr && domain_env[0] != '\0') {
    return;
  }
  const std::string id = Get({"ROS", "domain_id"}, "42");
  if (!id.empty()) {
    setenv("ROS_DOMAIN_ID", id.c_str(), 0);
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
    if (root_.children.count(name)) {
      section_order.push_back(name);
    }
  };
  push_if("ROS");
  push_if("Camera");
  for (const auto& ch : root_.children) {
    if (ch.first == "ROS" || ch.first == "Camera") {
      continue;
    }
    section_order.push_back(ch.first);
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

bool Settings::PersistCamera(const CameraSelection& cam) {
  std::string ignored;
  return PersistCamera(cam, ignored);
}

bool Settings::PersistCamera(const CameraSelection& cam, std::string& error_out) {
  CameraSelection sel = cam;
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
  const std::string raw = owner_->Get({"ROS", "domain_id"}, "42");
  try {
    const int v = std::stoi(raw);
    if (v < 0 || v > 255) {
      return Settings::DefaultRosDomainId;
    }
    return static_cast<std::uint8_t>(v);
  } catch (...) {
    return Settings::DefaultRosDomainId;
  }
}

std::string Settings::RosSection::NamespaceName() const {
  return SanitizeNamespace(owner_->Get({"ROS", "namespace"}, ""));
}

void Settings::RosSection::SetDomainId(std::uint8_t id) {
  MutableOwner()->Set({"ROS", "domain_id"}, std::to_string(static_cast<int>(id)));
}

void Settings::RosSection::SetNamespaceName(std::string ns) {
  MutableOwner()->Set({"ROS", "namespace"}, SanitizeNamespace(ns));
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

void Settings::CameraSection::SetSelection(const CameraSelection& sel) {
  MutableOwner()->Set({"Camera", "video_source"}, sel.video_source);
  MutableOwner()->Set({"Camera", "device_id"}, std::to_string(sel.device_id));
  MutableOwner()->Set({"Camera", "device_path"}, sel.device_path);
  MutableOwner()->Set({"Camera", "backend"}, sel.backend);
}

// --- JsbSimSection ---

std::string Settings::JsbSimSection::Get(std::string_view relative_key) const {
  auto parts = Settings::SplitPath(relative_key);
  std::vector<std::string_view> path;
  path.push_back("JSBSim");
  for (const auto& p : parts) {
    path.push_back(p);
  }
  return owner_->Get(path, "");
}

void Settings::JsbSimSection::Set(std::string_view relative_key, std::string value) {
  auto parts = Settings::SplitPath(relative_key);
  std::vector<std::string_view> path;
  path.push_back("JSBSim");
  for (const auto& p : parts) {
    path.push_back(p);
  }
  MutableOwner()->Set(path, std::move(value));
}

const Settings::Node* Settings::JsbSimSection::Tree() const {
  return owner_->Section("JSBSim");
}

}  // namespace ah
