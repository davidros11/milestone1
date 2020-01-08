#include "Parser.h"
#include "Command.h"
#include <iostream>
Parser::Parser() {
    output = new OutputQueue();
    input = new InputTable();
    simTable = new map<string, SimVar*>();
    funcTable = new funcMap();
    interpreter = new Interpreter(simTable);
    // initializes threads
    inThread = thread();
    outThread = thread();
    // initializes the commands
    comTable.insert(pair<string, Command*>(
            "openDataServer", new OpenServerCommand(input, interpreter, &inThread)));
    comTable.insert(pair<string, Command*>(
            "connectControlClient", new ConnectClientCommand(output, interpreter, &outThread)));
    comTable.insert(pair<string, Command*>(
            "var", new DefineVarCommand(output, input, simTable, interpreter)));
    comTable.insert(pair<string, Command*>(
            "setVar", new SetVarCommand(simTable, interpreter)));
    comTable.insert(pair<string, Command*>(
            "while", new WhileCommand(interpreter, this)));
    comTable.insert(pair<string, Command*>(
            "if", new IfCommand(interpreter)));
    comTable.insert(pair<string, Command*>(
            "Print", new PrintCommand(interpreter)));
    comTable.insert(pair<string, Command*>(
            "Sleep", new SleepCommand(interpreter)));
    comTable.insert(pair<string, Command*>(
            "defFunc", new DefineFuncCommand(funcTable)));
    comTable.insert(pair<string, Command*>(
            "callFunc", new CallFuncCommand(this, funcTable, interpreter, simTable)));
}
void Parser::parse(const vector<string>& code) {
    int pos = 0;
    int len = code.size();
    while(pos < len) {
        string token = code.at(pos);
        // checks if token is a key for a command
        if(comTable.find(token) != comTable.end()) {
            pos = comTable[token]->execute(pos, code);
        } // checks if it's a variable name
        else if(simTable->find(token) != simTable->end()) {
            pos = comTable["setVar"]->execute(pos, code);
        } // check if it's a function name
        else if(funcTable->find(token) != funcTable->end()) {
            pos = comTable["callFunc"]->execute(pos, code);
        } // if it's a closing bracket, move to the next line
        else if(token == "}") {
            pos += 2;
        } // if it's a newline, ignore it
        else if(token == "\n") {
            ++pos;
        }
        else {
            // if it's doesn't match anything, it must be a function definition
            pos = comTable["defFunc"]->execute(pos, code);
        }
    }
}

void Parser::init() {
    // deleting variables
    for(auto it = simTable->begin(); it != simTable->begin(); ++it) {
        delete it->second;
    }
    simTable->clear();
    // telling threads to stop
    output->stop();
    input->stop();
    // waiting for threads to end
    if(inThread.joinable()) {
        inThread.join();
    }
    if(outThread.joinable()) {
        outThread.join();
    }
}

Parser::~Parser() {
    init();
    delete output;
    delete input;
    delete simTable;
    delete funcTable;
    delete interpreter;
    for(pair<string, Command*> a : comTable) {
        delete a.second;
    }
}