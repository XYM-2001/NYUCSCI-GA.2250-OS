#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>

using namespace std;

struct Symbol
{
    string name;
    int module;
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

map<string, Symbol> symbolTable;
vector<Module> moduleBaseTable;
int currentModule = 0;

int readInt(string token)
{
    int out = 0;
    try
    {
        out = stoi(token);
    }
    catch (const std::invalid_argument &e)
    {
        cerr << "Invalid argument: " << e.what() << std::endl;
    }
    catch (const std::out_of_range &e)
    {
        cerr << "Out of range: " << e.what() << std::endl;
    }
    return out;
}

string readSym(string token)
{
    if (token.length() > 16)
    {
        cerr << "symbol name out of range";
    }
    if (!isalpha(token[0]))
    {
        cerr << "symbol name not starting with an Alpha char";
    }
    if (token.length() > 1)
    {
        regex pattern("^[a-zA-Z]+$|^[0-9]+$");
        if (!regex_match(token.substr(1), pattern))
        {
            cerr << "symbol name wrong format";
        }
    }

    return token;
}

char readIAER(string token)
{
    if (token.length() != 1)
    {
        cerr << "not a single char";
    }
    if (token[0] != 'M' && token[0] != 'A' && token[0] != 'R' && token[0] != 'I' && token[0] != 'E')
    {
        cerr << "not a valid operand";
    }
    return token[0];
}

// void passOne(ifstream &inputFile)
// {
//     int i = 0;

//     while (i < tokens.size())
//     {
//         Module new_module;
//         if (moduleBaseTable.empty())
//         {
//             new_module.baseAddress = 0;
//         }
//         else
//         {
//             new_module.baseAddress = moduleBaseTable.back().baseAddress + moduleBaseTable.back().instructions.size();
//         }

//         int defcount = readInt(tokens[i]);
//         i += 1;
//         vector<Symbol> deflist;
//         for (int j = 0; j < defcount; j++)
//         {
//             Symbol curr;
//             curr.name = readSym(tokens[i]);
//             curr.address = readInt(tokens[i + 1]);
//             deflist.push_back(curr);
//             i += 2;
//         }
//         new_module.definitions = deflist;

//         int usecount = readInt(tokens[i]);
//         i += 1;
//         vector<string> uselist;
//         for (int j = 0; j < usecount; j++)
//         {
//             uselist.push_back(readSym(tokens[i]));
//             i += 1;
//         }
//         new_module.useList = uselist;

//         int instcount = readInt(tokens[i]);
//         i += 1;
//         vector<Instruction> instructions;
//         for (int j = 0; j < instcount; j++)
//         {
//             Instruction curr_ins;
//             curr_ins.addrMode = readIAER(tokens[i]);
//             curr_ins.instruction = readInt(tokens[i + 1]);
//             if ((curr_ins.instruction / 1000) >= 10)
//             {
//                 cerr << "instruction opecode >= 10";
//             }
//             if (curr_ins.addrMode == 'A')
//             {
//                 if ((curr_ins.instruction % 1000) >= 512)
//                 {
//                     cerr << "instruction operand greater than machine size";
//                 }
//             }
//             else if (curr_ins.addrMode == 'I')
//             {
//                 if ((curr_ins.instruction % 1000) >= 900)
//                 {
//                     cerr << "instruction operand greater than 900";
//                 }
//             }
//             instructions.push_back(curr_ins);
//             i += 2;
//         }
//         new_module.instructions = instructions;
//         moduleBaseTable.push_back(new_module);
//     }
// }

void passTwo(ifstream &inputFile)
{
}

// vector<string> getToken(string filename)
// {
//     vector<string> tokens;
//     ifstream inputFile(filename);
//     if (!inputFile.is_open())
//     {
//         std::cerr << "Error: Unable to open file " << filename << std::endl;
//         return tokens;
//     }
//     std::string file_content((std::istreambuf_iterator<char>(inputFile)),
//                              (std::istreambuf_iterator<char>()));
//     inputFile.close();
//     regex delimiterPattern("[ \t\n]+");

//     sregex_token_iterator iter(file_content.begin(), file_content.end(), delimiterPattern, -1);
//     sregex_token_iterator end;

//     while (iter != end)
//     {
//         tokens.push_back(*iter);
//         ++iter;
//     }
//     return tokens;
// }

string getToken(ifstream &inputFile)
{
    std::string token;

    // Skip leading whitespace characters
    while (inputFile >> std::ws)
    {
        char c = inputFile.get();
        if (c == ' ' || c == '\t' || c == '\n')
        {
            continue; // Skip delimiter characters
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
        if (inputFile.eof() || c == ' ' || c == '\t' || c == '\n')
        {
            break; // Return the token if a delimiter or end of file is reached
        }
        else
        {
            token += c; // Append the character to the token
        }
    }

    return token;
}

void printModule(const Module &module)
{
    std::cout << "Base Address: " << module.baseAddress << std::endl;

    std::cout << "Definitions:" << std::endl;
    for (const Symbol &symbol : module.definitions)
    {
        std::cout << "Name: " << symbol.name << ", Address: " << symbol.address << std::endl;
    }

    std::cout << "Use List:" << std::endl;
    for (const std::string &use : module.useList)
    {
        std::cout << use << std::endl;
    }

    std::cout << "Instructions:" << std::endl;
    for (const Instruction &instruction : module.instructions)
    {
        std::cout << "Address Mode: " << instruction.addrMode << ", Operand: " << instruction.instruction << std::endl;
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
    // vector<string> tokens = getToken(argv[1]);

    // for (int i = 0; i < tokens.size(); i++)
    // {
    //     cout << tokens[i] << "\n";
    // }
    // cout << tokens.size();

    // passOne(inputFile);

    // for (Module i : moduleBaseTable)
    // {
    //     printModule(i);
    // }
    // cout << moduleBaseTable.size();
    return 0;
}