#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctime> 
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <regex>
#include <stdlib.h>
#include <signal.h>
#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include <chrono>
#include <poll.h>
#include "server.h"

//GRPC
#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "../../database/database.grpc.pb.h"
#include "../../utils/utils.h"
#include "html_helper.h"
#include "http_helper.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

#define PASS_MSG "\033[92mDATABASE HEALTHY\033[0m"
#define FAIL_MSG "\033[91mDATABASE DOWN\033[0m"
#define PING_TIMEOUT 1000
#define SLICE_SIZE 100
#define READ_BUFF_SIZE 1024

#define MAX_THREADS 100
#define DEBUG_PRINTF(fmt, ...) \
		do { if (verbose_flag) {printf(fmt, __VA_ARGS__);}} while (0)

using namespace std::chrono;

//Global Variables
static volatile bool verbose_flag = true;
vector<SERVER_ADDRESS> db_list;
SERVER_ADDRESS my_ip_address;
map<string, bool> db_health;

Server::Server(vector<SERVER_ADDRESS> database_addr_list, SERVER_ADDRESS admin_ip) {
	db_list = database_addr_list;
	my_ip_address = admin_ip;
	this->db_addr_list = database_addr_list;
	this->set_address(admin_ip);
	srand(time(0));
}

Server::~Server() {
	if (socketfd) close(socketfd);
}

void Server::set_address(SERVER_ADDRESS sa) {
	server_addr = sa;
}

map<string, bool> Server::servers() {
	return db_health;
}

bool Server::shutdown(string ip_addr) {
	ClientContext context;
	Success success;
	Empty empty;
 	shared_ptr<Channel> channel = grpc::CreateChannel(ip_addr, grpc::InsecureChannelCredentials());
	unique_ptr<Database::Stub> databaseStub = Database::NewStub(channel);
	databaseStub->shutdown(&context, empty, &success);
	return success.success();
}

vector<string> Server::read_slice(string ip_addr, int slice_no) {
	ClientContext context;
	RowRange row_range;
	Tablet tablet;
	shared_ptr<Channel> channel = grpc::CreateChannel(ip_addr, grpc::InsecureChannelCredentials());
	unique_ptr<Database::Stub> databaseStub = Database::NewStub(channel);

	row_range.set_start_row(slice_no * SLICE_SIZE);
	row_range.set_end_row((slice_no * SLICE_SIZE) + SLICE_SIZE);
	printf("sending %d %d\n", slice_no * SLICE_SIZE, (slice_no * SLICE_SIZE) + SLICE_SIZE);
	databaseStub->getRows(&context, row_range, &tablet);
	printf("got here\n");
    vector<string> slice_data;
  
    for (auto const& it : tablet.tablet()) {
        for (auto const& elt : it.second.row()) {
			ostringstream oss;
			oss << "(" << it.first << "," << elt.first << ") " << elt.second.contents();
			string element = oss.str();
			slice_data.push_back(element);
		}
	}
	return slice_data;
}

void *ping(void *arg) {
	shared_ptr<Channel> channel;

	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	while (1) {
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		duration<double, std::milli> time_span = t2 - t1;
		if (time_span.count() > PING_TIMEOUT) {
			for (std::vector<SERVER_ADDRESS>::iterator it = db_list.begin(); it != db_list.end(); ++it) {
				ClientContext context;
				Success success;
				Empty empty;
				char db_address[20];
				
				sprintf(db_address, "%s:%d", it->bind_ip, it->bind_port);
 				channel = grpc::CreateChannel(db_address, grpc::InsecureChannelCredentials());
				unique_ptr<Database::Stub> databaseStub = Database::NewStub(channel);
				databaseStub->ping(&context, empty, &success);
				if (success.success()) {
					db_health[db_address] = 1;
				} else {
					db_health[db_address] = 0;
				}
			}
			t1 = high_resolution_clock::now();
		}
	}
}

void Server::run() {
    int errno;
    bool run = true;

    if (socketfd<=0) socketfd = socket(PF_INET, SOCK_STREAM, 0);
	int enable = 1;
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
	    fprintf(stderr, "setsockopt(SO_REUSEADDR) failed");
	    exit(1);
	}
	if (socketfd < 0) {
		fprintf(stderr, "Cannot open socket %s\r\n", strerror(errno));
		exit(1);
	}

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_addr.s_addr = htons(0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_addr.bind_port);	

	DEBUG_PRINTF("%s %i\r\n", "Server started listening on port", server_addr.bind_port);

	bind(socketfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	listen(socketfd, MAX_THREADS);

	//Handle pings to databases to monitor health 
	pthread_t new_thread;
	pthread_create(&new_thread, NULL, ping, NULL);

	while(run) {
		if (socketfd<=0) socketfd = socket(PF_INET, SOCK_STREAM, 0);
		int enable = 1;
		if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		    fprintf(stderr, "setsockopt(SO_REUSEADDR) failed");
		    exit(1);
		}
		if (socketfd < 0) {
			fprintf(stderr, "Cannot open socket %s\r\n", strerror(errno));
			exit(1);
		}

		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_addr.s_addr = htons(0);
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(server_addr.bind_port);	

		DEBUG_PRINTF("%s %i\r\n", "Server started listening on port", server_addr.bind_port);
	
		bind(socketfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
		listen(socketfd, MAX_THREADS);
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
		int clientfd = accept(socketfd, (struct sockaddr*) &clientaddr, &(clientaddrlen));
		if (clientfd < 0) {
			DEBUG_PRINTF("%s\r\n", "Listening file descriptor is closed.");
			exit(0);
		}

		DEBUG_PRINTF("[DEBUG] %s (%s)\r\n", "New Connection", inet_ntoa(clientaddr.sin_addr));

		// spawn thread with client
		this->respond_to_client(clientfd);
		DEBUG_PRINTF("%s\r\n", "RUNNING");
	}

	pthread_exit(NULL);
}	

void Server::display_data_for_db(string db_id) {
	DEBUG_PRINTF("%s %i\r\n", "Display data for DB", server_addr.bind_port);
	vector<string> data = this->read_slice(db_id, this->curr_slice);
	string content = html_content_for_data(data);
	string response = http_ok_response(content);
	write(this->clientfd, response.c_str(), strlen(response.c_str()) * sizeof(char));
}

void Server::display_server_stati() {
	DEBUG_PRINTF("%s\r\n", "Displaying statuses...");
	map<string,bool> stati = this->servers();
	printf("Map size: %d\r\n", (int) stati.size());
	string content = html_content_for_status(stati);
	string response = http_ok_response(content);
	write(this->clientfd, response.c_str(), strlen(response.c_str()) * sizeof(char));
}

void Server::respond_to_client(int clientfd) {
	//first read what the client is requesting
	this->clientfd = clientfd;
	regex regx_get("GET\\s([^\\s]*)\\sHTTP.*");
	
	smatch matches;
	string read_buff = "";

	while(true) {
		// read the next character
		char next_char[READ_BUFF_SIZE];
		memset(next_char, 0, READ_BUFF_SIZE * sizeof(char));
		read(clientfd, &next_char, READ_BUFF_SIZE * sizeof(char));
		read_buff += string(next_char);
	
		if (regex_search(read_buff, matches, regx_get)) {
			break;
		}
	}

	string page = matches[1].str();

	regex regx_to_db("/to_db=(.*)");
	regex regx_kill_db("/kill=(.*)");
	regex regx_next_slice("/next_slice");

	if (regex_search(page, matches, regx_to_db)) {
		this->curr_slice = 0;
		string db = matches[1].str();
		this->display_data_for_db(db);
	} else if (regex_search(page, matches, regx_kill_db)) {
		string db_to_kill = matches[1].str();
		this->shutdown(db_to_kill);
		this->display_server_stati();
	} else if (regex_search(page, matches, regx_next_slice)) {
		this->curr_slice ++;
		string db = matches[1].str();
		this->display_data_for_db(db);
	} else {
		this->display_server_stati();
	}

	close(clientfd);
	this->clientfd = 0;
}

