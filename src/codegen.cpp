#include "codegen.h"
#include <sstream>

namespace csimple {

std::string Codegen::generate(const std::vector<StmtPtr> &stmts) {
    std::ostringstream ss;
    ss << "#include <stdio.h>\n";
    // first emit function definitions
    std::vector<const Stmt*> mainStmts;
    for (const auto &s : stmts) {
        if (auto fs = dynamic_cast<const FunctionStmt*>(s.get())) {
            // function signature
            ss << "int " << fs->name << "(";
            for (size_t i = 0; i < fs->params.size(); ++i) {
                if (i) ss << ", ";
                ss << "int " << fs->params[i];
            }
            ss << ") {\n";
            for (const auto &bs : fs->body) {
                ss << "    " << genStmt(bs.get()) << "\n";
            }
            ss << "}\n\n";
        } else {
            mainStmts.push_back(s.get());
        }
    }

    ss << "int main(void) {\n";
    for (const Stmt *s : mainStmts) {
        ss << "    " << genStmt(s) << "\n";
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
    if (auto rs = dynamic_cast<const ReturnStmt*>(s)) {
        std::ostringstream ss;
        ss << "return " << genExpr(rs->expr.get()) << ";";
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
