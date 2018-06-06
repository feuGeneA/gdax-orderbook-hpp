#include <chrono>
#include <iostream>
#include <thread>

#include "gdax-orderbook.hpp"

void printBestBidAndOffer(GDAXOrderBook & book)
{
    std::cout << "current best bid: Ξ" << book.bids.begin()->second << " @ $"
                               << book.bids.begin()->first/100.0 << "/Ξ ; ";
    std::cout << "current best offer: Ξ" << book.offers.begin()->second << " @ $"
                                 << book.offers.begin()->first/100.0 << "/Ξ"
                                 << std::endl;
}

int main(int argc, char* argv[]) {
    GDAXOrderBook book("ETH-USD");

    printBestBidAndOffer(book);

    size_t secondsToSleep = 5;
    std::cout << "waiting " << secondsToSleep << " seconds for the market to "
        "shift" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(secondsToSleep));

    printBestBidAndOffer(book);

    size_t histogram[30];
    for ( auto bucket : histogram ) bucket = 0;

    size_t numThreads = 5;
    secondsToSleep = 90;
    std::cout << "running for " << secondsToSleep << " seconds, with " <<
        numThreads << " threads constantly iterating over the whole order "
        "book." << std::endl;
    bool keepIterating = true;
    {
        std::vector<std::future<void>> futures;
        for (size_t i = 0 ; i < numThreads ; ++i)
        {
            futures.emplace_back(
                std::async(std::launch::async,
                           [&book, &keepIterating, &histogram] ()
            {
                GDAXOrderBook::ensureThreadAttached();

                GDAXOrderBook::bids_map_t::iterator bidIter;
                GDAXOrderBook::offers_map_t::iterator offerIter;
                std::chrono::steady_clock::time_point start, finish;

                while(keepIterating == true)
                {
                    start = std::chrono::steady_clock::now();

                    bidIter = book.bids.begin();
                    while(bidIter != book.bids.end()) { ++bidIter; }

                    offerIter = book.offers.begin();
                    while(offerIter != book.offers.end()) { ++offerIter; }

                    finish = std::chrono::steady_clock::now();

                    histogram[
                        static_cast<size_t>(
                            std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - start
                            ).count()/5
                        )
                    ]++;
                }
            }));
        }

        std::this_thread::sleep_for(std::chrono::seconds(secondsToSleep));

        keepIterating = false;
    }

    // find the largest histogram bucket so we can determine the scale factor
    // for all of the buckets
    double scaleFactor =
        [&histogram]() {
            size_t countOfBiggestBucket = 0;
            for ( size_t & i : histogram )
            {
                countOfBiggestBucket = std::max(i, countOfBiggestBucket);
            }
            return countOfBiggestBucket;
        }()/68; // 80 column display, minus chars used for row headers, =68
    std::cout << "histogram of times to iterate over the whole book:" << std::endl;
    for ( int i=0 ; i < sizeof(histogram)/sizeof(histogram[0]) ; ++i )
    {
        std::cout
            << std::right << std::setw(3) << std::setfill(' ') << i*5
            << "-"
            << std::right << std::setw(3) << std::setfill(' ') << (i+1)*5-1
            << " ms: ";
        for ( int j=0 ; j<histogram[i]/scaleFactor ; ++j )
        {
            std::cout << "*";
        }
        std::cout << std::endl;
    }
}
