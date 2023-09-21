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
    return 0;
}

string checkSym(string token)
{
    return token;
}

string readIAER(string token)
{
    return token;
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

        int defcount = readInt();
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