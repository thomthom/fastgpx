#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fastgpx {

namespace v1 {

/**
 * @brief Parses ISO 8601 date-time strings using `istringstream` and `std::get_time`.
 *
 * @param time_str The ISO 8601 date-time string to parse.
 */
std::chrono::system_clock::time_point parse_iso8601(const std::string& time_str);

} // namespace v1

namespace v2 {

/**
 * @brief Parses ISO 8601 date-time strings using `istringstream` and `std::chrono::parse` with
 *   `std::chrono::utc_clock`.
 *
 * @note This only parses `%Y-%m-%dT%H:%M:%SZ` format.
 *
 * @param time_str The ISO 8601 date-time string to parse.
 */
std::chrono::utc_clock::time_point parse_iso8601(const std::string& time_str);

} // namespace v2

namespace v3 {

/**
 * @brief Parses ISO 8601 date-time strings using `istringstream` and `std::chrono::parse` with
 *   `std::chrono::sys_time`.
 *
 * Attempting to parse a wider range of formats by parsing various formats until it succeeds.
 *
 * @param time_str The ISO 8601 date-time string to parse.
 */
std::chrono::system_clock::time_point parse_iso8601(const std::string& time_str);

} // namespace v3

namespace v4 {

/**
 * @brief Parses ISO 8601 date-time strings using `std::from_chars`.
 *
 * This is a more manual approach to parsing ISO 8601 date-time strings.
 * It assumes the format is `YYYY-MM-DDTHH:MM:SSZ`.
 *
 * @param time_str The ISO 8601 date-time string to parse.
 */
std::chrono::system_clock::time_point parse_iso8601(std::string_view time_str);

} // namespace v4

namespace v5 {

/**
 * @brief Parses ISO 8601 date-time strings using a custom parser.
 *
 * This is a more complete parser for ISO 8601 date-time strings, but not 100%
 * compliant with the standard.
 *
 * @note If the data string doesn't have a timezone indicator, it assumes Zulu time
 *   instead of local time as the ISO 8601 standard suggests.
 *
 * @param time_str The ISO 8601 date-time string to parse.
 */
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
 * | 2008-07-18T16:07:50.000Z      |     24 |                                  |
 * | 2008-07-18T16:07:50Z          |     20 |                                  |
 * | 2008-07-18T16:07:50.000+02:00 |     29 | Not per GPX definition.          |
 * | 2008-07-18T16:07:50+02:00     |     25 | Not per GPX definition.          |
 * | 2008-07-18T16:07:50.000       |     23 | Assume Zulu time?                |
 * | 2008-07-18T16:07:50           |     19 | Assume Zulu time?                |
 *
 * @param time_str The ISO 8601 date-time string to parse.
 */
std::chrono::system_clock::time_point parse_gpx_time(std::string_view time_str);

} // namespace v6

using v3::parse_iso8601;
using v6::parse_gpx_time;

} // namespace fastgpx
