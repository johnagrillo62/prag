#pragma once
#include "ast.h"
#include "ast_walker.h"
#include "language_info.h"
#include "languages.h"

namespace bhw
{
// Base walker that uses the type registry
class RegistryAstWalker : public AstWalker
{
  public:
    explicit RegistryAstWalker(bhw::Language lang) : target_language_(lang)
    {
        const auto& registry = bhw::getRegistry();
        auto it = registry.find(lang);
        if (it == registry.end())
        {
            throw std::runtime_error("Language not found in registry");
        }
        lang_info_ = &it->second;
    }
    Language getLang() override
    {
        return target_language_;
    }

  protected:
    bhw::Language target_language_;
    const bhw::LanguageInfo* lang_info_;

    // Get type string from registry
    std::string getTypeString(bhw::ReifiedTypeId type) const
    {
        auto it = lang_info_->type_map.find(type);
        if (it == lang_info_->type_map.end())
        {
            return "unknown";
        }
        return it->second.type_name;
    }

    // Get default value from registry
    std::string getDefaultValue(bhw::ReifiedTypeId type) const
    {
        auto it = lang_info_->type_map.find(type);
        if (it == lang_info_->type_map.end())
        {
            return "";
        }
        return it->second.default_value;
    }

    // Apply naming convention
    std::string applyNaming(const std::string& name, const std::string&) const
    {
        // TODO: Implement case conversion based on convention
        // For now, just return as-is
        return name;
    }

    // Substitute type arguments in template strings like "std::vector<{0}>"
    std::string substituteTypeArgs(const std::string& template_str,
                                   const std::vector<std::string>& args) const
    {
        std::string result = template_str;

        // Replace {0}, {1}, etc.
        for (size_t i = 0; i < args.size(); ++i)
        {
            std::string placeholder = "{" + std::to_string(i) + "}";
            size_t pos = result.find(placeholder);
            if (pos != std::string::npos)
            {
                result.replace(pos, placeholder.length(), args[i]);
            }
        }

        // Replace {...} with comma-separated args
        size_t pos = result.find("{...}");
        if (pos != std::string::npos)
        {
            std::string all_args;
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0)
                    all_args += ", ";
                all_args += args[i];
            }
            result.replace(pos, 5, all_args);
        }

        return result;
    }

    std::string generateSimpleType(const SimpleType& type, size_t) override
    {
        return getTypeString(type.reifiedType);
    }

    std::string generateGenericType(const GenericType& type, size_t) override
    {
        // Get the container template from registry
        std::string template_str = getTypeString(type.reifiedType);

        // Get type arguments
        std::vector<std::string> args;
        for (const auto& arg : type.args)
        {
            args.emplace_back(walkType(*arg));
        }

        // Substitute
        return substituteTypeArgs(template_str, args);
    }
};
} // namespace bhw
