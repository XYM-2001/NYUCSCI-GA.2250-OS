#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iomanip>

using namespace std;

int main(int argc, char *argv[])
{
    ifstream inputFile(argv[1]);
    inputFile.close();
    ifstream inputFile_new(argv[1]);
    if (!inputFile_new.is_open())
    {
        cerr << "Error: Unable to open file " << argv[1] << endl;
    }
    inputFile_new.close();
    return 0;
}