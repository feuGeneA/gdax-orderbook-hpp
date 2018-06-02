A copy of the GDAX order book for the product given during construction, exposed as two maps, one for bids and one for offers, each mapping price levels to order quantities, continually updated in real time via the `level2` channel of the Websocket feed of the GDAX API.

Spawns a separate thread to receive updates from the GDAX WebSocket Feed and process them into the maps.

To ensure high performance, implemented using concurrent data structures from libcds.  The price->quantity maps are instances of cds::container::SkipListMap, whose doc says it is lock-free.
