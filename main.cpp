#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

#include "CLI_Compress.hpp"

namespace {
CLICompress* g_active_cli = nullptr;

BOOL WINAPI handleConsoleSignal(DWORD signal) {
  switch (signal) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
      if (g_active_cli != nullptr) {
        g_active_cli->requestStop();
      }
      return TRUE;
    default:
      return FALSE;
  }
}
}  // namespace

int wmain(int argc, wchar_t* argv[]) {
  _setmode(_fileno(stdout), _O_U16TEXT);
  std::vector<std::wstring> args(argv, argv + argc);
  args.erase(args.begin());

  CLICompress cli{};
  g_active_cli = &cli;
  SetConsoleCtrlHandler(handleConsoleSignal, TRUE);

  if (!cli.checkFFMpegInstalled()) {
    std::wcerr << L"Error: FFMpeg not found in PATH." << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<fs::path> files_to_process;
  if (!cli.parseArguments(args, files_to_process)) {
    cli.printHelp();
    return EXIT_FAILURE;
  }

  cli.beginCompression();
  std::thread worker(&CLICompress::compressFiles, &cli, files_to_process);
  while (cli.isCurrentlyCompressing()) {
    if (cli.isStopRequested()) {
      std::wcout << L"\rStopping compression..." << std::flush;
    } else {
      cli.showCompressionProgress();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
  }
  if (worker.joinable()) {
    worker.join();
  }

  SetConsoleCtrlHandler(handleConsoleSignal, FALSE);
  g_active_cli = nullptr;

  if (cli.isStopRequested()) {
    std::wcout << L"\r" << std::wstring(40, L' ') << L"\r";
    std::wcout << L"Compression stopped.\n";
    return EXIT_FAILURE;
  }

  std::wcout << L"\nAll tasks finished.\n";
  return EXIT_SUCCESS;
}
