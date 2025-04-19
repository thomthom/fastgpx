#pragma once

#include <filesystem>
#include <string>

namespace fastgpx {

std::wstring utf8_to_utf16(const std::string& utf8_str);

/**
 * @brief Opens a file for reading in binary mode given a UTF-8 path.
 *
 * @param file_path UTF-8 file path.
 * @return FILE*
 */
FILE* open_file(const std::filesystem::path& file_path);

} // namespace fastgpx
