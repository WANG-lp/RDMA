
all: pingpong-server pingpong-client

CXXFLAGS += -O3 -std=c++0x

LIBS += -lrdmacm -libverbs -lrt

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

pingpong-server: pingpong-server.o Arguments.o pingpong-common.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

pingpong-client: pingpong-client.o Arguments.o pingpong-common.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

.PHONY:
clean:
	rm -f *.o pingpong-client pingpong-server
