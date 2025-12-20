#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "ast_walker.h"

namespace bhw
{
class WalkerRegistry
{
  public:
    static auto getWalkerRegistry() -> WalkerRegistry&;

    using Factory = std::function<std::unique_ptr<AstWalker>()>;

    // Singleton
    static WalkerRegistry& instance()
    {
        static WalkerRegistry reg;
        return reg;
    }

    // Register a generator
    void add(const std::string& name, Factory factory)
    {
        generators_[name] = factory;
        langs_.insert(name);
    }

    // Create a generator
    std::unique_ptr<AstWalker> create(const std::string& name) const
    {
        auto it = generators_.find(name);
        if (it == generators_.end())
            return nullptr;
        return it->second();
    }

    // Check if exists
    bool has(const std::string& name) const
    {
        return generators_.count(name) > 0;
    }

    // List all
    void list() const
    {
        std::cout << "Available generators:\n";
        for (const auto& [name, _] : generators_)
        {
            std::cout << "  - " << name << "\n";
        }
    }

    // List all
    auto getLangs() const
    {
        return langs_;
    }

  private:
    WalkerRegistry() = default;
    std::map<std::string, Factory> generators_;
    std::set<std::string> langs_;
};
} // namespace bhw
