HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)

TARGETS = frontend load_balancer

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

# %.o: %.cc
# 	g++ -std=c++11 $^ -c -o $@

utils.o: 
	cd ../utils && $(MAKE) utils.o

	# $(shell find frontend_server_helpers -type f)
quorum/quorum.o: frontend_defs.h
	cd quorum && $(MAKE) quorum.o

html_helper.o: frontend_server_helpers/html_helper.h frontend_server_helpers/html_helper.cc
	g++ -std=c++11 frontend_server_helpers/html_helper.cc -c -o $@

client.o: mail_server.o quorum/quorum.o frontend_defs.h mail_server.h file_server.h \
	frontend_server_helpers/client.h frontend_server_helpers/client.cc \
	frontend_server_helpers/html_helper.o
	g++ -std=c++11 frontend_server_helpers/client.cc -c -o $@

server.o: quorum/quorum.o frontend_defs.h frontend_server_helpers/server.h frontend_server_helpers/server.cc
	g++ -std=c++11 frontend_server_helpers/server.cc -c -o $@

file_server.o: quorum/quorum.o frontend_defs.h file_server.h file_server.cc 
	g++ -std=c++11 file_server.cc -c -o $@

mail_server.o: quorum/quorum.o frontend_defs.h mail_server.h mail_server.cc
	g++ -std=c++11 mail_server.cc -c -o $@

test_mail_server.o: test_mail_server.cc
	g++ -std=c++11 test_mail_server.cc -c -o $@

test_mail_server: test_mail_server.o quorum/quorum.o ../utils/utils.o mail_server.o ../database/database.pb.o ../database/database.grpc.pb.o
	g++ -std=c++11 $^ $(LDFLAGS) -pthread -o $@

frontend: frontend_defs.h frontend.cc \
	../utils/utils.o \
	frontend_server_helpers/html_helper.o \
	frontend_server_helpers/client.o \
	frontend_server_helpers/server.o \
	file_server.o mail_server.o quorum/quorum.o \
	../database/database.pb.o ../database/database.grpc.pb.o
	g++ -std=c++11 $^ $(LDFLAGS) -pthread -o $@

load_balancer: frontend_server_helpers/http_helper.h load_balancer.cc
	g++ -std=c++11 $^ -pthread -o $@

test_frontend: frontend_defs.h frontend_server_helpers/html_helper.h \
	frontend_server_helpers/http_helper.h test_file_server.cc \
	frontend_server_helpers/html_helper.o \
	frontend_server_helpers/client.o \
	frontend_server_helpers/server.o \
	file_server.o mail_server.o quorum/quorum.o
	g++ -std=c++11 $^ -pthread -o $@

clean::
	rm -fv $(TARGETS) *~ *.o
	rm -fv **/*.o
