#ifndef UNTITLED_COMMAND_H
#define UNTITLED_COMMAND_H
using namespace std;
#include "Utils.h"
#include <unistd.h>
#include <string>
#include <map>
#include "Interpreter.h"
#include <thread>
typedef map<string, pair<string, vector<string>>> funcMap;
class Parser;
class Command {
public:
    /**
     * Executes given command
     * @param pos - beginning position of the command in the vector
     * @param code - code vector
     * @return - position of new command
     */
    virtual int execute(int pos, const vector<string>& code) = 0;
    /**
     * Destructor.
     */
    virtual ~Command() = default;
};
class OpenServerCommand : public Command {
private:
    InputTable *input;
    Interpreter *inter;
    thread *inThread;
public:
    /**
     * Constructor.
     * @param in -an InpuTable to give to the thread
     * @param i - interpreter for parsing port parameter
     * @param inTh - pointer to server thread
     */
    OpenServerCommand(InputTable *in, Interpreter *i, thread *inTh);
    /**
     * Executes openDataServer command
     * @param pos - beginning position of the command in the vector
     * @param code - code vector
     * @return - position of new command
     */
    int execute(int pos, const vector<string>& code);
};
class ConnectClientCommand : public Command {
private:
    OutputQueue *output;
    Interpreter *inter;
    thread *outThread;
public:
    /**
     * Constructor for ConnectClientCommand.
     * @param out - OutputQueue to give to thread
     * @param i
     * @param outTh - pointer to client thread
     */
    ConnectClientCommand(OutputQueue *out, Interpreter *i, thread *outTh);
    /**
     * Executes connectControlClient command
     * @param pos - beginning position of the command in the vector
     * @param code - code vector
     * @return - position of new command
     */
    int execute(int pos, const vector<string>& code);
};
class DefineVarCommand : public Command {
private:
    map<string, SimVar*> *varTable;
    InputTable *input;
    OutputQueue *output;
    Interpreter *inter;
public:
    /**
     * Constructor for DefineVarCommand.
     * @param out - OutputQueue to give ToVar variables
     * @param in - InputTable to give FromVar variables
     * @param vars - variable table to update
     * @param inter - interpreter to parse assignment expressions
     */
    DefineVarCommand(OutputQueue *out, InputTable *in, map<string, SimVar*> *vars, Interpreter *inter);
    /**
     * Executes var command
     * @param pos - beginning position of the command in the vector
     * @param code - code vector
     * @return - position of new command
     */
    int execute(int pos, const vector<string>& code);
};
class SetVarCommand : public Command {
private:
    map<string, SimVar*> *varTable;
    Interpreter *inter;
public:
    /**
     * Constructor for SetVarCommand
     * @param vars - variable table to update
     * @param inter - interpreter to parse assignment expression
     */
    SetVarCommand(map<string, SimVar*> *vars, Interpreter *inter);
    /**
     * Executes the variable assignment command
     * @param pos - beginning position of the command in the vector
     * @param code - code vector
     * @return - position of new command
     */
    int execute(int pos, const vector<string>& code);
};
class PrintCommand : public Command {
private:
   Interpreter *inter;
public:
    /**
     * Consturctor for PrintCommand.
     * @param inter - interpreter for parsing expression in parenthesis
     */
    PrintCommand(Interpreter *inter);
    /**
    * Executes the Print command
    * @param pos - beginning position of the command in the vector
    * @param code - code vector
    * @return - position of new command
    */
    int execute(int pos, const vector<string>& code);
};
class SleepCommand : public Command {
private:
    Interpreter *inter;
public:
    /**
     * Constructor for SleepCommand.
     * @param inter - interpreter for parsing expresison in parenthesis
     */
    SleepCommand(Interpreter *inter);
    /**
    * Executes the sleep command
    * @param pos - beginning position of the command in the vector
    * @param code - code vector
    * @return - position of new command
    */
    int execute(int pos, const vector<string>& code);
};
class WhileCommand : public Command {
private:
    Interpreter *inter;
    Parser *parser;
public:
   /**
    * Constructor for WhileCommand
    * @param i - interpreter for parsing expressions in condition
    * @param p - parser for parsing code in loop
    */
   WhileCommand(Interpreter *i, Parser *p);
   /**
    * Executes while loop
    * @param pos - beginning position of the command in the vector
    * @param code - code vector
    * @return - position of new command
    */
   int execute(int pos, const vector<string>& code);
};
class IfCommand : public Command {
private:
    Interpreter *inter;

public:
    /**
     * Constructor for if command
     * @param inter - interpreter for parsing expressions in condition
     */
    IfCommand(Interpreter *inter);
    /**
    * Executes if statement
    * @param pos - beginning position of the command in the vector
    * @param code - code vector
    * @return - position of new command
    */
    int execute(int pos, const vector<string>& code);
};
class DefineFuncCommand : public Command {
private:
    funcMap *funcTable;
public:
    /**
     * Constructor for DefineFuncCommand.
     * @param f - function table for updating
     */
    DefineFuncCommand(funcMap *f);
    /**
    * Executes function definition command
    * @param pos - beginning position of the command in the vector
    * @param code - code vector
    * @return - position of new command
    */
    int execute(int pos, const vector<string>& code);
};
class CallFuncCommand : public Command {
private:
    funcMap *funcTable;
    Parser *parse;
    Interpreter *inter;
    map<string, SimVar*> *simTable;
public:
    /**
     * Constructor for FunctionCallCommand.
     * @param p - parser for parsing function code
     * @param f - function table for finding function
     * @param i - interpreter for parsing parameter
     * @param st - variable map for adding and removing parameter
     */
    CallFuncCommand(Parser *p, funcMap *f, Interpreter *i, map<string, SimVar*> *st);
    /**
    * Executes function call
    * @param pos - beginning position of the command in the vector
    * @param code - code vector
    * @return - position of new command
    */
    int execute(int pos, const vector<string>& code);
};
#endif //UNTITLED_COMMAND_H
