#pragma once

#include "parser.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace csimple {

struct FunctionSig {
    ValueType returnType{ValueType::VT_INT};
    std::vector<ValueType> paramTypes;
};

class Codegen {
public:
    Codegen() = default;
    std::string generate(const std::vector<StmtPtr> &stmts);

private:
    std::vector<std::unordered_map<std::string, ValueType>> scopes;
    std::unordered_map<std::string, FunctionSig> functions;
    ValueType currentReturnType{ValueType::VT_INT};
    bool hasCurrentReturnType{false};

    std::string genStmt(const Stmt *s, int indent = 1);
    std::string genExpr(const Expr *e);
    std::string genTruthExpr(const Expr *e);
    ValueType inferType(const Expr *e);
    ValueType lookupType(const std::string &name) const;
    const FunctionSig &lookupFunction(const std::string &name) const;
    void bindType(const std::string &name, ValueType t);
    void bindFunction(const FunctionStmt *fn);
    static std::string escapeCString(const std::string &raw);
    static std::string runtimePreamble();
    static std::string typeToCString(ValueType t);
    static std::string indentStr(int level) { return std::string(static_cast<size_t>(level) * 4, ' '); }
};

} // namespace csimple
