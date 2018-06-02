#ifndef GDAX_ORDERBOOK_HPP
#define GDAX_ORDERBOOK_HPP

#include <cstring>
#include <functional>
#include <iostream>
#include <string>

#include <cds/container/rwqueue.h>
#include <cds/container/skip_list_map_hp.h>
#include <cds/gc/hp.h>
#include <cds/init.h>

#include <rapidjson/document.h>

#include <websocketpp/client.hpp>
#include <websocketpp/concurrency/none.hpp>
#include <websocketpp/config/asio_client.hpp>

/**
 * A copy of the GDAX order book for the product given during construction,
 * exposed as two maps, one for bids and one for offers, each mapping price
 * levels to order quantities, continually updated in real time via the
 * `level2` channel of the Websocket feed of the GDAX API.
 *
 * Spawns two threads, one to receive updates from the GDAX WebSocket Feed and
 * store them in an internal queue, and another to pull updates out of that
 * queue and store them in the maps.
 *
 * Queue-then-map approach chosen based on the assumption that a queue is
 * faster than a map, so updates can be pulled off the wire as fast as
 * possible, with neither map insertion latency nor map garbage collection
 * slowing down the reception pipeline.  (Future improvement: profile queue
 * usage; if consistently empty, consider going straight from WebSocket to map;
 * if consistenly used, consider allowing configuration of queue size in order
 * to avoid overflow.)
 *
 * To ensure high performance, implemented using concurrent data structures
 * from libcds.  The internal queue is a cds::container::RWQueue, whose doc
 * says "The queue has two different locks: one for reading and one for
 * writing. Therefore, one writer and one reader can simultaneously access to
 * the queue."  The use case in this implementation has exactly one reader
 * thread and one writer thread.  The price->quantity maps are instances of
 * cds::container::SkipListMap, whose doc says it is lock-free.
 */
class GDAXOrderBook {
private:
    struct CDSInitializer {
        CDSInitializer() { cds::Initialize(); }
        ~CDSInitializer() { cds::Terminate(); }
    } m_cdsInitializer;

    cds::gc::HP m_cdsGarbageCollector;

public:
    static void ensureThreadAttached()
    {
        if (cds::threading::Manager::isThreadAttached() == false)
            cds::threading::Manager::attachThread();
    }

    GDAXOrderBook(std::string const& product = "BTC-USD")
        : m_cdsGarbageCollector(67*2),
            // per SkipListMap doc, 67 hazard pointers per instance
          receiveUpdatesThread(&GDAXOrderBook::receiveUpdates, this, product),
          processUpdatesThread(&GDAXOrderBook::processUpdates, this)
    {
        ensureThreadAttached();

        while ( ! m_bookInitialized ) { continue; }
    }

    using Price = double;
    using Size = double;
    using offers_map_t = cds::container::SkipListMap<cds::gc::HP, Price, Size>;
    using bids_map_t =
        cds::container::SkipListMap<
            cds::gc::HP,
            Price,
            Size,
            // reverse map ordering so best (highest) bid is at begin()
            typename cds::container::skip_list::make_traits<
                cds::opt::less<std::greater<Price>>>::type>;
    // *map_t::get(Price) returns an std::pair<Price, Size>*
    bids_map_t bids;
    offers_map_t offers;

    ~GDAXOrderBook()
    {
        // tell threads we're terminating, and wait for them to finish

        m_client.stop();
        receiveUpdatesThread.join();

        m_stopUpdating = true;
        processUpdatesThread.join();
    }

private:
    std::thread processUpdatesThread;
    std::thread receiveUpdatesThread;

    cds::container::RWQueue<
        std::string
#if PROFILE_MAX_QUEUE > 0
        ,typename cds::container::rwqueue::make_traits<
            cds::opt::item_counter<cds::atomicity::item_counter>
        >::type
#endif
    > m_queue;

public:
    size_t getQueueSize() { return m_queue.size(); }

private:
    struct websocketppPolicy
        : public websocketpp::config::asio_tls_client
        // would prefer transport::iostream instead of transport::asio,
        // because we only have one thread using the WebSocket, but the only
        // policy preconfigured with TLS support is the asio one.
    {
        typedef websocketpp::concurrency::none concurrency_type;
    };
    using WebSocketClient = websocketpp::client<websocketppPolicy>;
    WebSocketClient m_client;

    /**
     * Initiates WebSocket connection, subscribes to order book updates for the
     * given product, installs a message handler which will receive updates
     * and enqueue them to m_queue, and starts the asio event loop.
     */
    void receiveUpdates(std::string const& product)
    {
        ensureThreadAttached();

        try {
            m_client.clear_access_channels(websocketpp::log::alevel::all);
            m_client.set_access_channels(
                websocketpp::log::alevel::connect |
                websocketpp::log::alevel::disconnect);

            m_client.clear_error_channels(websocketpp::log::elevel::all);
            m_client.set_error_channels(
                websocketpp::log::elevel::info |
                websocketpp::log::elevel::warn |
                websocketpp::log::elevel::rerror |
                websocketpp::log::elevel::fatal);

            m_client.init_asio();

            m_client.set_message_handler(
                [this](websocketpp::connection_hdl,
                       websocketppPolicy::message_type::ptr msg)
                {
                    this->m_queue.enqueue(msg->get_payload());
#if PROFILE_MAX_QUEUE > 0
                    size_t queueSize = m_queue.size();
                    if ( queueSize > PROFILE_MAX_QUEUE ) {
                        std::cerr << queueSize << " updates are waiting to be "
                            "processed on the internal queue" << std::endl;
                    }
#endif
                });

            m_client.set_tls_init_handler(
                [](websocketpp::connection_hdl)
                {
                    websocketpp::lib::shared_ptr<boost::asio::ssl::context>
                        context = websocketpp::lib::make_shared<
                            boost::asio::ssl::context>(
                            boost::asio::ssl::context::tlsv1);

                    try {
                        context->set_options(
                            boost::asio::ssl::context::default_workarounds |
                            boost::asio::ssl::context::no_sslv2 |
                            boost::asio::ssl::context::no_sslv3 |
                            boost::asio::ssl::context::single_dh_use);
                    } catch (std::exception& e) {
                        std::cerr << "set_tls_init_handler() failed to set"
                            " context options: " << e.what() << std::endl;
                    }
                    return context;
                });

            m_client.set_open_handler(
                [&product, this](websocketpp::connection_hdl handle)
                {
                    // subscribe to updates to product's order book
                    websocketpp::lib::error_code errorCode;
                    this->m_client.send(handle,
                        "{"
                            "\"type\": \"subscribe\","
                            "\"product_ids\": [" "\""+product+"\"" "],"
                            "\"channels\": [" "\"level2\"" "]"
                        "}", websocketpp::frame::opcode::text, errorCode);
                    if (errorCode) {
                        std::cerr << "error sending subscription: " +
                            errorCode.message() << std::endl;
                    }
                });

            websocketpp::lib::error_code errorCode;
            auto connection =
                m_client.get_connection("wss://ws-feed.gdax.com", errorCode);
            if (errorCode) {
                std::cerr << "failed WebSocketClient::get_connection(): " <<
                    errorCode.message() << std::endl;
            }

            m_client.connect(connection);

            m_client.run();
        } catch (websocketpp::exception const & e) {
            std::cerr << "receiveUpdates() failed: " << e.what() << std::endl;
        }
    }

    bool m_stopUpdating = false; // for termination of processUpdates() thread
    bool m_bookInitialized = false; // to tell constructor to return

    /**
     * Continually dequeues order book updates from `m_queue` (busy-waiting if
     * there aren't any), and moves those updates to `bids` and `offers`, until
     * `m_stopUpdating` is true.
     */
    void processUpdates()
    {
        ensureThreadAttached();

        rapidjson::Document json;
        std::string update;
        bool queueEmpty;

        while ( true ) 
        {
            if ( m_stopUpdating ) return;

            queueEmpty = ! m_queue.dequeue(update);
            if (queueEmpty) continue;

            json.Parse(update.c_str());

            using std::stod;

            const char *const type = json["type"].GetString();
            if ( strcmp(type, "snapshot") == 0 )
            {
                parseSnapshotHalf(json, "bids", bids);
                parseSnapshotHalf(json, "asks", offers);
                m_bookInitialized = true;
            }
            else if ( strcmp(type, "l2update") == 0 )
            {
                for (auto i = 0 ; i < json["changes"].Size() ; ++i)
                {
                    const char* buyOrSell = json["changes"][i][0].GetString(),
                              * price     = json["changes"][i][1].GetString(),
                              * size      = json["changes"][i][2].GetString();

                    if ( strcmp(buyOrSell, "buy") == 0 )
                    {
                        parseUpdate(price, size, bids);
                    }
                    else
                    {
                        parseUpdate(price, size, offers);
                    }
                }
            }
        }
    }

    template<typename map_t>
    void parseSnapshotHalf(
        rapidjson::Document const& json,
        const char *const bidsOrOffers,
        map_t & map)
    {
        for (auto j = 0 ; j < json[bidsOrOffers].Size() ; ++j)
        {
            Price price = std::stod(json[bidsOrOffers][j][0].GetString());
            Size   size = std::stod(json[bidsOrOffers][j][1].GetString());

            map.insert(price, size);
        }
    }

    template<typename map_t>
    void parseUpdate(
        const char *const price,
        const char *const size,
        map_t & map)
    {
        if (std::stod(size) == 0) { map.erase(std::stod(price)); }
        else
        {
            map.update(
                std::stod(price),
                [size](bool & bNew,
                       std::pair<const Price, Size> & pair)
                {
                    pair.second = std::stod(size);
                });
        }
    }
};

#endif // GDAX_ORDERBOOK_HPP
