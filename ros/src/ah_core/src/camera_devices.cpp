#include "ah_core/camera_devices.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#if defined(__linux__)
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace ah_core
{
namespace
{
// Keep the probe short: macOS often only has a few indices; long probes spam
// OpenCV ("out device of bound"). Stop after one consecutive open failure.
constexpr int kMaxCameraProbeIndex = 6;
constexpr int kMaxConsecutiveOpenFails = 1;

int BackendApiPreference(const std::string & backend)
{
  if (backend.empty() || backend == "any") {
    return cv::CAP_ANY;
  }
#if defined(__APPLE__)
  if (backend == "avfoundation") {
    return cv::CAP_AVFOUNDATION;
  }
#endif
#if defined(__linux__)
  if (backend == "v4l2") {
    return cv::CAP_V4L2;
  }
#endif
  return cv::CAP_ANY;
}
}  // namespace

std::string DefaultBackendName()
{
#if defined(__APPLE__)
  return "avfoundation";
#elif defined(__linux__)
  return "v4l2";
#else
  return "any";
#endif
}

bool TryOpenIndex(int index, const std::string & backend_hint, std::string * opened_backend)
{
  // Prefer platform backend only (skip CAP_ANY fallback) to avoid double OpenCV
  // error spam and long hangs on missing indices.
  const int api = BackendApiPreference(backend_hint);
  cv::VideoCapture cap;
  if (!cap.open(index, api)) {
    return false;
  }
  (void)cap.grab();
  if (opened_backend != nullptr) {
    *opened_backend = backend_hint.empty() || backend_hint == "any"
      ? DefaultBackendName()
      : backend_hint;
  }
  cap.release();
  return true;
}

bool ParseDevicePath(const std::string & path, int * out_id)
{
  if (path.empty() || path == "synthetic") {
    if (out_id) {
      *out_id = -1;
    }
    return path == "synthetic";
  }
  if (path.rfind("index:", 0) == 0) {
    try {
      const int id = std::stoi(path.substr(6));
      if (out_id) {
        *out_id = id;
      }
      return id >= 0;
    } catch (...) {
      return false;
    }
  }
#if defined(__linux__)
  if (path.rfind("/dev/video", 0) == 0) {
    try {
      const int id = std::stoi(path.substr(std::string("/dev/video").size()));
      if (out_id) {
        *out_id = id;
      }
      return id >= 0;
    } catch (...) {
      return false;
    }
  }
#endif
  return false;
}

std::string PathForIndex(int index)
{
#if defined(__linux__)
  const std::string dev = "/dev/video" + std::to_string(index);
  struct stat st {};
  if (stat(dev.c_str(), &st) == 0) {
    return dev;
  }
#endif
  return "index:" + std::to_string(index);
}

CameraDevice MakeSyntheticDevice()
{
  return CameraDevice{-1, "synthetic", "Synthetic test pattern", "none"};
}

std::vector<CameraDevice> EnumerateCameraDevices(
  const std::string & backend_hint,
  int held_open_id)
{
  std::vector<CameraDevice> devices;

  // Always offer synthetic so operators never depend on a magic camera index.
  devices.push_back(MakeSyntheticDevice());

  std::vector<int> indices_to_try;
#if defined(__linux__)
  if (DIR * dir = opendir("/dev")) {
    while (dirent * ent = readdir(dir)) {
      const std::string name = ent->d_name;
      if (name.rfind("video", 0) != 0) {
        continue;
      }
      try {
        const int id = std::stoi(name.substr(5));
        indices_to_try.push_back(id);
      } catch (...) {
      }
    }
    closedir(dir);
    std::sort(indices_to_try.begin(), indices_to_try.end());
    indices_to_try.erase(
      std::unique(indices_to_try.begin(), indices_to_try.end()), indices_to_try.end());
  }
#endif

  if (indices_to_try.empty()) {
    for (int i = 0; i <= kMaxCameraProbeIndex; ++i) {
      indices_to_try.push_back(i);
    }
  }

  int consecutive_fails = 0;
  for (const int index : indices_to_try) {
    std::string opened_backend;
    if (!TryOpenIndex(index, backend_hint, &opened_backend)) {
      // Device may be held exclusively by CameraCapture — still list it.
      if (index == held_open_id && held_open_id >= 0) {
        consecutive_fails = 0;
        CameraDevice d;
        d.id = index;
        d.path = PathForIndex(index);
        d.name = "Camera " + std::to_string(index) + " (in use)";
        d.backend = backend_hint.empty() ? DefaultBackendName() : backend_hint;
        devices.push_back(std::move(d));
        continue;
      }
      ++consecutive_fails;
      if (consecutive_fails >= kMaxConsecutiveOpenFails) {
        break;
      }
      continue;
    }
    consecutive_fails = 0;
    CameraDevice d;
    d.id = index;
    d.path = PathForIndex(index);
    d.name = "Camera " + std::to_string(index);
    d.backend = opened_backend.empty() ? DefaultBackendName() : opened_backend;
    devices.push_back(std::move(d));
  }

  return devices;
}

// --- CameraCapture ---------------------------------------------------------

struct CameraCapture::Impl
{
  cv::VideoCapture cap;
  int device_id{-1};
  std::string backend;
};

namespace
{
// All-black (or near-black) frames after open: wrong index, lid closed, or
// macOS Camera TCC denied for this process (Terminal / ros2).
constexpr double kMinMeanLuma = 8.0;
constexpr int kWarmupFrames = 8;

double MeanLumaBgr(const cv::Mat & bgr)
{
  if (bgr.empty()) {
    return 0.0;
  }
  cv::Mat gray;
  if (bgr.channels() == 1) {
    gray = bgr;
  } else {
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
  }
  return cv::mean(gray)[0];
}

bool TryOpenCapture(
  cv::VideoCapture * cap,
  int device_id,
  int api,
  cv::Mat * last_frame,
  double * mean_luma,
  std::string * detail)
{
  if (!cap->open(device_id, api)) {
    if (detail) {
      *detail = "open() failed api=" + std::to_string(api);
    }
    return false;
  }

  // Do NOT force CAP_PROP_FRAME_WIDTH/HEIGHT on macOS AVFoundation — that often
  // yields a black stream while isOpened() stays true (and USB LED stays off).

  cv::Mat frame;
  for (int i = 0; i < kWarmupFrames; ++i) {
    if (!cap->read(frame) || frame.empty()) {
      cap->release();
      if (detail) {
        *detail = "read failed during warmup (" + std::to_string(i) +
                  "/" + std::to_string(kWarmupFrames) + ")";
      }
      return false;
    }
  }

  const double mean = MeanLumaBgr(frame);
  if (mean_luma) {
    *mean_luma = mean;
  }
  if (last_frame) {
    *last_frame = frame;
  }

  if (mean < kMinMeanLuma) {
    cap->release();
    if (detail) {
      *detail =
        "frames nearly black (mean_luma=" + std::to_string(mean) +
        " size=" + std::to_string(frame.cols) + "x" + std::to_string(frame.rows) +
        "). Wrong index, lid closed, or grant Camera access to Terminal "
        "(System Settings → Privacy & Security → Camera). USB LED should be on.";
    }
    return false;
  }

  if (detail) {
    *detail = "ok mean_luma=" + std::to_string(mean) + " size=" +
              std::to_string(frame.cols) + "x" + std::to_string(frame.rows);
  }
  return true;
}
}  // namespace

CameraCapture::CameraCapture()
: impl_(std::make_unique<Impl>())
{
}

CameraCapture::~CameraCapture()
{
  Close();
}

CameraCapture::CameraCapture(CameraCapture &&) noexcept = default;
CameraCapture & CameraCapture::operator=(CameraCapture &&) noexcept = default;

bool CameraCapture::Open(const CameraSelection & selection, std::string * error_out)
{
  Close();

  if (selection.video_source != "camera" || selection.device_id < 0) {
    if (error_out) {
      *error_out = "selection is not a camera device";
    }
    return false;
  }

  const std::string hint =
    selection.backend.empty() ? DefaultBackendName() : selection.backend;
  const int preferred_api = BackendApiPreference(hint);

  // Prefer platform backend, then CAP_ANY (some USB cams only work with one).
  std::vector<int> apis_to_try;
  apis_to_try.push_back(preferred_api);
  if (preferred_api != cv::CAP_ANY) {
    apis_to_try.push_back(cv::CAP_ANY);
  }

  std::string last_detail;
  for (const int api : apis_to_try) {
    cv::Mat last;
    double mean = 0.0;
    std::string detail;
    if (TryOpenCapture(&impl_->cap, selection.device_id, api, &last, &mean, &detail)) {
      impl_->device_id = selection.device_id;
      impl_->backend = (api == cv::CAP_ANY && preferred_api != cv::CAP_ANY) ? "any" : hint;
      return true;
    }
    last_detail = detail;
    impl_->cap.release();
  }

  if (error_out) {
    *error_out = "camera id=" + std::to_string(selection.device_id) +
                 " backend=" + hint + ": " + last_detail;
  }
  return false;
}

void CameraCapture::Close()
{
  if (impl_ && impl_->cap.isOpened()) {
    impl_->cap.release();
  }
  if (impl_) {
    impl_->device_id = -1;
    impl_->backend.clear();
  }
}

bool CameraCapture::IsOpen() const
{
  return impl_ && impl_->cap.isOpened();
}

int CameraCapture::DeviceId() const
{
  return impl_ ? impl_->device_id : -1;
}

bool CameraCapture::ReadBgr(cv::Mat * out_bgr)
{
  if (!out_bgr || !IsOpen()) {
    return false;
  }
  cv::Mat frame;
  if (!impl_->cap.read(frame) || frame.empty()) {
    return false;
  }
  if (frame.channels() == 1) {
    cv::cvtColor(frame, *out_bgr, cv::COLOR_GRAY2BGR);
  } else if (frame.channels() == 4) {
    cv::cvtColor(frame, *out_bgr, cv::COLOR_BGRA2BGR);
  } else {
    *out_bgr = frame;
  }
  return true;
}

}  // namespace ah_core
