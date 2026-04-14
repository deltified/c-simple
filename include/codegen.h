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
    std::string genStmt(const Stmt *s);
    std::string genExpr(const Expr *e);
};

} // namespace csimple
