#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
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
  bool isAlreadyCompressedFile(const fs::path& path) const;
  bool parseArguments(std::vector<std::wstring>& args,
                      std::vector<fs::path>& files_to_process);
  void compressFiles(const std::vector<fs::path>& files);
  fs::path makeCompressedFileName(const fs::path& output);
  std::vector<fs::path> findVideoFilesRecursive(const fs::path& directory);
  bool isCurrentlyCompressing() const { return m_compressing; }
  void beginCompression() {
    m_cancelRequested = false;
    m_compressing = true;
  }
  void requestStop() { m_cancelRequested = true; }
  bool isStopRequested() const { return m_cancelRequested; }

 private:
  std::vector<std::wstring> m_args;
  std::atomic<bool> m_compressing{false};
  std::atomic<bool> m_cancelRequested{false};
  std::atomic<long long> m_currentProcessedMs{0};
  std::atomic<long long> m_currentTotalMs{0};
  std::atomic<size_t> m_currentFileIndex{0};
  std::atomic<size_t> m_totalFiles{0};

 private:
  static std::wstring formatDuration(long long milliseconds);
  static long long parseProgressTimeMs(const std::wstring& value);
  long long probeDurationMs(const fs::path& file) const;
  void normalizeArgs(std::vector<std::wstring>& args, ParseResult& parser);
};
