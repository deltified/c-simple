#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "parser.h"
#include "codegen.h"

using namespace csimple;
namespace fs = std::filesystem;

static std::string readFile(const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open input file '" + path + "'");
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void writeFile(const fs::path &path, const std::string &content) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("failed to write '" + path.string() + "'");
    }
    out << content;
    if (!out) {
        throw std::runtime_error("failed to write '" + path.string() + "'");
    }
}

static int runProcess(const std::vector<std::string> &args) {
    if (args.empty()) {
        throw std::runtime_error("cannot run an empty command");
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("failed to fork");
    }
    if (pid == 0) {
        std::vector<char *> argv;
        argv.reserve(args.size() + 1);
        for (const auto &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    int status = 0;
    while (true) {
        if (waitpid(pid, &status, 0) >= 0) {
            break;
        }
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 1;
}

struct TempDir {
    fs::path path;

    TempDir() {
        char pattern[] = "/tmp/csimple-XXXXXX";
        char *created = mkdtemp(pattern);
        if (!created) {
            throw std::runtime_error("failed to create temporary directory");
        }
        path = created;
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

int main(int argc, char **argv) {
    std::string input;
    std::string inputPath;
    bool emitC = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--emit-c") {
            emitC = true;
            continue;
        }
        if (inputPath.empty()) {
            inputPath = arg;
            continue;
        }
        std::cerr << "unexpected argument: " << arg << "\n";
        return 1;
    }

    try {
        if (!inputPath.empty()) {
            input = readFile(inputPath);
        } else {
            std::ostringstream ss;
            ss << std::cin.rdbuf();
            input = ss.str();
        }

        Parser P(input);
        auto stmts = P.parseProgram();
        Codegen cg;
        std::string csrc = cg.generate(stmts);

        if (emitC) {
            std::cout << csrc;
            return 0;
        }

        TempDir tempDir;
        fs::path cPath = tempDir.path / "program.c";
        fs::path exePath = tempDir.path / "program";
        writeFile(cPath, csrc);

        int compileStatus = runProcess({"cc", "-std=c11", "-O2", cPath.string(), "-o", exePath.string()});
        if (compileStatus != 0) {
            return compileStatus;
        }

        return runProcess({exePath.string()});
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";
        return 2;
    }
}
