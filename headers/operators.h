#pragma once
#include <stdexcept>
#include <memory>
#include "load_file.h"

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
    bool next(Row& out) override;
};


class Filter : public Operator {
    std::unique_ptr<Operator> child;
    size_t targetColumn;
    std::string op;
    Value literal;

    bool matches(const Value& cell) const;
public:
    Filter(std::unique_ptr<Operator> child, size_t colIdx, std::string op, Value literal);
    bool next(Row& out) override;
};