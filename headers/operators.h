#pragma once
#include <stdexcept>
#include <memory>
#include "load_file.h"

enum class CmpOp {
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge
};

struct Predicate {
    enum class Kind { Compare, And, Or};
    Kind kind;

    size_t column = 0;
    CmpOp op = CmpOp::Eq;
    Value literal;

    std::unique_ptr<Predicate> left, right;
};

// Translate flat predicates into tree.
std::unique_ptr<Predicate> makeCompare(size_t column, const std::string& op, Value literal);
std::unique_ptr<Predicate> makeAnd(std::unique_ptr<Predicate> left, std::unique_ptr<Predicate> right);
std::unique_ptr<Predicate> makeOr(std::unique_ptr<Predicate> left, std::unique_ptr<Predicate> right);

// Executes predicate against row.
bool evaluate(const Predicate& predicate, const Row& row);

class Operator {
public:
    virtual ~Operator() = default;
    virtual bool next(Row& out) = 0;
};


class Scan : public Operator {
    const Table& table;
    size_t currentRow = 0;
public:
    Scan(const Table& table) : table(table) {}
    // Returns next row from table.
    bool next(Row& out) override;
};


class Filter : public Operator {
    std::unique_ptr<Operator> child;
    std::unique_ptr<Predicate> predicate;

public:
    Filter(std::unique_ptr<Operator> child, std::unique_ptr<Predicate> predicate);
    // Returns next matching row.
    bool next(Row& out) override;
};


class Project : public Operator {
    std::unique_ptr<Operator> child;
    std::vector<size_t> columns;
public:
    Project(std::unique_ptr<Operator> child, std::vector<size_t> columns);
    // Returns next row with only projected columns.
    bool next(Row& out) override;
};