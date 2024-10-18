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
  double haversine(LatLong ll1, LatLong ll2);

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
  double distance2d(LatLong ll1, LatLong ll2, bool use_haversine = false);

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
  double distance3d(LatLong ll1, LatLong ll2, bool use_haversine = false);

  namespace v2
  {

    /**
     * @brief Haversine distance returned in meters using osmium logic.
     *
     * @param ll1
     * @param ll2
     * @return double Meters.
     */
    double haversine(LatLong ll1, LatLong ll2) noexcept;

    /**
   * @brief Computes the distance in 2d between two point using gpxpy logic.
   *
   * @param ll1
   * @param ll2
   * @return double Meters
   */
    double distance2d(LatLong ll1, LatLong ll2);

    /**
   * @brief Computes the distance in 3d between two point using gpxpy logic.
   *
   * @param ll1
   * @param ll2
   * @return double Meters
   */
    double distance3d(LatLong ll1, LatLong ll2);

  } // namespace v2

} // namespace fastgpx
