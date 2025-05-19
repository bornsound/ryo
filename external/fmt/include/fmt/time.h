// Formatting library for C++ - time formatting
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_TIME_H_
#define FMT_TIME_H_

#include <ctime>
#include "format.h" // Required for fmt core functionality (basic_buffer, format_error, etc.)

FMT_BEGIN_NAMESPACE

namespace internal {
// Thread-safe replacements for localtime_r, gmtime_r, etc.
inline std::tm* localtime_r(const std::time_t* time, std::tm* tm) {
  return ::localtime_r(time, tm); // POSIX localtime_r
}

inline std::tm* gmtime_r(const std::time_t* time, std::tm* tm) {
  return ::gmtime_r(time, tm); // POSIX gmtime_r
}

// Fallbacks for platforms without localtime_r/gmtime_r (e.g., Windows)
inline std::tm* localtime_s(std::tm* tm, const std::time_t* time) {
  std::tm* result = std::localtime(time);
  if (result) *tm = *result;
  return result;
}

inline std::tm* gmtime_s(std::tm* tm, const std::time_t* time) {
  std::tm* result = std::gmtime(time);
  if (result) *tm = *result;
  return result;
}

// strftime wrappers for char and wchar_t
inline std::size_t strftime(char* str, std::size_t count, const char* format,
                            const std::tm* time) {
  return std::strftime(str, count, format, time);
}

inline std::size_t strftime(wchar_t* str, std::size_t count,
                            const wchar_t* format, const std::tm* time) {
  return std::wcsftime(str, count, format, time);
}
} // namespace internal

// Thread-safe replacement for std::localtime
inline std::tm localtime(std::time_t time) {
  std::tm tm = {};
  std::tm* result = internal::localtime_r(&time, &tm);
  if (!result) {
    result = internal::localtime_s(&tm, &time);
  }
  if (!result) {
    FMT_THROW(format_error("localtime failed: time_t value out of range"));
  }
  return tm;
}

// Thread-safe replacement for std::gmtime
inline std::tm gmtime(std::time_t time) {
  std::tm tm = {};
  std::tm* result = internal::gmtime_r(&time, &tm);
  if (!result) {
    result = internal::gmtime_s(&tm, &time);
  }
  if (!result) {
    FMT_THROW(format_error("gmtime failed: time_t value out of range"));
  }
  return tm;
}

// Formatter for std::tm
template <typename Char>
struct formatter<std::tm, Char> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    // Use basic_string_view to handle format string parsing
    basic_string_view<Char> format(ctx.begin(), ctx.end());
    auto it = ctx.begin();
    auto end = ctx.end();
    if (it != end && *it == ':') ++it;
    tm_format.reserve(end != it ? (end - it + 1) : 1);
    while (it != end && *it != '}') {
      tm_format.push_back(*it++);
    }
    tm_format.push_back('\0');
    return it;
  }

  template <typename FormatContext>
  auto format(const std::tm& tm, FormatContext& ctx) -> decltype(ctx.out()) {
    internal::basic_buffer<Char>& buf = internal::get_container(ctx.out());
    std::size_t start = buf.size();
    for (;;) {
      std::size_t size = buf.capacity() - start;
      std::size_t count =
          internal::strftime(&buf[start], size, &tm_format[0], &tm);
      if (count != 0) {
        buf.resize(start + count);
        break;
      }
      const std::size_t MIN_GROWTH = 10;
      buf.reserve(buf.capacity() + (size > MIN_GROWTH ? size : MIN_GROWTH));
    }
    return ctx.out();
  }

 private:
  basic_memory_buffer<Char> tm_format;
};

FMT_END_NAMESPACE

#endif // FMT_TIME_H_
