#pragma once

#include "parser.h"
#include <string>
#include <vector>

namespace csimple {

class Codegen {
public:
    Codegen() = default;
    std::string generate(const std::vector<StmtPtr> &stmts);

private:
    std::string genStmt(const Stmt *s, int indent = 1);
    std::string genExpr(const Expr *e);
    std::string genTruthExpr(const Expr *e);
    static std::string indentStr(int level) { return std::string(static_cast<size_t>(level) * 4, ' '); }
};

} // namespace csimple
