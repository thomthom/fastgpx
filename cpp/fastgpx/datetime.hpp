#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fastgpx {

class fastgpx_error : public std::runtime_error
{
public:
  explicit fastgpx_error(const std::string &message) : std::runtime_error(message) {}
};

class parse_error : public fastgpx_error
{
public:
  explicit parse_error(const std::string &message) : fastgpx_error(message) {}
  parse_error(const std::string &message, std::string_view source_str, std::string_view sub_str);
};

namespace v1 {

std::chrono::system_clock::time_point parse_iso8601(const std::string &time_str);

} // namespace v1

namespace v2 {

std::chrono::utc_clock::time_point parse_iso8601(const std::string &time_str);

} // namespace v2

namespace v3 {

std::chrono::system_clock::time_point parse_iso8601(const std::string &time_str);

} // namespace v3

namespace v4 {

std::chrono::system_clock::time_point parse_iso8601(std::string_view time_str);

} // namespace v4

namespace v5 {

std::chrono::system_clock::time_point parse_iso8601(std::string_view time_str);

} // namespace v5

namespace v6 {

/**
 * @brief Parses a limited set of the ISO 8601 assumed to be used for most `.gpx` files.
 *
 * Variations observed:
 * - Only Extended Format.
 * - With or without milliseconds. (3 fractional decimals)
 * - Zulu hours are the norm, but timezone offsets might occur.
 * - Some mention of missing timezone notation all together.
 *   While ISO 8601 describe this as local time, one can probably assume this is
 *   an omission by the author and it really should be Zulu hours.
 *
 * | Example                       | Length |                                  |
 * |-------------------------------|--------|----------------------------------|
 * | 2008-07-18T16:07:50.000+02:00 |     29 | Not per GPX definition.          |
 * | 2008-07-18T16:07:50.000Z      |     24 |                                  |
 * | 2008-07-18T16:07:50.000       |     23 | Assume Zulu time?                |
 * | 2008-07-18T16:07:50+02:00     |     25 | Not per GPX definition.          |
 * | 2008-07-18T16:07:50Z          |     20 |                                  |
 * | 2008-07-18T16:07:50           |     19 | Assume Zulu time?                |
 *
 * @param time_str
 * @return std::chrono::system_clock::time_point
 */
std::chrono::system_clock::time_point parse_gpx_time(std::string_view time_str);

} // namespace v6

using v3::parse_iso8601;
using v6::parse_gpx_time;

} // namespace fastgpx
