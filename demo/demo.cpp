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

    size_t numThreads = 5,
           longIterationRuntime = 75;
    secondsToSleep = 90;
    std::cout << "running stress test for " << secondsToSleep << " seconds, "
        "with " << numThreads << " threads constantly iterating over the "
        "whole order book, and notifying of any iterations that take greater "
        "than " << longIterationRuntime << " milliseconds..." << std::endl;
    std::vector<std::thread> threads;
    bool keepIterating = true;
    for (size_t i = 0 ; i < numThreads ; ++i)
    {
        threads.emplace_back(std::thread([&book,
                                          &keepIterating,
                                          &longIterationRuntime] ()
        {
            GDAXOrderBook::ensureThreadAttached();

            while(keepIterating == true)
            {
                auto start = std::chrono::steady_clock::now();

                auto bidIter = book.bids.begin();
                while(bidIter != book.bids.end()) { ++bidIter; }

                auto offerIter = book.offers.begin();
                while(offerIter != book.offers.end()) { ++offerIter; }

                auto finish = std::chrono::steady_clock::now();

                std::chrono::duration<double, std::milli> elapsed_ms = finish - start;

                if ( elapsed_ms.count() > longIterationRuntime )
                {
                    std::cerr << "long-running iteration! " << elapsed_ms.count() <<
                        " ms!" << std::endl;
                }
            }
        }));
    }

    std::this_thread::sleep_for(std::chrono::seconds(secondsToSleep));

    keepIterating = false;
    for (auto & i : threads) { i.join(); }
}
