#include "fastgpx/errors.hpp"

#include <format>

namespace fastgpx {
namespace {

std::string compose_message(const std::string& message, std::string_view source_str,
                            std::string_view sub_str)
{
  const std::ptrdiff_t offset = sub_str.data() - source_str.data();
  const std::string marker_line =
      std::format("{:>{}}{}", "", offset, std::string(sub_str.size(), '^'));
  return std::format("{}\n  \"{}\"\n   {}", message, source_str, marker_line);
}

} // namespace

parse_error::parse_error(const std::string& message, std::string_view source_str,
                         std::string_view sub_str)
    : fastgpx_error(compose_message(message, source_str, sub_str)) {};

} // namespace fastgpx
