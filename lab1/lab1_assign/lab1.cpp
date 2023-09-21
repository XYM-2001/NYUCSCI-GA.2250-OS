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
    int operand;
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

void passOne(vector<string> tokens)
{
    int i = 0;

    while (i < tokens.size())
    {
        Module new_module;
        if (moduleBaseTable.empty())
        {
            new_module.baseAddress = 0;
        }
        else
        {
            new_module.baseAddress = moduleBaseTable.back().baseAddress;
        }

        int defcount = readInt(tokens[i]);
        i += 1;
        vector<Symbol> deflist;
        for (int j = 0; j < defcount; j++)
        {
            Symbol curr;
            curr.name = readSym(tokens[i]);
            curr.address = readInt(tokens[i + 1]);
            deflist.push_back(curr);
            i += 2;
        }
        new_module.definitions = deflist;

        int usecount = readInt(tokens[i]);
        i += 1;
        vector<string> uselist;
        for (int j = 0; j < usecount; j++)
        {
            uselist.push_back(readSym(tokens[i]));
            i += 1;
        }
        new_module.useList = uselist;

        int instcount = readInt(tokens[i]);
        i += 1;
        vector<Instruction> instructions;
        for (int j = 0; j < instcount; j++)
        {
            Instruction curr_ins;
            curr_ins.addrMode = readIAER(tokens[i]);
            curr_ins.operand = readInt(tokens[i + 1]);
            i += 2;
        }

        moduleBaseTable.push_back(new_module);
    }
}

void passTwo(ifstream &inputFile)
{
}

vector<string> getToken(string filename)
{
    vector<string> tokens;
    ifstream inputFile(filename);
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return tokens;
    }
    string line;
    string file_content;
    while (getline(inputFile, line))
    {
        file_content += line;
    }
    inputFile.close();
    regex delimiterPattern("[ \t\n]+");

    sregex_token_iterator iter(file_content.begin(), file_content.end(), delimiterPattern, -1);
    sregex_token_iterator end;

    while (iter != end)
    {
        tokens.push_back(*iter);
        ++iter;
    }
    return tokens;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    vector<string> tokens = getToken(argv[1]);
    // for (int i = 0; i < tokens.size(); i++)
    // {
    //     cout << tokens[i] << "\n";
    // }
    // cout << tokens.size();

    // passOne(tokens);
    return 0;
}