#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast_parser.h"

namespace bhw
{

class ParserRegistry2
{
  public:
    using FactoryFn = std::function<std::unique_ptr<AstParser>()>;

    static ParserRegistry2& instance()
    {
        static ParserRegistry2 registry;
        return registry;
    }

    void registerParser(const std::string& ext, FactoryFn fn)
    {
        factories_[ext] = std::move(fn);
    }

    std::unique_ptr<AstParser> create(const std::string& ext) const
    {
        auto it = factories_.find(ext);
        if (it != factories_.end())
        {
            return it->second();
        }
        return nullptr;
    }

    std::vector<std::string> listRegistered() const
    {
        std::vector<std::string> keys;
        for (const auto& kv : factories_)
        {
            keys.push_back(kv.first);
        }
        return keys;
    }

  private:
    ParserRegistry2() = default;
    std::unordered_map<std::string, FactoryFn> factories_;
};

// ---------------- Auto-Register Base ----------------
template <typename Derived> struct AutoRegisterParser
{
    struct Register2
    {
        Register2()
        {
            for (auto ext : Derived::extensions())
            {
                ParserRegistry2::instance().registerParser(ext,
                                                           []() -> std::unique_ptr<AstParser>
                                                           { return std::make_unique<Derived>(); });
            }
        }
    };
    inline static Register2 _registrar{};
};

} // namespace bhw
