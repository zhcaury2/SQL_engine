#include "operators.h"
#include <algorithm>


static std::unique_ptr<Predicate> makeBinary(Predicate::Kind kind,
                                             std::unique_ptr<Predicate> left,
                                             std::unique_ptr<Predicate> right) {
    if (!left || !right) throw std::runtime_error("Missing predicate operand");
    auto predicate = std::make_unique<Predicate>();
    predicate->kind = kind;
    predicate->left = std::move(left);
    predicate->right = std::move(right);
    return predicate;
}


std::unique_ptr<Predicate> makeAnd(std::unique_ptr<Predicate> left, std::unique_ptr<Predicate> right) {
    return makeBinary(Predicate::Kind::And, std::move(left), std::move(right));
}

std::unique_ptr<Predicate> makeOr(std::unique_ptr<Predicate> left, std::unique_ptr<Predicate> right) {
    return makeBinary(Predicate::Kind::Or, std::move(left), std::move(right));
}

bool Scan::next(Row& out) {
    if (currentRow >= table.rows.size()) return false;
    out = table.rows[currentRow++];
    return true;
}

Filter::Filter(std::unique_ptr<Operator> c, std::unique_ptr<Predicate> p)
    : child(std::move(c)), predicate(std::move(p)) {
        if (!predicate) throw std::runtime_error("Missing predicate");
    }

bool Filter::next(Row& out) {
    while (child->next(out)) {
        if (evaluate(*predicate, out)) return true;
    }
    return false;
}

static int compareNumbers(double a, double b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static int compareValues(const Value& a, const Value& b) {
    if (a.index() != b.index()) return a.index() < b.index() ? -1 : 1;
    if (const double* x = std::get_if<double>(&a)) {
        return compareNumbers(*x, std::get<double>(b));
    }
    if (const std::string* s = std::get_if<std::string>(&a)) {
        int c = s->compare(std::get<std::string>(b));
        return c < 0 ? -1 : (c > 0 ? 1 : 0);
    }
    return 0;
}

static CmpOp parseOperator(const std::string& op) {
    if (op == "=") return CmpOp::Eq;
    if (op == "!=") return CmpOp::Ne;
    if (op == "<") return CmpOp::Lt;
    if (op == "<=") return CmpOp::Le;
    if (op == ">") return CmpOp::Gt;
    if (op == ">=") return CmpOp::Ge;
    throw std::runtime_error("Unknown operator: " + op);
}

static bool applyOperator(CmpOp op, int c) {
    switch (op) {
        case CmpOp::Eq: return c == 0;
        case CmpOp::Ne: return c != 0;
        case CmpOp::Lt: return c < 0;
        case CmpOp::Le: return c <= 0;
        case CmpOp::Gt: return c > 0;
        case CmpOp::Ge: return c >= 0;
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


static bool compareCell(const Predicate& predicate, const Value& cell) {
    if (std::holds_alternative<double>(cell) && std::holds_alternative<double>(predicate.literal)) {
        return applyOperator(predicate.op, compareNumbers(std::get<double>(cell), std::get<double>(predicate.literal)));
    }
    if (std::holds_alternative<std::string>(cell) && std::holds_alternative<std::string>(predicate.literal)) {
        const auto& a = std::get<std::string>(cell);
        const auto& b = std::get<std::string>(predicate.literal);
        return applyOperator(predicate.op, a.compare(b));
    }
    return false;
}

bool evaluate(const Predicate& predicate, const Row& row) {
    switch (predicate.kind) {
        case Predicate::Kind::Compare:
            return compareCell(predicate, row[predicate.column]);
        case Predicate::Kind::And:
            return evaluate(*predicate.left, row) && evaluate(*predicate.right, row);
        case Predicate::Kind::Or:
            return evaluate(*predicate.left, row) || evaluate(*predicate.right, row);
    }
    return false;
}

std::unique_ptr<Predicate> makeCompare(size_t column, const std::string& op, Value literal) {
    auto predicate = std::make_unique<Predicate>();
    predicate->kind = Predicate::Kind::Compare;
    predicate->column = column;
    predicate->op = parseOperator(op);
    predicate->literal = std::move(literal);
    return predicate;
}

Sort::Sort(std::unique_ptr<Operator> c, std::vector<SortKey> order)
    : child(std::move(c)), orderBy(std::move(order)) {
    if (orderBy.empty()) {
        throw std::runtime_error("Sort requires at least one sort key");
    }
}

bool Sort::next(Row& out) {
    if (!loaded) {
        Row row;
        while (child->next(row)) rows.push_back(std::move(row));
        std::stable_sort(rows.begin(), rows.end(), [&](const Row& a, const Row& b) {
        for (const auto& key : orderBy) {
            int c = compareValues(a[key.column], b[key.column]);
            if (c != 0) return key.descending ? c > 0 : c < 0;
        }
            return false;
        });
        loaded = true;
    }
    if (currentRow >= rows.size()) return false;
    out = rows[currentRow++];
    return true;
}


Limit::Limit(std::unique_ptr<Operator> c, size_t l)
    : child(std::move(c)), count(l) {}

bool Limit::next(Row& out) {
    if (returned >= count) return false;
    if (!child->next(out)) return false;
    ++returned;
    return true;
}

