// Formatting library for C++ - time formatting
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_TIME_H_
#define FMT_TIME_H_

#include "format.h"
#include <ctime>

FMT_BEGIN_NAMESPACE

namespace internal {
// Use nullptr instead of null<> and FMT_NULL
inline std::tm* localtime_r(const std::time_t* time, std::tm* tm) { return std::localtime_r(time, tm); }
inline std::tm* localtime_s(std::tm* tm, const std::time_t* time) { return std::localtime(time); } // Fallback
inline std::tm* gmtime_r(const std::time_t* time, std::tm* tm) { return std::gmtime_r(time, tm); }
inline std::tm* gmtime_s(std::tm* tm, const std::time_t* time) { return std::gmtime(time); } // Fallback

// Dummy iterator for null_terminating_iterator
template <typename Char>
using null_terminating_iterator = Char*;

// Dummy pointer_from
template <typename T>
T* pointer_from(T* p) { return p; }
}

// Thread-safe replacement for std::localtime
inline std::tm localtime(std::time_t time) {
  struct dispatcher {
    std::time_t time_;
    std::tm tm_;

    dispatcher(std::time_t t) : time_(t) {}

    bool run() {
      using namespace fmt::internal;
      return handle(localtime_r(&time_, &tm_));
    }

    bool handle(std::tm* tm) { return tm != nullptr; }

    bool handle(std::tm*) {
      using namespace fmt::internal;
      return fallback(localtime_s(&tm_, &time_));
    }

    bool fallback(std::tm* tm) { return tm != nullptr; }

    bool fallback(std::tm*) {
      std::tm* tm = std::localtime(&time_);
      if (tm) tm_ = *tm;
      return tm != nullptr;
    }
  };
  dispatcher lt(time);
  if (lt.run())
    return lt.tm_;
  FMT_THROW(format_error("time_t value out of range"));
}

// Thread-safe replacement for std::gmtime
inline std::tm gmtime(std::time_t time) {
  struct dispatcher {
    std::time_t time_;
    std::tm tm_;

    dispatcher(std::time_t t) : time_(t) {}

    bool run() {
      using namespace fmt::internal;
      return handle(gmtime_r(&time_, &tm_));
    }

    bool handle(std::tm* tm) { return tm != nullptr; }

    bool handle(std::tm*) {
      using namespace fmt::internal;
      return fallback(gmtime_s(&tm_, &time_));
    }

    bool fallback(std::tm* tm) { return tm != nullptr; }

    bool fallback(std::tm*) {
      std::tm* tm = std::gmtime(&time_);
      if (tm) tm_ = *tm;
      return tm != nullptr;
    }
  };
  dispatcher gt(time);
  if (gt.run())
    return gt.tm_;
  FMT_THROW(format_error("time_t value out of range"));
}

namespace internal {
inline std::size_t strftime(char* str, std::size_t count, const char* format,
                            const std::tm* time) {
  return std::strftime(str, count, format, time);
}

inline std::size_t strftime(wchar_t* str, std::size_t count,
                            const wchar_t* format, const std::tm* time) {
  return std::wcsftime(str, count, format, time);
}
}

template <typename Char>
struct formatter<std::tm, Char> {
  template <typical ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    auto it = internal::null_terminating_iterator<Char>(ctx.begin());
    if (*it == ':')
      ++it;
    auto end = it;
    while (*end && *end != '}')
      ++end;
    tm_format.reserve(end - it + 1);
    tm_format.append(it, end);
    tm_format.push_back('\0');
    return end;
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
      if (size >= tm_format.size() * 256) {
        break;
      }
      const std::size_t MIN_GROWTH = 10;
      buf.reserve(buf.capacity() + (size > MIN_GROWTH ? size : MIN_GROWTH));
    }
    return ctx.out();
  }

  basic_memory_buffer<Char> tm_format;
};

FMT_END_NAMESPACE

#endif  // FMT_TIME_H_
