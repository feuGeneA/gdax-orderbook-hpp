INCDIRS = -I ..
INCDIRS += -I ./dependencies/libcds-2.3.2
INCDIRS += -I ./dependencies/rapidjson-1.1.0/include
INCDIRS += -I ./dependencies/websocketpp-0.7.0

CDSLIBDIR = ./dependencies/libcds-2.3.2/build-release/bin

LIBS = -lboost_system # needed for boost::asio, used by websocketpp
LIBS += -lpthread
LIBS += -lssl # needed for websocketpp's TLS support (req'd by GDAX)
LIBS += -lcrypto # needed for websocketpp's TLS support (req'd by GDAX)
LIBS += -lcds

test: demo
	LD_LIBRARY_PATH=$(CDSLIBDIR) ./demo

debug: demo
	LD_LIBRARY_PATH=$(CDSLIBDIR) gdb ./demo

demo: demo.cpp ../gdax-orderbook.hpp \
      | dependencies/libcds-2.3.2/build-release/bin/libcds.so \
        dependencies/rapidjson-1.1.0 \
        dependencies/websocketpp-0.7.0
	g++ demo.cpp -std=c++11 -o demo $(INCDIRS) -L $(CDSLIBDIR) $(LIBS)

dependencies/libcds-2.3.2/build-release/bin/libcds.so: \
		| dependencies/libcds-2.3.2
	cd dependencies/libcds-2.3.2 ; if [ ! -d build-release ]; then mkdir build-release; fi
	cd dependencies/libcds-2.3.2/build-release ; cmake -DCMAKE\_BUILD_TYPE=RELEASE ../
	cd dependencies/libcds-2.3.2/build-release ; make -j4

dependencies/libcds-2.3.2: | dependencies/libcds-2.3.2.tar.gz
	tar xzvf dependencies/libcds-2.3.2.tar.gz -C dependencies

dependencies/libcds-2.3.2.tar.gz: dependencies
	wget -O $@ https://github.com/khizmax/libcds/archive/v2.3.2.tar.gz


dependencies/rapidjson-1.1.0: | dependencies/rapidjson-1.1.0.tar.gz
	tar xzvf dependencies/rapidjson-1.1.0.tar.gz -C dependencies

dependencies/rapidjson-1.1.0.tar.gz: dependencies
	wget -O $@ https://github.com/Tencent/rapidjson/archive/v1.1.0.tar.gz


dependencies/websocketpp-0.7.0: | dependencies/websocketpp-0.7.0.tar.gz
	tar xzvf dependencies/websocketpp-0.7.0.tar.gz -C dependencies

dependencies/websocketpp-0.7.0.tar.gz: dependencies
	wget -O $@ https://github.com/zaphoyd/websocketpp/archive/0.7.0.tar.gz


dependencies:
	mkdir dependencies

clean:
	rm -rf demo dependencies
