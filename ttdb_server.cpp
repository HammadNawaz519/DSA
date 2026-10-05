#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

using namespace std;

const int max_vars_per_frame = 16;
const int max_stack_depth = 64;

enum TokenType
{
    KEYWORD,
    IDENTIFIER,
    PARAM
};

struct Token
{
    TokenType type;
    string text;
};

struct Variable
{
    string name;
    int32_t value;
};

struct Frame
{
    string func_name;
    int32_t argc;
    Variable argv[max_vars_per_frame];
    int32_t returnLine;
    Variable locals[max_vars_per_frame];
    int32_t localCount;
};

struct Snapshot
{
    Frame callStack[max_stack_depth];
    int32_t stackDepth;
};

struct FuncEntry
{
    string funcName;
    int64_t byteOffsetInResolveBin;
};

struct PendingPatch
{
    int64_t byteOffsetOfOffsetField;
    string targetFuncName;
};

int main()
{
    return 0;
}
