#include <iostream>
#include <regex>
#include <string>
#include <fstream>
#include <iterator>

int main() {
    // Test 1: Direct string match
    std::string line = "SHIT_STRUCT(TestType, Fields)";
    std::regex classRe(R"(SHIT_(?:CLASS|STRUCT)\s*\(\s*(\w+)\s*,\s*(\w+)\s*\))");
    std::smatch m;
    if (std::regex_search(line, m, classRe)) {
        std::cout << "Test 1 PASS: matched '" << m[0].str() << "' type=" << m[1].str() << " mode=" << m[2].str() << "\n";
    } else {
        std::cout << "Test 1 FAIL: no match on '" << line << "'\n";
    }

    // Test 2: Read from file
    std::ifstream ifs("test_scan/test.h");
    if (!ifs.is_open()) {
        std::cout << "Test 2 FAIL: cannot open file\n";
        return 1;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    std::istringstream stream(content);
    std::string fileLine;
    int lineNum = 0;
    while (std::getline(stream, fileLine)) {
        ++lineNum;
        if (std::regex_search(fileLine, m, classRe)) {
            std::cout << "Test 2 PASS: line " << lineNum << " matched '" << m[0].str() << "'\n";
        }
    }

    // Test 3: Search in full content
    if (std::regex_search(content, m, classRe)) {
        std::cout << "Test 3 PASS: content search matched '" << m[0].str() << "'\n";
    } else {
        std::cout << "Test 3 FAIL: content search no match\n";
    }

    return 0;
}
