#pragma once

// Small string helpers shared by settings / ROS namespace handling (C++17).

#include <string>
#include <string_view>

namespace ah {

/// Characters stripped from both ends of INI keys/values (space, tab, quotes).
inline constexpr std::string_view TrimIniChars{" \t\""};

/// Remove any of @p chars_to_trim from both ends of @p full_string.
std::string Trim(const std::string& full_string, std::string_view chars_to_trim);

/// Sanitize ROS graph namespace: trim INI noise + slashes; reject "//" empty segments.
/// Empty result = root graph.
std::string SanitizeNamespace(const std::string& raw_namespace_setting);

}  // namespace ah
