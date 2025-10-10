// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A2519B6
#include "stock.hpp"
#include "P2random.h"
#include <getopt.h>
#include <sstream>
using namespace std;

void Stock::getOptions(int argc, char **argv) {
    opterr = static_cast<int>(false);   
    int choice = 0;
    int index = 0;
    option longOptions[] = {
        {"help", no_argument, nullptr, 'h'},
        {"verbose", no_argument, nullptr, 'v'},
        {"median", no_argument, nullptr, 'm'},
        {"trader_info", no_argument, nullptr, 'i'},
        {"time_travelers", no_argument, nullptr, 't'},
        {nullptr, 0, nullptr, '\0'},
    }; 

    while ((choice = getopt_long(argc, argv, "hvmit", static_cast<option *>(longOptions), &index)) != -1) {
        switch (choice) {
        case 'h':
            printHelp(*argv);
            exit(0);
        case 'v':
            verbose_output = true;
            break;
        case 'm':
            median_output = true;
            break;
        case 'i': 
            trader_detail_output = true;
            break;
        case 't': 
            time_traveler_output = true;
            break;
        default:
            cerr << "Unknown command line option\n";
            exit(1);
        } 
    }
}

void Stock::printHelp(char *command) {
    cout << "Usage: " << command << " [--verbose -v] [--median -m] [--trader_info -i] [--time_travelers] -t\n"
         << "This program works as a stock market simulation.\n"
         << "Given an order, or an intention to buy/sell shares of a certain stock,\n"
         << "This program aims to match orders based on stock ID, buy/sell intentions and price limits.\n"
         << "Note: all of the command line options above are optional. You may include one or more of them.\n";
}

void Stock::readInfo() {
    cout << "Processing orders...\n";
    string comment;
    getline(cin, comment);
    string mode;
    string type;
    cin >> mode >> type;
    string discard;
    cin >> discard >> num_traders >> discard >> num_stocks;
    stocks.resize(num_stocks); // tracks pqs, medians, time traveler so this is always resized no matter what
    if(trader_detail_output) {
        trader_data.resize(num_traders); // only tracks trader info data, so only resized when this output type has been enabled
    }
    if(type[0] == 'T') {
        processOrder(cin);
    } else {
        PR_read();
    }
}

void Stock::processOrder(istream &inputStream) {
    string type;
    int ts = 0;
    char T = '\0';
    uint32_t tr = 0;
    char S = '\0';
    uint32_t st = 0;
    char dollar_sign = '\0';
    int price = 0;
    char hash_tag = '\0';
    int quantity = 0;
    while(inputStream >> ts >> type >> T >> tr >> S >> st >> dollar_sign >> price >> hash_tag >> quantity) {
        if(ts < 0) {
            cerr << "The time stamp must be a non-negative value";
            exit(1);
        } else if (current_timestamp != -1 && ts < current_timestamp) {
            cerr << "Timestamps must be non-decreasing\n";
            exit(1);
        } else if (static_cast<int>(tr) < 0 || tr  >= num_traders) {
            cerr << "Trader ID must be within the range [0, <NUM_TRADERS>)\n";
            exit(1);
        } else if (static_cast<int>(st) < 0 || st >= num_stocks) {
            cerr << "Stock ID must be within the range [0, <NUM_STOCKS>)\n";
            exit(1);
        } else if (price <= 0) {
            cerr << "Price must be positive\n";
            exit(1);
        } else if (quantity <= 0) {
            cerr << "Quantity must be positive\n";
            exit(1);
        }
        bool isBuy = false;
        if(type[0] == 'B') {
            isBuy = true; 
        }
        // medians are only printed as timestamp changes
        // if new timestamp is discovered, print median with timestamp we are exiting, update timestamp
        if(current_timestamp != -1 && ts != current_timestamp) {
            if(median_output) {
                printMedian(current_timestamp);
            }
        }
        current_timestamp = ts;
        int id = unique_id_tracker;
        if(time_traveler_output) {
            timeTraveler(ts, st, isBuy, price);
        }
        Order order {price, quantity, tr, id};
        ++unique_id_tracker; // every order has a unique id, so this is updated after an order is created
        tryMatch(isBuy, st, order);
    }
    checkOutputType();
}

void Stock::PR_read() {
    unsigned int seed;
    unsigned int num_orders;
    unsigned int rate;
    string discard;
    cin >> discard >> seed >> discard >> num_orders >> discard >> rate;
    stringstream ss;
    P2random::PR_init(ss, seed, static_cast<unsigned int>(num_traders), static_cast<unsigned int>(num_stocks), num_orders, rate);
    processOrder(ss);
}

// determines trade based on market logic
void Stock::tryMatch(bool b, uint32_t s, Order & o) {
    if(b) { // if buy order
        auto & sells = stocks[s].sells; // referenences pqs for easier access
        auto & buys = stocks[s].buys;
        while(o.quantity > 0 && !sells.empty()) { // o is buying, compare is selling
            Order compare = sells.top();
            if(compare.price > o.price) { // sell order should be less than buy
                break;
            }
            sells.pop(); // match is made 
            int shares = min(o.quantity, compare.quantity);
            o.quantity -= shares;
            compare.quantity -= shares;
            int final_price = 0;
            if(o.unique_id < compare.unique_id) { // price is decided depending on which came firs
                final_price = o.price;
            } else {
                final_price = compare.price;
            }
            if(trader_detail_output) { 
                int net_price = shares * final_price;
                trader_data[o.trader_id].shares_bought += shares;
                trader_data[o.trader_id].net_transfer -= net_price;
                trader_data[compare.trader_id].shares_sold += shares;
                trader_data[compare.trader_id].net_transfer += net_price;
            }
            if(verbose_output) {
                cout << "Trader " << o.trader_id << " purchased " 
                << shares << " shares of Stock " << s << " from Trader " 
                << compare.trader_id << " for $" << final_price << "/share\n";
            }
            if(median_output) {
                stocks[s].medians.calculate(final_price);
            }
            ++totalTrades;
            if(compare.quantity > 0) { // makes sure to put back a sell or buy order if it still has remaining quantities after a trade
                sells.push(compare);
            }
        }
        if(o.quantity > 0) {
            buys.push(o);
        }
    }
    else {
        // same logic except in the case that we are look at a sell order and comparing it to a buy order
        auto & sells = stocks[s].sells;
        auto & buys = stocks[s].buys;
        while(o.quantity > 0 && !buys.empty()) { // o is selling, compare is buying
            Order compare = buys.top();
            if(o.price > compare.price) {
                break;
            }

            buys.pop();
            int shares = min(o.quantity, compare.quantity);
            o.quantity -= shares;
            compare.quantity -= shares;
            int final_price = 0;

            if(o.unique_id < compare.unique_id) {
                final_price = o.price;
            } else {
                final_price = compare.price;
            }
            if(trader_detail_output) {
                int net_price = shares * final_price;
                trader_data[compare.trader_id].shares_bought += shares;
                trader_data[compare.trader_id].net_transfer -= net_price;
                trader_data[o.trader_id].shares_sold += shares;
                trader_data[o.trader_id].net_transfer += net_price;
            }
            if(verbose_output) {
                cout << "Trader " << compare.trader_id << " purchased " 
                << shares << " shares of Stock " << s << " from Trader " 
                << o.trader_id << " for $" << final_price << "/share\n";
            }
            if(median_output) {
                stocks[s].medians.calculate(final_price);
            }
            ++totalTrades;
            if(compare.quantity > 0) {
                buys.push(compare);
            }
        }
        if(o.quantity > 0) {
            sells.push(o);
        }
    }
}

void Stock::printMedian(int t) {
    for(size_t i = 0; i < stocks.size(); ++i) {
        if(stocks[i].medians.lower_half.empty() && stocks[i].medians.upper_half.empty()) {
            continue;
        }
        cout << "Median match price of Stock " << i << " at time " << t << " is $" << stocks[i].medians.median << "\n";
    }
}

// initialize status is no trades 
void Stock::timeTraveler(int ts, uint32_t st, bool buy, int p) { 
    stockItem & s = stocks[st];
    switch(s.status) {
    case TimeStatus::noTrades: // status can only change from noTrades to canBuy if there is a sell order
        if(!buy) {
            s.possSellPrice = p;
            s.possSellTime = ts;
            s.status = TimeStatus::canBuy;
        }
        break;
    case TimeStatus::canBuy:
        if(!buy) { // you only have sells, so just keep the cheaper price as the better sell
            if(p < s.possSellPrice) {
                s.possSellPrice = p;
                s.possSellTime = ts;
            }
        } else { // at this point, you have a sell and a buy, so record profit created by this
            int profit = p - s.possSellPrice;
            if(profit > 0) {
                s.bestProfit = profit;
                s.bestSellPrice = s.possSellPrice;
                s.bestSellTime = s.possSellTime;
                s.bestBuyTime = ts;
                s.status = TimeStatus::completed;
            } 
        }
        break;
    case TimeStatus::completed: 
        if(!buy) { // if you are in completed state but you get a smaller sell price, this could be a potential better trade
            if(p < s.possSellPrice) { 
                s.possSellPrice = p;
                s.possSellTime = ts;
                s.status = TimeStatus::potential;
            }  
        } else { // if you get another buy, there is another profit, compare profits to see which one should be saved as best
            int profit = p - s.possSellPrice;
            if(profit > s.bestProfit) {
                s.bestProfit = profit;
                s.bestSellPrice = s.possSellPrice;
                s.bestSellTime = s.possSellTime;
                s.bestBuyTime = ts;
            } else if (profit == s.bestProfit) { // tiebreak case
                if(s.possSellTime < s.bestSellTime || (s.possSellTime == s.bestSellTime && ts < s.bestBuyTime)) { // earlier sell, and if not, then earlier buy
                    s.bestSellPrice = s.possSellPrice;
                    s.bestSellTime = s.possSellTime;
                    s.bestBuyTime = ts;
                }
            }
        }
        break;
    case TimeStatus::potential:
        if(!buy) {
            if(p < s.possSellPrice) { // while in potential, if another smaller price comes, this is now the better price, keep track of this but stay in potential
                s.possSellPrice = p;
                s.possSellTime = ts;
            }
        } else { 
            int profit = p - s.possSellPrice; // look at another buy order in potential, this gives you a new profit that can be compared with current best to see which one is better
            if(profit > s.bestProfit) {
                s.bestProfit = profit;
                s.bestSellPrice = s.possSellPrice;
                s.bestSellTime = s.possSellTime;
                s.bestBuyTime = ts;
                s.status = TimeStatus::completed;
            } else if (profit == s.bestProfit) { // same tiebreak logic as above
                if(s.possSellTime < s.bestSellTime || (s.possSellTime == s.bestSellTime && ts < s.bestBuyTime)) {
                    s.bestSellPrice = s.possSellPrice;
                    s.bestSellTime = s.possSellTime;
                    s.bestBuyTime = ts;
                }
            }
        }
        break;
    } 
}

void Stock::checkOutputType() { 
    if(median_output) {
        printMedian(current_timestamp);
    }
    cout << "---End of Day---\n";
    cout << "Trades Completed: " << totalTrades << "\n";
    if(trader_detail_output) {
        cout << "---Trader Info---\n";
        for(uint32_t i = 0; i < num_traders; ++i) {
            cout << "Trader " << i << " bought " << trader_data[i].shares_bought << " and sold " 
            << trader_data[i].shares_sold << " for a net transfer of $" 
            << trader_data[i].net_transfer << "\n";
        }
    }
    if(time_traveler_output) {
        cout << "---Time Travelers---\n";
        for(size_t i = 0; i < stocks.size(); ++i) {
            int bestBuy = stocks[i].bestProfit + stocks[i].bestSellPrice;
            if(stocks[i].bestProfit > 0) {
                cout << "A time traveler would buy Stock " << i << " at time "
                    << stocks[i].bestSellTime << " for $" << stocks[i].bestSellPrice
                    << " and sell it at time " << stocks[i].bestBuyTime
                    << " for $" << bestBuy << "\n";
            } else {
                cout << "A time traveler could not make a profit on Stock " << i << "\n";
            }
        }
    }
}

