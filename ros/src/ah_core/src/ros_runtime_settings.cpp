#include "ah_core/ros_runtime_settings.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace ah_core
{

std::string Trim(const std::string& full_string, const std::string_view chars_to_trim)
{
  const auto Start = full_string.find_first_not_of(chars_to_trim);
  if (Start == std::string::npos) {
    return {};
  }
  const auto End = full_string.find_last_not_of(chars_to_trim);
  return full_string.substr(Start, End - Start + 1);
}

std::string SanitizeNamespace(const std::string& raw_namespace_setting)
{
  std::string processed_namespace = Trim(raw_namespace_setting, TrimIniChars);
  processed_namespace = Trim(processed_namespace, "/");
  // In ROS, '//' is an empty path segment — treat as bad config → root (empty) namespace.
  if (processed_namespace.find("//") != std::string::npos) {
    return {};
  }
  return processed_namespace;
}

RosRuntimeSettings LoadRosRuntimeSettings()
{
  RosRuntimeSettings out;

  const char * settings_file_path_candidates[] = {
    std::getenv("AERO_HUB_SETTINGS"),
    "aerohub_settings.ini",
    "../aerohub_settings.ini",
    "../../aerohub_settings.ini",
    "/aero-hub/aerohub_settings.ini",
  };

  const bool IsDomainDefinedInEnvironment = (std::getenv("ROS_DOMAIN_ID") != nullptr &&
    std::getenv("ROS_DOMAIN_ID")[0] != '\0');
  const bool IsNamespaceDefinedInEnv = (std::getenv("AERO_HUB_ROS_NAMESPACE") != nullptr);

  if (IsNamespaceDefinedInEnv) {
    const char * ns_env = std::getenv("AERO_HUB_ROS_NAMESPACE");
    out.ros_namespace = SanitizeNamespace(ns_env);
  }

  for (const char * path : settings_file_path_candidates) {
    if (path == nullptr || path[0] == '\0') {
      continue;
    }
    std::ifstream in(path);
    if (!in) {
      continue;
    }
    out.settings_path = path;
    std::string line;
    bool in_ros = false;
    bool in_camera = false;
    while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (line.empty() || line[0] == ';' || line[0] == '#') {
        continue;
      }
      if (line.front() == '[') {
        in_ros = (line == "[ROS]");
        in_camera = (line == "[Camera]");
        continue;
      }
      const auto eq = line.find('=');
      if (eq == std::string::npos) {
        continue;
      }
      const std::string key = Trim(line.substr(0, eq), TrimIniChars);
      std::string val = Trim(line.substr(eq + 1), TrimIniChars);
      if (in_ros) {
        if (key == "domain_id" && !val.empty() && !IsDomainDefinedInEnvironment) {
          setenv("ROS_DOMAIN_ID", val.c_str(), 1);
        }
        if (key == "namespace" && !IsNamespaceDefinedInEnv) {
          out.ros_namespace = SanitizeNamespace(val);
        }
      }
      if (in_camera) {
        if (key == "video_source" && !val.empty()) {
          out.camera.video_source = val;
        }
        if (key == "device_id" && !val.empty()) {
          try {
            out.camera.device_id = std::stoi(val);
          } catch (...) {
          }
        }
        if (key == "device_path") {
          out.camera.device_path = val;
        }
        if (key == "backend") {
          out.camera.backend = val;
        }
      }
    }
    break;  // first readable settings file wins
  }

  // Normalize synthetic selection.
  if (out.camera.video_source == "synthetic" || out.camera.device_path == "synthetic") {
    out.camera.video_source = "synthetic";
    out.camera.device_id = -1;
    if (out.camera.device_path.empty()) {
      out.camera.device_path = "synthetic";
    }
  }

  return out;
}

bool PersistCameraSelectionToSettingsFile(
  const std::string & settings_path,
  const CameraSelection & camera,
  std::string * error_out)
{
  if (settings_path.empty()) {
    if (error_out) {
      *error_out = "empty settings path";
    }
    return false;
  }

  std::ifstream in(settings_path);
  if (!in) {
    std::ofstream out(settings_path, std::ios::app);
    if (!out) {
      if (error_out) {
        *error_out = "cannot write camera settings to " + settings_path;
      }
      return false;
    }
    out << "\n[Camera]\n"
        << "video_source=" << camera.video_source << '\n'
        << "device_id=" << camera.device_id << '\n'
        << "device_path=" << camera.device_path << '\n'
        << "backend=" << camera.backend << '\n';
    return true;
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(line);
  }
  in.close();

  std::vector<std::string> out_lines;
  bool in_camera = false;
  bool wrote_camera = false;
  auto write_camera_block = [&]() {
    out_lines.push_back("[Camera]");
    out_lines.push_back("video_source=" + camera.video_source);
    out_lines.push_back("device_id=" + std::to_string(camera.device_id));
    out_lines.push_back("device_path=" + camera.device_path);
    out_lines.push_back("backend=" + camera.backend);
    wrote_camera = true;
  };

  for (const auto & l : lines) {
    if (!l.empty() && l.front() == '[') {
      if (in_camera) {
        in_camera = false;
      }
      if (l == "[Camera]") {
        in_camera = true;
        write_camera_block();
        continue;
      }
      out_lines.push_back(l);
      continue;
    }
    if (in_camera) {
      continue;
    }
    out_lines.push_back(l);
  }
  if (!wrote_camera) {
    if (!out_lines.empty() && !out_lines.back().empty()) {
      out_lines.push_back("");
    }
    write_camera_block();
  }

  std::ofstream out(settings_path);
  if (!out) {
    if (error_out) {
      *error_out = "cannot rewrite settings " + settings_path;
    }
    return false;
  }
  for (size_t i = 0; i < out_lines.size(); ++i) {
    out << out_lines[i];
    if (i + 1 < out_lines.size()) {
      out << '\n';
    }
  }
  out << '\n';
  return true;
}

}  // namespace ah_core
