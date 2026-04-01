#include "CLI_Compress.hpp"

#include <cwctype>
#include <format>
#include <iostream>
#include <iterator>
#include <string_view>

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

bool CLICompress::isAlreadyCompressedFile(const fs::path& path) const {
  const std::wstring stem = path.stem().wstring();
  static constexpr std::wstring_view suffix = L"_compressed";

  if (stem.size() < suffix.size()) {
    return false;
  }

  return stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) ==
         0;
}

bool CLICompress::parseArguments(std::vector<std::wstring>& args,
                                 std::vector<fs::path>& files_to_process) {
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

  std::move(parser.files.begin(), parser.files.end(),
            std::back_inserter(files_to_process));
  if (result && parser.mode == CommandMode::Recursive &&
      parser.directory.has_value()) {
    files_to_process = findVideoFilesRecursive(*parser.directory);
  }

  return result;
}

void CLICompress::compressFiles(const std::vector<fs::path>& files) {
  m_compressing = true;

  for (const auto& file : files) {
    if (m_cancelRequested) {
      break;
    }

    if (isAlreadyCompressedFile(file)) {
      std::wcout << L"\r" << std::wstring(48, L' ') << L"\r";
      std::wcout << L"[SKIP] " << file.filename().wstring()
                 << L" is already compressed.\n";
      continue;
    }

    fs::path compressed_name = makeCompressedFileName(file);

    std::wstring command = std::format(
        L"ffmpeg -hwaccel cuda -i \"{}\" -vf \"fps=15\" -c:v av1_nvenc "
        L"-preset p7 -tune hq -rc vbr -cq 38 -b:v 0 -rc-lookahead 32 "
        L"-c:a copy -movflags +faststart \"{}\" > NUL 2>&1",

        file.wstring(), compressed_name.wstring());

    int result = _wsystem(command.c_str());
    std::wcout << L"\r" << std::wstring(40, L' ') << L"\r";

    if (m_cancelRequested) {
      std::wcout << L"[STOP] Compression cancelled by user.\n";
      break;
    }

    if (result == 0) {
      std::wcout << L"\x1b[32m[√]\x1b[0m " << file.filename().wstring()
                 << L" successfully compressed.\n";
    } else {
      std::wcerr << L"\x1b[31m[x]\x1b[0m Failed to compress: "
                 << file.filename().wstring() << L"\n";
    }
  }

  m_compressing = false;
}

fs::path CLICompress::makeCompressedFileName(const fs::path& output) {
  fs::path compressed_name =
      output.stem().wstring() + L"_compressed" + output.extension().wstring();
  return output.parent_path() / compressed_name;
}

std::vector<fs::path> CLICompress::findVideoFilesRecursive(
    const fs::path& directory) {
  std::vector<fs::path> video_files;
  for (const auto& entry : fs::recursive_directory_iterator(directory)) {
    if (entry.is_regular_file() && isCompressibleVideoFile(entry.path()) &&
        !isAlreadyCompressedFile(entry.path())) {
      video_files.push_back(entry.path());
    }
  }
  return video_files;
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
