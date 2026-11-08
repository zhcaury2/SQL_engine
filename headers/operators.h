#pragma once
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
    const Table& table;
    size_t currentRow = 0;
    Operator& child;
public:
    Filter(const Table& table, Operator& child) : table(table), child(child) {}
    bool next(Row& out) override;
};