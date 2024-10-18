#pragma once

namespace fastgpx
{
  struct LatLong;

  /**
   * @brief Haversine distance returned in meters using gpxpy logic.
   *
   * @param ll1
   * @param ll2
   * @return double Meters.
   */
  double haversine(const LatLong &ll1, const LatLong &ll2) noexcept;

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
  double distance2d(const LatLong &ll1, const LatLong &ll2, bool use_haversine = false) noexcept;

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
  double distance3d(const LatLong &ll1, const LatLong &ll2, bool use_haversine = false) noexcept;

  namespace v2
  {

    /**
     * @brief Haversine distance returned in meters using osmium logic.
     *
     * @param ll1
     * @param ll2
     * @return double Meters.
     */
    double haversine(const LatLong &ll1, const LatLong &ll2) noexcept;

    /**
     * @brief Computes the distance in 2d between two point using gpxpy logic.
     *
     * @param ll1
     * @param ll2
     * @return double Meters
     */
    double distance2d(const LatLong &ll1, const LatLong &ll2) noexcept;

    /**
     * @brief Computes the distance in 3d between two point using gpxpy logic.
     *
     * @param ll1
     * @param ll2
     * @return double Meters
     */
    double distance3d(const LatLong &ll1, const LatLong &ll2) noexcept;

  } // namespace v2

} // namespace fastgpx
