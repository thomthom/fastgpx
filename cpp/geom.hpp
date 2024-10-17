#pragma once

namespace fastgpx
{
  struct LatLong;

  double distance2d(LatLong ll1, LatLong ll2, bool use_haversine = false);
  double distance3d(LatLong ll1, LatLong ll2, bool use_haversine = false);

} // namespace fastgpx
