#include <fcntl.h>
#include <io.h>

#include <iostream>
#include <string>
#include <vector>

#include "CLI_Compress.hpp"

int wmain(int argc, wchar_t* argv[]) {
  _setmode(_fileno(stdout), _O_U16TEXT);
  std::vector<std::wstring> args(argv, argv + argc);
  args.erase(args.begin());  // erase program path from args

  CLICompress cli{};
  if (!cli.checkFFMpegInstalled()) {
    std::wcout << "Download FFMpeg";
    return EXIT_FAILURE;
  }
  if (!cli.parseArguments(args)) {
    std::wcout << "\n";
    cli.printHelp();
    return EXIT_FAILURE;
  }
  // while (compress) {
  //   cli.showCompressionProgress();
  //   std::this_thread::sleep_for(std::chrono::milliseconds(80));  // ~12 fps
  // }
  return EXIT_SUCCESS;
}