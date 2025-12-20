/*
================================================================================
pr-cpp - C++ to bhw::Ast Parser (using Clang C++ API)
================================================================================
*/

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <nlohmann/json.hpp>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Support/TargetSelect.h"

#include "ast.h"

using json = nlohmann::ordered_json;
using namespace clang;
using namespace bhw;

std::vector<Struct> g_structs;

// ============================================================================
// Diagnostics Consumer
// ============================================================================

class IgnoringDiagnosticConsumer : public DiagnosticConsumer {
public:
    void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel,
                         const Diagnostic &Info) override {
        // Ignore all diagnostics
    }
    
    bool IncludeInDiagnosticCounts() const override {
        return false;
    }
};

// Global static consumer to avoid deletion
static IgnoringDiagnosticConsumer g_diagConsumer;

// ============================================================================
// Type Converter: QualType → bhw::Type
// ============================================================================

class ClangTypeConverter {
public:
    bhw::Type convertType(clang::QualType qt, clang::ASTContext& ctx) {
        if (qt.isNull()) {
            bhw::SimpleType st;
            st.srcTypeString = "unknown";
            st.reifiedType = bhw::ReifiedTypeId::Unknown;
            return bhw::Type(std::move(st));
        }

        const clang::Type* type = qt.getTypePtr();

        if (clang::isa<clang::TemplateSpecializationType>(type)) {
            auto* templateType = clang::dyn_cast<const clang::TemplateSpecializationType>(type);
            return convertTemplateType(templateType, ctx);
        }

        if (clang::isa<clang::BuiltinType>(type)) {
            auto* builtinType = clang::dyn_cast<const clang::BuiltinType>(type);
            return convertBuiltinType(builtinType);
        }

        if (clang::isa<clang::RecordType>(type)) {
            auto* recordType = clang::dyn_cast<const clang::RecordType>(type);
            return convertRecordType(recordType);
        }

        // Fallback
        bhw::SimpleType st;
        st.srcTypeString = qt.getAsString();
        st.reifiedType = bhw::ReifiedTypeId::Unknown;
        return bhw::Type(std::move(st));
    }

private:
    bhw::Type convertBuiltinType(const clang::BuiltinType* bt) {
        std::string name;
        bhw::ReifiedTypeId id = bhw::ReifiedTypeId::Unknown;

        switch (bt->getKind()) {
            case clang::BuiltinType::Bool:
                name = "bool"; id = bhw::ReifiedTypeId::Bool; break;
            case clang::BuiltinType::SChar:
                name = "i8"; id = bhw::ReifiedTypeId::Int8; break;
            case clang::BuiltinType::UChar:
                name = "u8"; id = bhw::ReifiedTypeId::UInt8; break;
            case clang::BuiltinType::Short:
                name = "i16"; id = bhw::ReifiedTypeId::Int16; break;
            case clang::BuiltinType::UShort:
                name = "u16"; id = bhw::ReifiedTypeId::UInt16; break;
            case clang::BuiltinType::Int:
                name = "i32"; id = bhw::ReifiedTypeId::Int32; break;
            case clang::BuiltinType::UInt:
                name = "u32"; id = bhw::ReifiedTypeId::UInt32; break;
            case clang::BuiltinType::Long:
                name = "i64"; id = bhw::ReifiedTypeId::Int64; break;
            case clang::BuiltinType::ULong:
                name = "u64"; id = bhw::ReifiedTypeId::UInt64; break;
            case clang::BuiltinType::LongLong:
                name = "i64"; id = bhw::ReifiedTypeId::Int64; break;
            case clang::BuiltinType::ULongLong:
                name = "u64"; id = bhw::ReifiedTypeId::UInt64; break;
            case clang::BuiltinType::Float:
                name = "f32"; id = bhw::ReifiedTypeId::Float32; break;
            case clang::BuiltinType::Double:
                name = "f64"; id = bhw::ReifiedTypeId::Float64; break;
            default:
                name = "unknown"; break;
        }

        bhw::SimpleType st;
        st.srcTypeString = name;
        st.reifiedType = id;
        return bhw::Type(std::move(st));
    }

    bhw::Type convertTemplateType(const clang::TemplateSpecializationType* tst, clang::ASTContext& ctx) {
        std::string templateName = "unknown";
        clang::TemplateName tmpl = tst->getTemplateName();
        
        if (auto* templateDecl = tmpl.getAsTemplateDecl()) {
            templateName = templateDecl->getName();
        }
        
        bhw::ReifiedTypeId genericId = bhw::ReifiedTypeId::Unknown;
        if (templateName == "vector") genericId = bhw::ReifiedTypeId::List;
        else if (templateName == "map") genericId = bhw::ReifiedTypeId::Map;
        else if (templateName == "set") genericId = bhw::ReifiedTypeId::Set;
        else if (templateName == "optional") genericId = bhw::ReifiedTypeId::Optional;
        else if (templateName == "tuple") genericId = bhw::ReifiedTypeId::Tuple;
        else if (templateName == "pair") genericId = bhw::ReifiedTypeId::Pair;
        else if (templateName == "variant") genericId = bhw::ReifiedTypeId::Variant;

        bhw::GenericType gt;
        gt.reifiedType = genericId;

        for (const auto& arg : tst->template_arguments()) {
            if (arg.getKind() == clang::TemplateArgument::Type) {
                gt.args.push_back(std::make_unique<bhw::Type>(convertType(arg.getAsType(), ctx)));
            }
        }

        return bhw::Type(std::move(gt));
    }

    bhw::Type convertRecordType(const clang::RecordType* rt) {
        clang::RecordDecl* decl = rt->getDecl();
        std::string name = decl->getName().str();

        bhw::SimpleType st;
        st.srcTypeString = name;
        st.reifiedType = bhw::ReifiedTypeId::Unknown;
        return bhw::Type(std::move(st));
    }
};

// ============================================================================
// AST Visitor
// ============================================================================

class PragASTVisitor : public RecursiveASTVisitor<PragASTVisitor> {
public:
    explicit PragASTVisitor(clang::ASTContext& ctx) : ctx(ctx), typeConverter() {}

    bool VisitCXXRecordDecl(clang::CXXRecordDecl* decl) {
        if (!decl->isCompleteDefinition() || decl->isAnonymousStructOrUnion()) {
            return true;
        }

        bhw::Struct s;
        s.name = decl->getName().str();
        s.isAnonymous = false;

        for (auto* field : decl->fields()) {
            bhw::Field f;
            f.name = field->getName().str();
            f.type = std::make_unique<bhw::Type>(typeConverter.convertType(field->getType(), ctx));
            s.members.push_back(std::move(f));
        }

        g_structs.push_back(std::move(s));
        return true;
    }

private:
    clang::ASTContext& ctx;
    ClangTypeConverter typeConverter;
};

// ============================================================================
// Consumer & Action
// ============================================================================

class PragConsumer : public ASTConsumer {
public:
    explicit PragConsumer(clang::ASTContext& ctx) : visitor(ctx) {}

    void HandleTranslationUnit(clang::ASTContext& ctx) override {
        visitor.TraverseDecl(ctx.getTranslationUnitDecl());
    }

private:
    PragASTVisitor visitor;
};

class PragAction : public ASTFrontendAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci,
                                                    StringRef file) override {
        return std::make_unique<PragConsumer>(ci.getASTContext());
    }
};

// ============================================================================
// Output JSON
// ============================================================================

json typeToJson(const bhw::Type& type) {
    if (type.isSimple()) {
        auto* st = std::get_if<bhw::SimpleType>(&type.value);
        if (st) {
            return json{
                {"kind", "primitive"},
                {"name", st->srcTypeString}
            };
        }
    }
    else if (type.isGeneric()) {
        auto* gt = std::get_if<bhw::GenericType>(&type.value);
        if (gt) {
            json args = json::array();
            for (const auto& arg : gt->args) {
                if (arg) {
                    args.push_back(typeToJson(*arg));
                }
            }
            
            std::string pragName;
            switch (gt->reifiedType) {
                case bhw::ReifiedTypeId::List: pragName = "Vec"; break;
                case bhw::ReifiedTypeId::Map: pragName = "Map"; break;
                case bhw::ReifiedTypeId::Set: pragName = "Set"; break;
                case bhw::ReifiedTypeId::Optional: pragName = "Option"; break;
                case bhw::ReifiedTypeId::Tuple: pragName = "Tuple"; break;
                case bhw::ReifiedTypeId::Pair: pragName = "Pair"; break;
                case bhw::ReifiedTypeId::Variant: pragName = "Variant"; break;
                default: pragName = "Unknown"; break;
            }

            return json{
                {"kind", "generic"},
                {"name", pragName},
                {"args", args}
            };
        }
    }

    return json{
        {"kind", "primitive"},
        {"name", "unknown"}
    };
}

json structToJson(const bhw::Struct& s) {
    json fields = json::array();

    for (const auto& member : s.members) {
        if (std::holds_alternative<bhw::Field>(member)) {
            const bhw::Field& f = std::get<bhw::Field>(member);
            json fieldJson{
                {"name", f.name},
                {"type", typeToJson(*f.type)}
            };
            fields.push_back(fieldJson);
        }
    }

    return json{
        {"type", "Struct"},
        {"name", s.name},
        {"fields", fields}
    };
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: pr-cpp <input.cpp> [-output output.json]\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = "prag.json";

    for (int i = 2; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "-output") {
            outputFile = argv[i + 1];
            break;
        }
    }

    std::cout << "Parsing " << inputFile << " using Clang C++ API...\n";

    // Create diagnostic options on stack
    clang::DiagnosticOptions diagOpts;
    
    // Create diagnostic IDs
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagIDs(new clang::DiagnosticIDs());
    
    // Create diagnostics engine on stack
    clang::DiagnosticsEngine diags(diagIDs, diagOpts, &g_diagConsumer, false);

    // Create compiler instance on stack
    clang::CompilerInstance ci;
    ci.setDiagnostics(&diags);

    // Create compiler invocation
    std::vector<const char*> args = {
        "pr-cpp",
        "-x", "c++",
        "-std=c++17",
        inputFile.c_str()
    };

    clang::CompilerInvocation::CreateFromArgs(ci.getInvocation(), args, diags);

    // Create target options on stack
    clang::TargetOptions targetOpts;
    targetOpts.Triple = "x86_64-unknown-linux-gnu";
    
    clang::TargetInfo *targetInfo = clang::TargetInfo::CreateTargetInfo(diags, targetOpts);
    if (!targetInfo) {
        std::cerr << "Failed to create target info\n";
        return 1;
    }
    ci.setTarget(targetInfo);

    // Create file manager and source manager
    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());

    // Create and execute action
    PragAction action;
    if (!ci.ExecuteAction(action)) {
        std::cerr << "Parse failed\n";
        return 1;
    }

    // Build output JSON
    json items = json::array();
    for (const auto& s : g_structs) {
        items.push_back(structToJson(s));
    }

    json pragJson{
        {"type", "Module"},
        {"srcLanguage", "cpp"},
        {"items", items}
    };

    // Output JSON to stdout
    std::cout << pragJson.dump(2) << "\n";
    
    std::cerr << "Successfully parsed " << g_structs.size() << " structs\n";
    
    // Exit cleanly without running destructors to avoid double-free issues
    exit(0);
}
