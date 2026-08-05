#include "ah_common/string_util.hpp"

namespace ah
{

std::string Trim(const std::string & full_string, std::string_view chars_to_trim)
{
  const auto start = full_string.find_first_not_of(chars_to_trim);
  if (start == std::string::npos) {
    return {};
  }
  const auto end = full_string.find_last_not_of(chars_to_trim);
  return full_string.substr(start, end - start + 1);
}

std::string SanitizeNamespace(const std::string & raw_namespace_setting)
{
  std::string processed = Trim(raw_namespace_setting, kTrimIniChars);
  processed = Trim(processed, "/");
  // In ROS, '//' is an empty path segment — treat as bad config → root (empty).
  if (processed.find("//") != std::string::npos) {
    return {};
  }
  return processed;
}

}  // namespace ah
