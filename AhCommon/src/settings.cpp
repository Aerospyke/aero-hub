#include "ah_common/settings.hpp"

#include "ah_common/string_util.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace ah
{
namespace
{

struct DefaultEntry
{
  const char * key;
  const char * value;
};

// Keep in sync with aerohub_settings_template.ini
constexpr DefaultEntry kDefaults[] = {
  {"ROS/domain_id", "42"},
  {"ROS/namespace", ""},

  {"Camera/video_source", "synthetic"},
  {"Camera/device_id", "-1"},
  {"Camera/device_path", "synthetic"},
  {"Camera/backend", ""},

  {"JSBSim/command_line/executable", ""},
  {"JSBSim/command_line/realtime", "--realtime"},
  {"JSBSim/command_line/aircraft", "--aircraft=737"},
  {"JSBSim/command_line/initfile", "--initfile=reset00"},
  {"JSBSim/command_line/arg1", "--property=propulsion/engine[0]/set-running=1"},
  {"JSBSim/command_line/arg2", "--property=propulsion/engine[1]/set-running=1"},
  {"JSBSim/command_line/arg3", "--suspend"},
  {"JSBSim/command_line/arg4", ""},
  {"JSBSim/command_line/arg5", ""},
  {"JSBSim/command_line/arg6", ""},

  {"JSBSim/ports/input", "5138"},
  {"JSBSim/ports/output", "5139"},
  {"JSBSim/ports/telnet", "5137"},
  {"JSBSim/ports/flightgear", "5508"},

  {"JSBSim/rates/UI", "100"},
  {"JSBSim/rates/output", "10"},
  {"JSBSim/rates/FlightGear", "30"},

  {"JSBSim/aircraft/pitch-trim", "-0.32"},
  {"JSBSim/aircraft/pitch-trim-rate", "0.2"},

  {"JSBSim/joystick/elevator-axis", "1"},
  {"JSBSim/joystick/aileron-axis", "0"},
  {"JSBSim/joystick/rudder-axis", "2"},
  {"JSBSim/joystick/throttle-axis", "3"},
  {"JSBSim/joystick/aileron-trim-axis", "4"},
  {"JSBSim/joystick/elevator-trim-axis", "5"},
  {"JSBSim/joystick/axis-0-deadband", "0.04"},
  {"JSBSim/joystick/axis-1-deadband", "0.04"},
  {"JSBSim/joystick/axis-2-deadband", "0.07"},

  {"JSBSim/airport/magvar", "12.0"},
  {"JSBSim/airport/runway-length-ft", "11095"},
  {"JSBSim/airport/ILS-runway-near-latitude", "33.937363033"},
  {"JSBSim/airport/ILS-runway-near-longitude", "-118.382713917"},
  {"JSBSim/airport/ILS-runway-far-latitude", "33.933649383"},
  {"JSBSim/airport/ILS-runway-far-longitude", "-118.419018333"},
  {"JSBSim/airport/ILS-frequency", "109.9"},
  {"JSBSim/airport/ILS-course-mag", "251.0"},
  {"JSBSim/airport/ILS-GS", "3.0"},
  {"JSBSim/airport/ILS-TDZE", "97.8"},
};

std::string SectionOf(const std::string & full_key)
{
  const auto slash = full_key.rfind('/');
  if (slash == std::string::npos) {
    return {};
  }
  return full_key.substr(0, slash);
}

std::string LeafOf(const std::string & full_key)
{
  const auto slash = full_key.rfind('/');
  if (slash == std::string::npos) {
    return full_key;
  }
  return full_key.substr(slash + 1);
}

}  // namespace

Settings Settings::load()
{
  return loadFromPath(resolvePath());
}

Settings Settings::loadFromPath(std::string path)
{
  Settings s;
  s.path_ = std::move(path);
  s.populateDefaults();

  std::ifstream probe(s.path_);
  s.was_file_loaded_ = static_cast<bool>(probe);
  probe.close();

  if (s.was_file_loaded_) {
    s.overlayFromFile(s.path_);
  }

  s.applyProcessEnvOverrides();
  s.exportDomainIdToEnvIfUnset();

  if (!s.was_file_loaded_) {
    std::string err;
    if (!s.save(&err)) {
      // Best-effort create; caller can still use in-memory defaults.
      (void)err;
    }
  }

  return s;
}

std::string Settings::resolvePath()
{
  const char * env = std::getenv("AERO_HUB_SETTINGS");
  if (env != nullptr && env[0] != '\0') {
    return std::string(env);
  }
  const char * candidates[] = {
    kSettingsFileName,
    "../aerohub_settings.ini",
    "../../aerohub_settings.ini",
    "/aero-hub/aerohub_settings.ini",
  };
  for (const char * c : candidates) {
    std::ifstream in(c);
    if (in) {
      return std::string(c);
    }
  }
  return std::string(kSettingsFileName);
}

void Settings::populateDefaults()
{
  entries_.clear();
  for (const auto & d : kDefaults) {
    entries_[d.key] = d.value;
  }
}

void Settings::overlayFromFile(const std::string & path)
{
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
    const std::string trimmed_line = Trim(line, kTrimIniChars);
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
    if (eq == std::string::npos) {
      continue;
    }
    std::string key = Trim(trimmed_line.substr(0, eq), kTrimIniChars);
    const std::string val = Trim(trimmed_line.substr(eq + 1), kTrimIniChars);
    if (key.empty() || section.empty()) {
      continue;
    }
    // Qt QSettings IniFormat nests groups with '\' under a section, e.g.
    //   [JSBSim]
    //   airport\magvar=12
    // Normalize to forward-slash flat keys: JSBSim/airport/magvar
    std::replace(key.begin(), key.end(), '\\', '/');
    entries_[section + "/" + key] = val;
  }
}

void Settings::applyProcessEnvOverrides()
{
  // Namespace env overrides file (same as prior ah_core LoadRosRuntimeSettings).
  const char * ns_env = std::getenv("AERO_HUB_ROS_NAMESPACE");
  if (ns_env != nullptr) {
    entries_["ROS/namespace"] = SanitizeNamespace(ns_env);
  }
  // ROS_DOMAIN_ID: leave entries_ from file/defaults; exportDomainIdToEnvIfUnset
  // pushes file→env when env is unset so rclcpp sees the same domain.
}

void Settings::exportDomainIdToEnvIfUnset() const
{
  const char * domain_env = std::getenv("ROS_DOMAIN_ID");
  if (domain_env != nullptr && domain_env[0] != '\0') {
    return;
  }
  const std::string id = get("ROS/domain_id", "42");
  if (!id.empty()) {
    setenv("ROS_DOMAIN_ID", id.c_str(), 0);
  }
}

std::string Settings::get(std::string_view key, std::string_view fallback) const
{
  const std::string k(key);
  const auto it = entries_.find(k);
  if (it == entries_.end()) {
    return std::string(fallback);
  }
  return it->second;
}

void Settings::set(std::string_view key, std::string value)
{
  entries_[std::string(key)] = std::move(value);
}

std::vector<std::pair<std::string, std::string>> Settings::entriesWithPrefix(
  std::string_view prefix) const
{
  std::vector<std::pair<std::string, std::string>> out;
  for (const auto & kv : entries_) {
    if (kv.first.compare(0, prefix.size(), prefix) == 0) {
      out.emplace_back(kv.first, kv.second);
    }
  }
  return out;
}

bool Settings::save(std::string * error_out) const
{
  if (path_.empty()) {
    if (error_out) {
      *error_out = "empty settings path";
    }
    return false;
  }

  // Group keys by INI section (all but last path component).
  std::map<std::string, std::vector<std::pair<std::string, std::string>>> by_section;
  for (const auto & kv : entries_) {
    by_section[SectionOf(kv.first)].emplace_back(LeafOf(kv.first), kv.second);
  }

  // Stable section order: ROS, Camera, then JSBSim* alpha, then any other.
  std::vector<std::string> section_order;
  auto push_if = [&](const std::string & name) {
    if (by_section.count(name)) {
      section_order.push_back(name);
    }
  };
  push_if("ROS");
  push_if("Camera");
  for (const auto & sec : by_section) {
    if (sec.first == "ROS" || sec.first == "Camera") {
      continue;
    }
    section_order.push_back(sec.first);
  }

  std::ofstream out(path_);
  if (!out) {
    if (error_out) {
      *error_out = "cannot write " + path_;
    }
    return false;
  }

  out << "; AeroHub settings (generated/updated by AhCommon).\n"
      << "; Defaults are always applied first; only keys present in a prior file overlay them.\n\n";

  bool first = true;
  for (const auto & sec : section_order) {
    if (!first) {
      out << '\n';
    }
    first = false;
    out << '[' << sec << "]\n";
    for (const auto & kv : by_section[sec]) {
      out << kv.first << '=' << kv.second << '\n';
    }
  }
  return true;
}

bool Settings::persistCamera(const CameraSelection & cam, std::string * error_out)
{
  CameraSelection sel = cam;
  if (sel.video_source == "synthetic" || sel.device_path == "synthetic") {
    sel.video_source = "synthetic";
    sel.device_id = -1;
    if (sel.device_path.empty()) {
      sel.device_path = "synthetic";
    }
  }
  this->camera().setSelection(sel);
  return save(error_out);
}

// --- Ros ---

std::uint8_t Settings::Ros::domainId() const
{
  const std::string raw = owner_->get("ROS/domain_id", "42");
  try {
    const int v = std::stoi(raw);
    if (v < 0 || v > 255) {
      return Settings::kDefaultRosDomainId;
    }
    return static_cast<std::uint8_t>(v);
  } catch (...) {
    return Settings::kDefaultRosDomainId;
  }
}

std::string Settings::Ros::namespaceName() const
{
  return SanitizeNamespace(owner_->get("ROS/namespace", ""));
}

void Settings::Ros::setDomainId(std::uint8_t id)
{
  mutableOwner()->set("ROS/domain_id", std::to_string(static_cast<int>(id)));
}

void Settings::Ros::setNamespaceName(std::string ns)
{
  mutableOwner()->set("ROS/namespace", SanitizeNamespace(ns));
}

// --- Camera ---

std::string Settings::Camera::videoSource() const
{
  return owner_->get("Camera/video_source", "synthetic");
}

int Settings::Camera::deviceId() const
{
  try {
    return std::stoi(owner_->get("Camera/device_id", "-1"));
  } catch (...) {
    return -1;
  }
}

std::string Settings::Camera::devicePath() const
{
  return owner_->get("Camera/device_path", "synthetic");
}

std::string Settings::Camera::backend() const
{
  return owner_->get("Camera/backend", "");
}

CameraSelection Settings::Camera::selection() const
{
  CameraSelection s;
  s.video_source = videoSource();
  s.device_id = deviceId();
  s.device_path = devicePath();
  s.backend = backend();
  if (s.video_source == "synthetic" || s.device_path == "synthetic") {
    s.video_source = "synthetic";
    s.device_id = -1;
    if (s.device_path.empty()) {
      s.device_path = "synthetic";
    }
  }
  return s;
}

void Settings::Camera::setSelection(const CameraSelection & sel)
{
  mutableOwner()->set("Camera/video_source", sel.video_source);
  mutableOwner()->set("Camera/device_id", std::to_string(sel.device_id));
  mutableOwner()->set("Camera/device_path", sel.device_path);
  mutableOwner()->set("Camera/backend", sel.backend);
}

// --- JsbSim ---

std::string Settings::JsbSim::get(std::string_view relative_key) const
{
  std::string key = "JSBSim/";
  key.append(relative_key.data(), relative_key.size());
  return owner_->get(key, "");
}

void Settings::JsbSim::set(std::string_view relative_key, std::string value)
{
  std::string key = "JSBSim/";
  key.append(relative_key.data(), relative_key.size());
  mutableOwner()->set(key, std::move(value));
}

}  // namespace ah
