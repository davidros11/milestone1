#ifndef UNTITLED_PARSER_H
#define UNTITLED_PARSER_H
#include "Utils.h"
#include "Interpreter.h"
#include <vector>
#include <thread>
#include "Command.h"
#include <unordered_map>
typedef map<string, pair<string, vector<string>>> funcMap;
class Parser {
private:
    // server thread
    thread inThread;
    // client thread
    thread outThread;
    // variable map
    map<string, SimVar*> *simTable;
    // queue that contains data to be sent to the simulator
    OutputQueue *output;
    // data that was sent from the simulator
    InputTable *input;
    // function map
    funcMap *funcTable;
    // command map
    unordered_map <string, Command*> comTable;
    // for parsing math expressions
    Interpreter *interpreter;
public:
    /**
     * Constructor.
     */
    Parser();
    /**
     * Removes all variables and functions, and closes all threads.
     */
    void init();
    /**
     * Parse code contained in a vector of strings
     * @param code - the vector
     */
    void parse(const vector<string>& code);
    /**
     * Destructor. Frees all memory.
     */
    ~Parser();
};
#endif //UNTITLED_PARSER_H
