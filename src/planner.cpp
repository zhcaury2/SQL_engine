#include "planner.h"

// Returns the index of a column in the table.
static size_t getColumnIndex(const Table& table, const std::string& columnName) {
    auto it = table.colIndex.find(columnName);
    if (it == table.colIndex.end()) {
        throw std::runtime_error("Unknown column: " + columnName);
    }
    return it->second;
}

// Convert leaf to Value
static Value makeLiteral(const Expr& leaf) {
    if (leaf.valueType == TokenType::Number) {
        try {
            return Value(std::stod(leaf.value));
        } catch (const std::logic_error&) {
            throw std::runtime_error("Invalid numeric condition value: " + leaf.value);
        }
    }
    return Value(leaf.value);
}

static std::unique_ptr<Predicate> buildPredicate(const Expr& expr, const Table& table) {
    switch (expr.kind) {
        case Expr::Kind::Comparison:
            return makeCompare(getColumnIndex(table, expr.column),
                               expr.op,
                               makeLiteral(expr));
        case Expr::Kind::And:
            return makeAnd(buildPredicate(*expr.left, table),
                           buildPredicate(*expr.right, table));
        case Expr::Kind::Or:
            return makeOr(buildPredicate(*expr.left, table),
                          buildPredicate(*expr.right, table));
    }
    throw std::runtime_error("Invalid predicate expression");
}

// Links Scan -> Filter -> Sort -> Project -> Limit into a plan.
std::unique_ptr<Operator> buildPlan(const SelectStatement& stmt, const std::map<std::string, Table>& catalog) {
    auto tableIt = catalog.find(stmt.table);
    if (tableIt == catalog.end()) {
        throw std::runtime_error("Unknown table: " + stmt.table);
    }
    const Table& table = tableIt->second;
    
    std::unique_ptr<Operator> plan = std::make_unique<Scan>(table);
    if (stmt.where) {
        auto predicate = buildPredicate(*stmt.where, table);
        plan = std::make_unique<Filter>(std::move(plan), std::move(predicate));
    }
    if (!stmt.orderBy.empty()) {
        std::vector<SortKey> sortKeys;
        for (const auto& key : stmt.orderBy) {
            sortKeys.push_back({getColumnIndex(table, key.column), key.descending});
        }
        plan = std::make_unique<Sort>(std::move(plan), std::move(sortKeys));
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

    if (stmt.limit.has_value()) {
        plan = std::make_unique<Limit>(std::move(plan), *stmt.limit);
    }

    return plan;
}