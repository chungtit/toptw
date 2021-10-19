#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <random>

using namespace std;

//int number_vehicle = 15;

struct Parameters {
    int number_customers;
    int number_locations;
    int time_horizon;
    int vehicle_capacity;
    int number_vehicle;
};

struct TimeWindow {
    int open;
    int close;
};

struct Coordinate {
    float x;
    float y;
};

struct Request {
    string customer_id;
    string location_id;
    Coordinate coordinate;
    int demand;
    bool inRoute;
    TimeWindow timeWindow;
};

struct Rate {
    int time;
    int cost;
    float distancne;
};

struct Technician {
    vector<Request> route;
    vector<int> waitTime;
    vector<int> shift;
    vector<int> maxShift;
    vector<int> arrivingTime;
    vector<int> startTime;
    int usedSize;
};

struct BestPair {
    float highestRatio;
    int shift;
    int routeId;
    int position;
    int indexOfRequest;
};

struct RequestAndScore {
    string _id;
    int score;
};

//Matrix save on cost, distance, time
map < string, map<string, Rate> > travelTimeMatrix;

//Simple function
int maxOf(int, int);
int minOf(int, int);
//End simple function

/*
 * Compare score to sort request
 */
bool compareScoreToSort(RequestAndScore, RequestAndScore);

/*
 * Calcualte distance between two requests
 */
float calculateDistance(Coordinate visit1, Coordinate visit2);

/*
 * Insert a request to technician and update it's parameters
 */
void insertToRoute(Request &, int pos, Technician &, int shift, vector < Request > &);

/*
 * Calculate shift, arrivingTime, waittingTime, maxShift in insertionStep
 */
int calculateShift(Request &, Technician &, int) ;
int calculateArrivingTime(Request, Technician, int);
int calculateWaitTime(Request, Technician, int);
int calculateMaxShift(Request, Technician, int);
int calculateStartTime(Request, Technician, int);
float calculateRatio(int, float);

/*
 * Update parameter while insert a request to route
 */
void updateAfter(Technician &, int);
void updateBefore(Technician &, int);

/*
 * Calculate objective: cost of all vehicle in problem
 */
int objective(vector < Technician >);

/*
 * Check time window for a route
 * @return true if route has saptified
 */
bool checkTimeWindow(Technician);

/*
 * Read data from input file(.txt)
 */
void readData(string, Parameters &, vector < Request > &);

/*
 * Initial Technician: insert depot to zero
 */
void initTechnician(vector < Technician > &, vector < Request > &, Parameters);

/*
 * Check all request in route
 * @return true if all request in Route
 */
bool checkAllRequest(vector < Request > requests);

/*
 * Display route for problem
 */
void displayRoute(vector < Technician >);

/*
 *  Algorithm insert request to Route
 */
void firstInsertion(vector < Technician > &, vector < Request > &, Parameters);
void scoreInsertion(vector < Technician > &, vector < Request > &, Parameters);
void greedyBasicInsertion(vector < Technician > &, vector < Request > &, Parameters);
void regretInsertion(vector < Technician > & solution, vector < Request > & requests, Parameters parameters){

}

/*
 * Insertion Step in ILS
 */
void insertionStep(vector < Technician > &, vector < Request > &, Parameters);

/*
 * Update parameter after remove list request
 */
void updateAfterErase(Technician &, Parameters);

/*
 * Remove location_is from vehicle with a customer
 */
void fixRequestInRoute(string, vector < Request > &);

/*
 * Algorithm removal
 */
void randomRemoval(vector < Technician > &, int, vector < Request > &, Parameters);
void worstRemoval(vector < Technician > &, int, float, vector < Request > &, Parameters);
void shawRemoval(vector < Technician > &, int, float, vector < Request > &, Parameters);
void time_orientedRemoval(vector < Technician > &, int, float, vector < Request > &, Parameters);

/*
 * Calculate shaw score for shaw removal
 */
int calculateShawScore(Request, Request, vector < Technician > &);

/*
 * No comment
 */
void sortListLocation(int r, vector < int >, vector < Technician> &, vector < Request >);

/*
 * Shaking step in ILS
 */
void shakingStep(vector < Technician > &, vector < Request > &, Parameters);

/*
 * Heristic of ILS
 */
void heuristic(vector < Technician > &, vector < Request > &, Parameters);

/*
 * Creat inittial solution
 */
void initSolution(vector < Technician > &, vector < Request > &, Parameters);



int main() {
    srand(time(NULL));

    int number_intance = 5;

    const clock_t begin_time = clock();
    cout << "VRPRD start..." << endl;

    string path;
    Parameters parameters;

    for(int i = 0; i < number_intance; i++){

        cout << "+)Intance " << i << " start..." << endl;

        const clock_t begin_time_intance = clock();
        path = "instance/instance_" + to_string(i) + "-triangle.txt";
        vector < Request > requests(0);
        vector < Technician > solution(0);
        readData(path, parameters, requests);
        initSolution(solution, requests, parameters);
        cout << "\tCreate initial solution success." << endl;
        cout << "\tImprove solution..." << endl;
        heuristic(solution, requests, parameters);
        displayRoute(solution);

        cout << "\t=> Intance " << i << " success in " << float(clock() - begin_time_intance) << endl;
        break;
    }

    cout << "Spend total time: " << float( clock() - begin_time) / CLOCKS_PER_SEC << endl;
    cout << "VRPRD success!!!" << endl;
    return 0;
}

/*
 * Area Define function
 */

int maxOf(int num1, int num2){
    if(num1 > num2){
        return num1;
    }else{
        return num2;
    }
}

int minOf(int num1, int num2){
    if(num1 < num2){
        return num1;
    }else{
        return num2;
    }
}

float calculateDistance(Coordinate visit1, Coordinate visit2) {
    return float(sqrt(pow(visit1.x - visit2.x, 2.0) + pow(visit1.y - visit2.y, 2.0)));
}

int calculateShift(Request & _request, Technician & _tech, int pos) {
    return travelTimeMatrix[_tech.route[pos-1].location_id][_request.location_id].time
           + calculateWaitTime(_request, _tech, pos)
           + travelTimeMatrix[_request.location_id][_tech.route[pos].location_id].time
           - travelTimeMatrix[_tech.route[pos-1].location_id][_tech.route[pos].location_id].time;
}

int calculateArrivingTime(Request _request, Technician _tech, int pos){
    return _tech.startTime[pos-1] + travelTimeMatrix[_tech.route[pos-1].location_id][_request.location_id].time;
}

int calculateWaitTime(Request _request, Technician _tech, int pos){
    return maxOf(0, _request.timeWindow.open - calculateArrivingTime(_request, _tech, pos));
}

int calculateMaxShift(Request _request, Technician _tech, int pos){
    return minOf(_request.timeWindow.close - _tech.startTime[pos], _tech.waitTime[pos+1] + _tech.maxShift[pos+1]);
}

int calculateStartTime(Request _request, Technician _tech, int pos){
    return maxOf(_request.timeWindow.open, _tech.startTime[pos-1] + travelTimeMatrix[_tech.route[pos-1].location_id][_request.location_id].time);
}

float calculateRatio(int shift, float cost){
    //Formulate for ratio ?
    //return 100.0 / (shift * shift + cost);
    //return 100.0 / shift;
    return 100.0 / (shift * cost); //best
    //return 100.0 / (shift + cost);
    //return 100.0 / ((shift + cost)*cost);
    //return 100.0 / (shift * shift * cost);
    //return 100.0 / (shift * pow(cost, 4.0));
}

void insertToRoute(Request & _request, int pos, Technician & _tech, int shift, vector < Request > & requests){
    _request.inRoute = true;

    //Update all position of customer in route
    for(int i = 0; i < requests.size(); i++){
        if(requests[i].customer_id == _request.customer_id){
            requests[i].inRoute = true;
        }
    }


    _tech.route.insert(_tech.route.begin() + pos, _request);
    _tech.shift.insert(_tech.shift.begin() + pos, shift);
    _tech.waitTime.insert(_tech.waitTime.begin() + pos, calculateWaitTime(_request, _tech, pos));
    _tech.arrivingTime.insert(_tech.arrivingTime.begin() + pos, calculateArrivingTime(_request, _tech, pos));
    _tech.startTime.insert(_tech.startTime.begin() + pos, maxOf(_request.timeWindow.open, _tech.arrivingTime[pos]));
    _tech.maxShift.insert(_tech.maxShift.begin() + pos, 0);

    _tech.usedSize += _request.demand;
}

void readData(string path, Parameters & parameters, vector < Request > & requests) {
    //open file to read
    ifstream file;
    file.open(path, ios::in);

    char line[256];

    /*
     * Read genaral paramaters
     */
    file.getline(line, 256);
    //cout << line << endl;
    file >> parameters.number_customers >> parameters.number_locations >> parameters.time_horizon >> parameters.vehicle_capacity;
    //cout << parameters.number_customers << " " << parameters.number_locations << " " << parameters.time_horizon << " " << parameters.vehicle_capacity << endl;
    parameters.number_vehicle = parameters.number_customers;

    //read tab, end line, title
    for (int i = 0; i < 4; i++) {
        file.getline(line, 256);
        //cout << line << endl;
    }

    /*
     * Read request: all parameter
     */
    //Read customer schedule
    Request one_location;

    string temp;

    //Depot
    file >> one_location.customer_id >> one_location.demand >> one_location.location_id >> temp;
    one_location.inRoute = false;
    one_location.timeWindow.open = stoi(temp.substr(1, temp.find(",") - 1));
    one_location.timeWindow.close = stoi(temp.substr(temp.find(",") + 1, temp.length() - temp.find(",")));
    //cout << one_location.customer_id << " " << one_location.demand << " " << one_location.location_id << " [" << one_location.timeWindow.open << ", " << one_location.timeWindow.close << "]" << endl;
    requests.push_back(one_location);

    //Escape end line
    file.getline(line, 256);
    //cout << line;

    //Location 1->number of location
    string line_s;
    string customer_id;
    int demand;


    for (int i = 0; i <= parameters.number_customers; i++) {
        file.getline(line, 256);
        line_s = string(line);
        //cout << endl << line << endl;
        customer_id = line_s.substr(0, line_s.find_first_of(" "));
        line_s.erase(0, line_s.find_first_of(" "));
        while (line_s[0] == ' ') {
            line_s.erase(0, 1);
        }

        demand = stoi(line_s.substr(0, line_s.find_first_of("\t")));
        line_s.erase(0, line_s.find_first_of("\t"));
        while (line_s[0] == '\t') {
            line_s.erase(0, 1);
        }

        while (line_s.length() >= 5) {
            one_location.customer_id = customer_id;
            one_location.demand = demand;
            one_location.location_id = line_s.substr(0, line_s.find_first_of(" "));
            line_s.erase(0, line_s.find_first_of(" "));
            while (line_s[0] == ' ') {
                line_s.erase(0, 1);
            }

            temp = line_s.substr(0, line_s.find_first_of("\t"));
            line_s.erase(0, line_s.find_first_of("\t"));
            while (line_s[0] == '\t') {
                line_s.erase(0, 1);
            }

            one_location.inRoute = false;
            one_location.timeWindow.open = stoi(temp.substr(1, temp.find_first_of(",") - 1));
            one_location.timeWindow.close = stoi(temp.substr(temp.find(",") + 1, temp.length() - temp.find(",")));
            //cout << one_location.customer_id << " " << one_location.demand << " " << one_location.location_id << " [" << one_location.timeWindow.open << ", " << one_location.timeWindow.close << "]" << endl;
            requests.push_back(one_location);
        }
    }

    //Read end line, title
    for (int i = 0; i < 4; i++) {
        file.getline(line, 256);
    }

    //Read location coordinate
    int location_id;
    for (int i = 0; i < parameters.number_locations; i++) {
        file >> location_id >> requests[i].coordinate.x >> requests[i].coordinate.y;
        //cout << location_id << " " << request[i].coordinate.x << " " << request[i].coordinate.y << endl;
    }

    //Read end line, title
    for (int i = 0; i < 5; i++) {
        file.getline(line, 256);
        //cout << line << endl;
    }

    //Read Travel time matrix to global variable: travelTimeMatrix
    string src;
    string dst;
    while (!file.eof()) {
        file.getline(line, 256);
        //cout << "Line:str " <<  line << endl;
        line_s = string(line);
        if (line_s.length() == 0) {
            break;
        }

        src = line_s.substr(1, line_s.find(',') - 1);
        dst = line_s.substr(line_s.find(',') + 2, line_s.find(')') - line_s.find(',') - 2);
        line_s.erase(0, line_s.find(')') + 2);

        travelTimeMatrix[src][dst].time = stoi(line_s.substr(0, line_s.find(" ")));
        travelTimeMatrix[src][dst].cost = stoi(line_s.substr(line_s.find(" "), line_s.length() - line_s.find(" ")));

        //cout << "(" << src << ", " << dst << ") ";
        //cout << travelTimeMatrix[src][dst].time << " " << travelTimeMatrix[src][dst].cost << endl;
    }

    //Calcualte distance
    for(int i = 0; i < requests.size(); i++){
        for(int j = 0; j < requests.size(); j++){
            travelTimeMatrix[requests[i].location_id][requests[j].location_id].distancne = calculateDistance(requests[i].coordinate, requests[j].coordinate);
        }
    }
}

void updateAfter(Technician & _tech, int pos){
    int waitOlder;

    for(int i = pos + 1; i < _tech.route.size(); i++){
        waitOlder = _tech.waitTime[i];
        _tech.waitTime[i] = maxOf(0, waitOlder - _tech.shift[i-1]);
        _tech.arrivingTime[i] = _tech.arrivingTime[i] + _tech.shift[i-1];
        _tech.shift[i] = maxOf(0, _tech.shift[i-1] - waitOlder);
        if(_tech.shift[i] == 0){
            break;
        }
        _tech.startTime[i] = _tech.startTime[i] + _tech.shift[i];
        _tech.maxShift[i] = _tech.maxShift[i] - _tech.shift[i];
    }
}

void updateBefore(Technician & _tech, int pos){
    for(int i = pos; i > 0; i--){
        _tech.maxShift[i] = calculateMaxShift(_tech.route[i], _tech, i);
    }
}

void scoreInsertion(vector<Technician> & solution, vector < Request > & requests, Parameters parameters){
    BestPair bestPair;
    int shift;
    float ratio;
    bool hasChange = true;

    while (!checkAllRequest(requests)) {
        if (!hasChange) {
            cout << "\t- Need more vehicle\n";
            break;
        }
        bestPair.highestRatio = -1;
        bestPair.position = 0;
        bestPair.routeId = 0;
        bestPair.indexOfRequest = 0;
        bestPair.shift = 0;
        //Loop for all vehicle
        for (int route = 0; route < solution.size(); route++) {
            //Loop for all position in route
            for (int pos = 1; pos < solution[route].route.size(); pos++) {
                //Loop for all request
                for (int i = 0; i < requests.size(); i++) {
                    if (!requests[i].inRoute &&
                            requests[i].demand + solution[route].usedSize <= parameters.vehicle_capacity &&
                            requests[i].timeWindow.close > solution[route].startTime[pos - 1] +
                                                      travelTimeMatrix[solution[route].route[pos -
                                                                                             1].location_id][requests[i].location_id].time) {
                        shift = calculateShift(requests[i], solution[route], pos);
                        if (shift <= solution[route].waitTime[pos] + solution[route].maxShift[pos]) {
                            ratio = calculateRatio(shift, travelTimeMatrix[solution[route].route[pos -
                                                                                                 1].location_id][requests[i].location_id].cost +
                                                          travelTimeMatrix[requests[i].location_id][solution[route].route[pos].location_id].cost);
                            if (ratio > bestPair.highestRatio) {
                                bestPair.highestRatio = ratio;
                                bestPair.indexOfRequest = i;
                                bestPair.routeId = route;
                                bestPair.position = pos;
                                bestPair.shift = shift;
                            }
                        }

                    }
                }
            }
        }

        if (bestPair.highestRatio != -1) {
            //Insert request has best pair to solution
            insertToRoute(requests[bestPair.indexOfRequest], bestPair.position, solution[bestPair.routeId],
                          bestPair.shift, requests);

            //Update value for after insert request
            updateAfter(solution[bestPair.routeId], bestPair.position);

            //Update maxshift for before insert's request
            updateBefore(solution[bestPair.routeId], bestPair.position);
        } else {
            hasChange = false;
        }
        //displayRoute();
    }
}

void firstInsertion(vector < Technician > & solution, vector < Request > & requests, Parameters parameters){
    BestPair bestPair;
    int shift;
    float ratio;
    bool hasChange = true;

    while(!checkAllRequest(requests)){
        if(!hasChange){
            cout << "\t- Need more vehicle\n";
            break;
        }
        bestPair.highestRatio = 100000000;
        bestPair.position = 0;
        bestPair.routeId = 0;
        bestPair.indexOfRequest = 0;
        bestPair.shift = 0;
        //Loop for all vehicle
        for(int route = 0; route < solution.size(); route++){
            //Loop for all position in route
            for(int pos = 1; pos < solution[route].route.size(); pos++){
                //Loop for all request
                for(int i = 0; i < requests.size(); i++){
                    if(!requests[i].inRoute &&
                            requests[i].demand + solution[route].usedSize <= parameters.vehicle_capacity &&
                            requests[i].timeWindow.close > solution[route].startTime[pos-1] + travelTimeMatrix[solution[route].route[pos-1].location_id][requests[i].location_id].time)
                    {
                        shift = calculateShift(requests[i], solution[route], pos);
                        if(shift <= solution[route].waitTime[pos] + solution[route].maxShift[pos]){
                            ratio = calculateRatio(shift, travelTimeMatrix[solution[route].route[pos-1].location_id][requests[i].location_id].cost + travelTimeMatrix[requests[i].location_id][solution[route].route[pos].location_id].cost);
                            if(ratio < bestPair.highestRatio){
                                bestPair.highestRatio = ratio;
                                bestPair.indexOfRequest = i;
                                bestPair.routeId = route;
                                bestPair.position = pos;
                                bestPair.shift = shift;
                            }
                        }

                    }
                }
            }
        }

        if(bestPair.highestRatio != 100000000) {
            //Insert request has best pair to solution
            insertToRoute(requests[bestPair.indexOfRequest], bestPair.position, solution[bestPair.routeId], bestPair.shift, requests);

            //Update value for after insert request
            updateAfter(solution[bestPair.routeId], bestPair.position);

            //Update maxshift for before insert's request
            updateBefore(solution[bestPair.routeId], bestPair.position);
        }else{
            hasChange = false;
        }
        //displayRoute();
    }
    return;
}

void greedyBasicInsertion(vector<Technician> & solution, vector < Request > & requests, Parameters parameters){
    BestPair bestPair;
    int shift;
    float ratio;
    bool hasChange = true;

    while (!checkAllRequest(requests)) {
        if (!hasChange) {
            cout << "\t- Need more vehicle\n";
            break;
        }
        bestPair.highestRatio = -1;
        bestPair.position = 0;
        bestPair.routeId = 0;
        bestPair.indexOfRequest = 0;
        bestPair.shift = 0;
        //Loop for all vehicle
        for (int route = 0; route < solution.size(); route++) {
            //Loop for all position in route
            for (int pos = 1; pos < solution[route].route.size(); pos++) {
                //Loop for all request
                for (int i = 0; i < requests.size(); i++) {
                    if (!requests[i].inRoute &&
                            requests[i].demand + solution[route].usedSize <= parameters.vehicle_capacity &&
                            requests[i].timeWindow.close > solution[route].startTime[pos - 1] +
                                                      travelTimeMatrix[solution[route].route[pos -
                                                                                             1].location_id][requests[i].location_id].time) {
                        shift = calculateShift(requests[i], solution[route], pos);
                        if (shift <= solution[route].waitTime[pos] + solution[route].maxShift[pos]) {
                            ratio = calculateRatio(1, (travelTimeMatrix[solution[route].route[pos - 1].location_id][requests[i].location_id].cost +
                                                          travelTimeMatrix[requests[i].location_id][solution[route].route[pos].location_id].cost -
                                                            travelTimeMatrix[solution[route].route[pos-1].location_id][solution[route].route[pos].location_id].cost));
                            if (ratio > bestPair.highestRatio) {
                                bestPair.highestRatio = ratio;
                                bestPair.indexOfRequest = i;
                                bestPair.routeId = route;
                                bestPair.position = pos;
                                bestPair.shift = shift;
                            }
                        }

                    }
                }
            }
        }

        if (bestPair.highestRatio != -1) {
            //Insert request has best pair to solution
            insertToRoute(requests[bestPair.indexOfRequest], bestPair.position, solution[bestPair.routeId],
                          bestPair.shift, requests);

            //Update value for after insert request
            updateAfter(solution[bestPair.routeId], bestPair.position);

            //Update maxshift for before insert's request
            updateBefore(solution[bestPair.routeId], bestPair.position);
        } else {
            hasChange = false;
        }
        //displayRoute();
    }
}

void insertionStep(vector < Technician > & solution, vector < Request > & requests, Parameters parameters){
    bool static isInit = true;
    if(isInit) {
        firstInsertion(solution, requests, parameters);
        isInit = false;
    }else {
        int select_insertion = rand() % 2;
        switch(select_insertion){
            case 0:
                scoreInsertion(solution, requests, parameters);
                break;
            case 1:
                greedyBasicInsertion(solution, requests, parameters);
                break;
        }

        //scoreInsertion(solution);
        //greedyBasicInsertion(solution);
    }
}

void initTechnician(vector < Technician > & tech, vector < Request > & requests, Parameters parameters){
    Technician depot;
    requests[0].inRoute = true;

    depot.route.push_back(requests[0]);
    depot.arrivingTime.push_back(0);
    depot.maxShift.push_back(0);
    depot.shift.push_back(0);
    depot.waitTime.push_back(0);
    depot.startTime.push_back(0);
    depot.usedSize = 0;

    requests[requests.size() - 1].inRoute = true;
    depot.route.push_back(requests[requests.size() - 1]);
    depot.arrivingTime.push_back(0);
    depot.maxShift.push_back(parameters.time_horizon);
    depot.shift.push_back(0);
    depot.waitTime.push_back(0);
    depot.startTime.push_back(0);
    depot.usedSize = 0;

    for(int i = 0; i < parameters.number_vehicle; i++){
        tech.push_back(depot);
    }


}

void initSolution(vector < Technician > & solution, vector < Request > & requests, Parameters parameters){
    initTechnician(solution, requests, parameters);
    //cout << "Start initial Solution..." << endl;
    insertionStep(solution, requests, parameters);
    //cout << "Finish initial Solution." << endl;
}

int objective(vector < Technician > solution) {
    int score = 0;
    for(int i = 0; i < solution.size(); i++) {
        for (int j = 0; j < solution[i].route.size() - 1; j++) {
            score += travelTimeMatrix[solution[i].route[j].location_id][solution[i].route[j + 1].location_id].cost;
        }
    }
    return score;
}

bool checkAllRequest(vector < Request > requests){
    for(int i = 0; i < requests.size(); i++){
        if(!requests[i].inRoute){
            return false;
        }
    }
    return true;
}

bool checkTimeWindow(Technician _tech){
    for(int i = 0; i < _tech.route.size() - 1; i++){
        if(_tech.startTime[i] + travelTimeMatrix[_tech.route[i].location_id][_tech.route[i+1].location_id].time > _tech.route[i+1].timeWindow.close){
            //cout << "Pos: " << i << endl << endl;
            return false;
        }
    }
    return true;
}

void shakingStep(vector < Technician > & newSolution, vector < Request > & requests, Parameters parameters){
    //int number_erase =  int(parameters.number_customers * 4 / 5);
    int number_erase =  rand() % (int(parameters.number_customers * 2 / 3) - 3) + 4;

    //int number_erase = rand() % int(parameters.number_customers * 1 /2) + int(parameters.number_customers * 1/ 3);

    int selectRemoval = rand() % 3;

    switch(selectRemoval){
        case 0:
            randomRemoval(newSolution, number_erase, requests, parameters);
            break;
        case 1:
            worstRemoval(newSolution, number_erase, 2.5, requests, parameters);
            break;
        case 2:
            shawRemoval(newSolution, number_erase, number_erase, requests, parameters);
            break;
    }


//    //randomRemoval(newSolution, number_erase);
//    worstRemoval(newSolution, number_erase, 1.534);
//    shawRemoval(newSolution, number_erase, 1.54);

//    cout << "Shaking Step: " << endl;
//    displayRoute(newSolution);
}

void randomRemoval(vector < Technician > & newSolution, int number_erase, vector < Request > & requests, Parameters parameters){

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();

    vector < int > list_customer;
    for(int i = 1; i <= parameters.number_customers; i++){
        list_customer.push_back(i);
    }

    shuffle (list_customer.begin(), list_customer.end(), default_random_engine(seed));

    for(int i = 0; i < number_erase; i++){
        for(Technician& _tech: newSolution){
            if(_tech.route.size() > 2){
                for(int pos = 0; pos < _tech.route.size(); pos++){
                    if(_tech.route[pos].customer_id == to_string(list_customer[i])){
                        _tech.usedSize -= _tech.route[pos].demand;
                        fixRequestInRoute(_tech.route[pos].customer_id, requests);
                        _tech.route.erase(_tech.route.begin() + pos);
                    }
                }
                //Update parameters
                updateAfterErase(_tech, parameters);
            }

        }
    }
}

int calculateShawScore(Request request_1, Request request_2, vector < Technician > & solution){
    int s1 = 0, s2 = 0, status_2 = 0;
    for(Technician _tech: solution){
        for(int i = 1; i < _tech.route.size()-1; i++){
            if(_tech.route[i].location_id == request_1.location_id){
                s1 = _tech.startTime[i];
                status_2++;
            }
            if(_tech.route[i].location_id == request_2.location_id){
                s2 = _tech.startTime[i];
                status_2++;
            }
            if(status_2 == 2){
                break;
            }
        }
        if(status_2 == 2){
            break;
        }
    }
    return int(travelTimeMatrix[request_1.location_id][request_2.location_id].distancne) + abs(s1 - s2) + abs(request_1.demand - request_2.demand);
}

void sortListLocation(int r, vector < int > list_location, vector < Technician> & solution, vector < Request > requests){
    int temp;
    for(int i = 0; i < list_location.size() - 1; i++){
        for(int j = i + 1; j < list_location.size(); j++){
            if(calculateShawScore(requests[r], requests[list_location[i]], solution) > calculateShawScore(requests[r], requests[list_location[j]], solution)){
                temp = list_location[i];
                list_location[i] = list_location[j];
                list_location[j] = temp;
            }
        }
    }
}

void shawRemoval(vector < Technician > & solution, int number_erase, float p, vector < Request > & requests, Parameters parameters){
    vector < int > list_location;
    double  y;

    for(Technician _tech: solution){
        for(int i = 1; i < _tech.route.size() - 1; i++){
            list_location.push_back(stoi(_tech.route[i].location_id));
        }
    }
    int r_index = int(rand() % list_location.size());
    int r = list_location[r_index];
    list_location.erase(list_location.begin() + r_index, list_location.begin() + r_index + 1);

    vector < int > list_remove;

    list_remove.push_back(r);

    while(list_remove.size() < number_erase){
        r = list_remove[rand() % list_remove.size()];
        sortListLocation(r, list_location, solution, requests);

        y = ((double) rand() / (RAND_MAX));
        r_index = int(floor(pow(y, p) * list_location.size()));

        list_remove.push_back(list_location[r_index]);
        list_location.erase(list_location.begin() + r_index, list_location.begin() + r_index + 1);
    }

    float flag;
    for(int i = 0; i < list_remove.size(); i++) {
        flag = false;
        for (Technician &_tech: solution) {
            if (_tech.route.size() > 2) {
                for (int pos = 1; pos < _tech.route.size() - 1; pos++) {
                    if (to_string(list_remove[i]) == _tech.route[pos].location_id) {
                        _tech.usedSize -= _tech.route[pos].demand;
                        fixRequestInRoute(_tech.route[pos].customer_id, requests);
                        _tech.route.erase(_tech.route.begin() + pos);
                        flag = true;
                        break;
                    }
                }

                //Update paramters
                updateAfterErase(_tech, parameters);
            }
            if (flag) {
                break;
            }
        }
    }
}

bool compareScoreToSort(RequestAndScore r1, RequestAndScore r2){
    return r1.score > r2.score;
}

void worstRemoval(vector < Technician > & solution, int number_erase, float p, vector < Request > & requests, Parameters parameters){
    double y;
    int x;
    int sizeOfCustomer = parameters.number_customers;
    bool flag;

    while(number_erase > 0){
        vector < RequestAndScore > list_rs;

        for(Technician _tech: solution){
            if(_tech.route.size() > 2){
                for(int i = 1; i < _tech.route.size() - 1; i++) {
                    RequestAndScore rs;
                    rs._id = _tech.route[i].location_id;
                    rs.score = travelTimeMatrix[_tech.route[i-1].location_id][_tech.route[i].location_id].cost + travelTimeMatrix[_tech.route[i].location_id][_tech.route[i+1].location_id].cost - travelTimeMatrix[_tech.route[i-1].location_id][_tech.route[i+1].location_id].cost;
                    list_rs.push_back(rs);
                }
            }
        }

        //Sort descending
        sort(list_rs.begin(), list_rs.end(), compareScoreToSort);

        y = ((double) rand() / (RAND_MAX));
        x = int(floor(pow(y, p) * sizeOfCustomer));

//        cout << "X = " << x << endl;
//        cout << "Request: " << list_rs[x]._id << endl;

        //Remove x
        flag = false;
        for(Technician & _tech: solution){
            if(_tech.route.size() > 2){
                for(int pos = 1; pos < _tech.route.size() - 1; pos++){
                    if(list_rs[x]._id == _tech.route[pos].location_id){
                        _tech.usedSize -= _tech.route[pos].demand;
                        fixRequestInRoute(_tech.route[pos].customer_id, requests);
                        _tech.route.erase(_tech.route.begin() + pos);
                        flag = true;
                        break;
                    }
                }

                //Update paramters
                updateAfterErase(_tech, parameters);
            }
            if(flag){
                break;
            }
        }

        number_erase--;
        sizeOfCustomer--;
    }
}

void updateAfterErase(Technician & _tech, Parameters parameters){
    for(int i = 1; i < _tech.route.size(); i++){
        _tech.arrivingTime[i] = calculateArrivingTime(_tech.route[i],  _tech, i);
        _tech.startTime[i] = calculateStartTime(_tech.route[i], _tech, i);
        _tech.waitTime[i] = calculateWaitTime(_tech.route[i], _tech, i);
    }

    _tech.maxShift[_tech.route.size() - 1] = parameters.time_horizon - _tech.startTime[_tech.route.size() - 1];
    //_tech.maxShift[_tech.route.size() - 1] = parameters.time_horizon;

    for(int i = _tech.route.size() - 2; i >= 0; i--){
        _tech.maxShift[i] = calculateMaxShift(_tech.route[i], _tech, i);
    }
}

void fixRequestInRoute(string customer_id, vector < Request > & requests){
    for(int i = 0; i < requests.size(); i++){
        if(requests[i].customer_id == customer_id){
            requests[i].inRoute = false;
        }
    }
}

void heuristic(vector < Technician > & solution, vector < Request > & requests, Parameters parameters){
    vector < Technician > newSolution;
	
    newSolution = solution;

    int numberOfTimeNoImprovement = 0;

//    while(numberOfTimeNoImprovement < 4000){
//        shakingStep(newSolution, requests, parameters);
//        insertionStep(newSolution, requests, parameters);
//        //cout << "object: " << objective(newSolution) << endl;
//        if (objective(newSolution) < objective(solution)) {
//            solution = newSolution;
//            numberOfTimeNoImprovement = 0;
//        }else{
//            numberOfTimeNoImprovement++;
//        }
//    }

      for(int i = 0; i < 5000; i++){
          newSolution = solution;
          shakingStep(newSolution, requests, parameters);
          insertionStep(newSolution, requests, parameters);
          if(objective(newSolution) < objective(solution)){
              solution = newSolution;

          }{
              //newSolution.clear();
              newSolution = solution;
          }
      }

//    while(numberOfTimeNoImprovement < 7500){
//        newSolution = solution;
//
//        shakingStep(newSolution, requests, parameters);
//        insertionStep(newSolution, requests, parameters);
//
//        if(objective(newSolution) < objective(solution)){
//            solution = newSolution;
//            numberOfTimeNoImprovement = 0;
//        }else{
//            numberOfTimeNoImprovement++;
//        }
//    }
}

void displayRoute(vector < Technician > solution){
    cout << "\tRoute for problem: " << endl;
    for(int i = 0; i < solution.size(); i++) {
        if(solution[i].route.size() <= 2){
            continue;
        }
        cout << "\t\tRoute " << i + 1 << ": ";
        for (int j = 0; j < solution[i].route.size(); j++) {
            //cout << "\t" << solution[i].route[j].customer_id << "\t";
            cout << "\t\t" << solution[i].route[j].customer_id;
            cout << "(id" << solution[i].route[j].location_id << ")" << "\t";
//                 << ", s" << solution[i].startTime[j]
//                 << ", m" << solution[i].maxShift[j]
//                 << ", w" << solution[i].waitTime[j]
//                 << ")" << "\t";
        }
        //cout << " used: " << solution[i].usedSize;
        if(!checkTimeWindow(solution[i])){
            cout << "\tInval time window!";
        }
        cout << endl;
    }
    cout << "\twith cost: " << objective(solution) << endl;

}
