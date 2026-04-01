#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class CLICompress {
 public:
  bool m_flagFiles{false};
  bool m_flagRecursion{false};
  bool m_flagHelp{true};

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
  std::vector<fs::path> m_files;
  std::vector<std::wstring> m_args;
  fs::path m_directory;

 private:
  void normalizeArgs(std::vector<std::wstring>& args);
};