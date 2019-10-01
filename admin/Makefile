HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)

TARGETS = admin

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

admin: admin.cc admin_helpers/html_helper.o admin_helpers/server.o ../database/database.pb.o ../database/database.grpc.pb.o
	g++ -std=c++11 $^ $(LDFLAGS) -pthread -o $@

server.o: admin_helpers/html_helper.o admin_helpers/server.h admin_helpers/server.cc
	g++ -std=c++11 admin_helpers/server.cc -c -o $@

html_helper.o: admin_helpers/html_helper.h admin_helpers/html_helper.cc
	g++ -std=c++11 admin_helpers/server.cc -c -o $@

clean::
	rm -fv $(TARGETS) *~ *.o
	rm -fv **/*.o
