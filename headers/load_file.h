#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

using Value = std::variant<std::monostate, double, std::string>;
using Row = std::vector<Value>;

struct Table {
    std::string name;
    std::vector<std::string> columns;
    std::unordered_map<std::string, size_t> colIndex;
    std::vector<Row> rows;
};

Table loadCsv(const std::string& filename, const std::string& name);

