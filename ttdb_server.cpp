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

bool read_record(FILE *file_ptr, int64_t target_pos, string &line_output, int64_t &next_pos)
{
    if (fseek(file_ptr, target_pos, SEEK_SET) != 0)
    {
        return false;
    }
    int64_t record_offset = 0;
    int32_t record_size = 0;
    if (fread(&record_offset, sizeof(int64_t), 1, file_ptr) != 1)
    {
        return false;
    }
    if (fread(&record_size, sizeof(int32_t), 1, file_ptr) != 1)
    {
        return false;
    }
    vector<char> str_buffer(record_size + 1, 0);
    if (fread(str_buffer.data(), 1, record_size, file_ptr) != (size_t)record_size)
    {
        return false;
    }
    line_output = string(str_buffer.data(), record_size);
    next_pos = target_pos + 8 + 4 + record_size;
    return true;
}

bool execute_program(const string &resolve_path, int64_t main_offset, Timeline &timeline_instance)
{
    FILE *exec_file = fopen(resolve_path.c_str(), "rb");
    if (!exec_file)
    {
        cerr << "Error: cannot open " << resolve_path << " for execution" << endl;
        return false;
    }

    Snapshot current_snapshot;
    current_snapshot.stackDepth = 0;

    string caller_arg_names[max_stack_depth][max_vars_per_frame];

    int64_t current_offset = main_offset;
    string line_text;
    int64_t next_offset = 0;

    if (!read_record(exec_file, current_offset, line_text, next_offset))
    {
        cerr << "Error: failed to read main function header" << endl;
        fclose(exec_file);
        return false;
    }

    Frame &main_frame = current_snapshot.callStack[0];
    main_frame.func_name = "main";
    main_frame.argc = 0;
    main_frame.returnLine = -1;
    main_frame.localCount = 0;
    current_snapshot.stackDepth = 1;

    timeline_instance.record(&current_snapshot);
    current_offset = next_offset;

    while (current_snapshot.stackDepth > 0)
    {
        if (!read_record(exec_file, current_offset, line_text, next_offset))
        {
            break;
        }

        vector<Token> tokens = tokenize_line(line_text);
        if (tokens.empty())
        {
            current_offset = next_offset;
            continue;
        }

        string cmd_word = tokens[0].text;
        Frame &curr_frame = current_snapshot.callStack[current_snapshot.stackDepth - 1];

        if (cmd_word == "set")
        {
            if (tokens.size() >= 3)
            {
                string target_var = tokens[1].text;
                int32_t evaluated_val = resolve_value(curr_frame, tokens[2].text);
                set_variable(curr_frame, target_var, evaluated_val);
            }
            current_offset = next_offset;
        }
        else if (cmd_word == "add")
        {
            if (tokens.size() >= 3)
            {
                string dest_var = tokens[1].text;
                int32_t val_one = resolve_value(curr_frame, dest_var);
                int32_t val_two = resolve_value(curr_frame, tokens[2].text);
                set_variable(curr_frame, dest_var, val_one + val_two);
            }
            current_offset = next_offset;
        }
        else if (cmd_word == "sub")
        {
            if (tokens.size() >= 3)
            {
                string dest_var = tokens[1].text;
                int32_t val_one = resolve_value(curr_frame, dest_var);
                int32_t val_two = resolve_value(curr_frame, tokens[2].text);
                set_variable(curr_frame, dest_var, val_one - val_two);
            }
            current_offset = next_offset;
        }
        else if (cmd_word == "mul")
        {
            if (tokens.size() >= 3)
            {
                string dest_var = tokens[1].text;
                int32_t val_one = resolve_value(curr_frame, dest_var);
                int32_t val_two = resolve_value(curr_frame, tokens[2].text);
                set_variable(curr_frame, dest_var, val_one * val_two);
            }
            current_offset = next_offset;
        }
        else if (cmd_word == "div")
        {
            if (tokens.size() >= 3)
            {
                string dest_var = tokens[1].text;
                int32_t val_one = resolve_value(curr_frame, dest_var);
                int32_t val_two = resolve_value(curr_frame, tokens[2].text);
                if (val_two != 0)
                {
                    set_variable(curr_frame, dest_var, val_one / val_two);
                }
            }
            current_offset = next_offset;
        }
        else if (cmd_word == "call")
        {
            if (tokens.size() >= 2 && current_snapshot.stackDepth < max_stack_depth)
            {
                int64_t target_offset = strtoll(tokens[1].text.c_str(), NULL, 0);

                Frame &new_frame = current_snapshot.callStack[current_snapshot.stackDepth];
                new_frame.returnLine = next_offset;
                new_frame.localCount = 0;
                new_frame.argc = 0;

                int arg_count = tokens.size() - 2;
                vector<int32_t> arg_values;
                vector<string> arg_var_names;
                for (int i = 0; i < arg_count && i < max_vars_per_frame; i++)
                {
                    arg_var_names.push_back(tokens[2 + i].text);
                    arg_values.push_back(resolve_value(curr_frame, tokens[2 + i].text));
                }

                string target_header;
                int64_t target_next = 0;
                if (!read_record(exec_file, target_offset, target_header, target_next))
                {
                    cerr << "Error: failed to read target function at offset " << target_offset << endl;
                    fclose(exec_file);
                    return false;
                }

                vector<Token> target_tokens = tokenize_line(target_header);
                new_frame.func_name = (target_tokens.size() >= 2) ? target_tokens[1].text : "func";
                new_frame.argc = arg_count;

                for (int i = 0; i < arg_count && i < max_vars_per_frame; i++)
                {
                    string param_name = (i + 2 < (int)target_tokens.size()) ? target_tokens[2 + i].text : ("p" + to_string(i));
                    new_frame.argv[i].name = param_name;
                    new_frame.argv[i].value = arg_values[i];
                    caller_arg_names[current_snapshot.stackDepth][i] = arg_var_names[i];
                }

                current_snapshot.stackDepth++;
                current_offset = target_next;
            }
            else
            {
                current_offset = next_offset;
            }
        }
        else if (cmd_word == "func_end")
        {
            if (current_snapshot.stackDepth > 1)
            {
                int callee_index = current_snapshot.stackDepth - 1;
                int caller_index = current_snapshot.stackDepth - 2;
                Frame &callee = current_snapshot.callStack[callee_index];
                Frame &caller = current_snapshot.callStack[caller_index];

                for (int i = 0; i < callee.argc; i++)
                {
                    string caller_var = caller_arg_names[callee_index][i];
                    if (!caller_var.empty() && !is_number(caller_var))
                    {
                        set_variable(caller, caller_var, callee.argv[i].value);
                    }
                }

                int64_t ret_addr = callee.returnLine;
                current_snapshot.stackDepth--;
                current_offset = ret_addr;
            }
            else
            {
                current_snapshot.stackDepth--;
                current_offset = next_offset;
            }
        }
        else
        {
            current_offset = next_offset;
        }

        timeline_instance.record(&current_snapshot);
    }

    fclose(exec_file);
    return true;
}

void writeHeader(FILE *f, const TTDBHeader &h)
{
    fwrite(h.magic, 1, 4, f);
    fwrite(&h.version, sizeof(int32_t), 1, f);
    fwrite(&h.stepCount, sizeof(int32_t), 1, f);
    fwrite(&h.indexOffset, sizeof(int64_t), 1, f);
}

void writeTdbg(Timeline &timeline, const char *tdbgPath)
{
    FILE *tdbg_file = fopen(tdbgPath, "wb");
    if (!tdbg_file)
    {
        cerr << "Error: cannot open " << tdbgPath << " for serialization" << endl;
        return;
    }

    TTDBHeader file_header;
    file_header.magic[0] = 'T';
    file_header.magic[1] = 'T';
    file_header.magic[2] = 'D';
    file_header.magic[3] = 'B';
    file_header.version = 1;
    file_header.stepCount = timeline.getStepCount();
    file_header.indexOffset = 0;

    writeHeader(tdbg_file, file_header);

    int32_t total_steps = timeline.getStepCount();
    int64_t *index_array = new int64_t[total_steps];

    TimelineNode *curr_node = timeline.begin();
    for (int32_t step_idx = 0; step_idx < total_steps && curr_node; step_idx++)
    {
        index_array[step_idx] = ftell(tdbg_file);
        fwrite(curr_node->data, sizeof(Snapshot), 1, tdbg_file);
        curr_node = curr_node->next;
    }

    file_header.indexOffset = ftell(tdbg_file);
    fwrite(index_array, sizeof(int64_t), total_steps, tdbg_file);
    delete[] index_array;

    fseek(tdbg_file, 0, SEEK_SET);
    writeHeader(tdbg_file, file_header);

    fclose(tdbg_file);
}

int main()
{
    return 0;
}
