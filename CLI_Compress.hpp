#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Parser.hpp"

namespace fs = std::filesystem;

class CLICompress {
 public:
  CLICompress();
  ~CLICompress() = default;

  void printHelp();
  bool checkFFMpegInstalled();
  void showCompressionProgress();
  bool isCompressibleVideoFile(const fs::path& path);
  bool parseArguments(std::vector<std::wstring>& args);
  void compressFiles(const std::vector<fs::path>& files);
  fs::path makeCompressedFileName(const fs::path& output);
  std::vector<fs::path> findVideoFilesRecursive(const fs::path& directory);

 private:
  std::vector<std::wstring> m_args;

 private:
  void normalizeArgs(std::vector<std::wstring>& args, ParseResult& parser);
};