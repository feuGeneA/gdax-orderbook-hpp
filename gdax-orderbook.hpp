#ifndef GDAX_ORDERBOOK_HPP
#define GDAX_ORDERBOOK_HPP

#include <cstring>
#include <future>
#include <iostream>
#include <string>

#include <cds/container/skip_list_map_hp.h>
#include <cds/gc/hp.h>
#include <cds/init.h>

#include <rapidjson/document.h>

#include <websocketpp/client.hpp>
#include <websocketpp/concurrency/none.hpp>
#include <websocketpp/config/asio_client.hpp>

/**
 * A copy of the GDAX order book for the currency pair product given during
 * construction, exposed as two maps, one for bids and one for offers, each
 * mapping price levels to order quantities, continually updated in real time
 * via the `level2` channel of the Websocket feed of the GDAX API.
 *
 * Spawns a separate thread to receive updates from the GDAX WebSocket Feed and
 * process them into the maps.
 *
 * To ensure high performance, implemented using concurrent data structures
 * from libcds.  The price->quantity maps are instances of
 * cds::container::SkipListMap, whose doc says it is lock-free.
 */
class GDAXOrderBook {
private:
    // libcds requires paired Initialize/Terminate calls
    struct CDSInitializer {
        CDSInitializer() { cds::Initialize(); }
        ~CDSInitializer() { cds::Terminate(); }
    } m_cdsInitializer;

    // a libcds garbage collector is required for our map structures
    cds::gc::HP m_cdsGarbageCollector;

public:
    /**
     * libcds requires each and every thread to first "attach" itself to the
     * lib before using any data structures. This method is used internally,
     * and is called during construction on behalf of the constructing thread,
     * and should be called by any additional client threads that access the
     * price->quanitity maps.
     */
    static void ensureThreadAttached()
    {
        if (cds::threading::Manager::isThreadAttached() == false)
            cds::threading::Manager::attachThread();
    }

    GDAXOrderBook(std::string const& product = "BTC-USD")
        : m_cdsGarbageCollector(67*2),
            // per SkipListMap doc, 67 hazard pointers per instance
          m_threadTerminator(
            std::async(
                std::launch::async,
                &GDAXOrderBook::handleUpdates,
                this,
                product))
    {
        ensureThreadAttached();
        m_bookInitialized.get_future().wait();
    }

    using Price = unsigned int; // cents
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

    ~GDAXOrderBook() { m_client.stop(); }

private:
    struct websocketppConfig
        : public websocketpp::config::asio_tls_client
    {
        typedef websocketpp::concurrency::none concurrency_type;
            // we only have one thread using the WebSocket
    };
    using websocketclient_t = websocketpp::client<websocketppConfig>;
    websocketclient_t m_client;

    std::promise<void> m_bookInitialized; // to signal constructor to finish

    std::future<void> m_threadTerminator; // for graceful thread destruction

    /**
     * Initiates WebSocket connection, subscribes to order book updates for the
     * given product, installs a message handler which will receive updates
     * and process them into the maps, and starts the asio event loop.
     */
    void handleUpdates(std::string const& product)
    {
        ensureThreadAttached();

        rapidjson::Document json;

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
                [this, &json] (websocketpp::connection_hdl,
                               websocketppConfig::message_type::ptr msg)
                {
                    json.Parse(msg->get_payload().c_str());
                    const char *const type = json["type"].GetString();
                    if ( strcmp(type, "l2update") == 0 )
                    {
                        processUpdates(json, bids, offers);
                    }
                    else if ( strcmp(type, "snapshot") == 0 )
                    {
                        processSnapshot(json, bids, offers, m_bookInitialized);
                    }
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
                [this, &product](websocketpp::connection_hdl handle)
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
                std::cerr << "failed websocketclient_t::get_connection(): " <<
                    errorCode.message() << std::endl;
            }

            m_client.connect(connection);

            m_client.run();
        } catch (websocketpp::exception const & e) {
            std::cerr << "handleUpdates() failed: " << e.what() << std::endl;
        }
    }

    /**
     * Simply delegates snapshot processing to a helper function (different
     * template instantiations of the same function, one for each type of map
     * (bid, offer)), and signals when the snapshot has been processed.
     */
    static void processSnapshot(
        rapidjson::Document & json,
        bids_map_t & bids,
        offers_map_t & offers,
        std::promise<void> & finished)
    {
        processSnapshotHalf(json, "bids", bids);
        processSnapshotHalf(json, "asks", offers);
        finished.set_value();
    }

    /**
     * Helper to permit code re-use on either type of map (bids or offers).
     * Traverses already-parsed json document and inserts initial-price
     * snapshots for entire half (bids or offers) of the order book.
     */
    template<typename map_t>
    static void processSnapshotHalf(
        rapidjson::Document const& json,
        const char *const bidsOrOffers,
        map_t & map)
    {
        for (auto j = 0 ; j < json[bidsOrOffers].Size() ; ++j)
        {
            Price price = std::stod(json[bidsOrOffers][j][0].GetString())*100;
            Size   size = std::stod(json[bidsOrOffers][j][1].GetString());

            map.insert(price, size);
        }
    }

    /**
     * Traverses already-parsed json document, and, assuming it's a "l2update"
     * document, updates price->quantity maps based on the order book changes
     * that have occurred.
     */
    static void processUpdates(
        rapidjson::Document & json,
        bids_map_t & bids,
        offers_map_t & offers)
    {
        for (auto i = 0 ; i < json["changes"].Size() ; ++i)
        {
            const char* buyOrSell = json["changes"][i][0].GetString(),
                      * price     = json["changes"][i][1].GetString(),
                      * size      = json["changes"][i][2].GetString();

            if ( strcmp(buyOrSell, "buy") == 0 )
            {
                updateMap(price, size, bids);
            }
            else
            {
                updateMap(price, size, offers);
            }
        }
    }

    /**
     * Helper to permit code re-use on either type of map (bids or offers).
     * Simply updates a single map entry with the specified price/size.
     */
    template<typename map_t>
    static void updateMap(
        const char *const price,
        const char *const size,
        map_t & map)
    {
        if (std::stod(size) == 0) { map.erase(std::stod(price)); }
        else
        {
            map.update(
                std::stod(price)*100,
                [size](bool & bNew,
                       std::pair<const Price, Size> & pair)
                {
                    pair.second = std::stod(size);
                });
        }
    }
};

#endif // GDAX_ORDERBOOK_HPP
