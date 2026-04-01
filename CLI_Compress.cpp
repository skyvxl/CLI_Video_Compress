#include "CLI_Compress.hpp"

#include <cwctype>
#include <iostream>

CLICompress::CLICompress() {}

void CLICompress::printHelp() {
  std::wcout << "Usage: cli_video_compress [OPTIONS] [FILES...]\n"
                "\nOptions:\n"
                "\t-f, --files [FILES...]\tCompress the specified video files "
                "(mp4, mkv)\n"

                "\t-r, --recursive [DIR]\tRecursively process all video files "
                "in a folder\n"

                "\t-h, --help\t\tShow this message\n"
                "\nExamples:\n"
                "\t cli_compress.exe -f .\\video1.mp4 .\\video2.mkv\n"
                "\t cli_compress.exe -r .\\Videos\\\n"
                "\t cli_compress.exe -r 'C:\\Users\\Alin0\\Videos\\'\n"
                "\nNote: The -f and -r flags cannot be used simultaneously.\n";
}

bool CLICompress::checkFFMpegInstalled() {
  int result = system("ffmpeg --help > NUL 2>&1");
  if (result == 0) {
    return true;
  }
  return false;
}

void CLICompress::showCompressionProgress() {
  static const std::wstring frames[] = {L"⠋", L"⠙", L"⠹", L"⠸", L"⠼",
                                        L"⠴", L"⠦", L"⠧", L"⠇", L"⠏"};
  static size_t frameIndex = 0;

  std::wcout << L"\r" << L"\x1b[36m" << frames[frameIndex] << L"\x1b[0m"
             << L" Compressing video..." << std::flush;

  frameIndex = (frameIndex + 1) % (sizeof(frames) / sizeof(frames[0]));
}

bool CLICompress::isCompressibleVideoFile(const fs::path& path) {
  if (!path.has_extension()) {
    return false;
  }
  std::wstring ext = path.extension().wstring();
  for (auto& c : ext) {
    c = std::towlower(c);
  }
  return ext == L".mp4" || ext == L".mkv";
}

bool CLICompress::parseArguments(std::vector<std::wstring>& args) {
  ParseResult parser{};

  normalizeArgs(args, parser);

  bool result = true;

  if (args.empty()) {
    parser.mode = CommandMode::Help;
    result = false;
  }

  for (const auto& arg : args) {
    if (arg == L"-h" || arg == L"--help") {
      parser.mode = CommandMode::Help;
      result = false;
    } else if (arg == L"-f" || arg == L"--files") {
      if (parser.mode != CommandMode::None &&
          parser.mode != CommandMode::Files) {
        parser.errors.push_back(
            L"Flag conflict: cannot combine -f with other modes.");
        result = false;
      }
      parser.mode = CommandMode::Files;
    } else if (arg == L"-r" || arg == L"--recursive") {
      if (parser.mode != CommandMode::None &&
          parser.mode != CommandMode::Recursive) {
        parser.errors.push_back(
            L"Flag conflict: cannot combine -r with other modes.");
        result = false;
      }
      parser.mode = CommandMode::Recursive;
    } else if (!arg.empty() && arg.front() == L'-') {
      parser.errors.push_back(L"Unknown argument: " + arg);
      result = false;
    } else {
      if (parser.mode == CommandMode::Files) {
        fs::path p(arg);
        if (!fs::exists(p)) {
          parser.errors.push_back(L"File does not exist: " + arg);
          result = false;
        } else if (!fs::is_regular_file(p)) {
          parser.errors.push_back(
              L"Expected a file, but provided a different type: " + arg);
          result = false;
        } else if (!isCompressibleVideoFile(p)) {
          parser.errors.push_back(L"Unsupported video format: " + arg);
          result = false;
        } else {
          parser.files.push_back(p);
        }
      } else if (parser.mode == CommandMode::Recursive) {
        if (parser.directory.has_value()) {
          parser.errors.push_back(
              L"The -r flag accepts only one directory. Redundant argument: " +
              arg);
          result = false;
        } else {
          fs::path p(arg);
          if (!fs::exists(p)) {
            parser.errors.push_back(L"Directory does not exist: " + arg);
            result = false;
          } else if (!fs::is_directory(p)) {
            parser.errors.push_back(
                L"Expected a directory, but provided a file: " + arg);
            result = false;
          } else {
            parser.directory = p;
            parser.isAbsolutePath = p.is_absolute();
          }
        }
      } else {
        parser.errors.push_back(
            L"Unexpected argument without a flag (-f or -r): " + arg);
        result = false;
      }
    }
  }

  if (parser.mode == CommandMode::Files && parser.files.empty() &&
      parser.errors.empty()) {
    parser.errors.push_back(
        L"No files provided for compression after the -f flag.");
    result = false;
  }
  if (parser.mode == CommandMode::Recursive && !parser.directory.has_value() &&
      parser.errors.empty()) {
    parser.errors.push_back(
        L"No directory provided for search after the -r flag.");
    result = false;
  }
  if (parser.mode == CommandMode::None && parser.errors.empty() &&
      !args.empty()) {
    result = false;
  }

  if (!parser.errors.empty()) {
    for (const auto& error : parser.errors) {
      std::wcerr << L"\x1b[31m[ERROR]: " << error << L"\x1b[0m\n";
    }
    result = false;
  }

  return result;
}

void CLICompress::normalizeArgs(std::vector<std::wstring>& args,
                                ParseResult& parser) {
  std::vector<std::wstring> normalized;
  std::wstring current_path_buffer;
  bool in_quotes = false;
  if (args.empty()) {
    parser.mode = CommandMode::Help;
  }
  for (const auto& arg : args) {
    if (arg == L"-f" || arg == L"-r" || arg == L"-h" || arg == L"--files" ||
        arg == L"--recursive" || arg == L"--help") {
      if (arg == L"-f" || arg == L"--files") {
        if (parser.mode != CommandMode::None) {
          parser.mode = CommandMode::None;
          parser.errors.push_back(L"Cannot use -f/--files with other flags.");
        } else {
          parser.mode = CommandMode::Files;
        }
      }
      if (arg == L"-r" || arg == L"--recursive") {
        if (parser.mode != CommandMode::None) {
          parser.mode = CommandMode::None;
          parser.errors.push_back(
              L"Cannot use -r/--recursive with other flags.");
        } else {
          parser.mode = CommandMode::Recursive;
        }
      }
      if (arg == L"-h" || arg == L"--help") {
        if (parser.mode != CommandMode::None) {
          parser.mode = CommandMode::None;
        } else {
          parser.mode = CommandMode::Help;
        }
      }
      normalized.push_back(arg);
      continue;
    }

    if (!in_quotes) {
      if (!arg.empty() && arg.front() == L'\'') {
        in_quotes = true;

        current_path_buffer = arg.substr(1);

        if (arg.back() == L'\'') {
          in_quotes = false;
          current_path_buffer.pop_back();
          normalized.push_back(current_path_buffer);
          current_path_buffer.clear();
        }
      } else if (!arg.empty()) {
        normalized.push_back(arg);
      }
    } else {
      current_path_buffer += L' ';
      if (!arg.empty() && arg.back() == L'\'') {
        in_quotes = false;

        current_path_buffer += arg.substr(0, arg.size() - 1);

        normalized.push_back(current_path_buffer);
        current_path_buffer.clear();
      } else {
        current_path_buffer += arg;
      }
    }
  }

  args = std::move(normalized);
}
