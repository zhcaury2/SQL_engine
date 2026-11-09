#pragma once
#include <string>
#include <vector>
#include "operators.h"

// Runs plan, prints results neatly.
void printResult(Operator& plan, const std::vector<std::string>& names);