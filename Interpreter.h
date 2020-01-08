#ifndef UNTITLED_INTERPRETER_H
#define UNTITLED_INTERPRETER_H
using namespace std;

#include <string>
#include <map>
#include <queue>
#include "Utils.h"

class Interpreter {
private:
    map<string, SimVar*>* varMap;
    /**
     * Implementation of shunting yard algorithm.
     * @param equation - the equation
     * @return - a queue containing the equation in postfix notation
     */
    queue<string>* shuntingYard(const string& equation);
public:
    /**
     * Constructor.
     */
    Interpreter(map<string, SimVar*>* vars);
    /**
     * Sets the variables in the map according to the inputed string.
     * @param str - the string containing the variable names and values
     * @return - the result of the equation. 0 if the syntax is bad
     */
    double interpret(const string& equation);
};
#endif //UNTITLED_INTERPRETER_H
