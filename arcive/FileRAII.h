#pragma once

#include <cstdio>

class FileRAII {
 public:
  FileRAII(const char* filename, const char* mode);
  ~FileRAII();

  FileRAII(const FileRAII&) = delete;
  FileRAII& operator=(const FileRAII&) = delete;

  FileRAII(FileRAII&& other) noexcept;
  FileRAII& operator=(FileRAII&& other) noexcept;

  FILE* get() const { return file_; }

 private:
  FILE* file_{nullptr};
};
