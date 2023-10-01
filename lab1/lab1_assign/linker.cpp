#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iomanip>
#include <utility>
#include <cstring>
#include <bits/stdc++.h>

using namespace std;

struct Symbol
{
    string name;
    int address;
    string message;
};

struct Instruction
{
    char addrMode;
    int instruction;
};

struct Module
{
    int baseAddress;
    vector<Symbol> definitions;
    vector<string> useList;
    vector<Instruction> instructions;
};

map<string, pair<int, string>> symbolTable;
map<string, int> symbolused;
vector<Module> moduleBaseTable;
vector<pair<int, string>> memory_map;
int totalcode = 0;
int linenum = 1;
int offset = 0;
int prev_offset;
int line_changed = 0;
int prev_token;

void __parseerror(int errcode)
{
    string errstr[] = {
        "NUM_EXPECTED",           // Number expect, anything >= 2^30 is not a number either
        "SYM_EXPECTED",           // Symbol Expected
        "MARIE_EXPECTED",         // Addressing Expected which is M/A/R/I/E
        "SYM_TOO_LONG",           // Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR",         // total num_instr exceeds memory size (512)
    };
    cout << "Parse Error line " << linenum << " offset " << offset << ": " << errstr[errcode] << endl;
    exit(1);
}

void printModule(const Module &module)
{
    cout << "Base Address: " << module.baseAddress << endl;

    cout << "Definitions:" << endl;
    for (const Symbol &symbol : module.definitions)
    {
        cout << "  Name: " << symbol.name << ", Address: " << symbol.address << endl;
    }

    cout << "Use List:" << endl;
    for (const string &use : module.useList)
    {
        cout << "  " << use << endl;
    }

    cout << "Instructions:" << endl;
    for (const Instruction &instr : module.instructions)
    {
        cout << "  Addr Mode: " << instr.addrMode << ", Instruction: " << instr.instruction << endl;
    }
}

void printSymboltable()
{
    cout << "Symbol Table" << endl;
    for (auto it = symbolTable.begin(); it != symbolTable.end(); ++it)
    {
        cout << it->first << "=" << it->second.first << " " << it->second.second << endl;
    }
}

void printMemorymap()
{
    cout << "Memory Map" << endl;
    for (int i = 0; i < memory_map.size(); i++)
    {
        cout << setw(3) << setfill('0') << i;
        cout << ": " << memory_map[i].first << " " << memory_map[i].second << endl;
    }
}

Symbol createSymbol(string symbolname, int val)
{
    Symbol out;
    out.address = val;
    out.name = symbolname;
    return out;
}

string getToken(ifstream &inputFile)
{
    offset += prev_token;
    if (line_changed > 0)
    {
        linenum += line_changed;
        prev_offset = offset - 1;
        offset = 0;
        line_changed = 0;
    }
    string token;
    while (true)
    {
        char c = inputFile.get();
        offset += 1;
        if (c == ' ' || c == '\t' || c == '\n')
        {
            if (c == '\n')
            {
                linenum++; // Increment line number on newline
                prev_offset = offset - 1;
                offset = 0;
            }
            continue; // Skip delimiter characters
        }
        else if (inputFile.eof())
        {
            if (offset == 0)
            {
                line_changed = 1;
            }
            return token;
        }
        else
        {
            token += c; // Append the character to the token
            break;
        }
    }

    // Read characters until a delimiter or end of file is encountered
    while (true)
    {
        char c = inputFile.get();
        if (c == ' ' || c == '\t' || c == '\n' || inputFile.eof())
        {
            if (c == '\n')
            {
                line_changed = 1;
            }
            break; // Return the token if a delimiter or end of file is reached
        }
        else
        {
            token += c; // Append the character to the token
        }
    }

    return token;
}

int readInt(ifstream &inputFile)
{
    string token;
    if ((token = getToken(inputFile)).empty())
    {
        return EOF;
    }
    if (line_changed > 0)
    {
        prev_token = token.length();
        int out = 0;
        try
        {
            out = stoi(token);
        }
        catch (const invalid_argument &e)
        {
            if (inputFile.eof())
            {
                linenum -= line_changed + 1;
                offset = prev_offset + 1;
            }
            inputFile.close();
            __parseerror(0);
        }
        catch (const out_of_range &e)
        {
            if (inputFile.eof())
            {
                linenum -= line_changed + 1;
                offset = prev_offset + 1;
            }
            inputFile.close();
            __parseerror(0);
        }
        if (out >= pow(2, 30))
        {
            if (inputFile.eof())
            {
                linenum -= line_changed + 1;
                offset = prev_offset + 1;
            }
            inputFile.close();
            __parseerror(0);
        }
        return out;
    }
    prev_token = token.length();
    int out = 0;
    try
    {
        out = stoi(token);
    }
    catch (const invalid_argument &e)
    {
        inputFile.close();
        __parseerror(0);
    }
    catch (const out_of_range &e)
    {
        if (inputFile.eof())
        {
            linenum -= line_changed + 1;
            offset = prev_offset + 1;
        }
        inputFile.close();
        __parseerror(0);
    }
    if (out >= pow(2, 30))
    {
        inputFile.close();
        __parseerror(0);
    }
    return out;
}

string readSym(ifstream &inputFile)
{
    string token;
    if (line_changed > 0)
    {
        string token = getToken(inputFile);
        prev_token = token.length();
        if (prev_token > 16)
        {
            inputFile.close();
            __parseerror(3);
        }
        if (!isalpha(token[0]))
        {
            if (inputFile.eof())
            {
                linenum -= line_changed + 1;
                offset = prev_offset + 1;
            }
            inputFile.close();
            __parseerror(1);
        }
        if (token.length() > 1)
        {
            regex pattern("^[a-zA-Z]+$|^[0-9]+$");
            if (!regex_match(token.substr(1), pattern))
            {
                if (inputFile.eof())
                {
                    linenum -= line_changed + 1;
                    offset = prev_offset + 1;
                }
                inputFile.close();
                __parseerror(1);
            }
            return token;
        }
        else
        {
            return token;
        }
    }
    else
    {
        string token = getToken(inputFile);
        prev_token = token.length();
        if (prev_token > 16)
        {
            inputFile.close();
            __parseerror(3);
        }
        if (!isalpha(token[0]))
        {
            inputFile.close();
            __parseerror(1);
        }
        if (token.length() > 1)
        {
            regex pattern("^[a-zA-Z]+$|^[0-9]+$");
            if (!regex_match(token.substr(1), pattern))
            {
                inputFile.close();
                __parseerror(1);
            }
            return token;
        }
        else
        {
            return token;
        }
    }
    return token;
}

char readIAER(ifstream &inputFile)
{
    string token;
    if (line_changed > 0)
    {
        token = getToken(inputFile);
        prev_token = token.length();
        if (token != "M" && token != "A" && token != "R" && token != "I" && token != "E")
        {
            if (inputFile.eof())
            {
                linenum -= line_changed + 1;
                offset = prev_offset + 1;
            }
            inputFile.close();
            __parseerror(2);
        }
    }
    else
    {
        token = getToken(inputFile);
        prev_token = token.length();
        if (token != "M" && token != "A" && token != "R" && token != "I" && token != "E")
        {
            inputFile.close();
            __parseerror(2);
        }
    }
    return token[0];
}

void passOne(ifstream &inputFile)
{
    int prev_length = 0;
    while (true)
    {
        Module new_module;
        if (moduleBaseTable.empty())
        {
            new_module.baseAddress = 0;
        }
        else
        {
            new_module.baseAddress = moduleBaseTable.back().baseAddress + prev_length;
        }

        int defcount = readInt(inputFile);
        if (defcount == EOF)
        {
            return;
        }
        if (defcount > 16)
        {
            inputFile.close();
            __parseerror(4);
        }
        vector<string> deflist;
        for (int i = 0; i < defcount; i++)
        {
            string sym = readSym(inputFile);
            int val = readInt(inputFile);

            if (symbolTable.count(sym) > 0)
            {
                cout << "Warning: Module " << moduleBaseTable.size() + 1 << ": " << sym << " redefinition ignored" << endl;
                symbolTable[sym].second = "Error: This variable is multiple times defined; first value used";
            }
            else
            {
                symbolTable[sym].first = val + new_module.baseAddress;
                deflist.push_back(sym);
            }
        }

        int usecount = readInt(inputFile);
        if (usecount > 16)
        {
            inputFile.close();
            __parseerror(5);
        }
        for (int i = 0; i < usecount; i++)
        {
            string sym = readSym(inputFile);
        }

        int instcount = readInt(inputFile);
        for (string sym : deflist)
        {
            if (symbolTable[sym].first - new_module.baseAddress > instcount - 1)
            {
                cout << "Warning: Module " << moduleBaseTable.size() + 1 << ": " << sym
                     << " too big " << symbolTable[sym].first - new_module.baseAddress
                     << " (max=" << instcount - 1 << ") assume zero relative" << endl;
                symbolTable[sym].first = new_module.baseAddress;
            }
        }
        totalcode += instcount;
        if (totalcode >= 512)
        {
            inputFile.close();
            __parseerror(6);
        }
        prev_length = instcount;
        for (int i = 0; i < instcount; i++)
        {
            char addrMode = readIAER(inputFile);
            int instruction = readInt(inputFile);
        }
        moduleBaseTable.push_back(new_module);
    }
    if (totalcode >= 512)
    {
        inputFile.close();
        __parseerror(6);
    }
}

void passTwo(ifstream &inputFile_new)
{
    for (int i = 0; i < moduleBaseTable.size(); i++)
    {
        map<string, int> useused;
        int defcount = readInt(inputFile_new);
        vector<Symbol> deflist;
        for (int j = 0; j < defcount; j++)
        {
            string sym = readSym(inputFile_new);
            int val = readInt(inputFile_new);
            deflist.push_back(createSymbol(sym, val));
        }
        moduleBaseTable[i].definitions = deflist;

        int usecount = readInt(inputFile_new);
        vector<string> uselist;
        for (int j = 0; j < usecount; j++)
        {
            string sym = readSym(inputFile_new);
            symbolused[sym] = 1;
            uselist.push_back(sym);
        }
        moduleBaseTable[i].useList = uselist;

        int instcount = readInt(inputFile_new);
        for (int j = 0; j < instcount; j++)
        {
            string error;
            char addrMode = readIAER(inputFile_new);
            int instruction = readInt(inputFile_new);
            int opcode = instruction / 1000;
            int oprand = instruction % 1000;
            if (opcode >= 10)
            {
                error = "Error: Illegal opcode; treated as 9999";
                instruction = 9999;
                opcode = instruction / 1000;
                oprand = instruction % 1000;
            }
            if (addrMode == 'M')
            {
                if (oprand >= moduleBaseTable.size() && error.empty())
                {
                    error = "Error: Illegal module operand ; treated as module=0";
                    instruction = opcode * 1000 + moduleBaseTable[0].baseAddress;
                }
                else
                {
                    instruction = opcode * 1000 + moduleBaseTable[oprand].baseAddress;
                }
            }
            else if (addrMode == 'A')
            {
                if (oprand >= 512 && error.empty())
                {
                    error = "Error: Absolute address exceeds machine size; zero used";
                    instruction -= oprand;
                }
            }
            else if (addrMode == 'R')
            {
                if (oprand > (instcount - 1) && error.empty())
                {
                    error = "Error: Relative address exceeds module size; relative zero used";
                    instruction -= oprand;
                    instruction += moduleBaseTable[i].baseAddress;
                }
                else
                {
                    instruction = moduleBaseTable[i].baseAddress + instruction;
                }
            }
            else if (addrMode == 'I')
            {
                if (oprand >= 900 && error.empty())
                {
                    error = "Error: Illegal immediate operand; treated as 999";
                    instruction = opcode * 1000 + 999;
                }
            }
            else
            {
                if (oprand >= uselist.size() && error.empty())
                {
                    error = "Error: External operand exceeds length of uselist; treated as relative=0";
                    instruction -= oprand;
                }
                else if (symbolTable.count(uselist[oprand]) == 0 && error.empty())
                {
                    error = "Error: " + uselist[oprand] + " is not defined; zero used";
                    instruction -= oprand;
                    useused[uselist[oprand]] = oprand;
                }
                else
                {
                    instruction = opcode * 1000 + symbolTable[uselist[oprand]].first;
                    useused[uselist[oprand]] = oprand;
                }
            }
            memory_map.push_back(make_pair(instruction, error));

            cout << setw(3) << setfill('0') << memory_map.size() - 1;
            cout << ": ";
            cout << setw(4) << setfill('0') << memory_map.back().first;
            cout << " " << memory_map.back().second << endl;
        }
        for (int j = 0; j < moduleBaseTable[i].useList.size(); j++)
        {
            if (useused.count(moduleBaseTable[i].useList[j]) == 0)
            {
                cout << "Warning: Module " << i + 1 << ": uselist[" << j << "]=" << moduleBaseTable[i].useList[j] << " was not used" << endl;
            }
        }
    }
}

void checkUnusedvar()
{
    for (int i = 0; i < moduleBaseTable.size(); i++)
    {
        for (Symbol sym : moduleBaseTable[i].definitions)
        {
            if (symbolused.count(sym.name) == 0)
            {
                cout << "Warning: Module " << i + 1 << ": " << sym.name << " was defined but never used" << endl;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile.is_open())
    {
        cerr << "Error: Unable to open file " << argv[1] << endl;
        return 1;
    }

    passOne(inputFile);
    printSymboltable();
    cout << endl;
    inputFile.close();

    ifstream inputFile_new(argv[1]);
    if (!inputFile_new.is_open())
    {
        cerr << "Error: Unable to open file " << argv[1] << endl;
        return 1;
    }
    cout << "Memory Map" << endl;
    passTwo(inputFile_new);
    checkUnusedvar();
    cout << endl;
    inputFile_new.close();
    return 0;
}