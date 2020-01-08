#ifndef UNTITLED_UTILS_H
#define UNTITLED_UTILS_H
using namespace std;
#include <string>
#include <queue>
#include <mutex>
#include <map>
#include <condition_variable>
#include <atomic>
//#include <algorithm>
/**
 * Converts the code into tokens.
 * @param str - the code
 * @param seps - vector of separator strings
 * @param omit - sub-vector of seps containing what shouldn't be included in the string
 * @return - a vector of tokens
 */
vector<string> lexer(const string& str, const vector<string>& seps, const vector<string>& omit);
// wrapper object for the table that will contain the input from the simulator
class InputTable {
private:
    map<string, double> input;
    mutex lock;
    atomic<bool> run;
    // contains the simulator variable names in the same order they appear in the xml file
    vector<string> simVec;
public:
    /**
     * Constructor; initializes fields.
     */
    InputTable();
    /**
     * updates the variable values in the map.
     * @param vals - the sub-vector of values
     */
    void update(const vector<string>& vals);
    /**
     * sets one variable value;
     * @param key - the variable name
     * @param val - the value
     */
    void set(const string& key, double val);
    /**
     * Gets a variable value.
     * @param key - the key
     * @return - the value
     */
    double get(const string& key);
    /**
     * Checks if should stop updating.
     * @return - true if should stop updating, false otherwise
     */
    bool shouldStop();
    /**
     * notify to stop updating.
     */
    void stop();
};
// wrapper object for the queue for the output to the simulator
class OutputQueue {
private:
    queue<string> output;
    mutex lock;
    mutex cwMutex;
    condition_variable cv;
    atomic_bool run;
public:
    /**
     * Constructor; initializes fields.
     */
    OutputQueue();
    /**
     * Pushes output to queue. Also notifies to stop waiting in condition variable
     * @param str - the output
     */
    void push(const string& str);
    /**
     * Checks if queue is empty.
     * @return - true if queue is empty, false otherwise
     */
    bool isEmpty();
    /**
     * waits if the queue is empty
     */
    void lockIfEmpty();
    /**
     * removes string from queue and returns it
     * @return - string at end of queue
     */
    string pop();
    /**
     * notifies that output thread should stop waiting for more things to send
     */
    void stop();
    /**
     * Checks if should keep looking waiting for strings to send, barring what's already in the queue
     * @return
     */
    bool shouldStop();
};
// object used to store variables from the code
class SimVar {
public:
    /**
     * Sets varaible value
     * @param val - the new values
     */
    virtual void setVal(double val) = 0;
    /**
     * Gets variable value.
     * @return - the value
     */
    virtual double getVal() = 0;
    /**
     * Default destructor
     */
    virtual ~SimVar() = default;
};
// variable that notifies the simulator when it'changed
class ToVar : public SimVar {
private:
    double value;
    string sim;
    OutputQueue *output;
public:
    /**
     * Constructor for Tovar. Gets simulator variable path and outputqueue. value is initialized to 0.
     * @param s - simulator variable path
     * @param q - output queue
     */
    ToVar(const string& s, OutputQueue *q);
    /**
     * Sets variable value and notifies simulator.
     * @param val - the new value
     */
    void setVal(double val);
    /**
     * Gets variable value.
     * @return - the value
     */
    double getVal() { return value; }
};
// variable that gets its value from the simulator
class FromVar : public SimVar {
private:
    InputTable *input;
    string sim;
public:
    /**
     * Constructor.
     * @param s - simulator variable path
     * @param m - input table
     */
    FromVar(const string& s, InputTable *m);
    /**
     * Sets the value of the variable's entry in the input table.
     * @param val - the value
     */
    void setVal(double val) { input->set(sim, val); }
    /**
     * Gets the variables value from the relevant entry in the input table
     * @return - the value
     */
    double getVal() { return input->get(sim); }
};
// variable that isn't connected to the simulator
class NeuVar : public  SimVar {
private:
    double value;
public:
    /**
     * Default consturctor. initializes value to 0.
     */
    NeuVar() { value = 0; }
    /**
     * Constructor.
     * @param val - the value
     */
    NeuVar(double val) { value = val; }
    /**
     * Sets variable value.
     * @param val - the new value
     */
    void setVal(double val) { value = val; }
    /**
     * Get variable value.
     * @return - the new values
     */
    double getVal() { return value; }
};
#endif //UNTITLED_UTILS_H
