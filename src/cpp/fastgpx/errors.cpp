#include "fastgpx/errors.hpp"

#include <algorithm>
#include <cstddef>
#include <format>

namespace fastgpx {
namespace {

std::string compose_message(const std::string& message, std::string_view source_str,
                            std::size_t offset, std::size_t length)
{
  // Clamp so that a caller cannot ask for a marker line wider than the quoted source, which would
  // both look wrong and size the padding by an arbitrary number.
  const std::size_t marker_offset = std::min(offset, source_str.size());
  const std::size_t marker_length = std::min(length, source_str.size() - marker_offset);

  const std::string marker_line =
      std::format("{:>{}}{}", "", marker_offset, std::string(marker_length, '^'));
  return std::format("{}\n  \"{}\"\n   {}", message, source_str, marker_line);
}

} // namespace

parse_error::parse_error(const std::string& message, std::string_view source_str,
                         std::size_t offset, std::size_t length)
    : fastgpx_error(compose_message(message, source_str, offset, length)) {};

} // namespace fastgpx
