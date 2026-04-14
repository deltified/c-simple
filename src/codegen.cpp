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

std::string Codegen::genStmt(const Stmt *s, int indent) {
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
        // builtin: print(...)
        if (auto ce = dynamic_cast<const CallExpr*>(es->expr.get())) {
            if (ce->callee == "print") {
                std::ostringstream ss;
                ss << "{ char _buf[1024]; int _pos = 0; ";
                for (size_t i = 0; i < ce->args.size(); ++i) {
                    if (i == 0) {
                        ss << "_pos += snprintf(_buf + _pos, sizeof(_buf) - _pos, \"%d\", " << genExpr(ce->args[i].get()) << "); ";
                    } else {
                        ss << "_pos += snprintf(_buf + _pos, sizeof(_buf) - _pos, \" %d\", " << genExpr(ce->args[i].get()) << "); ";
                    }
                }
                ss << "puts(_buf); }";
                return ss.str();
            }
        }

        std::ostringstream ss;
        ss << genExpr(es->expr.get()) << ";";
        return ss.str();
    }
    if (auto is = dynamic_cast<const IfStmt*>(s)) {
        if (is->branches.empty()) {
            return std::string("/* invalid if */");
        }

        std::ostringstream ss;
        for (size_t i = 0; i < is->branches.size(); ++i) {
            const auto &branch = is->branches[i];
            if (i == 0) {
                ss << "if (" << genTruthExpr(branch.cond.get()) << ") {\n";
            } else {
                ss << " else if (" << genTruthExpr(branch.cond.get()) << ") {\n";
            }

            for (const auto &bs : branch.body) {
                ss << indentStr(indent + 1) << genStmt(bs.get(), indent + 1) << "\n";
            }
            ss << indentStr(indent) << "}";
        }

        if (!is->elseBody.empty()) {
            ss << " else {\n";
            for (const auto &bs : is->elseBody) {
                ss << indentStr(indent + 1) << genStmt(bs.get(), indent + 1) << "\n";
            }
            ss << indentStr(indent) << "}";
        }

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
    if (auto ce = dynamic_cast<const CallExpr*>(e)) {
        std::ostringstream ss;
        ss << ce->callee << "(";
        for (size_t i = 0; i < ce->args.size(); ++i) {
            if (i) ss << ", ";
            ss << genExpr(ce->args[i].get());
        }
        ss << ")";
        return ss.str();
    }
    if (auto be = dynamic_cast<const BinaryExpr*>(e)) {
        std::ostringstream ss;
        ss << "(" << genExpr(be->left.get()) << " " << be->op << " " << genExpr(be->right.get()) << ")";
        return ss.str();
    }
    if (auto ue = dynamic_cast<const UnaryExpr*>(e)) {
        return std::string("(") + ue->op + genExpr(ue->operand.get()) + ")";
    }
    return std::string("0");
}

std::string Codegen::genTruthExpr(const Expr *e) {
    return std::string("(") + genExpr(e) + " != 0)";
}

} // namespace csimple
