#pragma once

#include <filesystem>
#include <string>

namespace fastgpx {

#ifdef _WIN32
/**
 * @brief Helper for Windows to convert UTF-8 strings to UTF-16 for dealing with
 *   file operations.
 *
 * @param utf8_str
 */
std::wstring utf8_to_utf16(const std::string& utf8_str);
#endif

/**
 * @brief Opens a file for reading in binary mode given a UTF-8 path.
 *
 * @param file_path UTF-8 file path.
 * @return FILE*
 */
FILE* open_file(const std::filesystem::path& file_path);

} // namespace fastgpx
