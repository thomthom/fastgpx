#include "fastgpx/filesystem.hpp"

#ifdef _WIN32
  #include <windows.h> // for _wfopen
  #include <stdexcept>
#endif

namespace fastgpx {

#ifdef _WIN32
std::wstring utf8_to_utf16(const std::string &utf8_str)
{
  if (utf8_str.empty())
  {
    return std::wstring(); // Return an empty wide string if input is empty
  }

  // Step 1: Determine the size of the wide string buffer required
  int wide_char_count =
      MultiByteToWideChar(CP_UTF8,                           // Source string is in UTF-8
                          0,                                 // No special flags
                          utf8_str.c_str(),                  // Input UTF-8 string
                          static_cast<int>(utf8_str.size()), // Length of the input string
                          nullptr,                           // No output buffer yet
                          0                                  // Request size of the output buffer
      );

  if (wide_char_count == 0)
  {
    throw std::runtime_error("Error converting UTF-8 to UTF-16: MultiByteToWideChar failed.");
  }

  // Step 2: Allocate the necessary buffer
  std::wstring utf16_str(wide_char_count, 0);

  // Step 3: Perform the conversion
  int result = MultiByteToWideChar(CP_UTF8,                           // Source string is in UTF-8
                                   0,                                 // No special flags
                                   utf8_str.c_str(),                  // Input UTF-8 string
                                   static_cast<int>(utf8_str.size()), // Length of the input string
                                   &utf16_str[0],  // Output buffer for the wide string
                                   wide_char_count // Size of the output buffer
  );

  if (result == 0)
  {
    throw std::runtime_error("Error converting UTF-8 to UTF-16: MultiByteToWideChar failed.");
  }

  return utf16_str;
}
#endif

FILE *open_file(const std::filesystem::path &file_path)
{
  FILE *file = nullptr;

#ifdef _WIN32
  // On Windows, open with wide string support
  const auto pathu16 = utf8_to_utf16(file_path.string());
  file = _wfopen(
      pathu16.c_str(),
      L"rb"); // Open for reading in binary mode
              // file = _wfopen(file_path.c_str(), L"rb"); // Open for reading in binary mode
#else
  // On other platforms, open with standard fopen
  file = fopen(file_path.string().c_str(), "rb"); // Open for reading in binary mode
#endif

  return file;
}

} // namespace fastgpx
