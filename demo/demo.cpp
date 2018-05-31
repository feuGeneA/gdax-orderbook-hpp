#include <chrono>
#include <iostream>
#include <thread>

#include "gdax-orderbook.hpp"

int main(int argc, char* argv[]) {
    GDAXOrderBook book("ETH-USD");

    auto bestBid = book.bids.begin();
    while (true)
    {
        auto nextBid = bestBid;
        ++nextBid;
        if (nextBid == book.bids.end()) break;
        ++bestBid;
    }
    std::cout << "best bid: Ξ" << bestBid->second << " @ $"
                               << bestBid->first << "/Ξ ; ";
    std::cout << "best offer: Ξ" << book.asks.begin()->second << " @ $"
                                 << book.asks.begin()->first << "/Ξ"
                                 << std::endl;

    size_t secondsToSleep = 5;
    std::cout << "sleeping for " << secondsToSleep << " seconds" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(secondsToSleep));

    bestBid = book.bids.begin();
    while (true)
    {
        auto nextBid = bestBid;
        ++nextBid;
        if (nextBid == book.bids.end()) break;
        ++bestBid;
    }
    std::cout << "best bid: Ξ" << bestBid->second << " @ $"
                               << bestBid->first << "/Ξ ; ";
    std::cout << "best offer: Ξ" << book.asks.begin()->second << " @ $"
                                 << book.asks.begin()->first << "/Ξ"
                                 << std::endl;
}
