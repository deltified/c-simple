#pragma once

#include "parser.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace csimple {

enum class ValueType {
    VT_INT,
    VT_STRING,
};

class Codegen {
public:
    Codegen() = default;
    std::string generate(const std::vector<StmtPtr> &stmts);

private:
    std::vector<std::unordered_map<std::string, ValueType>> scopes;

    std::string genStmt(const Stmt *s, int indent = 1);
    std::string genExpr(const Expr *e);
    std::string genTruthExpr(const Expr *e);
    ValueType inferType(const Expr *e);
    ValueType lookupType(const std::string &name) const;
    void bindType(const std::string &name, ValueType t);
    static std::string escapeCString(const std::string &raw);
    static std::string runtimePreamble();
    static std::string indentStr(int level) { return std::string(static_cast<size_t>(level) * 4, ' '); }
};

} // namespace csimple
