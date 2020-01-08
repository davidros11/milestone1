#include "Utils.h"
/**
 * Returns a vector containing the variable paths in the order they appear in the xml file
 * @return - the vector.
 */
vector<string> getVec() {
    vector<string> vec;
    vec.resize(36);
    vec[0] = "/instrumentation/airspeed-indicator/indicated-speed-kt";
    vec[1] = "/sim/time/warp";
    vec[2] = "/controls/switches/magnetos";
    vec[3] = "/instrumentation/heading-indicator/offset-deg";
    vec[4] = "/instrumentation/altimeter/indicated-altitude-ft";
    vec[5] = "/instrumentation/altimeter/pressure-alt-ft";
    vec[6] = "/instrumentation/attitude-indicator/indicated-pitch-deg";
    vec[7] = "/instrumentation/attitude-indicator/indicated-roll-deg";
    vec[8] = "/instrumentation/attitude-indicator/internal-pitch-deg";
    vec[9] = "/instrumentation/attitude-indicator/internal-roll-deg";
    vec[10] = "/instrumentation/encoder/indicated-altitude-ft";
    vec[11] = "/instrumentation/encoder/pressure-alt-ft";
    vec[12] = "/instrumentation/gps/indicated-altitude-ft";
    vec[13] = "/instrumentation/gps/indicated-ground-speed-k";
    vec[14] = "/instrumentation/gps/indicated-vertical-speed";
    vec[15] = "/instrumentation/heading-indicator/indicated-heading-deg";
    vec[16] = "/instrumentation/magnetic-compass/indicated-heading-deg";
    vec[17] = "/instrumentation/slip-skid-ball/indicated-slip-skid";
    vec[18] = "/instrumentation/turn-indicator/indicated-turn-rate";
    vec[19] = "/instrumentation/vertical-speed-indicator/indicated-speed-fpm";
    vec[20] = "/controls/flight/aileron";
    vec[21] = "/controls/flight/elevator";
    vec[22] = "/controls/flight/rudder";
    vec[23] = "/controls/flight/flaps";
    vec[24] = "/controls/engines/engine/throttle";
    vec[25] = "/controls/engines/current-engine/throttle";
    vec[26] = "/controls/switches/master-avionics";
    vec[27] = "/controls/switches/starter";
    vec[28] = "/engines/active-engine/auto-start";
    vec[29] = "/controls/flight/speedbrake";
    vec[30] = "/sim/model/c172p/brake-parking";
    vec[31] = "/controls/engines/engine/primer";
    vec[32] = "/controls/engines/current-engine/mixture";
    vec[33] = "/controls/switches/master-bat";
    vec[34] = "/controls/switches/master-alt";
    vec[35] = "/engines/engine/rpm";
    return vec;
}
vector<string> lexer(const string& str, const vector<string>& seps, const vector<string>& omit) {
    auto lex = vector<string>();
    int len = str.length();
    int last = 0;
    int cur = 0;
    bool found = false;
    bool isStr = false;
    bool shouldKeep = true;
    while (cur < len) {
        // don't seperate what's in quotation marks if quotation marks are expected
        if(!isStr || str[cur] == '\"') {
            for(const string& s : seps) {
                if(s == str.substr(cur, s.length())) {
                    string token = str.substr(last, cur - last);
                    if(!token.empty()) {
                        lex.push_back(token);
                    }
                    for(const string& o : omit) {
                        if(s == o) {
                            shouldKeep = false;
                        }
                    }
                    if(shouldKeep) {
                        lex.push_back(s);

                    } else {
                        shouldKeep = true;
                    }
                    cur += s.length();
                    last = cur;
                    found = true;
                    if(s == "\"") {
                        isStr = !isStr;
                    }
                    break;
                }
            }
        }
        if(found) {
            found = false;

        } else {
            cur++;
        }
    }
    string final = str.substr(last, cur - last);
    if(!final.empty()) {
        lex.push_back(final);
    }
    return lex;
}
InputTable::InputTable() {
    run = ATOMIC_VAR_INIT(true);
    simVec = getVec();
}
void InputTable::update(const vector<string> &vals) {
    int len = min(vals.size(), simVec.size());
    // updates all the entries
    lock.lock();
    for(int i = 0; i < len; i++) {
        input[simVec[i]] = stod(vals[i]);
    }
    lock.unlock();
}
void InputTable::set(const string& key, double val) {
    lock.lock();
    input[key] = val;
    lock.unlock();
}
double InputTable::get(const string& key) {
    double res;
    lock.lock();
    res = input[key];
    lock.unlock();
    return res;
}
void InputTable::stop() {
    run.store(false);
}
bool InputTable::shouldStop() {
    return !run.load();
}
OutputQueue::OutputQueue() {
    run = ATOMIC_VAR_INIT(true);
}
void OutputQueue::push(const string& str) {
    lock.lock();
    output.push(str);
    lock.unlock();
    cv.notify_all();
}
bool OutputQueue::isEmpty() {
    bool res;
    lock.lock();
    res = output.empty();
    lock.unlock();
    return res;
}
void OutputQueue::lockIfEmpty() {
    if(isEmpty() && !shouldStop()) {
        unique_lock<mutex> cwLock(cwMutex);
        cv.wait(cwLock);
    }
}
string OutputQueue::pop() {
    string res;
    lock.lock();
    res = output.front();
    output.pop();
    lock.unlock();
    return res;
}
void OutputQueue::stop() {
    run.store(false);
    // in case output thread is waiting
    cv.notify_all();
}
bool OutputQueue::shouldStop() {
    return !run.load() && isEmpty();
}
ToVar::ToVar(const string& s, OutputQueue *q) {
    value = 0;
    sim = s;
    output = q;
}
void ToVar::setVal(double val) {
    value = val;
    // pushes new value to output queue
    output->push("set " + sim + " " + to_string(value) + "\r\n");
}
FromVar::FromVar(const string& s, InputTable *m) {
    sim = s;
    input = m;
}