// Copyright 2016-2017 Benjamin 'Benno' Falkner. All rights reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

#include "mdb.h"

#include <array>
#include <bit>
#include <corecrt.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// for clang tidy to see C functions
extern "C"
{
#include <stdint.h>
#include <stdio.h>
#include <string.h>
}

constexpr int MDB_VER_JET3 = 0;
constexpr int MDB_VER_JET4 = 1;
constexpr int MDB_VER_ACCDB2007 = 0x02;
constexpr int MDB_VER_ACCDB2010 = 0x0103;
constexpr int MDB_PAGE_SIZE = 4096;

constexpr size_t PASSWORD_LEN = 40;
constexpr size_t PASSWORD_LEN_V3 = 20;
constexpr size_t PASSWORD_OFFSET = 0x42;

// clang-format off
constexpr std::array<unsigned char,18> JET3_XOR = { 0x86, 0xfb, 0xec, 0x37, 0x5d, 0x44, 0x9c, 0xfa, 0xc6,
                                                    0x5e, 0x28, 0xe6, 0x13, 0xb6, 0x8a, 0x60, 0x54, 0x94 }; 

constexpr std::array<unsigned short, 20> JET4_XOR = { 0x6aba, 0x37ec, 0xd561, 0xfa9c, 0xcffa,
                                                      0xe628, 0x272f, 0x608a, 0x0568, 0x367b,
                                                      0xe3c9, 0xb1df, 0x654b, 0x4313, 0x3ef3,
                                                      0x33b1, 0xf008, 0x5b79, 0x24ae, 0x2a7c };

// clang-format on
namespace
{
auto readMDBPage(const std::wstring &filename) -> std::vector<char>
{
  std::vector<char> buffer(MDB_PAGE_SIZE);

  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open())
  {
    return {};
    // throw std::runtime_error("ERROR: could not open "); // +filename); //
    // NOLINT(clang-diagnostic-error)
  }

  file.read(buffer.data(), MDB_PAGE_SIZE);
  if (file.gcount() != MDB_PAGE_SIZE)
  {
    return {};

    // throw std::runtime_error("ERROR: could not read page from "); //
    // NOLINT(clang-diagnostic-error)
    //  +filename + " (" +
    //  std::to_string(file.gcount()) +
    //  "/" +
    //  std::to_string(MDB_PAGE_SIZE) +
    //  ")");
  }

  return buffer;
}

auto readMDBPage(const char *filen, size_t pageSize, char *buffer) -> size_t
{

    FILE *file = nullptr;
  errno_t const err = fopen_s(&file, filen, "rb");
  if (err != 0 || file == nullptr)
  {
    std::cout << "ERROR: could not open " << filen << "\n";
    exit(1);
  }

  size_t size = fread(buffer, 1, pageSize, file);
  if (size != pageSize)
  {
    std::cout << "ERROR: could not read page from " << filen << " " << size << pageSize << "\n";
    exit(1);
  }
  fclose(file);
  return size;
}
auto scanMDBPage(const std::vector<char> &buffer) -> std::string
{
    constexpr int bounds = 0x66;
    // --- Bounds Check ---
  if (buffer.size() < sizeof(short) + bounds)
  {
    std::cerr << "ERROR: buffer too small\n";
    return "";
  }

  // --- Page ID Check ---
  if (static_cast<unsigned char>(buffer[0]) != 0)
  {
    std::cerr << "ERROR: not a valid MDB page\n";
    return "";
  }

  // --- Version Extraction ---
  const int version = *reinterpret_cast<const int *>(&buffer[0x14]);
  switch (version)
  {
  case MDB_VER_JET3:
  case MDB_VER_JET4:
  case MDB_VER_ACCDB2007:
  case MDB_VER_ACCDB2010:
    break;
  default:
    std::cerr << "ERROR: unknown version: " << version << "\n";
    return "";
  }

  // --- Password Extraction ---
  if (version == MDB_VER_JET3)
  {
    std::array<uint8_t, PASSWORD_LEN> pwd;
    std::memcpy(pwd.data(), buffer.data() + PASSWORD_OFFSET, PASSWORD_LEN_V3);
    for (int i = 0; i < 18; ++i)
    {
      pwd[i] ^= static_cast<char>(JET3_XOR[i]);
    }
    auto *decoded = reinterpret_cast<char *>(pwd.data());
    return {decoded, strnlen(decoded, PASSWORD_LEN)};
  }

  if (version == MDB_VER_JET4)
  {
    std::array<uint8_t, PASSWORD_LEN> pwd;
    std::memcpy(pwd.data(), buffer.data() + PASSWORD_OFFSET, PASSWORD_LEN);
    auto pwd4 = std::bit_cast<std::array<unsigned short, PASSWORD_LEN / 2>>(pwd);

    unsigned short magic;
    std::memcpy(&magic, &buffer[bounds], sizeof(unsigned short));

    magic ^= JET4_XOR[18];

    for (int i = 0; i < 18; ++i)
    {
      pwd4[i] ^= JET4_XOR[i];
      if (pwd4[i] > 255)
      {
        pwd4[i] ^= magic;
      }
      pwd[i] = static_cast<char>(pwd4[i]);
    }

    // --- Convert to std::string safely ---
    auto *decoded = reinterpret_cast<char *>(pwd.data());
    return {decoded, strnlen(decoded, PASSWORD_LEN)};
  }

  return "";
}
} // namespace

auto getPasswd(const std::wstring &filePath) -> std::string
{
  // Read data
  auto page = readMDBPage(filePath);
  auto pwd = scanMDBPage(page);
  return pwd;
}
