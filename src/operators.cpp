#include "operators.h"

bool Scan::next(Row& out) {
    if (currentRow >= table.rows.size()) return false;
    out = table.rows[currentRow++];
    return true;
}

Filter::Filter(std::unique_ptr<Operator> c, size_t colIdx, std::string o, Value lit)
    : child(std::move(c)), targetColumn(colIdx), op(std::move(o)), literal(std::move(lit)) {
        if (this->op != "=" && this->op != "!=" && this->op != "<" && this->op != "<=" &&
            this->op != ">" && this->op != ">=") {
            throw std::runtime_error("Unknown operator: " + this->op);
        }
}

bool Filter::next(Row& out) {
    while (child->next(out)) {
        if (matches(out[targetColumn])) return true;
    }
    return false;
}

static int compareNumbers(double a, double b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static bool applyOperator(const std::string& op, int c) {
    if (op == "=") return c == 0;
    if (op == "!=") return c != 0;
    if (op == "<") return c < 0;
    if (op == "<=") return c <= 0;
    if (op == ">") return c > 0;
    if (op == ">=") return c >= 0;
    throw std::runtime_error("Unknown operator: " + op);
}

bool Filter::matches(const Value& cell) const {
    if (std::holds_alternative<double>(cell) && std::holds_alternative<double>(literal)) {
        return applyOperator(op, compareNumbers(std::get<double>(cell), std::get<double>(literal)));
    }
    if (std::holds_alternative<std::string>(cell) && std::holds_alternative<std::string>(literal)) {
        const auto& a = std::get<std::string>(cell);
        const auto& b = std::get<std::string>(literal);
        return applyOperator(op, a.compare(b));
    }
    return false;
}

Project::Project(std::unique_ptr<Operator> c, std::vector<size_t> cols)
    : child(std::move(c)), columns(std::move(cols)) {}

bool Project::next(Row& out) {
    Row row;
    if (!child->next(row)) return false;
    out.clear();
    out.reserve(columns.size());
    for (size_t col : columns) out.push_back(row[col]);
    return true;
}

