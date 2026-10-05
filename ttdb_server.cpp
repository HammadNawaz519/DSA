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

struct TimelineNode
{
    Snapshot *data;
    TimelineNode *next;
    TimelineNode *prev;
};

class Timeline
{
    TimelineNode *head, *tail;
    int32_t stepCount;

public:
    Timeline()
    {
        head = nullptr;
        tail = nullptr;
        stepCount = 0;
    }

    ~Timeline()
    {
        TimelineNode *curr_node = head;
        while (curr_node)
        {
            TimelineNode *next_node = curr_node->next;
            delete curr_node->data;
            delete curr_node;
            curr_node = next_node;
        }
    }

    void record(Snapshot *s)
    {
        TimelineNode *new_node = new TimelineNode();
        new_node->data = new Snapshot(*s);
        new_node->next = nullptr;
        new_node->prev = tail;
        if (tail)
        {
            tail->next = new_node;
        }
        else
        {
            head = new_node;
        }
        tail = new_node;
        stepCount++;
    }

    TimelineNode *begin()
    {
        return head;
    }

    int32_t getStepCount()
    {
        return stepCount;
    }
};

struct TTDBHeader
{
    char magic[4];
    int32_t version;
    int32_t stepCount;
    int64_t indexOffset;
};

int main()
{
    return 0;
}
