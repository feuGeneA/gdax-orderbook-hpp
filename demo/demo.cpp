#include <chrono>
#include <iostream>
#include <thread>

#include "gdax-orderbook.hpp"

int main(int argc, char* argv[]) {
    GDAXOrderBook book("ETH-USD");

    std::cout << "current best bid: Ξ" << book.bids.begin()->second << " @ $"
                               << book.bids.begin()->first << "/Ξ ; ";
    std::cout << "current best offer: Ξ" << book.offers.begin()->second << " @ $"
                                 << book.offers.begin()->first << "/Ξ"
                                 << std::endl;

    size_t secondsToSleep = 5;
    std::cout << "waiting " << secondsToSleep << " seconds for the market to "
        "shift" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(secondsToSleep));

    std::cout << "current best bid: Ξ" << book.bids.begin()->second << " @ $"
                               << book.bids.begin()->first << "/Ξ ; ";
    std::cout << "current best offer: Ξ" << book.offers.begin()->second << " @ $"
                                 << book.offers.begin()->first << "/Ξ"
                                 << std::endl;
}
