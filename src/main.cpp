#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include "tokenize.h"
#include "planner.h"
#include "parse.h"
#include "load_file.h"
#include "printer.h"
#include "helpers.h"
namespace fs = std::filesystem;

static std::map<std::string, Table> loadTables(const std::string& directory) {
    std::map<std::string, Table> tables;
    for (const auto& entry : fs::directory_iterator(directory)){
        if (entry.path().extension() != ".csv") {
            continue;
        }
        std::string tableName = toLower(entry.path().stem().string());
        tables.emplace(tableName, loadCsv(entry.path().string(), tableName));
    }
    return tables;
}


static std::vector<std::string> headerNames(const SelectStatement& stmt,
                                             const Table& table) {
    if (stmt.columns.size() == 1 && stmt.columns[0] == "*") return table.columns;
    return stmt.columns;
}

int main() {
    std::map<std::string, Table> tables;
    try {
        tables = loadTables("data");
    } catch (const std::exception& e) {
        std::cerr << "Error loading tables: " << e.what() << std::endl;
        return 1;
    }

    std::string query;
    while (std::cout << "Enter SQL query: ", std::getline(std::cin, query)) {
        if (query == "exit") break;
        if (query.empty()) continue;
        try {
            Parser parser(parseSQLquery(query));
            SelectStatement stmt = parser.parseSelect();
            auto plan = buildPlan(stmt, tables);
            const Table& table = tables.at(stmt.table);
            auto names = headerNames(stmt, table);
            printResult(*plan, names);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    return 0;
}