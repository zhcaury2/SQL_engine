#include "planner.h"

// Returns the index of a column in the table.
static size_t getColumnIndex(const Table& table, const std::string& columnName) {
    auto it = table.colIndex.find(columnName);
    if (it == table.colIndex.end()) {
        throw std::runtime_error("Unknown column: " + columnName);
    }
    return it->second;
}

// Converts the condition value to a literal.
static Value makeLiteral(const SelectStatement& stmt) {
    if (stmt.conditionValueType == TokenType::Number) {
        try {
            return Value(std::stod(stmt.conditionValue));
        } catch (const std::logic_error&) {
            throw std::runtime_error("Invalid numeric condition value: " + stmt.conditionValue);
        }
    }
    return Value(stmt.conditionValue);
}

// Links Scan -> Filter -> Project into a plan.
std::unique_ptr<Operator> buildPlan(const SelectStatement& stmt, const std::map<std::string, Table>& catalog) {
    auto tableIt = catalog.find(stmt.table);
    if (tableIt == catalog.end()) {
        throw std::runtime_error("Unknown table: " + stmt.table);
    }
    const Table& table = tableIt->second;
    
    std::unique_ptr<Operator> plan = std::make_unique<Scan>(table);
    if (stmt.hasCondition) {
        size_t conditionColumnIndex = getColumnIndex(table, stmt.conditionColumn);

        Value conditionVal = makeLiteral(stmt);
        plan = std::make_unique<Filter>(std::move(plan), conditionColumnIndex, stmt.conditionSymbol, conditionVal);
    }
    std::vector<size_t> projectionIndices;

    if (stmt.columns.size() == 1 && stmt.columns[0] == "*") {
        for (size_t i = 0; i < table.columns.size(); ++i) {
            projectionIndices.push_back(i);
        }
    } else {
        for (const auto& column : stmt.columns) {
            projectionIndices.push_back(getColumnIndex(table, column));
        }
    }
    plan = std::make_unique<Project>(std::move(plan), projectionIndices);
    return plan;
}