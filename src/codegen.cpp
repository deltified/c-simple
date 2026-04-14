#include "codegen.h"
#include <sstream>

namespace csimple {

std::string Codegen::generate(const std::vector<StmtPtr> &stmts) {
    std::ostringstream ss;
    ss << "#include <stdio.h>\n";
    ss << "int main(void) {\n";
    for (const auto &s : stmts) {
        ss << "    " << genStmt(s.get()) << "\n";
    }
    ss << "    return 0;\n";
    ss << "}\n";
    return ss.str();
}

std::string Codegen::genStmt(const Stmt *s) {
    if (auto ls = dynamic_cast<const LetStmt*>(s)) {
        std::ostringstream ss;
        ss << "int " << ls->name << " = " << genExpr(ls->expr.get()) << ";";
        return ss.str();
    }
    if (auto es = dynamic_cast<const ExprStmt*>(s)) {
        std::ostringstream ss;
        ss << genExpr(es->expr.get()) << ";";
        return ss.str();
    }
    return std::string("/* unknown stmt */");
}

std::string Codegen::genExpr(const Expr *e) {
    if (auto ne = dynamic_cast<const NumberExpr*>(e)) {
        return ne->value; // numbers are emitted as-is (int by default)
    }
    if (auto ie = dynamic_cast<const IdentExpr*>(e)) {
        return ie->name;
    }
    if (auto be = dynamic_cast<const BinaryExpr*>(e)) {
        std::ostringstream ss;
        ss << "(" << genExpr(be->left.get()) << " " << be->op << " " << genExpr(be->right.get()) << ")";
        return ss.str();
    }
    return std::string("0");
}

} // namespace csimple
