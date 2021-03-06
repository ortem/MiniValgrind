/**
    Mini Valgrind
    Types.h

    @author Artem Mukhin
*/

#pragma once

#include <cstdlib>
#include <list>
#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <vector>
#include <stack>
#include <memory>
#include <algorithm>

enum VType
{
    T_INT, T_PTR, T_ARR
};
enum EType
{
    E_UNARY, E_BIN, E_VAL, E_VAR, E_FUNC, E_ARRAT
}; // expression type