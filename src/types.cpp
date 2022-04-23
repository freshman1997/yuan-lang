#include "types.h"

static bool is_value_equal(const Value *lhs, const Value *rhs)
{
    if (lhs->get_type() == ValueType::t_number) {
        if (rhs->get_type() == ValueType::t_number) {
            return true;
        }
        return false;
    }
    return false;
}

