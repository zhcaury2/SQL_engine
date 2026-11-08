#include "operators.h"

bool Scan::next(Row& out) {
    if (currentRow >= table.rows.size()) return false;
    out = table.rows[currentRow++];
    return true;
}

bool Filter::next(Row& out) {
    while (child.next(out)) {
        if (currentRow < table.rows.size()) {
            out = table.rows[currentRow++];
            return true;
        }
    }
    return false;
}