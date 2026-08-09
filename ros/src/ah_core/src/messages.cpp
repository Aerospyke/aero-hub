// Status JSON + detections parse (no JSON library in ah_core).

#include "ah_core/messages.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace ah_core
{
namespace
{

float ParseJsonFloatAt(const std::string & s, size_t key_pos, size_t key_len, bool * ok)
{
  *ok = false;
  size_t i = key_pos + key_len;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == ':')) {
    ++i;
  }
  if (i < s.size() && s[i] == ':') {
    ++i;
  }
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) {
    ++i;
  }
  try {
    size_t consumed = 0;
    const float v = std::stof(s.substr(i), &consumed);
    *ok = consumed > 0;
    return v;
  } catch (...) {
    return 0.f;
  }
}

/// First occurrence of "key": after @p from (must be before @p until if until != npos).
float ParseJsonFloatAfterKey(
  const std::string & s, size_t from, size_t until, const char * key, bool * ok)
{
  *ok = false;
  const std::string needle = std::string("\"") + key + "\"";
  const size_t k = s.find(needle, from);
  if (k == std::string::npos || (until != std::string::npos && k >= until)) {
    return 0.f;
  }
  return ParseJsonFloatAt(s, k, needle.size(), ok);
}

/// Last occurrence of "key": in [from, until) — used for track_id before bbox_normalized.
float ParseJsonFloatLastKeyBefore(
  const std::string & s, size_t from, size_t until, const char * key, bool * ok)
{
  *ok = false;
  const std::string needle = std::string("\"") + key + "\"";
  size_t last = std::string::npos;
  for (size_t p = from; p < until; ) {
    const size_t found = s.find(needle, p);
    if (found == std::string::npos || found >= until) {
      break;
    }
    last = found;
    p = found + 1;
  }
  if (last == std::string::npos) {
    return 0.f;
  }
  return ParseJsonFloatAt(s, last, needle.size(), ok);
}

}  // namespace

std::string JsonEscape(const std::string & s)
{
  std::string out;
  out.reserve(s.size());
  for (const char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

std::string CreateStatusJson(
  double stamp_sec,
  const char * video_status,
  bool tracking_started,
  bool segmentation_active,
  bool is_ai_tracking_active,
  bool following_active,
  const char * tracker_type,
  const CameraSelection & cam,
  float track_x, float track_y, float track_w, float track_h,
  int locked_track_id)
{
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss.precision(3);
  oss << '{'
      << "\"is_ai_tracking_active\":" << (is_ai_tracking_active ? "true" : "false") << ','
      << "\"was_tracking_started\":" << (tracking_started ? "true" : "false") << ','
      << "\"segmentation_active\":" << (segmentation_active ? "true" : "false") << ','
      << "\"following_active\":" << (following_active ? "true" : "false") << ','
      << R"("video_status":")" << video_status << "\","
      << R"("tracker_type":")" << JsonEscape(tracker_type ? tracker_type : "stub") << "\","
      << R"("follower_mode":"none",)"
      << R"("video_source":")" << JsonEscape(cam.video_source) << "\","
      << "\"camera_device_id\":" << cam.device_id << ','
      << R"("camera_device_path":")" << JsonEscape(cam.device_path) << "\","
      << R"("camera_backend":")" << JsonEscape(cam.backend) << "\","
      << "\"locked_track_id\":" << locked_track_id << ','
      << "\"tracking_bbox_x\":" << track_x << ','
      << "\"tracking_bbox_y\":" << track_y << ','
      << "\"tracking_bbox_w\":" << track_w << ','
      << "\"tracking_bbox_h\":" << track_h << ','
      << "\"stamp\":" << stamp_sec
      << '}';
  return oss.str();
}

std::vector<DetectionBox> ParseDetectionsJson(const std::string & json)
{
  // Pair track_id[i] with bbox_normalized[i] by document order (ah_yolo always emits
  // track_id before bbox in each detection object). This avoids sticky wrong IDs from
  // windowed key search when multiple detections share one JSON payload.
  std::vector<int> track_ids;
  {
    const std::string tid_key = "\"track_id\"";
    size_t p = 0;
    while (true) {
      const size_t k = json.find(tid_key, p);
      if (k == std::string::npos) {
        break;
      }
      bool okt = false;
      const float tid_f = ParseJsonFloatAt(json, k, tid_key.size(), &okt);
      track_ids.push_back(okt ? static_cast<int>(tid_f) : -1);
      p = k + tid_key.size();
    }
  }

  std::vector<DetectionBox> parsed;
  size_t pos = 0;
  size_t det_index = 0;
  while (true) {
    const size_t bb = json.find("\"bbox_normalized\"", pos);
    if (bb == std::string::npos) {
      break;
    }
    // Scope x,y,w,h to this bbox object only (avoid leaking into later objects).
    const size_t brace = json.find('{', bb);
    size_t bb_end = std::string::npos;
    if (brace != std::string::npos) {
      bb_end = json.find('}', brace);
      if (bb_end != std::string::npos) {
        ++bb_end;
      }
    }
    bool okx = false, oky = false, okw = false, okh = false, okc = false;
    DetectionBox d;
    d.x = ParseJsonFloatAfterKey(json, bb, bb_end, "x", &okx);
    d.y = ParseJsonFloatAfterKey(json, bb, bb_end, "y", &oky);
    d.w = ParseJsonFloatAfterKey(json, bb, bb_end, "w", &okw);
    d.h = ParseJsonFloatAfterKey(json, bb, bb_end, "h", &okh);
    // confidence lives in the same detection object, just before bbox_normalized.
    const size_t conf_from = (det_index == 0) ? 0 : pos;
    d.confidence = ParseJsonFloatLastKeyBefore(json, conf_from, bb, "confidence", &okc);
    (void)okc;
    d.track_id = (det_index < track_ids.size()) ? track_ids[det_index] : -1;
    if (okx && oky && okw && okh && d.w > 0.f && d.h > 0.f) {
      parsed.push_back(d);
    }
    ++det_index;
    pos = bb + 17;  // length of "bbox_normalized" including quotes
  }
  return parsed;
}

}  // namespace ah_core
