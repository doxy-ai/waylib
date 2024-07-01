// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <https://unlicense.org>

#ifndef __OPEN_URL_HPP__
#define __OPEN_URL_HPP__

#include <string_view>
#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include <windows.h>
#else
    #include <cstdlib>
#endif

#ifdef OPEN_URL_NAMESPACE
namespace OPEN_URL_NAMESPACE {
#endif

inline bool open_url(std::string_view url) {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    return ShellExecuteA(NULL, "open", std::string(url)).c_str(), NULL, NULL, SW_SHOWNORMAL) >= 32;
#elif __APPLE__
    return system(( "open " + std::string(url)).c_str()) != -1;
#elif __linux__
    return system(("xdg-open " + std::string(url)).c_str()) != -1;
#else
    #error "Unsupported Platform"
#endif

}

#ifdef OPEN_URL_NAMESPACE
} // End namespace
#endif

#endif // __OPEN_URL_HPP__