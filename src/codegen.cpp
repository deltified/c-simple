#include "codegen.h"
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace csimple {

std::string Codegen::generate(const std::vector<StmtPtr> &stmts) {
    std::ostringstream ss;
    scopes.clear();
    functions.clear();

    std::vector<const FunctionStmt*> functionDecls;
    std::vector<const Stmt*> mainStmts;
    for (const auto &s : stmts) {
        if (auto fs = dynamic_cast<const FunctionStmt*>(s.get())) {
            bindFunction(fs);
            functionDecls.push_back(fs);
        } else {
            mainStmts.push_back(s.get());
        }
    }

    ss << runtimePreamble() << "\n";

    for (const auto *fs : functionDecls) {
        ss << typeToCString(fs->returnType) << " " << fs->name << "(";
        for (size_t i = 0; i < fs->params.size(); ++i) {
            if (i) ss << ", ";
            ss << typeToCString(fs->params[i].type) << " " << fs->params[i].name;
        }
        ss << ");\n";
    }

    if (!functionDecls.empty()) {
        ss << "\n";
    }

    for (const auto *fs : functionDecls) {
        scopes.push_back({});
        currentReturnType = fs->returnType;
        hasCurrentReturnType = true;
        for (const auto &param : fs->params) {
            bindType(param.name, param.type);
        }

        ss << typeToCString(fs->returnType) << " " << fs->name << "(";
        for (size_t i = 0; i < fs->params.size(); ++i) {
            if (i) ss << ", ";
            ss << typeToCString(fs->params[i].type) << " " << fs->params[i].name;
        }
        ss << ") {\n";
        for (const auto &bs : fs->body) {
            ss << indentStr(1) << genStmt(bs.get(), 1) << "\n";
        }
        ss << "}\n\n";

        scopes.pop_back();
        hasCurrentReturnType = false;
        currentReturnType = ValueType::VT_INT;
    }

    ss << "int main(void) {\n";
    scopes.push_back({});
    for (const Stmt *s : mainStmts) {
        ss << indentStr(1) << genStmt(s, 1) << "\n";
        }
    ss << "    return 0;\n";
    ss << "}\n";
    scopes.pop_back();

    return ss.str();
}

std::string Codegen::genStmt(const Stmt *s, int indent) {
    if (auto ls = dynamic_cast<const LetStmt*>(s)) {
        ValueType t = inferType(ls->expr.get());
        bindType(ls->name, t);

        std::ostringstream ss;
        if (t == ValueType::VT_STRING) {
            ss << "cs_string " << ls->name << " = " << genExpr(ls->expr.get()) << ";";
        } else {
            ss << "int " << ls->name << " = " << genExpr(ls->expr.get()) << ";";
        }
        return ss.str();
    }
    if (auto as = dynamic_cast<const AssignStmt*>(s)) {
        ValueType targetType = lookupType(as->name);
        ValueType exprType = inferType(as->expr.get());

        if (targetType == ValueType::VT_STRING && exprType != ValueType::VT_STRING) {
            throw std::runtime_error("cannot assign non-string expression to string variable '" + as->name + "'");
        }
        if (targetType == ValueType::VT_INT && exprType == ValueType::VT_STRING) {
            throw std::runtime_error("cannot assign string expression to int variable '" + as->name + "'");
        }

        if (targetType == ValueType::VT_STRING) {
            if (auto be = dynamic_cast<const BinaryExpr*>(as->expr.get())) {
                if (be->op == "+") {
                    if (auto lhsId = dynamic_cast<const IdentExpr*>(be->left.get())) {
                        if (lhsId->name == as->name && inferType(be->right.get()) == ValueType::VT_STRING) {
                            std::ostringstream ss;
                            ss << "cs_string_append(&" << as->name << ", " << genExpr(be->right.get()) << ");";
                            return ss.str();
                        }
                    }
                }
            }
        }

        std::ostringstream ss;
        ss << as->name << " = " << genExpr(as->expr.get()) << ";";
        return ss.str();
    }
    if (auto rs = dynamic_cast<const ReturnStmt*>(s)) {
        if (!hasCurrentReturnType) {
            throw std::runtime_error("return statement is only allowed inside functions");
        }
        ValueType exprType = inferType(rs->expr.get());
        if (exprType != currentReturnType) {
            throw std::runtime_error("return type does not match function signature");
        }
        std::ostringstream ss;
        ss << "return " << genExpr(rs->expr.get()) << ";";
        return ss.str();
    }
    if (auto es = dynamic_cast<const ExprStmt*>(s)) {
        // builtin: print(...)
        if (auto ce = dynamic_cast<const CallExpr*>(es->expr.get())) {
            if (ce->callee == "print") {
                std::ostringstream ss;
                ss << "{ ";
                for (size_t i = 0; i < ce->args.size(); ++i) {
                    if (i != 0) {
                        ss << "fputc(' ', stdout); ";
                    }
                    if (inferType(ce->args[i].get()) == ValueType::VT_STRING) {
                        ss << "fputs((" << genExpr(ce->args[i].get()) << ").data, stdout); ";
                    } else {
                        ss << "printf(\"%d\", " << genExpr(ce->args[i].get()) << "); ";
                    }
                }
                ss << "fputc('\\n', stdout); }";
                return ss.str();
            }
        }

        std::ostringstream ss;
        if (inferType(es->expr.get()) == ValueType::VT_STRING) {
            ss << "(void)" << genExpr(es->expr.get()) << ";";
        } else {
            ss << genExpr(es->expr.get()) << ";";
        }
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
    if (auto se = dynamic_cast<const StringExpr*>(e)) {
        std::ostringstream ss;
        ss << "cs_string_from_lit(\"" << escapeCString(se->value) << "\", " << se->value.size() << ")";
        return ss.str();
    }
    if (auto ie = dynamic_cast<const IdentExpr*>(e)) {
        return ie->name;
    }
    if (auto ce = dynamic_cast<const CallExpr*>(e)) {
        const FunctionSig &sig = lookupFunction(ce->callee);
        if (sig.paramTypes.size() != ce->args.size()) {
            throw std::runtime_error("argument count mismatch in call to '" + ce->callee + "'");
        }

        for (size_t i = 0; i < ce->args.size(); ++i) {
            ValueType argType = inferType(ce->args[i].get());
            if (argType != sig.paramTypes[i]) {
                throw std::runtime_error("argument type mismatch in call to '" + ce->callee + "'");
            }
        }

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
        ValueType leftType = inferType(be->left.get());
        ValueType rightType = inferType(be->right.get());

        if (be->op == "+" && leftType == ValueType::VT_STRING && rightType == ValueType::VT_STRING) {
            std::ostringstream ss;
            ss << "cs_string_concat(" << genExpr(be->left.get()) << ", " << genExpr(be->right.get()) << ")";
            return ss.str();
        }
        if ((be->op == "==" || be->op == "!=") &&
            leftType == ValueType::VT_STRING && rightType == ValueType::VT_STRING) {
            std::ostringstream ss;
            if (be->op == "==") {
                ss << "cs_string_eq(" << genExpr(be->left.get()) << ", " << genExpr(be->right.get()) << ")";
            } else {
                ss << "(!cs_string_eq(" << genExpr(be->left.get()) << ", " << genExpr(be->right.get()) << "))";
            }
            return ss.str();
        }

        std::ostringstream ss;
        ss << "(" << genExpr(be->left.get()) << " " << be->op << " " << genExpr(be->right.get()) << ")";
        return ss.str();
    }
    if (auto ue = dynamic_cast<const UnaryExpr*>(e)) {
        if (inferType(ue->operand.get()) == ValueType::VT_STRING) {
            if (ue->op == "!") {
                return std::string("((") + genExpr(ue->operand.get()) + ").len == 0)";
            }
            throw std::runtime_error("unsupported unary operator for string expression");
        }
        return std::string("(") + ue->op + genExpr(ue->operand.get()) + ")";
    }
    return std::string("0");
}

std::string Codegen::genTruthExpr(const Expr *e) {
    if (inferType(e) == ValueType::VT_STRING) {
        return std::string("((") + genExpr(e) + ").len != 0)";
    }
    return std::string("(") + genExpr(e) + " != 0)";
}

ValueType Codegen::inferType(const Expr *e) {
    if (dynamic_cast<const NumberExpr*>(e)) return ValueType::VT_INT;
    if (dynamic_cast<const StringExpr*>(e)) return ValueType::VT_STRING;
    if (auto ie = dynamic_cast<const IdentExpr*>(e)) return lookupType(ie->name);
    if (dynamic_cast<const UnaryExpr*>(e)) return ValueType::VT_INT;
    if (auto ce = dynamic_cast<const CallExpr*>(e)) return lookupFunction(ce->callee).returnType;

    if (auto be = dynamic_cast<const BinaryExpr*>(e)) {
        ValueType lt = inferType(be->left.get());
        ValueType rt = inferType(be->right.get());

        if (be->op == "+") {
            if (lt == ValueType::VT_STRING || rt == ValueType::VT_STRING) {
                if (lt != ValueType::VT_STRING || rt != ValueType::VT_STRING) {
                    throw std::runtime_error("string concatenation requires both operands to be strings");
                }
                return ValueType::VT_STRING;
            }
            return ValueType::VT_INT;
        }

        if (be->op == "==" || be->op == "!=") {
            if (lt == ValueType::VT_STRING || rt == ValueType::VT_STRING) {
                if (lt != ValueType::VT_STRING || rt != ValueType::VT_STRING) {
                    throw std::runtime_error("string comparison requires both operands to be strings");
                }
            }
            return ValueType::VT_INT;
        }

        if (be->op == "<" || be->op == ">" || be->op == "<=" || be->op == ">=") {
            if (lt == ValueType::VT_STRING || rt == ValueType::VT_STRING) {
                throw std::runtime_error("ordering comparisons for strings are not supported");
            }
            return ValueType::VT_INT;
        }

        return ValueType::VT_INT;
    }

    return ValueType::VT_INT;
}

ValueType Codegen::lookupType(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
    }
    return ValueType::VT_INT;
}

const FunctionSig &Codegen::lookupFunction(const std::string &name) const {
    auto it = functions.find(name);
    if (it == functions.end()) {
        throw std::runtime_error("unknown function '" + name + "'");
    }
    return it->second;
}

void Codegen::bindType(const std::string &name, ValueType t) {
    if (scopes.empty()) scopes.push_back({});
    scopes.back()[name] = t;
}

void Codegen::bindFunction(const FunctionStmt *fn) {
    FunctionSig sig;
    sig.returnType = fn->returnType;
    sig.paramTypes.reserve(fn->params.size());
    for (const auto &param : fn->params) {
        sig.paramTypes.push_back(param.type);
    }
    functions[fn->name] = std::move(sig);
}

std::string Codegen::escapeCString(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    static const char *hex = "0123456789ABCDEF";

    for (unsigned char c : raw) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (std::isprint(c)) {
                    out.push_back(static_cast<char>(c));
                } else {
                    out += "\\x";
                    out.push_back(hex[(c >> 4) & 0xF]);
                    out.push_back(hex[c & 0xF]);
                }
                break;
        }
    }
    return out;
}

std::string Codegen::runtimePreamble() {
    return std::string(
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "\n"
        "typedef struct {\n"
        "    char *data;\n"
        "    int len;\n"
        "    int cap;\n"
        "} cs_string;\n"
        "\n"
        "static cs_string cs_string_empty(void) {\n"
        "    cs_string s;\n"
        "    s.data = (char*)malloc(1);\n"
        "    if (!s.data) { fprintf(stderr, \"OOM\\n\"); exit(1); }\n"
        "    s.data[0] = '\\0';\n"
        "    s.len = 0;\n"
        "    s.cap = 0;\n"
        "    return s;\n"
        "}\n"
        "\n"
        "static cs_string cs_string_from_lit(const char *lit, int len) {\n"
        "    cs_string s;\n"
        "    s.cap = len > 0 ? len : 0;\n"
        "    s.data = (char*)malloc((size_t)s.cap + 1u);\n"
        "    if (!s.data) { fprintf(stderr, \"OOM\\n\"); exit(1); }\n"
        "    if (len > 0) memcpy(s.data, lit, (size_t)len);\n"
        "    s.data[len] = '\\0';\n"
        "    s.len = len;\n"
        "    return s;\n"
        "}\n"
        "\n"
        "static void cs_string_reserve(cs_string *s, int need) {\n"
        "    if (need <= s->cap) return;\n"
        "    int newCap = s->cap > 0 ? s->cap : 8;\n"
        "    while (newCap < need) {\n"
        "        newCap *= 2;\n"
        "    }\n"
        "    char *newData = (char*)realloc(s->data, (size_t)newCap + 1u);\n"
        "    if (!newData) { fprintf(stderr, \"OOM\\n\"); exit(1); }\n"
        "    s->data = newData;\n"
        "    s->cap = newCap;\n"
        "}\n"
        "\n"
        "static void cs_string_append(cs_string *dst, cs_string rhs) {\n"
        "    int need = dst->len + rhs.len;\n"
        "    cs_string_reserve(dst, need);\n"
        "    if (rhs.len > 0) memcpy(dst->data + dst->len, rhs.data, (size_t)rhs.len);\n"
        "    dst->len = need;\n"
        "    dst->data[dst->len] = '\\0';\n"
        "}\n"
        "\n"
        "static cs_string cs_string_concat(cs_string a, cs_string b) {\n"
        "    cs_string out = cs_string_empty();\n"
        "    cs_string_reserve(&out, a.len + b.len);\n"
        "    if (a.len > 0) memcpy(out.data, a.data, (size_t)a.len);\n"
        "    if (b.len > 0) memcpy(out.data + a.len, b.data, (size_t)b.len);\n"
        "    out.len = a.len + b.len;\n"
        "    out.data[out.len] = '\\0';\n"
        "    return out;\n"
        "}\n"
        "\n"
        "static int cs_string_eq(cs_string a, cs_string b) {\n"
        "    if (a.len != b.len) return 0;\n"
        "    if (a.len == 0) return 1;\n"
        "    return memcmp(a.data, b.data, (size_t)a.len) == 0;\n"
        "}\n"
    );
}

std::string Codegen::typeToCString(ValueType t) {
    return t == ValueType::VT_STRING ? std::string("cs_string") : std::string("int");
}

} // namespace csimple
