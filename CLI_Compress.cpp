#include "CLI_Compress.hpp"

#include <iostream>
#include <unordered_set>

CLICompress::CLICompress() {}

void CLICompress::printHelp() {
  std::wcout << "Usage: cli_video_compress [OPTIONS] [FILES...]\n"
                "\nOptions:\n"
                "\t-f, --file [FILES...]\tCompress the specified video files "
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
  int result = system("ffmpeg --help  > NUL 2>&1");
  if (result == 0) {
    return true;
  }
  return false;
}

bool CLICompress::parseArguments(std::vector<std::wstring>& args) {
  normalizeArgs(args);
  bool incorrectArgs = false;

  /*
  1. -f file // correct
  2. --file file // correct
  3. -f --file file // incorrect
  4. --file -f file // incorrect
  5. -f file1 file2 ...
  6. --file file1 file2 ...
  */

  if (args.empty()) {
    this->printHelp();
    return false;
  }
}

void CLICompress::normalizeArgs(std::vector<std::wstring>& args) {
  std::vector<std::wstring> normalized;
  std::wstring current_path_buffer;
  bool in_quotes = false;
  for (const auto& arg : args) {
    if (arg == L"-f" || arg == L"-r" || arg == L"-h" || arg == L"--file" ||
        arg == L"--recursive" || arg == L"--help") {
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

  for (auto& n : normalized) {
    std::wcout << n << L'\n';
  }

  args = std::move(normalized);
}
