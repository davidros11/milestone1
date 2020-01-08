#include "Interpreter.h"
#include <stack>
#include <queue>
#include <map>
#define BAD_EXP "ilegal math expression"
/**
 * Checks if the inputed string is + or -
 * @param token - the string
 * @return - true if it is + or - and false otherwise
 */
bool isPlusMinus(const string& token) {
    return token == "+" || token == "-";
}
/**
 * Checks if the inputed string is * or /
 * @param token - the string
 * @return - true if it is * or / and false otherwise
 */
bool isMulDiv(const string& token) {
    return token == "*" || token == "/";
}
/**
 * Checks if the inputed string is an operator
 * @param token - the string
 * @return - true if it is an operator, false otherwsie
 */
bool isOp(const string& token) {
    return isPlusMinus(token) | isMulDiv(token);
}
/**
 * Checks if the character is a number.
 * @param c - the character
 * @return true if it is a number, false otherwise
 */
bool isNum(char c) {
    return c >= 48 && c <= 57;
}
/**
* Checks if the character is a letter.
* @param c - the character
* @return true if it is a letter, false otherwise
*/
bool isChar(char c) {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}
/**
 * Checks if the string is a unary operator.
 * @param token - the string
 * @return yes if it is a unary operator, false otherwise
 */
bool isUnary(const string& token) {
    return token == "++" || token == "--";
}
/**
 * Finds the length of a sequence if letters and or number in a string.
 * @param str - the string
 * @param pos - the position of the first character in the sequence
 * @param var - is set to true if there is at least one letter in the sequence, indicating it is a variable name,
 * and is set to false if there are only digits.
 * @return - the length of the sequence
 * @exception - if the sequence is a number and more than one point is found, or it is has letters in it and any
 * points at all, or if the first character is a symbol, an exception is thrown.
 */
int getVarNum(const string &str, int pos, bool& var) {
    int len = str.length();
    int res = 0;
    bool isPoint = false;
    // we assume it is a number initially
    var = false;
    while(pos < len) {
        if(isNum(str[pos])) {
            ++res;
        } // if a character is found, it must be a variable name
        else if(isChar(str[pos]) || str[pos] == '_') {
            if(isPoint) { // if it contained any points, an exception is thrown
                throw 1;
            }
            var = true;
            ++res;
        } // if a point is found
        else if (str[pos] == '.') {
            if(isPoint || var) { // and we already found a point, or we found a letter
                throw 1; // an exception is thrown
            }
            isPoint = true;
            ++res;
        }
        else {
            break;
        }
        ++pos;
    }
    if(res == 0) {
        throw 1;
    }
    return res;
}

Interpreter::Interpreter(map<string, SimVar*> *vars) {
    varMap = vars;
}
queue<string>* Interpreter::shuntingYard(const string& equation) {
    stack<string> opStack = stack<string>();
    auto *output = new queue<string>();
    int paren = 0;
    int len = equation.length();
    int i = 0;
    while(i < len) {
        string temp;
        temp += equation[i];
        string temp2;
        if(isPlusMinus(temp)) { // if + or - is found
            if(i == 0) { // if it the first character, it's a unary operator. push ++ or -- into the operator stack
                opStack.push(temp + temp);
                i++;
                continue;
            }
            else if (isOp(temp2 += equation[i-1])) {
                throw BAD_EXP; // if there are two operators in a row, throw an exception.
            }
            else if(equation[i-1] == '(') {
                /* if there is a ( right before the operator it's a unary operator. push ++ or -- into the stack
                appropriately */
                opStack.push(temp + temp);
                i++;
                continue;
            }
            while(!opStack.empty()) {
                // if there is a * or / in the operator stack, push it into the output queue
                string fromStack = opStack.top();
                if(isMulDiv(fromStack) || isUnary(fromStack)) {
                    output->push(opStack.top());
                    opStack.pop();
                }
                else {
                    break;
                }
            }
            opStack.push(temp);
        }
        else if(isMulDiv(temp)) {
            /* if * or / is the first character, or if it's right after a (, throw an exception, because
            there's no such unary operator. */
            if(i == 0) {
                delete output;
                return nullptr;
            }
            else if(equation[i-1] == '(' || isOp(temp2 += equation[i-1])) {
                delete output;
                return nullptr;
            }
            opStack.push(temp);
        }
        else if(temp == "(") {
            // adds to the total amount of open brackets
            if(i != 0) {
                if(equation[i-1] == ')') {
                    delete output;
                    return nullptr;
                }
            }
            ++paren;
            opStack.push(temp);
        }
        else if(temp == ")") {
            if(paren < 1) {
                delete output;
                return nullptr; // you can't have a ) right at the
            }
            if(equation[i-1] == '(' || isOp(string(equation[i-1], 1))) {
                delete output;
                return nullptr; // () or an operator right before ) isn't correct syntax. throw exception
            }
            while(opStack.top() != "(") {
                // add all operators between the parenthesis to the ooutput queue
                output->push(opStack.top());
                opStack.pop();
                if(opStack.empty()) {
                    delete output;
                    return nullptr;
                }
            }
            opStack.pop();
            --paren;
        }
        else {
            // checks if the there is a number or variable name
            bool var = false;
            int tokLen;
            try {
                tokLen  = getVarNum(equation, i, var);
            } catch (int a) {
                delete output;
                return nullptr;
            }
            temp = equation.substr(i,tokLen);
            if(var) {
                if(varMap->find(temp) != varMap->end()){
                    output->push(temp); // if the variable name is in the map, add it the output queue
                }
                else {
                    delete output;
                    return nullptr; // otherwise, it's undefined. throw exception
                }
            }
            else {
                output->push(temp); // if it's a number, push it into the output queue
            }
            i += tokLen;
            continue;
        }
        i++;
    }
    // if there are any open brackets, throw exception
    if(paren != 0) {
        delete output;
        return nullptr;
    }
    // push remaining operators into the output queue
    while(!opStack.empty()) {
        output->push(opStack.top());
        opStack.pop();
    }
    // return output queue
    return output;
}
double Interpreter::interpret(const string& equation) {
    // call shunting yard
    queue<string>* rpn = shuntingYard(equation);
    if(rpn == nullptr) {
        return 0;
    }
    // result stack
    stack<double> resStack = stack<double>();
    // as long as the postfix queue isn't empty
    while(!rpn->empty()) {
        string temp = rpn->front();
        rpn->pop();
        /* if it's unary operator, take an operand from the result queue and make an expression with it,
         * and then and add it to the result queue. */
        if(isUnary(temp)) {
            double operand = resStack.top();
            resStack.pop();
            if (temp == "++") {
                resStack.push(operand);
            }
            if (temp == "--") {
                resStack.push(-operand);
            }
        }
        else if(isOp(temp)) {
            double operand1 = resStack.top();
            resStack.pop();
            double operand2 = resStack.top();
            resStack.pop();
            switch(temp[0]) {
                case '-':
                    resStack.push(operand2 - operand1);
                    break;
                case '+':
                    resStack.push(operand2 + operand1);
                    break;
                case '*':
                    resStack.push(operand2 * operand1);
                    break;
                case '/':
                    resStack.push(operand2 / operand1);
                    break;
                default:
                    break;
            }
        }
        else if(varMap->find(temp) != varMap->end()) {
            // if it's in the map, add it's value to the stack
            resStack.push((*varMap)[temp]->getVal());
        }
        else {
            // otherwise, it must be a number. Add it to the result stack
            resStack.push(stod(temp));
        }
    }
    delete rpn;
    // take what's left in the result queue.
    return resStack.top();
}