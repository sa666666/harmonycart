//============================================================================
//
//  BBBBB    SSSS   PPPPP   FFFFFF
//  BB  BB  SS  SS  PP  PP  FF
//  BB  BB  SS      PP  PP  FF
//  BBBBB    SSSS   PPPPP   FFFF    --  "Brad's Simple Portability Framework"
//  BB  BB      SS  PP      FF
//  BB  BB  SS  SS  PP      FF
//  BBBBB    SSSS   PP      FF
//
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef BSPF_HXX
#define BSPF_HXX

/**
  This file defines various basic data types and preprocessor variables
  that need to be defined for different operating systems.

  @author Bradford W. Mott and Stephen Anthony
*/

#include <climits>
#include <cstdint>
// Types for 8/16/32/64-bit signed and unsigned integers
using Int8   = int8_t;
using uInt8  = uint8_t;
using Int16  = int16_t;
using uInt16 = uint16_t;
using Int32  = int32_t;
using uInt32 = uint32_t;
using Int64  = int64_t;
using uInt64 = uint64_t;

// The following code should provide access to the standard C++ objects and
// types: cout, cerr, string, ostream, istream, etc.
#include <array>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <utility>
#include <vector>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::istream;
using std::ostream;
using std::fstream;
using std::iostream;
using std::ostringstream;
using std::istringstream;
using std::stringstream;
using std::unique_ptr;
using std::shared_ptr;
using std::make_unique;
using std::make_shared;
using std::array;
using std::vector;
using std::runtime_error;

// Common array types
using IntArray = std::vector<Int32>;
using uIntArray = std::vector<uInt32>;
using BoolArray = std::vector<bool>;
using ByteArray = std::vector<uInt8>;
using ShortArray = std::vector<uInt16>;
using StringList = std::vector<std::string>;
using ByteBuffer = std::unique_ptr<uInt8[]>;  // NOLINT
using DWordBuffer = std::unique_ptr<uInt32[]>;  // NOLINT

using AdjustFunction = std::function<void(int)>;

// We use KB a lot; let's make a literal for it
constexpr size_t operator "" _KB(unsigned long long size)
{
   return static_cast<size_t>(size * 1024);
}

// Output contents of a vector
template<typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
  for(const auto& elem: v)
    out << elem << " ";
  return out;
}

static const string EmptyString("");

// This is defined by some systems, but Stella has other uses for it
#undef PAGE_SIZE
#undef PAGE_MASK

// Adaptable refresh is currently not available on MacOS
// In the future, this may expand to other systems
#if !defined(BSPF_MACOS)
  #define ADAPTABLE_REFRESH_SUPPORT
#endif

namespace BSPF
{
  static constexpr float PI_f = 3.141592653589793238462643383279502884F;
  static constexpr double PI_d = 3.141592653589793238462643383279502884;

  // CPU architecture type
  // This isn't complete yet, but takes care of all the major platforms
  #if defined(__i386__) || defined(_M_IX86)
    static const string ARCH = "i386";
  #elif defined(__x86_64__) || defined(_WIN64)
    static const string ARCH = "x86_64";
  #elif defined(__powerpc__) || defined(__ppc__)
    static const string ARCH = "ppc";
  #elif defined(__arm__) || defined(__thumb__)
    static const string ARCH = "arm32";
  #elif defined(__aarch64__)
    static const string ARCH = "arm64";
  #else
    static const string ARCH = "NOARCH";
  #endif

  // Get next power of two greater than or equal to the given value
  inline size_t nextPowerOfTwo(size_t size) {
    if(size < 2) return 1;
    size_t power2 = 1;
    while(power2 < size)
      power2 <<= 1;
    return power2;
  }

  // Get next multiple of the given value
  // Note that this only works when multiple is a power of two
  inline size_t nextMultipleOf(size_t size, size_t multiple) {
    return (size + multiple - 1) & ~(multiple - 1);
  }

  // Make 2D-arrays using std::array less verbose
  template<typename T, size_t ROW, size_t COL>
  using array2D = std::array<std::array<T, COL>, ROW>;

  // Combines 'max' and 'min', and clamps value to the upper/lower value
  // if it is outside the specified range
  template<typename T> inline T clamp(T val, T lower, T upper)
  {
    return (val < lower) ? lower : (val > upper) ? upper : val;
  }
  template<typename T> inline void clamp(T& val, T lower, T upper, T setVal)
  {
    if(val < lower || val > upper)  val = setVal;
  }
  template<typename T> inline T clampw(T val, T lower, T upper)
  {
    return (val < lower) ? upper : (val > upper) ? lower : val;
  }

  // Test whether the vector contains the given value
  template<typename T>
  bool contains(const std::vector<T>& v, const T& elem) {
    return !(v.empty() || std::find(v.begin(), v.end(), elem) == v.end());
  }

  // Convert string to given case
  inline const string& toUpperCase(string& s)
  {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
  }
  inline const string& toLowerCase(string& s)
  {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
  }

  // Convert string to integer, using default value on any error
  inline int stringToInt(const string& s, const int defaultValue = 0)
  {
    try        { return std::stoi(s); }
    catch(...) { return defaultValue; }
  }

  // Compare two strings, ignoring case
  inline int compareIgnoreCase(const string& s1, const string& s2)
  {
  #if (defined BSPF_WINDOWS || defined __WIN32__) && !defined __GNUG__
    return _stricmp(s1.c_str(), s2.c_str());
  #else
    return strcasecmp(s1.c_str(), s2.c_str());
  #endif
  }
  inline int compareIgnoreCase(const char* s1, const char* s2)
  {
  #if (defined BSPF_WINDOWS || defined __WIN32__) && !defined __GNUG__
    return _stricmp(s1, s2);
  #else
    return strcasecmp(s1, s2);
  #endif
  }

  // Test whether the first string starts with the second one (case insensitive)
  inline bool startsWithIgnoreCase(const string& s1, const string& s2)
  {
  #if (defined BSPF_WINDOWS || defined __WIN32__) && !defined __GNUG__
    return _strnicmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
  #else
    return strncasecmp(s1.c_str(), s2.c_str(), s2.length()) == 0;
  #endif
  }
  inline bool startsWithIgnoreCase(const char* s1, const char* s2)
  {
  #if (defined BSPF_WINDOWS || defined __WIN32__) && !defined __GNUG__
    return _strnicmp(s1, s2, strlen(s2)) == 0;
  #else
    return strncasecmp(s1, s2, strlen(s2)) == 0;
  #endif
  }

  // Test whether two strings are equal (case insensitive)
  inline bool equalsIgnoreCase(const string& s1, const string& s2)
  {
    return compareIgnoreCase(s1, s2) == 0;
  }

  // Find location (if any) of the second string within the first,
  // starting from 'startpos' in the first string
  inline size_t findIgnoreCase(const string& s1, const string& s2, size_t startpos = 0)
  {
    auto pos = std::search(s1.cbegin()+startpos, s1.cend(),
      s2.cbegin(), s2.cend(), [](char ch1, char ch2) {
        return toupper(uInt8(ch1)) == toupper(uInt8(ch2));
      });
    return pos == s1.cend() ? string::npos : pos - (s1.cbegin()+startpos);
  }

  // Test whether the first string ends with the second one (case insensitive)
  inline bool endsWithIgnoreCase(const string& s1, const string& s2)
  {
    if(s1.length() >= s2.length())
    {
      const char* end = s1.c_str() + s1.length() - s2.length();
      return compareIgnoreCase(end, s2.c_str()) == 0;
    }
    return false;
  }

  // Test whether the first string contains the second one (case insensitive)
  inline bool containsIgnoreCase(const string& s1, const string& s2)
  {
    return findIgnoreCase(s1, s2) != string::npos;
  }

  // Test whether the first string matches the second one (case insensitive)
  // - the first character must match
  // - the following characters must appear in the order of the first string
  inline bool matches(const string& s1, const string& s2)
  {
    if(BSPF::startsWithIgnoreCase(s1, s2.substr(0, 1)))
    {
      size_t pos = 1;
      for(uInt32 j = 1; j < s2.size(); ++j)
      {
        size_t found = BSPF::findIgnoreCase(s1, s2.substr(j, 1), pos);
        if(found == string::npos)
          return false;
        pos += found + 1;
      }
      return true;
    }
    return false;
  }

  // C++11 way to get local time
  // Equivalent to the C-style localtime() function, but is thread-safe
  inline std::tm localTime()
  {
    std::time_t currtime;
    std::time(&currtime);
    std::tm tm_snapshot;
  #if (defined BSPF_WINDOWS || defined __WIN32__) && (!defined __GNUG__ || defined __MINGW32__)
    localtime_s(&tm_snapshot, &currtime);
  #else
    localtime_r(&currtime, &tm_snapshot);
  #endif
    return tm_snapshot;
  }

  // Coverity complains if 'getenv' is used unrestricted
  inline string getenv(const string& env_var)
  {
  #if (defined BSPF_WINDOWS || defined __WIN32__) && !defined __GNUG__
    char* buf = nullptr;
    size_t sz = 0;
    if(_dupenv_s(&buf, &sz, env_var.c_str()) == 0 && buf != nullptr)
    {
      string val(buf);
      free(buf);
      return val;
    }
    return EmptyString;
  #else
    try {
      const char* val = std::getenv(env_var.c_str());
      return val ? string(val) : EmptyString;
    }
    catch(...) {
      return EmptyString;
    }
  #endif
  }
} // namespace BSPF

#endif
