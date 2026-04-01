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

  for (auto& arg : args) {
    std::wcout << arg << "\n";
  }

  CLICompress cli{};
  cli.printHelp();
  if (!cli.checkFFMpegInstalled()) {
    std::cout << "Download FFMpeg";
    return EXIT_FAILURE;
  }
  cli.parseArguments(args);
  return EXIT_SUCCESS;
}