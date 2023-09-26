#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iomanip>

using namespace std;

struct Symbol
{
    string name;
    int address;
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

map<string, int> symbolTable;
vector<Module> moduleBaseTable;
vector<int> memory_map;
int totalcode = 0;
int linenum = 1;
int offset = 0;
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
    std::cout << "Parse Error line " << linenum << " offset " << offset << ": " << errstr[errcode] << endl;
}

void printModule(const Module &module)
{
    std::cout << "Base Address: " << module.baseAddress << std::endl;

    std::cout << "Definitions:" << std::endl;
    for (const Symbol &symbol : module.definitions)
    {
        std::cout << "  Name: " << symbol.name << ", Address: " << symbol.address << std::endl;
    }

    std::cout << "Use List:" << std::endl;
    for (const std::string &use : module.useList)
    {
        std::cout << "  " << use << std::endl;
    }

    std::cout << "Instructions:" << std::endl;
    for (const Instruction &instr : module.instructions)
    {
        std::cout << "  Addr Mode: " << instr.addrMode << ", Instruction: " << instr.instruction << std::endl;
    }
}

void printSymboltable()
{
    cout << "Symbol Table" << endl;
    for (auto it = symbolTable.begin(); it != symbolTable.end(); ++it)
    {
        cout << it->first << "=" << it->second << endl;
    }
}

void printMemorymap()
{
    cout << "Memory Map" << endl;
    for (int i = 0; i < memory_map.size(); i++)
    {
        cout << setw(3) << setfill('0') << i;
        cout << ": " << memory_map[i] << endl;
    }
}

Symbol createSymbol(string symbolname, int val)
{
    Symbol out;
    out.address = val;
    out.name = symbolname;
    return out;
}

string getToken(std::ifstream &inputFile)
{
    offset += prev_token;
    if (line_changed)
    {
        linenum += 1;
        offset = 0;
        line_changed = 0;
    }
    std::string token;
    // Skip leading whitespace characters
    while (true)
    {
        char c = inputFile.get();
        offset += 1;
        if (c == ' ' || c == '\t' || c == '\n')
        {
            if (c == '\n')
            {
                linenum++; // Increment line number on newline
                offset = 0;
            }
            continue; // Skip delimiter characters
        }
        else if (inputFile.eof())
        {
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
                line_changed = 1; // Increment line number on newline
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
    prev_token = token.length();
    int out = 0;
    try
    {
        out = stoi(token);
    }
    catch (const std::invalid_argument &e)
    {
        cerr << "Syntax error: invalid token" << endl;
        exit(1);
    }
    catch (const std::out_of_range &e)
    {
        cerr << "Syntax error: token out of range" << endl;
        exit(1);
    }
    return out;
}

string readSym(ifstream &inputFile)
{
    string token = getToken(inputFile);
    prev_token = token.length();
    if (prev_token > 16)
    {
        __parseerror(3);
    }
    if (!isalpha(token[0]))
    {
        cerr << "Syntax error: non-alpha symbol name" << endl;
        exit(1);
    }
    if (token.length() > 1)
    {
        regex pattern("^[a-zA-Z]+$|^[0-9]+$");
        if (!regex_match(token.substr(1), pattern))
        {
            cerr << "Syntax error: wrong symbol name pattern" << endl;
            exit(1);
        }
    }

    return token;
}

char readIAER(ifstream &inputFile)
{
    string token = getToken(inputFile);
    prev_token = token.length();
    if (token.length() != 1)
    {
        cerr << "Syntax error: addrmode exceeds length 1" << endl;
        exit(1);
    }
    if (token[0] != 'M' && token[0] != 'A' && token[0] != 'R' && token[0] != 'I' && token[0] != 'E')
    {
        cerr << "Syntax error: invalid char" << endl;
        exit(1);
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
        for (int i = 0; i < defcount; i++)
        {
            string sym = readSym(inputFile);
            int val = readInt(inputFile);
            symbolTable[sym] = val + new_module.baseAddress;
        }

        int usecount = readInt(inputFile);
        for (int i = 0; i < usecount; i++)
        {
            string sym = readSym(inputFile);
        }

        int instcount = readInt(inputFile);
        totalcode += instcount;
        prev_length = instcount;
        for (int i = 0; i < instcount; i++)
        {
            char addrMode = readIAER(inputFile);
            int instruction = readInt(inputFile);
            if ((instruction / 1000) >= 10)
            {
                cerr << "instruction opcode >= 10";
            }
            if (addrMode == 'A')
            {
                if ((instruction % 1000) >= 512)
                {
                    cerr << "instruction operand greater than machine size";
                }
            }
            else if (addrMode == 'I')
            {
                if ((instruction % 1000) >= 900)
                {
                    cerr << "instruction operand greater than 900";
                }
            }
        }
        moduleBaseTable.push_back(new_module);
    }
    if (totalcode >= 512)
    {
        __parseerror(6);
    }
}

void passTwo(ifstream &inputFile_new)
{
    // for (int i = 0; i < moduleBaseTable.size(); i++)
    // {
    //     int defcount = readInt(inputFile_new);
    //     cout << defcount << " ";
    //     for (int j = 0; j < defcount; j++)
    //     {
    //         string sym = readSym(inputFile_new);
    //         int val = readInt(inputFile_new);
    //         cout << sym << " " << val << " ";
    //     }
    //     cout << endl;
    //     int usecount = readInt(inputFile_new);
    //     cout << usecount << " ";
    //     for (int j = 0; j < usecount; j++)
    //     {
    //         string sym = readSym(inputFile_new);
    //         cout << sym << " ";
    //     }
    //     cout << endl;
    //     int instcount = readInt(inputFile_new);
    //     cout << instcount << " ";
    //     for (int j = 0; j < instcount; j++)
    //     {
    //         char addrMode = readIAER(inputFile_new);
    //         int instruction = readInt(inputFile_new);
    //         cout << addrMode << " " << instruction << " ";
    //     }
    //     cout << endl;
    // }

    for (int i = 0; i < moduleBaseTable.size(); i++)
    {
        int defcount = readInt(inputFile_new);
        for (int j = 0; j < defcount; j++)
        {
            string sym = readSym(inputFile_new);
            int val = readInt(inputFile_new);
        }

        int usecount = readInt(inputFile_new);
        vector<string> uselist;
        for (int j = 0; j < usecount; j++)
        {
            string sym = readSym(inputFile_new);
            uselist.push_back(sym);
        }

        int instcount = readInt(inputFile_new);
        for (int j = 0; j < instcount; j++)
        {
            char addrMode = readIAER(inputFile_new);
            int instruction = readInt(inputFile_new);
            int opcode = instruction / 1000;
            int oprand = instruction % 1000;
            if (addrMode == 'M')
            {
                instruction = opcode * 1000 + moduleBaseTable[i].baseAddress;
            }
            else if (addrMode == 'A')
            {
                if (oprand >= 512)
                {
                    cerr << "operand exceeding machine size" << endl;
                }
            }
            else if (addrMode == 'R')
            {
                instruction = moduleBaseTable[i].baseAddress + instruction;
            }
            else if (addrMode == 'I')
            {
                if (oprand >= 900)
                {
                    cerr << "immediate address must be less than 900" << endl;
                }
            }
            else
            {
                instruction = opcode * 1000 + symbolTable[uselist[oprand]];
            }
            memory_map.push_back(instruction);
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
    }
    // string token;
    // while (!(token = getToken(inputFile)).empty())
    // {
    //     cout << "Token: " << token << " linenum: " << linenum << " offset: " << offset << endl;
    //     prev_token = token.length();
    // }

    passOne(inputFile);
    printSymboltable();
    inputFile.close();
    prev_token = 0;
    linenum = 1;

    ifstream inputFile_new(argv[1]);
    if (!inputFile_new.is_open())
    {
        cerr << "Error: Unable to open file " << argv[1] << endl;
    }

    passTwo(inputFile_new);
    printMemorymap();
    // string token;
    // while (!(token = getToken(inputFile_new)).empty())
    // {
    //     cout << "Token: " << token << " linenum: " << linenum << " offset: " << offset << endl;
    //     prev_token = token.length();
    // }
    inputFile_new.close();
    return 0;
}