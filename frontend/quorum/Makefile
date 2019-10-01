HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)

TARGETS = test_quorum

ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl
endif

all: $(TARGETS)

%.o: %.cc
	g++ -std=c++11 $^ -c -o $@

test_quorum: quorum.o test_quorum.o ../../database/database.pb.o ../../database/database.grpc.pb.o
	g++ -std=c++11 $^ $(LDFLAGS) -pthread -o test_quorum

test_quorum.o: test_quorum.cc
	g++ -std=c++11 $^ -pthread -c test_quorum.cc

quorum.o: quorum.cc
	cd ../../database; $(MAKE) all
	g++ -std=c++11 $^ -pthread -c quorum.cc

clean:
	rm -fv $(TARGETS) *~ *.o
