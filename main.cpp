// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A2519B6
#include <iostream>
#include "stock.hpp"
#include <getopt.h>
using namespace std; 

int main (int argc, char **argv) {
    ios_base::sync_with_stdio(false);
    Stock stock;
    stock.getOptions(argc, argv);
    stock.readInfo();
    return 0;
}
