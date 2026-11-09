#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "tokenize.h"
#include "planner.h"
#include "parse.h"
#include "load_file.h"
namespace fs = std::filesystem;

static std::map<std::string, Table> loadTables(const std::string& directory) {
    std::map<std::string, Table> tables;
    for (const auto& entry : fs::directory_iterator(directory)){
        if (entry.path().extension() != ".csv") {
            continue;
        }
        std::string tableName = entry.path().stem().string();
        tables.emplace(tableName, loadCsv(entry.path().string(), tableName));
    }
    return tables;
}

static void printValue(const Value& v) {
    if (const double* d = std::get_if<double>(&v)) {
        std::cout << *d;
    } else if (const std::string* s = std::get_if<std::string>(&v)) {
        std::cout << *s;
    } else {
        std::cout << "NULL";
    }
}

static void printRow(const Row& row) {
    for (const auto& value : row) {
        printValue(value);
        std::cout << " ";
    }
    std::cout << std::endl;
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
            for (size_t i = 0; i < names.size(); ++i)
                std::cout << (i > 0 ? " | " : "") << names[i];
            std::cout << "\n";

            Row row;
            while (plan->next(row)) {
                printRow(row);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing query: " << e.what() << std::endl;
        }
    }
    return 0;
}