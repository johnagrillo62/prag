#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

#include "ast_parser.h"
namespace bhw
{
class ParserRegistry
{
  public:
    using Factory = std::function<std::unique_ptr<bhw::AstParser>()>;
    // Create a parser
    [[nodiscard]] auto create(const std::string& name) const
        -> std::optional<std::unique_ptr<bhw::AstParser>>
    {

        auto it = parsers_.find(name);
        if (it == parsers_.end())
        {
            std::cout << "none";
            return std::nullopt;
        }

        return it->second();
    }

    // List all
    void list() const
    {
        for (const auto& [name, _] : parsers_)
        {
            std::cout << " - " << name << " ";
        }
    }

    // List all
    [[nodiscard]] auto getLangs() const -> const std::set<std::string>&
    {
        return languages_;
    }
    // Register a generator
    void add(const std::string& name, const Factory& factory)
    {
        parsers_[name] = factory;
        languages_.insert(name);
    }

    // Check if exists
    [[nodiscard]] auto has(const std::string& name) const -> bool
    {
        return parsers_.contains(name);
    }
    static auto getParserRegistry() -> ParserRegistry&;

    static auto instance() -> ParserRegistry&
    {
        static ParserRegistry reg;
        return reg;
    }

  private:
  private:
    ParserRegistry() = default;
    std::map<std::string, Factory> parsers_;
    std::set<std::string> languages_;
};
} // namespace bhw
