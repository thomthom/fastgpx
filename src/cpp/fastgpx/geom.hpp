#pragma once

namespace fastgpx {
struct LatLong;

/**
 * @brief Geometry logic matching gpxpy.
 *
 * This is slightly faster than \ref v2, but not as accurate.
 */
namespace v1 {
/**
 * @brief Haversine distance returned in meters using gpxpy logic.
 *
 * @param ll1
 * @param ll2
 * @return double Meters.
 */
double haversine(const LatLong& ll1, const LatLong& ll2) noexcept;

/**
 * @brief Computes the distance in 2d between two point using gpxpy logic.
 *
 * @note Haversine might still be used for larger distances.
 *
 * @param ll1
 * @param ll2
 * @param use_haversine Force haversine
 * @return double Meters
 */
double distance2d(const LatLong& ll1, const LatLong& ll2, bool use_haversine = false) noexcept;

/**
 * @brief Computes the distance in 3d between two point using gpxpy logic.
 *
 * @note Haversine might still be used for larger distances.
 *
 * @param ll1
 * @param ll2
 * @param use_haversine Force haversine
 * @return double Meters
 */
double distance3d(const LatLong& ll1, const LatLong& ll2, bool use_haversine = false) noexcept;
} // namespace v1

/**
 * @brief Geometry logic matching osmium.
 *
 * This is slightly slower than \ref v1, but more accurate.
 */
namespace v2 {

/**
 * @brief Haversine distance returned in meters using osmium logic.
 *
 * @param ll1
 * @param ll2
 * @return double Meters.
 */
double haversine(const LatLong& ll1, const LatLong& ll2) noexcept;

/**
 * @brief Computes the distance in 2d between two point using gpxpy logic.
 *
 * @param ll1
 * @param ll2
 * @return double Meters
 */
double distance2d(const LatLong& ll1, const LatLong& ll2) noexcept;

/**
 * @brief Computes the distance in 3d between two point using gpxpy logic.
 *
 * @param ll1
 * @param ll2
 * @return double Meters
 */
double distance3d(const LatLong& ll1, const LatLong& ll2) noexcept;

} // namespace v2

using v2::distance2d;
using v2::distance3d;
using v2::haversine;

} // namespace fastgpx
