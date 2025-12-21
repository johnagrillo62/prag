#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

namespace bhw
{
namespace test
{

std::string normalize(const std::string& code);
std::string readFile(const std::string& path);
void showDetailedDiff(const std::string& input, const std::string& output);
std::vector<std::string> getTestFiles(const std::string& directory, const std::string& extension);


std::string printLines(const std::string& text);

  
} // namespace test
} // namespace bhw
