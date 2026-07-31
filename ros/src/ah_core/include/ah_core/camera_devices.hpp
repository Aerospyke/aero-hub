#pragma once

// Camera enumeration, selection helpers, and long-lived capture (Task_30 / Task_31).
// No hard-coded single device index — list, then select by path/id.

#include <memory>
#include <string>
#include <vector>

// OpenCV Mat only — full VideoCapture stays in .cpp (pimpl).
#include <opencv2/core.hpp>

namespace ah_core
{

struct CameraDevice
{
  int id{-1};
  std::string path;
  std::string name;
  std::string backend;
};

struct CameraSelection
{
  std::string video_source{"synthetic"};  // synthetic | camera
  int device_id{-1};
  std::string device_path;
  std::string backend;
};

/// Platform default backend name (avfoundation / v4l2 / any).
std::string DefaultBackendName();

/// Brief open probe for index; returns false if open fails.
bool TryOpenIndex(
  int index,
  const std::string & backend_hint,
  std::string * opened_backend);

/// Parse "synthetic", "index:N", "/dev/videoN". Returns false if unrecognized.
bool ParseDevicePath(const std::string & path, int * out_id);

/// Stable-ish path for an index (prefer /dev/videoN on Linux when present).
std::string PathForIndex(int index);

/// Always includes synthetic entry first, then openable hardware devices.
/// If @p held_open_id is a device we already own exclusively, list it even when
/// a second open probe fails (macOS exclusive capture).
std::vector<CameraDevice> EnumerateCameraDevices(
  const std::string & backend_hint,
  int held_open_id = -999);

/// Synthetic test-pattern entry (id=-1, path=synthetic).
CameraDevice MakeSyntheticDevice();

/// Long-lived capture for /ah/video/compressed (Task_31).
class CameraCapture
{
public:
  CameraCapture();
  ~CameraCapture();

  CameraCapture(const CameraCapture &) = delete;
  CameraCapture & operator=(const CameraCapture &) = delete;
  CameraCapture(CameraCapture &&) noexcept;
  CameraCapture & operator=(CameraCapture &&) noexcept;

  /// Open selected camera (closes any previous device first).
  bool Open(const CameraSelection & selection, std::string * error_out = nullptr);

  void Close();

  [[nodiscard]] bool IsOpen() const;

  [[nodiscard]] int DeviceId() const;

  /// Grab one BGR frame. Returns false if not open or grab failed.
  bool ReadBgr(cv::Mat * out_bgr);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ah_core
