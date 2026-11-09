#include "printer.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <variant>

//Converts all values to strings for printing.
static std::string valueToString(const Value& v) {
    if (const double* d = std::get_if<double>(&v)) {
        std::ostringstream ss;
        ss << *d;
        return ss.str();
    }
    if (const std::string* s = std::get_if<std::string>(&v)) return *s;
    return "NULL";
}

//Prints one line of results.
static void printLine(const std::vector<std::string>& line,
                      const std::vector<size_t>& widths) {
    for (size_t c = 0; c < line.size(); ++c) {
        if (c > 0) std::cout << " | ";
        std::cout << std::left << std::setw(widths[c]) << line[c];
    }
    std::cout << "\n";
}

void printResult(Operator& plan, const std::vector<std::string>& names) {
    std::vector<std::vector<std::string>> cells;
    Row row;
    while (plan.next(row)) {
        std::vector<std::string> line;
        for (const Value& v : row) line.push_back(valueToString(v));
        cells.push_back(std::move(line));
    }

    std::vector<size_t> widths(names.size());
    for (size_t c = 0; c < names.size(); ++c) widths[c] = names[c].size();
    for (const auto& line : cells)
        for (size_t c = 0; c < line.size(); ++c)
            widths[c] = std::max(widths[c], line[c].size());

    printLine(names, widths);
    for (size_t c = 0; c < widths.size(); ++c) {
        if (c > 0) std::cout << "-+-";
        std::cout << std::string(widths[c], '-');
    }
    std::cout << "\n";
    for (const auto& line : cells) printLine(line, widths);

    std::cout << "(" << cells.size() << (cells.size() == 1 ? " row)" : " rows)") << "\n";
}