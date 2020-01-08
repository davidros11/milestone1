#include <iostream>
#include <fstream>
#include "Parser.h"
int main(int argc, char *argv[]) {
    // if there's no file, print an error and exit
    if(argc < 2) {
        cout << "No file" << endl;
        return 0;
    }
    ifstream codeFile(argv[1]);
    // if the file isn't found, print an error and exit
    if(!codeFile) {
        cout << "File not found" << endl;
        return 0;
    }
    // put entire file int string
    string code((std::istreambuf_iterator<char>(codeFile)),
                    std::istreambuf_iterator<char>());
    codeFile.close();
    // call the lexer
    vector<string> separators = {"->", "<-", "==", "!=", "<=", "=>", "(", ")", "\n", "{", "}",
                           " ", "<", ">", "\"", "=", ",", "\t" };
    vector<string> omit = {" ", "\t"};
    auto lex = lexer(code, separators, omit);
    // parse the code
    auto parser = new Parser();
    parser->parse(lex);
    delete parser;
    return 0;
}