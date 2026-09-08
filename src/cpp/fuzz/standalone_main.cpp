// Standalone driver for the fuzz targets on toolchains without libFuzzer (MSVC, GCC).
//
// Replays every file given on the command line - directories are walked recursively - through
// `LLVMFuzzerTestOneInput`. This is what the fuzz_<name>_corpus CTest entries use, so the seed
// corpora and any crash inputs committed to them are exercised by every compiler.

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size);

namespace {

// `path::string()` converts to the narrow system code page on Windows and throws for file names
// it cannot represent (e.g. gpx/test/テスト.gpx). UTF-8 never fails.
std::string PathToUtf8(const std::filesystem::path& path)
{
  const std::u8string u8 = path.u8string();
  return std::string(u8.begin(), u8.end());
}

bool RunFile(const std::filesystem::path& path)
{
  std::ifstream file(path, std::ios::binary);
  if (!file)
  {
    std::cerr << "error: unable to read " << PathToUtf8(path) << "\n";
    return false;
  }
  const std::vector<std::uint8_t> input((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
  std::cout << "Running: " << PathToUtf8(path) << " (" << input.size() << " bytes)\n";
  LLVMFuzzerTestOneInput(input.data(), input.size());
  return true;
}

} // namespace

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "usage: " << argv[0] << " <file-or-directory>...\n";
    return 2;
  }

  std::size_t count = 0;
  bool ok = true;
  for (int i = 1; i < argc; ++i)
  {
    const std::filesystem::path path(argv[i]);
    if (std::filesystem::is_directory(path))
    {
      for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
      {
        if (entry.is_regular_file())
        {
          ok = RunFile(entry.path()) && ok;
          ++count;
        }
      }
    }
    else
    {
      ok = RunFile(path) && ok;
      ++count;
    }
  }

  std::cout << "Executed " << count << " inputs\n";
  return ok ? 0 : 1;
}
