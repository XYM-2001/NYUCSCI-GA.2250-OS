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

void passOne(vector<string> tokens)
{
    // string signal = "None";
    // for (int i = 0; i < tokens.size(); ++i)
    // {
    //     if (signal == "None")
    //     {
    //     }
    // }
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
    while (getline(inputFile, line))
    {
        if (!line.empty())
        {
            regex delimiter("[ \t\n]+");
            sregex_token_iterator it(line.begin(), line.end(), delimiter, -1);
            sregex_token_iterator end;
            while (it != end)
            {
                tokens.push_back(it->str());
                ++it;
            }
        }
    }
    inputFile.close();
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
    for (string i : tokens)
    {
        cout << i << "\n";
    }
    cout << tokens.size();

    // passOne(tokens);
    return 0;
}