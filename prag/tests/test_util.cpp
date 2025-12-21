#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

// ============================================================================
// Utility Functions
// ============================================================================

namespace bhw
{
namespace test
{

std::string normalize(const std::string& code)
{
    std::string result;
    bool in_space = false;
    for (size_t i = 0; i < code.size(); ++i)
    {
        char c = code[i];
        // Skip preprocessor directives
        if (c == '#')
        {
            while (i < code.size() && code[i] != '\n')
                ++i;
            continue;
        }
        // Skip single-line comments
        if (c == '/' && i + 1 < code.size() && code[i + 1] == '/')
        {
            i += 2;
            while (i < code.size() && code[i] != '\n')
                ++i;
            continue;
        }
        // Skip multi-line comments
        if (c == '/' && i + 1 < code.size() && code[i + 1] == '*')
        {
            i += 2;
            while (i + 1 < code.size() && !(code[i] == '*' && code[i + 1] == '/'))
                ++i;
            i += 1;
            continue;
        }
        // Normalize whitespace
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!in_space && !result.empty())
            {
                result += ' ';
                in_space = true;
            }
            continue;
        }
        // Remove space before certain punctuation
        if (!result.empty() && result.back() == ' ' && (c == ';' || c == '}' || c == ')'))
        {
            result.pop_back();
        }
        in_space = false;
        result += c;
    }

    // Trim trailing whitespace
    while (!result.empty() && result.back() == ' ')
    {
        result.pop_back();
    }

    return result;
}

void showDetailedDiff(const std::string& input, const std::string& output)
{
    // Find first difference
    size_t diffPos = 0;
    size_t minLen = std::min(input.size(), output.size());

    while (diffPos < minLen && input[diffPos] == output[diffPos])
    {
        diffPos++;
    }

    //    std::cout << "\n";
    // std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    // std::cout << "MISMATCH at position " << diffPos << "\n";
    // std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

    // Show context around difference
    const size_t CONTEXT = 50;
    size_t start = (diffPos > CONTEXT) ? diffPos - CONTEXT : 0;
    size_t end = std::min(std::max(input.size(), output.size()), diffPos + CONTEXT);

    // Extract context
    std::string inputContext =
        (start < input.size()) ? input.substr(start, std::min(end - start, input.size() - start))
                               : "";
    std::string outputContext =
        (start < output.size()) ? output.substr(start, std::min(end - start, output.size() - start))
                                : "";
    // Show side-by-side
    std::cout << "Expected: " << inputContext << "\n";
    std::cout << "Got:      " << outputContext << "\n";
    std::cout << "          " << std::string(diffPos - start, ' ') << "^\n\n";
   
    return;
    // Character details at mismatch
    if (diffPos < input.size() || diffPos < output.size())
    {
        std::cout << "At position " << diffPos << ":\n";

        if (diffPos < input.size())
        {
            char c = input[diffPos];
            std::cout << "  Expected: ";
            if (c == ' ')
                std::cout << "[SPACE]";
            else if (c == '\n')
                std::cout << "[NEWLINE]";
            else if (c == '\t')
                std::cout << "[TAB]";
            else
                std::cout << "'" << c << "'";
            std::cout << " (0x" << std::hex << std::setw(2) << std::setfill('0')
                      << (int)(unsigned char)c << std::dec << ")\n";
        }
        else
        {
            std::cout << "  Expected: [END OF STRING]\n";
        }

        if (diffPos < output.size())
        {
            char c = output[diffPos];
            std::cout << "  Got:      ";
            if (c == ' ')
                std::cout << "[SPACE]";
            else if (c == '\n')
                std::cout << "[NEWLINE]";
            else if (c == '\t')
                std::cout << "[TAB]";
            else
                std::cout << "'" << c << "'";
            std::cout << " (0x" << std::hex << std::setw(2) << std::setfill('0')
                      << (int)(unsigned char)c << std::dec << ")\n";
        }
        else
        {
            std::cout << "  Got:      [END OF STRING]\n";
        }
    }

    std::cout << "\n";

    // Length comparison
    std::cout << "String lengths:\n";
    std::cout << "  Expected: " << input.size() << " chars\n";
    std::cout << "  Got:      " << output.size() << " chars\n";
    std::cout << "  Diff:     " << (int)output.size() - (int)input.size() << "\n\n";

    // Full strings (truncated if too long)
    const size_t MAX_DISPLAY = 500;

    std::cout << "━━━ Full Expected (first " << std::min(input.size(), MAX_DISPLAY)
              << " chars) ━━━\n";
    std::cout << input.substr(0, MAX_DISPLAY);
    if (input.size() > MAX_DISPLAY)
        std::cout << "\n... (" << (input.size() - MAX_DISPLAY) << " more chars)";
    std::cout << "\n\n";

    std::cout << "━━━ Full Got (first " << std::min(output.size(), MAX_DISPLAY) << " chars) ━━━\n";
    std::cout << output.substr(0, MAX_DISPLAY);
    if (output.size() > MAX_DISPLAY)
        std::cout << "\n... (" << (output.size() - MAX_DISPLAY) << " more chars)";
    std::cout << "\n\n";

    // Visual character-by-character comparison
    std::cout << "━━━ Character-by-character (first 100 chars) ━━━\n";
    size_t displayLen = std::min({input.size(), output.size(), size_t(100)});

    for (size_t i = 0; i < displayLen; i++)
    {
        if (i % 50 == 0)
        {
            std::cout << "\nPos " << std::setw(3) << i << ": ";
        }

        if (input[i] == output[i])
        {
            std::cout << ".";
        }
        else
        {
            std::cout << "X";
        }
    }
    std::cout << "\n\n";

    std::cout << "Legend: . = match, X = mismatch\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
}

std::string printLines(const std::string& text)
{
    
    std::stringstream out;
 
    std::istringstream stream(text);
    std::string line;
    int lineNumber = 1;
    while (std::getline(stream, line))
    {
      out << lineNumber++ << ": " << line << "\n";
    }

    return out.str();
}

  
std::string readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error("Cannot open file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::vector<std::string> getTestFiles(const std::string& directory, const std::string& extension)
{
    std::vector<std::string> files;
    std::filesystem::path cwd = std::filesystem::current_path();
    if (!std::filesystem::exists(directory))
        return files;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            // Filter by extension if provided
            if (!extension.empty() && entry.path().extension() != extension)
                continue;

            files.push_back(entry.path().string());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}
} // namespace util
} // namespace bhw
