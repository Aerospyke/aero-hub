#pragma once

// Std-only aerohub_settings.ini load / defaults / write (no Qt).
// In-memory: real tree (groups + leaf values). On disk: QSettings IniFormat.
// Access: settings.Ros().DomainId(), settings.Camera().VideoSource(), settings.JsbSim()...

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ah {

/// Camera selection fields ([Camera] section) — shared by core + dashboard.
struct CameraSelection {
  std::string video_source{"synthetic"};  // synthetic | camera
  int device_id{-1};
  std::string device_path{"synthetic"};
  std::string backend;
};

/// Full AeroHub settings file (ROS + Camera + JSBSim).
///
/// **Memory:** tree of groups; each node has leaf values and child groups.
/// **Disk:** Qt QSettings IniFormat — top-level [Section], nested keys with '\\'
/// (e.g. [JSBSim] airport\\magvar=...). Load also accepts legacy [JSBSim/airport]
/// section headers.
///
/// Always starts from built-in defaults, then overlays keys present in the INI.
/// Load() only looks for "./aerohub_settings.ini" in the process CWD (no parent
/// search, no alternate paths). Missing file → stderr warning + defaults
/// (+ attempt to write a new file in CWD).
class Settings {
 public:
  static constexpr std::uint8_t DefaultRosDomainId = 42;
  static constexpr char SettingsFileName[] = "aerohub_settings.ini";

  /// One group in the settings tree (mirrors a QSettings group).
  struct Node {
    /// Leaf key → value at this group level.
    std::map<std::string, std::string> values;
    /// Subgroup name → child node.
    std::map<std::string, Node> children;

    [[nodiscard]] bool Empty() const { return values.empty() && children.empty(); }
  };

  /// Load "./aerohub_settings.ini" from the current working directory only.
  static Settings Load();
  /// Load an explicit path (tests / callers that already know the path).
  static Settings LoadFromPath(std::string path);

  Settings(const Settings&) = default;
  Settings& operator=(const Settings&) = default;
  Settings(Settings&&) noexcept = default;
  Settings& operator=(Settings&&) noexcept = default;

  [[nodiscard]] const std::string& Path() const { return path_; }

  [[nodiscard]] bool WasFileLoaded() const { return was_file_loaded_; }

  /// Root of the in-memory tree (children are top-level sections: ROS, Camera, JSBSim, …).
  [[nodiscard]] const Node& Root() const { return root_; }

  [[nodiscard]] Node& Root() { return root_; }

  /// Direct child group of root, or nullptr if missing.
  [[nodiscard]] const Node* Section(std::string_view name) const;
  [[nodiscard]] Node* Section(std::string_view name);

  /// Flatten under a top-level section for legacy callers (path uses '/').
  /// e.g. EntriesUnder("JSBSim") → ("airport/magvar","12.0"), …
  [[nodiscard]] std::vector<std::pair<std::string, std::string>> EntriesUnder(std::string_view top_section) const;

  /// Get/set by path segments from root (e.g. "ROS", "domain_id").
  [[nodiscard]] std::string Get(std::vector<std::string_view> path, std::string_view fallback = {}) const;
  void Set(std::vector<std::string_view> path, std::string value);

  /// Get/set using '/' or '\\' separated path from root (convenience).
  [[nodiscard]] std::string GetPath(std::string_view path, std::string_view fallback = {}) const;
  void SetPath(std::string_view path, std::string value);

  bool Save(std::string& error_out) const;
  bool Save() const;

  bool PersistCamera(const CameraSelection& camera, std::string& error_out) const;
  bool PersistCamera(const CameraSelection& camera) const;

  class RosSection {
   public:
    explicit RosSection(const Settings* owner) : owner_(owner) {}

    [[nodiscard]] std::uint8_t DomainId() const;
    [[nodiscard]] std::string NamespaceName() const;
    [[nodiscard]] std::string RmwImplementation() const;
    /// YOLO weights directory (relative to CWD or absolute); published as AERO_HUB_YOLO_MODELS.
    [[nodiscard]] std::string YoloModelsDir() const;
    void SetDomainId(std::uint8_t value) const;
    void SetNamespaceName(const std::string& value) const;
    void SetRmwImplementation(const std::string& value) const;
    void SetYoloModelsDir(const std::string& value) const;

   private:
    const Settings* owner_;

    Settings* MutableOwner() const { return const_cast<Settings*>(owner_); }
  };

  class CameraSection {
   public:
    explicit CameraSection(const Settings* owner) : owner_(owner) {}

    [[nodiscard]] std::string VideoSource() const;
    [[nodiscard]] int DeviceId() const;
    [[nodiscard]] std::string DevicePath() const;
    [[nodiscard]] std::string Backend() const;
    [[nodiscard]] CameraSelection Selection() const;
    void SetSelection(const CameraSelection& sel) const;

   private:
    const Settings* owner_;

    Settings* MutableOwner() const { return const_cast<Settings*>(owner_); }
  };

  class JsbSimSection {
   public:
    explicit JsbSimSection(const Settings* owner) : owner_(owner) {}

    /// Relative path under JSBSim (e.g. "ports/input" or "ports\\input").
    [[nodiscard]] std::string Get(std::string_view relative_key) const;
    void Set(std::string_view relative_key, std::string value) const;
    [[nodiscard]] const Node* Tree() const;

   private:
    const Settings* owner_;

    Settings* MutableOwner() const { return const_cast<Settings*>(owner_); }
  };

  [[nodiscard]] RosSection Ros() const { return RosSection(this); }

  [[nodiscard]] CameraSection Camera() const { return CameraSection(this); }

  [[nodiscard]] JsbSimSection JsbSim() const { return JsbSimSection(this); }

 private:
  Settings() = default;

  void PopulateDefaults();
  void OverlayFromFile(const std::string& path);
  /// Publish process env vars that ROS/tools read (from INI only — no shell defaults).
  /// Sets ROS_DOMAIN_ID, RMW_IMPLEMENTATION, AERO_HUB_YOLO_MODELS.
  void PublishRuntimeEnvFromSettings() const;
  static std::string ResolvePath();

  static std::vector<std::string> SplitPath(std::string_view path);
  static void FlattenNode(const Node& node, const std::string& prefix,
                          std::vector<std::pair<std::string, std::string>>& out);
  Node* EnsurePath(const std::vector<std::string>& path_to_group);
  const Node* FindNode(const std::vector<std::string>& path_to_group) const;

  std::string path_;
  bool was_file_loaded_{false};
  Node root_;
};

}  // namespace ah
