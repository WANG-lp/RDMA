
all: pingpong-server pingpong-client

CXXFLAGS += -O3

LIBS += -lrdmacm -libverbs -lrt

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@

pingpong-server: pingpong-server.o RDMAServerSocket.o RDMAClientSocket.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

pingpong-client: pingpong-client.o RDMAClientSocket.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

.PHONY:
clean:
	rm -f *.o pingpong-client pingpong-server
