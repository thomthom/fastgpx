// Fuzz target: fastgpx::ParseGpx.
//
// Feeds arbitrary bytes to the GPX parser and then walks every lazily computed property. That
// covers the XML parsing (pugixml), the coordinate parsing, the geometry calculations and the
// on-demand <time> parsing. Each segment's points are then passed to polyline::encode, as a
// user converting a track to a polyline would. The parser range-checks coordinates, so the
// encoder must accept every parsed point; a value_error from it is a finding.
//
// Before that, every point is copied, and the copy's time parsed, to check that a copy keeps its
// time whether the text is stored inline or on the heap, and that equality and `LatLong::Hash`
// agree. This comes first because time bounds throw on the first malformed <time>.

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

#include "fastgpx/errors.hpp"
#include "fastgpx/fastgpx.hpp"
#include "fastgpx/polyline.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
  const std::string input(reinterpret_cast<const char*>(data), size);

  try
  {
    const fastgpx::Gpx gpx = fastgpx::ParseGpx(input);

    for (const auto& track : gpx.tracks)
    {
      for (const auto& segment : track.segments)
      {
        for (const auto& point : segment.points)
        {
          fastgpx::LatLong copy = point;
          assert(copy.time == point.time);
          if (copy.time.has_value())
          {
            try
            {
              (void)copy.time->value();
            }
            catch (const fastgpx::parse_error&)
            {
              // Left as text; still equal to the original, checked below.
            }
          }
          assert(copy.time == point.time);
          if (copy == point)
          {
            assert(copy.Hash() == point.Hash());
          }
        }
      }
    }

    // The derived values are computed lazily and cached per level, so ask every level.
    (void)gpx.GetBounds();
    (void)gpx.GetLength2D();
    (void)gpx.GetLength3D();
    (void)gpx.GetTimeBounds();
    for (const auto& track : gpx.tracks)
    {
      (void)track.GetBounds();
      (void)track.GetLength2D();
      (void)track.GetLength3D();
      (void)track.GetTimeBounds();
      for (const auto& segment : track.segments)
      {
        (void)segment.GetBounds();
        (void)segment.GetLength2D();
        (void)segment.GetLength3D();
        (void)segment.GetTimeBounds();

        (void)fastgpx::polyline::encode(segment.points);
      }
    }
  }
  catch (const fastgpx::fastgpx_error&)
  {
    // Rejecting malformed input with the library's own error type is the expected behaviour.
    // Any other exception escapes and is reported by the fuzzer.
  }

  return 0;
}
