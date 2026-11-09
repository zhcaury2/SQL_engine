#include "load_file.h"
#include <fstream>
#include <vector>
#include <utility>
#include <stdexcept>
#include <cstdlib>
#include <cctype>

static std::string toLower(std::string s) {
    for (char& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

//Splits line into a vector of strings. Used for initial parsing of file.
static std::vector<std::string> splitLine(const std::string& line) {
    std::vector<std::string> fields;
    size_t start = 0;
    while (start <= line.size()) {
        size_t end = line.find(',', start);
        if (end == std::string::npos) {
            end = line.size();
        }
        fields.push_back(line.substr(start, end - start));
        start = end + 1;
    }
    return fields;
}

// Attempts to convert s to double, returning true if successful.
static bool toDouble(const std::string& s, double& out){
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
}

// Reads all rows of file as strings.
static std::vector<std::vector<std::string>> readRawRows(std::ifstream& file, size_t expectedFields){
    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::vector<std::string> fields = splitLine(line);
        if (fields.size() != expectedFields) {
            throw std::runtime_error("Invalid CSV row: expected " + std::to_string(expectedFields) + " fields, got " + std::to_string(fields.size()));
        }
        rows.push_back(std::move(fields));
    }
    return rows;
}

// Returns true if all values in column 'col' are numbers.
static bool isNumbers(const std::vector<std::vector<std::string>>& raw, size_t col) {
    for (const auto& row : raw) {
        const std::string& value = row[col];
        if (value.empty()) continue;
        double parsed;
        if (!toDouble(value, parsed)) return false;
    }
    return true;
}

// Converts one cell to appropriate type.
static Value convertCell(const std::string& text, bool columnIsNumeric){
    if (text.empty()) return std::monostate{};
    double value;
    if (columnIsNumeric && toDouble(text, value)) return value;
    return text;
}

// Converts rows to appropriate type.
static Row convertRow(const std::vector<std::string>& raw, const std::vector<bool>& columnIsNumeric) {
    Row row;
    for (size_t col = 0; col < raw.size(); ++col) {
        row.push_back(convertCell(raw[col], columnIsNumeric[col]));
    }
    return row;
}

// Returns which columns are numbers.
static std::vector<bool> columnAsNumbers(const std::vector<std::vector<std::string>>& raw, size_t cols) {
    std::vector<bool> numbers(cols);
    for (size_t col = 0; col < cols; ++col) {
        numbers[col] = isNumbers(raw, col);
    }
    return numbers;
}

// Loads a CSV file into a Table.
Table loadCsv(const std::string& filename, const std::string& name) {
    Table table;
    table.name = name;
    std::ifstream file(filename);

    if (!file) throw std::runtime_error("Invalid file: " + filename);
    std::string line;

    if (!std::getline(file, line)) throw std::runtime_error("CSV file is empty: " + filename);
    if (!line.empty() && line.back() == '\r') line.pop_back();

    table.columns = splitLine(line);
    for (size_t i = 0; i < table.columns.size(); ++i) {
        table.colIndex[toLower(table.columns[i])] = i;
    }

    auto rawRows = readRawRows(file, table.columns.size());
    auto columnIsNumeric = columnAsNumbers(rawRows, table.columns.size());
    for (const auto& raw : rawRows) {
        table.rows.push_back(convertRow(raw, columnIsNumeric));
    }
    return table;
}