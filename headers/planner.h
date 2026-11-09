#pragma once
#include "operators.h"
#include "parse.h"
#include <map>

std::unique_ptr<Operator> buildPlan(const SelectStatement& stmt, const std::map<std::string, Table>& catalog);