#include "FileRAII.h"

#include <cstdio>
#include <stdexcept>

FileRAII::FileRAII(const char* filename, const char* mode) {
  if (fopen_s(&file_, filename, mode) != 0 || !file_) {
    throw std::runtime_error("Couldn't open the file");
  }
}

FileRAII::~FileRAII() {
  if (file_) {
    std::fclose(file_);
  }
}

FileRAII::FileRAII(FileRAII&& other) noexcept { other.file_ = nullptr; }

FileRAII& FileRAII::operator=(FileRAII&& other) noexcept {
  if (this != &other) {
    if (file_) std::fclose(file_);
    file_ = other.file_;
    other.file_ = nullptr;
  }
  return *this;
}
