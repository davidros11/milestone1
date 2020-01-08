#include "Command.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <chrono>
#include "Parser.h"
#include <arpa/inet.h>
typedef map<string, pair<string, vector<string>>> funcMap;
typedef pair<string, pair<string, vector<string>>> func;
/**
 * This function receives information abcto the simulator and sends it to a shared data structure,
 * and is performed by a thread.
 * @param port - port number to listen with
 * @param input - shared map based data structure
 * @param blocker - condition variable to block main thread
 * @param flag - atomic boolean to signify that main thread stopped waiting
 */
void inputFunc(int port, InputTable *input, condition_variable *blocker, atomic<bool> *flag) {
    // making sockaddr
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    // preparing socket
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener == -1) {
        // socket failed
        return;
    }
    if(bind(listener, (sockaddr *)&address, sizeof(address)) == -1) {
        //bind failed
        close(listener);
        return;
    }
    listen(listener, 1);
    int conn = accept(listener, (sockaddr *)&address, (socklen_t *)&address);
    // to avoid a race condition, the condition variable is notified until the main thread stops waiting
    while(flag->load()) {
        blocker->notify_one();
    }
    delete flag;
    delete blocker;
    char buffer[1024] = {0};
    while(true) {
        // checking if thread should stop
        if(input->shouldStop()) {
            close(conn);
            close(listener);
            return;
        }
        // reading data
        int bytesRead = read(conn, buffer, 1024);
        if(bytesRead < 1) {
            continue;
        }
        // sending data to shared data structure
        string rawValues(buffer);
        rawValues = rawValues.substr(0, rawValues.find('\n')+1);
        auto values = lexer(rawValues, {","}, {","});
        input->update(values);

    }
}
/**
 * This function sends information to the simulator from a shared data sturcture
 * @param ip - the simulator server ip address
 * @param port - the simulator server port
 * @param output - the shared data queue-based structure
 * @param blocker - condition variable to block main thread
 * @param flag - atomic boolean to signify that main thread stopped waiting
 */
void outputFunc(const string& ip, int port, OutputQueue *output, condition_variable *blocker, atomic<bool> *flag ) {
    // preparing socket
    int sender = socket(AF_INET, SOCK_STREAM, 0);
    if(sender == -1) {
        return;
    }
    struct sockaddr_in address;
    const char *addr = ip.c_str();
    address.sin_addr.s_addr = inet_addr(addr);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    int connection = -1;
    while(connection == -1) {
        connection = connect(sender, (sockaddr *)&address, sizeof(address));
    }
    while(flag->load()) {
        blocker->notify_one();
    }
    delete flag;
    delete blocker;
    while(true) {
        // waits if the queue is empty
        output->lockIfEmpty();
        /* checks if thread should stop. if main thread wants to end program and queue is empty,
         * the thread will stop*/
        if(output->shouldStop()) {
            close(sender);
            return;
        }
        // sends data to simulator
        const string set = output->pop();
        const char *message = set.c_str();
        send(sender, message, set.length(), 0);
    }
}
/**
 * merges and returns some tokens from a vector.
 * @param pos - beginning position in vector
 * @param code - the vector
 * @param fin - vector of tokens; the function will return if it find one of these in code
 * @return - the resulting string
 */
string mergeTokens(int pos, const vector<string>& code, const vector<string>& fin) {
    string token;
    string exp;
    int len = code.size();
    while(pos < len) {
        token = code.at(pos);
        for(const string &a : fin) {
            if (token == a) {
                return exp;
            }
        }
        exp += token;
        ++pos;
    }
    return exp;
}
/**
 * returns the position after one of the strings in fin is found
 * @param pos - beginning position
 * @param code - the vector
 * @param fin - the vector containing the tokens where the function should stop
 * @return - the position after one of the tokens from fin is found
 */
int moveTill(int pos, const vector<string>& code, const vector<string>& fin) {
    string token;
    int len = code.size();
    while(pos < len) {
        token = code.at(pos);
        for(const string &a : fin) {
            if (token == a) {
                return pos + 1;
            }
        }
        ++pos;
    }
    return pos;
}
// helper class containing a boolean expression
class BoolExp {
private:
    string op;
    string exp1;
    string exp2;
    Interpreter *inter;
public:
    /**
     * Constructor for BoolExp, creates boolean expression from code vector.
     * @param pos - beginning position
     * @param code - the vector
     * @param i - interpreter for parsing expressions
     */
    BoolExp(int pos, const vector<string>& code, Interpreter *i) {
        inter = i;
        ++pos;
        vector<string> bools = { "==", "!=", ">", "<", "<=", ">=" };
        vector<string> fin = {"{"};
        exp1 = mergeTokens(pos, code, bools);
        pos = moveTill(pos, code, bools);
        op = code.at(pos - 1);
        exp2 = mergeTokens(pos, code, fin);
    }
    /**
     * returns result of boolean expression
     * @return
     */
    bool evaluate() {
        double operand1 = inter->interpret(exp1);
        double operand2 = inter->interpret(exp2);
        if(op == "==") {
            return operand1 == operand2;
        }
        if(op == ">=") {
            return operand1 >= operand2;
        }
        if(op == "<=") {
            return operand1 <= operand2;
        }
        if(op == ">") {
            return operand1 > operand2;
        }
        if(op == "<") {
            return operand1 < operand2;
        }
        return operand1 != operand2;
    }
};
/**
 * returns end of current scope
 * @param pos - beginning position (should be after '{')
 * @param code - code vector
 * @return - position of the closing bracket of the current scope
 */
int getScopeEnd(int pos, const vector<string> &code) {
    int openBrackets = 0;
    while(openBrackets != 0 || code.at(pos) != "}") {
        if(code.at(pos) == "{") {
            ++openBrackets;
        }
        else if(code.at(pos) == "}") {
            --openBrackets;
        }
        ++pos;
    }
    return pos;
}
OpenServerCommand::OpenServerCommand(InputTable *in, Interpreter *i, thread *inTh) {
    input = in;
    inter = i;
    inThread = inTh;
}
int OpenServerCommand::execute(int pos, const vector<string>& code) {
    ++pos;
    // gets port number
    int port = (int)inter->interpret(mergeTokens(pos, code, {"\n"}));
    // makes conditional variable
    auto *blocker = new condition_variable();
    mutex blockLock;
    unique_lock<mutex> ul(blockLock);
    auto flag = new atomic<bool>(true);
    // runs input thread
    *inThread = thread(inputFunc, port, input, blocker, flag);
    // waits until connection is established with simulator client
    blocker->wait(ul);
    flag->store(false);
    return moveTill(pos, code, {"\n"});
}
ConnectClientCommand::ConnectClientCommand(OutputQueue *out, Interpreter *i, thread *outTh) {
    output = out;
    inter = i;
    outThread = outTh;
}
int ConnectClientCommand::execute(int pos, const vector<string>& code) {
    // gets server ip
    pos+= 3;
    string ip = code.at(pos);
    pos += 3;
    // gets server port
    int port = (int)inter->interpret("(" + mergeTokens(pos, code, {"\n"}));
    auto *blocker = new condition_variable();
    mutex blockLock;
    unique_lock<mutex> ul(blockLock);
    auto flag = new atomic<bool>(true);
    // runs output thread
    *outThread = thread(outputFunc, ip, port, output, blocker, flag);
    // waits until connection is established with simulator server
    blocker->wait(ul);
    flag->store(false);
    return moveTill(pos, code, {"\n"});
}
DefineVarCommand::DefineVarCommand(OutputQueue *out, InputTable *in, map<string, SimVar*> *vars, Interpreter *i) {
    output = out;
    input = in;
    varTable = vars;
    inter = i;
}
int DefineVarCommand::execute(int pos, const vector<string>& code) {
    ++pos;
    SimVar *newVar;
    string name = code.at(pos);
    ++pos;
    string token = code.at(pos);
    if(token == "=") {
        // if initialized with =, it's a NeuVar. it isn't affected by or affecting the simulator directly
        ++pos;
        double result = inter->interpret(mergeTokens(pos, code, {"\n"}));
        newVar = new NeuVar(result);
    }
    else if(token == "->") {
        // if initialized with ->, it's a ToVar. It notifies the simulator whenever it is changed
        pos += 4;
        newVar = new ToVar(code.at(pos), output);
    }
    else if(token == "<-") {
        // if initialized with <- it's a FromVar. it gets its value from the simulator input
        pos += 4;
        newVar = new FromVar(code.at(pos), input);
    }
    else {
        // the variable is automatically initialized a NeuVar with value 0
        newVar = new NeuVar();
    }
    // inserted to map
    varTable->insert(pair<string, SimVar*>(name, newVar));
    return moveTill(pos, code, {"\n"});
}
SetVarCommand::SetVarCommand(map<string, SimVar*> *vars, Interpreter *i) {
    varTable = vars;
    inter = i;
}
int SetVarCommand::execute(int pos, const vector<string>& code) {
    string name = code.at(pos);
    ++pos;
    if(code.at(pos) == "=") {
        ++pos;
        double value = inter->interpret(mergeTokens(pos, code, {"\n"}));
        varTable->at(name)->setVal(value);
    }
    return moveTill(pos, code, {"\n"});
}
PrintCommand::PrintCommand(Interpreter *i) {
    inter = i;
}
int PrintCommand::execute(int pos, const vector<string>& code) {
    pos += 2;
    // if it's string, it prints the token between the quotes
    if(code.at(pos) == "\"") {
        ++pos;
        cout << code.at(pos) << endl;
    } else {
        // otherwise it's an expression. Parse it and print the result.
        --pos;
        cout << inter->interpret(mergeTokens(pos, code, {"\n"})) << endl;
    }
    return moveTill(pos, code, {"\n"});
}
SleepCommand::SleepCommand(Interpreter *i) {
    inter = i;
}
int SleepCommand::execute(int pos, const vector<string>& code) {
    ++pos;
    // sleep for the number of milliseconds in the parenthesis
    this_thread::sleep_for(chrono::milliseconds(
            (int)inter->interpret(mergeTokens(pos, code, {"\n"}))));
    return moveTill(pos, code, {"\n"});
}
WhileCommand::WhileCommand(Interpreter *i, Parser *p) {
    inter = i;
    parser = p;
}
int WhileCommand::execute(int pos, const vector<string>& code) {
    // creates boolean expression
    BoolExp condition = BoolExp(pos, code, inter);
    pos = moveTill(pos, code, {"{"});
    ++pos;
    int loopEnd = getScopeEnd(pos, code);
    // creates subvector for loop scope
    auto b = code.begin() + pos;
    auto e = code.begin() + loopEnd - 1;
    auto scope = vector<string>(b, e);
    // parsing scope until repeatedly until condition becomes false
    while(condition.evaluate()) {
        parser->parse(scope);
    }
    return loopEnd + 1;
}
IfCommand::IfCommand(Interpreter *i) {
    inter = i;
}
int IfCommand::execute(int pos, const vector<string>& code) {
    // creates boolean expression
    BoolExp condition = BoolExp(pos, code, inter);
    pos = moveTill(pos, code, {"{"});
    ++pos;
    int scopeEnd = getScopeEnd(pos, code);
    // if condition is true, return position in scope
    if(condition.evaluate()) {
        return pos;
    }
    // otherwise, return position after scope
    return scopeEnd + 2;
}

DefineFuncCommand::DefineFuncCommand(funcMap *f) {
    funcTable = f;
}
int DefineFuncCommand::execute(int pos, const vector<string>& code) {
    // saves function name and parameter name
    string funcName = code.at(pos);
    pos += 3;
    string param = code.at(pos);
    pos = moveTill(pos, code, {"{"}) + 1;
    // creates scope sub-vector for function
    int funcEnd = getScopeEnd(pos, code);
    auto b = code.begin() + pos;
    auto e = code.begin() + funcEnd - 1;
    auto scope = vector<string>(b, e);
    // inserting function into map
    auto p = pair<string, vector<string>>(param, scope);
    funcTable->insert(func(funcName, p));
    return funcEnd + 1;
}
CallFuncCommand::CallFuncCommand(Parser *p, funcMap *f, Interpreter *i, map<string, SimVar*> *st) {
    parse = p;
    funcTable = f;
    inter = i;
    simTable = st;
}
int CallFuncCommand::execute(int pos, const vector<string>& code) {
    // saving function name
    string funcName = code.at(pos);
    ++pos;
    // getting parameter name and value
    double paramVal = inter->interpret(mergeTokens(pos, code, {"\n"}));
    string param = funcTable->at(funcName).first;
    // getting sub-vector
    auto scope = funcTable->at(funcName).second;
    // adding parameter to variables
    auto newVar = new NeuVar(paramVal);
    simTable->insert(pair<string, SimVar*>(param, newVar));
    // parsing function scope
    parse->parse(scope);
    // removing parameter
    simTable->erase(param);
    delete newVar;
    return moveTill(pos, code, {"\n"});
}