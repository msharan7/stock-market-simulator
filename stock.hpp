// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A2519B6
#ifndef STOCK_HPP
#define STOCK_HPP
#include <queue>
#include <vector>
#include <iostream>
#include <functional>
#include <algorithm>
using namespace std;

struct Order {
    int price = 0;
    int quantity = 0;
    uint32_t trader_id = 0;
    int unique_id = 0;
};

// this is only really used for trader info output
struct Trader {
    int shares_bought = 0;
    int shares_sold = 0;
    int net_transfer = 0;
};

// custom comparater for buy pq
struct buyComp {
    bool operator()(const Order & x, const Order & y) const {
        if(x.price != y.price) {
            return x.price < y.price; // y is larger and therefore more priority
        } else {
            return x.unique_id > y.unique_id; // tie case. Here, if comp(x,y) is true than y (earlier) has more priority
        }
    }
};

// customer comparator for sell pq
struct sellComp {
    bool operator()(const Order & x, const Order & y) const {
        if(x.price != y.price) {
            return x.price > y.price; // y (smaller) has more priority if true
        } else {
            return x.unique_id > y.unique_id; // same logic as buycomp (see above)
        }
    }
};

struct RunningMedian {
    int median = 0;
    priority_queue<int> lower_half;
    priority_queue<int, vector<int>, greater<int>> upper_half;
    void calculate(int x) {
        if(lower_half.empty() && upper_half.empty()) { // just for one value
            lower_half.push(x);
            median = x;
            return;
        }
        if(lower_half.size() > upper_half.size()) { // median is the extra value in lower
            if(x < median) {
                upper_half.push(lower_half.top());
                lower_half.pop();
                lower_half.push(x); // pqs sizes are equal now
            } else {
                upper_half.push(x);
            }
            median = (lower_half.top() + upper_half.top())/2; // even calculation
        } 
        else if (lower_half.size() == upper_half.size()) { // curretly even, adding one will make it odd
            if(x < median) {
                lower_half.push(x);
                median = lower_half.top();
            } else {
                upper_half.push(x);
                median = upper_half.top();
            }
        }
        else {
            if(x > median) { // currently odd, adding one will make it even
                lower_half.push(upper_half.top()); // since upper half is larger, current median is in upper half
                upper_half.pop();
                upper_half.push(x);
            } else {
                lower_half.push(x);
            }
            median = (lower_half.top() + upper_half.top())/2;
        }
    }
};

enum class TimeStatus : char {
    noTrades,
    canBuy,
    completed,
    potential,
};

struct stockItem {
    priority_queue<Order, vector<Order>, buyComp> buys; // max heap
    priority_queue<Order, vector<Order>, sellComp> sells; // min heap
    RunningMedian medians;
    int possSellTime = 0; // this records lowest prices to sell a stock for (to maximize time traveler profit)
    int possSellPrice = 0;

    // all of the values below are used to help determine the best profit that can be made
    int bestSellPrice = 0; 
    int bestSellTime = 0;
    int bestBuyTime = 0;
    int bestProfit = 0;

    TimeStatus status = TimeStatus::noTrades;
};

class Stock {
public: 
    void getOptions(int argc, char **argv);
    void readInfo();
    void checkOutputType();

private:
    void printHelp(char *command);
    void PR_read();
    void processOrder(istream &inputStream);
    void tryMatch(bool b, uint32_t s, Order & o);
    void printMedian(int t);
    void timeTraveler(int ts, uint32_t st, bool buy, int p);
    vector<Trader> trader_data;
    vector<stockItem> stocks;
    bool verbose_output = false; 
    bool median_output = false;
    bool trader_detail_output = false;
    bool time_traveler_output = false;
    uint32_t num_traders = 0;
    uint32_t num_stocks = 0;
    int unique_id_tracker = 0;
    int totalTrades = 0;
    int current_timestamp = -1;
};

#endif