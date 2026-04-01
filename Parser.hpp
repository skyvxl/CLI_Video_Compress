#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class CommandMode { None, Help, Files, Recursive };

struct ParseResult {
  CommandMode mode = CommandMode::None;
  std::vector<fs::path> files;
  std::optional<fs::path> directory;
  std::vector<std::wstring> errors;
  bool isAbsolutePath = false;

  bool ok() const { return errors.empty(); }
  bool shouldShowHelp() const {
    return mode == CommandMode::Help || !errors.empty();
  }
};