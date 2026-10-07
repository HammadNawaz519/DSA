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

bool validate_structural_integrity(const string &source_path)
{
    ifstream input_file(source_path);
    if (!input_file.is_open())
    {
        cerr << "Error: cannot open source file " << source_path << endl;
        return false;
    }

    vector<string> func_stack;
    string raw_line;
    int line_num = 0;

    while (getline(input_file, raw_line))
    {
        line_num++;
        size_t first_non_space = raw_line.find_first_not_of(" \t\r\n");
        if (first_non_space == string::npos)
        {
            continue;
        }

        stringstream line_stream(raw_line);
        string first_word;
        line_stream >> first_word;

        if (first_word == "func")
        {
            if (!func_stack.empty())
            {
                cerr << "Structural error at line " << line_num << ": nested function declaration rejected." << endl;
                return false;
            }
            string current_func_name;
            line_stream >> current_func_name;
            if (current_func_name.empty())
            {
                cerr << "Structural error at line " << line_num << ": missing function name." << endl;
                return false;
            }
            func_stack.push_back(current_func_name);
        }
        else if (first_word == "func_end")
        {
            if (func_stack.empty())
            {
                cerr << "Structural error at line " << line_num << ": unmatched func_end." << endl;
                return false;
            }
            func_stack.pop_back();
        }
        else
        {
            if (func_stack.empty())
            {
                cerr << "Structural error at line " << line_num << ": instruction outside of function declaration." << endl;
                return false;
            }
        }
    }

    if (!func_stack.empty())
    {
        cerr << "Structural error: unclosed function at end of file: " << func_stack.back() << endl;
        return false;
    }

    return true;
}

int main()
{
    return 0;
}

bool generate_resolve_bin(const string &source_path, const string &resolve_path, vector<FuncEntry> &func_table, vector<PendingPatch> &pending_patches)
{
    ifstream input_file(source_path);
    if (!input_file.is_open())
    {
        cerr << "Error: cannot open " << source_path << endl;
        return false;
    }

    FILE *out_file = fopen(resolve_path.c_str(), "wb");
    if (!out_file)
    {
        cerr << "Error: cannot create " << resolve_path << endl;
        return false;
    }

    int64_t current_offset = 0;
    string raw_line;

    while (getline(input_file, raw_line))
    {
        size_t first_idx = raw_line.find_first_not_of(" \t\r\n");
        if (first_idx == string::npos)
        {
            continue;
        }
        size_t last_idx = raw_line.find_last_not_of(" \t\r\n");
        string clean_line = raw_line.substr(first_idx, last_idx - first_idx + 1);

        stringstream line_stream(clean_line);
        string first_word;
        line_stream >> first_word;

        if (first_word == "func")
        {
            string current_func_name;
            line_stream >> current_func_name;

            FuncEntry entry;
            entry.funcName = current_func_name;
            entry.byteOffsetInResolveBin = current_offset;
            func_table.push_back(entry);

            int32_t str_size = clean_line.size();
            fwrite(&current_offset, sizeof(int64_t), 1, out_file);
            fwrite(&str_size, sizeof(int32_t), 1, out_file);
            fwrite(clean_line.data(), 1, str_size, out_file);
            current_offset = current_offset + 8 + 4 + str_size;
        }
        else if (first_word == "call")
        {
            string target_func;
            line_stream >> target_func;

            string rest_of_line;
            string rem_token;
            while (line_stream >> rem_token)
            {
                if (!rest_of_line.empty())
                {
                    rest_of_line += " ";
                }
                rest_of_line += rem_token;
            }

            string hex_placeholder = "0x0000000000000000";
            string formatted_line = "call " + hex_placeholder;
            if (!rest_of_line.empty())
            {
                formatted_line += " " + rest_of_line;
            }

            int64_t patch_offset = current_offset + 8 + 4 + 5;
            PendingPatch patch_record;
            patch_record.byteOffsetOfOffsetField = patch_offset;
            patch_record.targetFuncName = target_func;
            pending_patches.push_back(patch_record);

            int32_t str_size = formatted_line.size();
            fwrite(&current_offset, sizeof(int64_t), 1, out_file);
            fwrite(&str_size, sizeof(int32_t), 1, out_file);
            fwrite(formatted_line.data(), 1, str_size, out_file);
            current_offset = current_offset + 8 + 4 + str_size;
        }
        else
        {
            int32_t str_size = clean_line.size();
            fwrite(&current_offset, sizeof(int64_t), 1, out_file);
            fwrite(&str_size, sizeof(int32_t), 1, out_file);
            fwrite(clean_line.data(), 1, str_size, out_file);
            current_offset = current_offset + 8 + 4 + str_size;
        }

int64_t patch_resolve_bin(const string &resolve_path, const vector<FuncEntry> &func_table, const vector<PendingPatch> &pending_patches)
{
    FILE *patch_file = fopen(resolve_path.c_str(), "r+b");
    if (!patch_file)
    {
        cerr << "Error: cannot open " << resolve_path << " for patching" << endl;
        return -1;
    }

    for (size_t patch_idx = 0; patch_idx < pending_patches.size(); patch_idx++)
    {
        const PendingPatch &curr_patch = pending_patches[patch_idx];
        bool func_found = false;
        int64_t target_offset = 0;

        for (size_t entry_idx = 0; entry_idx < func_table.size(); entry_idx++)
        {
            if (func_table[entry_idx].funcName == curr_patch.targetFuncName)
            {
                func_found = true;
                target_offset = func_table[entry_idx].byteOffsetInResolveBin;
                break;
            }
        }

        if (!func_found)
        {
            cerr << "Error: call to undefined function: " << curr_patch.targetFuncName << endl;
            fclose(patch_file);
            return -1;
        }

        char hex_buf[32];
        snprintf(hex_buf, sizeof(hex_buf), "0x%016lx", (unsigned long)target_offset);

        if (fseek(patch_file, curr_patch.byteOffsetOfOffsetField, SEEK_SET) != 0)
        {
            cerr << "Error: failed to seek to patch location" << endl;
            fclose(patch_file);
            return -1;
        }

        fwrite(hex_buf, 1, 18, patch_file);
    }

    fclose(patch_file);

    int64_t main_offset = -1;
    for (size_t entry_idx = 0; entry_idx < func_table.size(); entry_idx++)
    {
        if (func_table[entry_idx].funcName == "main")
        {
            main_offset = func_table[entry_idx].byteOffsetInResolveBin;
            break;
        }
    }

    if (main_offset == -1)
    {
        cerr << "Error: main function does not exist." << endl;
        return -1;
    }

    return main_offset;
}

vector<Token> tokenize_line(const string &line_text)
{
    vector<Token> token_list;
    stringstream line_stream(line_text);
    string token_word;
    int token_idx = 0;

    while (line_stream >> token_word)
    {
        Token current_token;
        current_token.text = token_word;
        if (token_idx == 0)
        {
            current_token.type = KEYWORD;
        }
        else if (token_idx == 1)
        {
            current_token.type = IDENTIFIER;
        }
        else
        {
            current_token.type = PARAM;
        }
        token_list.push_back(current_token);
        token_idx++;
    }

    return token_list;
}

bool is_number(const string &token_str)
{
    if (token_str.empty())
    {
        return false;
    }
    size_t start_idx = 0;
    if (token_str[0] == '-' || token_str[0] == '+')
    {
        if (token_str.size() == 1)
        {
            return false;
        }
        start_idx = 1;
    }
    for (size_t char_idx = start_idx; char_idx < token_str.size(); char_idx++)
    {
        if (!isdigit(token_str[char_idx]))
        {
            return false;
        }
    }
    return true;
}

int32_t resolve_value(const Frame &curr_frame, const string &token_text)
{
    if (is_number(token_text))
    {
        return atoi(token_text.c_str());
    }

    for (int32_t var_idx = 0; var_idx < curr_frame.localCount; var_idx++)
    {
        if (curr_frame.locals[var_idx].name == token_text)
        {
            return curr_frame.locals[var_idx].value;
        }
    }

    for (int32_t arg_idx = 0; arg_idx < curr_frame.argc; arg_idx++)
    {
        if (curr_frame.argv[arg_idx].name == token_text)
        {
            return curr_frame.argv[arg_idx].value;
        }
    }

    return 0;
}

void set_variable(Frame &curr_frame, const string &target_name, int32_t new_val)
{
    for (int32_t var_idx = 0; var_idx < curr_frame.localCount; var_idx++)
    {
        if (curr_frame.locals[var_idx].name == target_name)
        {
            curr_frame.locals[var_idx].value = new_val;
            return;
        }
    }

    for (int32_t arg_idx = 0; arg_idx < curr_frame.argc; arg_idx++)
    {
        if (curr_frame.argv[arg_idx].name == target_name)
        {
            curr_frame.argv[arg_idx].value = new_val;
            return;
        }
    }

    if (curr_frame.localCount < max_vars_per_frame)
    {
        curr_frame.locals[curr_frame.localCount].name = target_name;
        curr_frame.locals[curr_frame.localCount].value = new_val;
        curr_frame.localCount++;
    }
}

int main()
{
    return 0;
}
