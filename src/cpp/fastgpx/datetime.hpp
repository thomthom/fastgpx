#pragma once

#include <chrono>
#include <string_view>

namespace fastgpx {

/**
 * @brief Parses a limited set of the ISO 8601 assumed to be used for most `.gpx` files.
 *
 * Variations observed:
 * - Only Extended Format.
 * - With or without fractional seconds. Three digits is the norm; xsd:dateTime allows any
 *   number, and one and seven digits occur in the wild (gpxpy's test corpus has both).
 * - Zulu hours are the norm, but timezone offsets might occur.
 * - Some mention of missing timezone notation all together.
 *   While ISO 8601 describe this as local time, one can probably assume this is
 *   an omission by the author and it really should be Zulu hours.
 *
 * | Example                       |                                  |
 * |-------------------------------|----------------------------------|
 * | 2008-07-18T16:07:50.000Z      | Fast path.                       |
 * | 2008-07-18T16:07:50Z          | Fast path.                       |
 * | 2008-07-18T16:07:50.5Z        |                                  |
 * | 2008-07-18T16:07:50.1234567Z  |                                  |
 * | 2008-07-18T16:07:50.000+02:00 | Not per GPX definition.          |
 * | 2008-07-18T16:07:50+02:00     | Not per GPX definition.          |
 * | 2008-07-18T16:07:50.000       | Assume Zulu time?                |
 * | 2008-07-18T16:07:50           | Assume Zulu time?                |
 *
 * @param time_str The ISO 8601 date-time string to parse.
 */
std::chrono::system_clock::time_point parse_gpx_time(std::string_view time_str);

} // namespace fastgpx
