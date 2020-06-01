/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "translator.h"
#include "model.h"
#include "util.h"

#include <stack>
#include <filesystem>

class YamlNode;
class YamlMap;
class YamlSequence;

class Analyzer
{
public:
    using string = std::string;
    using fspath = std::filesystem::path;
    using models_t = std::unordered_map<string, Model>;

    explicit Analyzer(const Translator& translator, fspath basePath = {});

    const Model& loadModel(const string& filePath, InOut inOut);
    static const models_t& allModels() { return _allModels; }

private:
    static models_t _allModels;

    const fspath _baseDir;
    const Translator& _translator;

    struct Context {
        fspath fileDir;
        Model* model;
        const Identifier scope;
    };
    const Context* _context = nullptr;
    size_t _indent = 0;
    friend class ContextOverlay; // defined in analyzer.cpp

    enum IsTopLevel : bool { Inner = false, TopLevel = true };
    enum SubschemasStrategy : bool { ImportSubschemas = false,
                                     InlineSubschemas = true };

    [[nodiscard]] const Context& context() const
    {
        if (!_context)
            throw Exception(
                "Internal error: trying to access the context before creation");
        return *_context;
    }
    [[nodiscard]] Model& currentModel() const { return *context().model; }
    [[nodiscard]] const Identifier& currentScope() const { return context().scope; }
    [[nodiscard]] InOut currentRole() const { return currentScope().role; }
    [[nodiscard]] const Call* currentCall() const { return currentScope().call; }

    [[nodiscard]] std::pair<const Model&, string>
    loadDependency(const string& relPath);
    void fillDataModel(Model& m, const YamlNode& yaml, const string& filename);

    [[nodiscard]] TypeUsage analyzeTypeUsage(const YamlMap& node,
                                             IsTopLevel isTopLevel = Inner);
    [[nodiscard]] TypeUsage analyzeMultitype(const YamlSequence& yamlTypes);
    [[nodiscard]] ObjectSchema
    analyzeSchema(const YamlMap& yamlSchema,
                  SubschemasStrategy subschemasStrategy = ImportSubschemas);

    void mergeFromSchema(ObjectSchema& target, const ObjectSchema& sourceSchema,
                         const string& baseName = {}, bool required = true);

    template <typename... ArgTs>
    [[nodiscard]] VarDecl makeVarDecl(TypeUsage type, const string& baseName,
                                      const Identifier& scope, ArgTs&&... args)
    {
        return { std::move(type),
                 _translator.mapIdentifier(baseName, scope.qualifiedName()),
                 baseName, std::forward<ArgTs>(args)... };
    }

    template <typename... ArgTs>
    void addVarDecl(VarDecls& varList, ArgTs&&... varDeclArgs)
    {
        currentModel().addVarDecl(varList,
                              makeVarDecl(std::forward<ArgTs>(varDeclArgs)...));
    }

    [[nodiscard]] auto logOffset() const
    {
        return string(_indent * 2, ' ');
    }
};
