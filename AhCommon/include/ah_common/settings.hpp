#pragma once

// Std-only aerohub_settings.ini load / defaults / write (no Qt).
// Access: settings.ros().domainId(), settings.camera().videoSource(), settings.jsbSim()...

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ah
{

/// Camera selection fields ([Camera] section) — shared by core + dashboard.
struct CameraSelection
{
  std::string video_source{"synthetic"};  // synthetic | camera
  int device_id{-1};
  std::string device_path{"synthetic"};
  std::string backend;
};

/// Full AeroHub settings file (ROS + Camera + JSBSim).
///
/// Always starts from built-in defaults, then overlays keys present in the INI
/// (a partial file still yields a complete set). If the file is missing, defaults
/// are written to a new aerohub_settings.ini in the working directory (or the
/// path from AERO_HUB_SETTINGS).
class Settings
{
 public:
  static constexpr std::uint8_t kDefaultRosDomainId = 42;
  static constexpr char kSettingsFileName[] = "aerohub_settings.ini";

  /// Resolve path (env + candidates), load, apply defaults overlay, write if missing.
  /// Also applies ROS_DOMAIN_ID from file into the process env when unset (rclcpp).
  static Settings load();

  /// Load a specific path (no candidate search). Writes defaults if file missing.
  static Settings loadFromPath(std::string path);

  Settings(const Settings &) = default;
  Settings & operator=(const Settings &) = default;
  Settings(Settings &&) noexcept = default;
  Settings & operator=(Settings &&) noexcept = default;

  [[nodiscard]] const std::string & path() const { return path_; }
  [[nodiscard]] bool wasFileLoaded() const { return was_file_loaded_; }

  /// Flat keys as stored ("ROS/domain_id", "JSBSim/ports/input", …).
  [[nodiscard]] const std::map<std::string, std::string> & entries() const { return entries_; }

  /// Entries whose key starts with @p prefix (e.g. "JSBSim/").
  [[nodiscard]] std::vector<std::pair<std::string, std::string>> entriesWithPrefix(
    std::string_view prefix) const;

  [[nodiscard]] std::string get(std::string_view key, std::string_view fallback = {}) const;
  void set(std::string_view key, std::string value);

  /// Write current entries to path_ (full file).
  bool save(std::string * error_out = nullptr) const;

  /// Update [Camera] keys and rewrite the whole file.
  bool persistCamera(const CameraSelection & camera, std::string * error_out = nullptr);

  // --- Nested accessors: settings.ros().domainId() ---

  class Ros
  {
   public:
    explicit Ros(const Settings * owner) : owner_(owner) {}
    [[nodiscard]] std::uint8_t domainId() const;
    /// Sanitized graph namespace; empty = root.
    [[nodiscard]] std::string namespaceName() const;
    void setDomainId(std::uint8_t id);
    void setNamespaceName(std::string ns);

   private:
    const Settings * owner_;
    Settings * mutableOwner() const { return const_cast<Settings *>(owner_); }
  };

  class Camera
  {
   public:
    explicit Camera(const Settings * owner) : owner_(owner) {}
    [[nodiscard]] std::string videoSource() const;
    [[nodiscard]] int deviceId() const;
    [[nodiscard]] std::string devicePath() const;
    [[nodiscard]] std::string backend() const;
    [[nodiscard]] CameraSelection selection() const;
    void setSelection(const CameraSelection & sel);

   private:
    const Settings * owner_;
    Settings * mutableOwner() const { return const_cast<Settings *>(owner_); }
  };

  class JsbSim
  {
   public:
    explicit JsbSim(const Settings * owner) : owner_(owner) {}
    /// Value of key under JSBSim/ (e.g. "ports/input" → JSBSim/ports/input).
    [[nodiscard]] std::string get(std::string_view relative_key) const;
    void set(std::string_view relative_key, std::string value);

   private:
    const Settings * owner_;
    Settings * mutableOwner() const { return const_cast<Settings *>(owner_); }
  };

  [[nodiscard]] Ros ros() const { return Ros(this); }
  [[nodiscard]] Camera camera() const { return Camera(this); }
  [[nodiscard]] JsbSim jsbSim() const { return JsbSim(this); }

 private:
  Settings() = default;

  void populateDefaults();
  void overlayFromFile(const std::string & path);
  void applyProcessEnvOverrides();
  void exportDomainIdToEnvIfUnset() const;
  static std::string resolvePath();

  std::string path_;
  bool was_file_loaded_{false};
  std::map<std::string, std::string> entries_;
};

}  // namespace ah
