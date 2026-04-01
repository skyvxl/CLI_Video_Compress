#include "CLI_Compress.hpp"

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <format>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string_view>

CLICompress::CLICompress() {}

void CLICompress::printHelp() {
  std::wcout << "Usage: cli_compress [OPTIONS] [FILES...]\n"
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

  const long long processedMs = m_currentProcessedMs.load();
  const long long totalMs = m_currentTotalMs.load();
  const size_t currentFile = m_currentFileIndex.load();
  const size_t totalFiles = m_totalFiles.load();

  int percent = 0;
  if (totalMs > 0) {
    percent = static_cast<int>((processedMs * 100) / totalMs);
    percent = std::clamp(percent, 0, 100);
  }

  constexpr size_t barWidth = 24;
  const size_t filled =
      totalMs > 0 ? static_cast<size_t>((percent * barWidth) / 100) : 0;
  const std::wstring bar =
      std::wstring(filled, L'#') + std::wstring(barWidth - filled, L'-');

  std::wcout << L"\r" << L"\x1b[36m" << frames[frameIndex] << L"\x1b[0m"
             << L" [" << bar << L"] ";

  if (totalFiles > 0) {
    std::wcout << currentFile << L"/" << totalFiles << L" ";
  }

  if (totalMs > 0) {
    std::wcout << formatDuration(processedMs) << L" / "
               << formatDuration(totalMs) << L" (" << percent << L"%)";
  } else {
    std::wcout << L"Compressing video...";
  }

  std::wcout << std::flush;

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

  return stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::wstring CLICompress::formatDuration(long long milliseconds) {
  if (milliseconds < 0) {
    milliseconds = 0;
  }

  const long long totalSeconds = milliseconds / 1000;
  const long long hours = totalSeconds / 3600;
  const long long minutes = (totalSeconds % 3600) / 60;
  const long long seconds = totalSeconds % 60;

  if (hours > 0) {
    return std::format(L"{:02}:{:02}:{:02}", hours, minutes, seconds);
  }

  return std::format(L"{:02}:{:02}", minutes, seconds);
}

long long CLICompress::parseProgressTimeMs(const std::wstring& value) {
  int hours = 0;
  int minutes = 0;
  double seconds = 0.0;
  wchar_t separator1 = L'\0';
  wchar_t separator2 = L'\0';

  std::wistringstream stream(value);
  stream >> hours >> separator1 >> minutes >> separator2 >> seconds;
  if (!stream || separator1 != L':' || separator2 != L':') {
    return 0;
  }

  const double totalMs =
      ((hours * 3600.0) + (minutes * 60.0) + seconds) * 1000.0;
  return static_cast<long long>(totalMs);
}

long long CLICompress::probeDurationMs(const fs::path& file) const {
  const std::wstring command = std::format(
      L"ffprobe -v error -show_entries format=duration "
      L"-of default=noprint_wrappers=1:nokey=1 \"{}\" 2>NUL",
      file.wstring());

  FILE* pipe = _wpopen(command.c_str(), L"rt");
  if (pipe == nullptr) {
    return 0;
  }

  wchar_t buffer[256]{};
  std::wstring output;
  if (fgetws(buffer, static_cast<int>(std::size(buffer)), pipe) != nullptr) {
    output = buffer;
  }

  _pclose(pipe);

  output.erase(std::remove(output.begin(), output.end(), L'\n'), output.end());
  output.erase(std::remove(output.begin(), output.end(), L'\r'), output.end());

  if (output.empty()) {
    return 0;
  }

  try {
    const long double seconds = std::stold(output);
    if (seconds <= 0.0L) {
      return 0;
    }

    return static_cast<long long>(seconds * 1000.0L);
  } catch (...) {
    return 0;
  }
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
  m_totalFiles = files.size();
  m_currentFileIndex = 0;
  m_currentProcessedMs = 0;
  m_currentTotalMs = 0;

  for (size_t index = 0; index < files.size(); ++index) {
    const auto& file = files[index];
    m_currentFileIndex = index + 1;
    m_currentProcessedMs = 0;
    m_currentTotalMs = probeDurationMs(file);

    if (m_cancelRequested) {
      break;
    }

    if (isAlreadyCompressedFile(file)) {
      std::wcout << L"\r" << std::wstring(96, L' ') << L"\r";
      std::wcout << L"[SKIP] " << file.filename().wstring()
                 << L" is already compressed.\n";
      continue;
    }

    fs::path compressed_name = makeCompressedFileName(file);

    std::wstring command = std::format(
        L"ffmpeg -nostdin -progress pipe:1 -v error -hwaccel cuda -i \"{}\" "
        L"-vf \"fps=15\" -c:v av1_nvenc "
        L"-preset p7 -tune hq -rc vbr -cq 38 -b:v 0 -rc-lookahead 32 "
        L"-c:a copy -movflags +faststart \"{}\" 2>NUL",

        file.wstring(), compressed_name.wstring());

    FILE* pipe = _wpopen(command.c_str(), L"rt");
    if (pipe == nullptr) {
      std::wcerr << L"\r" << std::wstring(40, L' ') << L"\r";
      std::wcerr << L"\x1b[31m[x]\x1b[0m Failed to start ffmpeg for: "
                 << file.filename().wstring() << L"\n";
      continue;
    }

    wchar_t buffer[512]{};
    while (fgetws(buffer, static_cast<int>(std::size(buffer)), pipe) !=
           nullptr) {
      std::wstring line = buffer;
      line.erase(std::remove(line.begin(), line.end(), L'\n'), line.end());
      line.erase(std::remove(line.begin(), line.end(), L'\r'), line.end());

      if (line.rfind(L"out_time=", 0) == 0) {
        m_currentProcessedMs = parseProgressTimeMs(line.substr(9));
      } else if (line == L"progress=end") {
        const long long totalMs = m_currentTotalMs.load();
        if (totalMs > 0) {
          m_currentProcessedMs = totalMs;
        }
      }
    }

    int result = _pclose(pipe);
    std::wcout << L"\r" << std::wstring(96, L' ') << L"\r";

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
