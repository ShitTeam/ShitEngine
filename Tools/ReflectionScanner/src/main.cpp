#include "Scanner.h"
#include "Generator.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

void printUsage(const char* exeName) {
    std::cerr << "Usage: " << exeName << " --input <dir> --output <dir>\n";
    std::cerr << "       [--include <dir>]... [--system-include <dir>]... [--include-root <dir>]\n\n";
    std::cerr << "  --input   <dir>    源码目录（递归扫描 .h 文件）\n";
    std::cerr << "  --output  <dir>    生成代码输出目录\n";
    std::cerr << "  --include <dir>    附加 include 路径（-I，可重复）\n";
    std::cerr << "  --system-include <dir>  系统 include 路径（-isystem，可重复）\n";
    std::cerr << "  --include-root <dir>    从 sourceFile 路径中移除的前缀，默认为 input dir\n";
    std::cerr << "  --help             显示此帮助\n\n";
    std::cerr << "Example:\n";
    std::cerr << "  ReflectionScanner --input Engine/include/ShitEngine/Component \\\n";
    std::cerr << "    --output Engine/generated/reflection \\\n";
    std::cerr << "    --include Engine/include \\\n";
    std::cerr << "    --include-root Engine/include\n";
}

struct Args {
    std::string inputDir;
    std::string outputDir;
    std::vector<std::string> includePaths;
    std::vector<std::string> systemIncludePaths;
    std::string includeRoot;
    std::string resourceDir;  ///< libclang resource dir（含 builtin headers）
};

bool parseArgs(int argc, char* argv[], Args& args) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            printUsage(argv[0]);
            return false;
        } else if (arg == "--input" && i + 1 < argc) {
            args.inputDir = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            args.outputDir = argv[++i];
        } else if (arg == "--include" && i + 1 < argc) {
            args.includePaths.push_back(argv[++i]);
        } else if (arg == "--system-include" && i + 1 < argc) {
            args.systemIncludePaths.push_back(argv[++i]);
        } else if (arg == "--include-root" && i + 1 < argc) {
            args.includeRoot = argv[++i];
        } else if (arg == "--resource-dir" && i + 1 < argc) {
            args.resourceDir = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }
    }

    if (args.inputDir.empty() || args.outputDir.empty()) {
        std::cerr << "Error: --input and --output are required\n\n";
        printUsage(argv[0]);
        return false;
    }

    // 默认 includeRoot 为 inputDir 的父级
    if (args.includeRoot.empty()) {
        args.includeRoot = fs::path(args.inputDir).parent_path().string();
    }

    return true;
}

int main(int argc, char* argv[]) {
    Args args;
    if (!parseArgs(argc, argv, args)) return 1;

    if (!fs::exists(args.inputDir)) {
        std::cerr << "Error: input directory not found: " << args.inputDir << "\n";
        return 1;
    }

    // 把输入目录本身也作为 include 路径
    args.includePaths.push_back(args.inputDir);

    // 标准化 includeRoot（统一分隔符）
    std::string rootNorm = args.includeRoot;
    for (auto& ch : rootNorm) if (ch == '\\') ch = '/';
    if (!rootNorm.empty() && rootNorm.back() != '/') rootNorm.push_back('/');

    std::cout << "ReflectionScanner v1.0.0\n";
    std::cout << "  Input:    " << args.inputDir << "\n";
    std::cout << "  Output:   " << args.outputDir << "\n";
    std::cout << "  Includes: " << args.includePaths.size() << " path(s)\n";
    std::cout << "  Root:     " << args.includeRoot << "\n";
    if (!args.resourceDir.empty())
        std::cout << "  Resource:" << args.resourceDir << "\n";
    std::cout << "\n";

    // 扫描
    Scanner scanner(args.includePaths, args.systemIncludePaths, args.resourceDir);
    ScanResult result = scanner.scanDirectory(args.inputDir);

    // 修正 sourceFile 为相对于 includeRoot 的路径
    for (auto& type : result.types) {
        std::string src = type.sourceFile;
        for (auto& ch : src) if (ch == '\\') ch = '/';
        if (src.substr(0, rootNorm.size()) == rootNorm) {
            type.sourceFile = src.substr(rootNorm.size());
        } else {
            // 回退：只取文件名
            type.sourceFile = fs::path(type.sourceFile).filename().string();
        }
    }

    std::cout << "Scanned " << result.totalFilesScanned << " files, "
              << result.reflectedFiles << " with reflection markers.\n";
    std::cout << "Found " << result.types.size() << " reflected types.\n\n";

    if (result.types.empty()) {
        std::cout << "Nothing to generate.\n";
        return 0;
    }

    // 列出来
    for (const auto& type : result.types) {
        std::cout << "  " << type.name;
        if (!type.baseName.empty())
            std::cout << " : " << type.baseName;
        std::cout << " (" << type.fields.size() << " fields";
        if (type.mode == ReflectionMode::WhiteListFields) std::cout << ", whitelist";
        std::cout << ")\n";
    }

    // 生成
    fs::create_directories(args.outputDir);

    for (const auto& type : result.types) {
        std::string path = Generator::generateTypeFile(type, args.outputDir);
        std::cout << "  [gen] " << path << "\n";
    }

    std::string regPath = Generator::generateRegisterAll(result, args.outputDir);
    std::cout << "  [gen] " << regPath << "\n";
    std::cout << "Done.\n";

    return 0;
}
