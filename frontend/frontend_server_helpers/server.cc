#include <stdio.h>
#include <iostream>
#include <fstream>
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
#include "http_helper.h"
#include "server.h"

#define MAX_THREADS 100
#define DEBUG_PRINTF(fmt, ...) \
		do { if (verbose_flag) {printf(fmt, __VA_ARGS__);}} while (0)
#define COOKIE_NAME "cis505"
#define COOKIE_LEN 16

static volatile bool verbose_flag = true;

using namespace std::chrono;


Server::Server(vector<SERVER_ADDRESS> server_addr_list, int index) {
	this->server_addr_list = server_addr_list;
	this->server_addr_index = index;
	this->set_address(server_addr_list.at(index));
	this->clients_by_cookie = map<string,Client>();
	srand(time(0));
}

Server::~Server() {
	if (socketfd) close(socketfd);
}


void Server::set_address(SERVER_ADDRESS sa) {
	server_addr = sa;
}

void Server::run() {

    int errno;
    
    bool run = true;
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
		triage_new_client_connection(clientfd);
	}
}
#define READ_BUFF_SIZE 1024


void Server::triage_new_client_connection(int clientfd) {
	//first read what the client is requesting
	DEBUG_PRINTF("[DEBUG] %s\r\n", "triage_new_client_connection()");
	regex regx_doublecrlf("\\r\\n\\r\\n");
	regex regx_ispost(".*POST.*");
	regex regx_endpost;
	regex regx_boundary("boundary=(.*)\\r\\n");
	
	smatch matches;
	string read_buff = "";

	bool post = false;
	bool boundary_found = false;
	string boundary_str = string("");
	while(true) {
		// read the next character
		char next_char[READ_BUFF_SIZE];
		memset(next_char, 0, READ_BUFF_SIZE * sizeof(char));
		read(clientfd, &next_char, READ_BUFF_SIZE * sizeof(char));
		read_buff += string(next_char);
		if (post) {
			// printf("\r\n\r\n\r\n %s \r\n\r\n\r\n", read_buff.c_str());
		}
		// check if we've reached the end of a command
		if (!post && regex_search(read_buff, matches, regx_ispost)) {
			post = true;
		}
		if (!boundary_found && regex_search(read_buff, matches, regx_boundary)) {
			boundary_found = true;
			boundary_str = matches[1].str();
			printf("[DEBUG] Boundary = %s\r\n", boundary_str.c_str());
			//construct a regex based on the boundary
			regex regx_boundary(boundary_str + string("[\\s\\S]*") + boundary_str + 
			string("([\\s\\S]*)--") + boundary_str + string(".*\r\n"));
			regx_endpost = regx_boundary;
		}
		if (!post && regex_search(read_buff, matches, regx_doublecrlf)) {
			break;
		}
		if (post && regex_search(read_buff, matches, regx_endpost)) {
			break;
		}
	}
	DEBUG_PRINTF("[DEBUG] CLIENT:\'%s\'\r\n", read_buff.c_str());
	regex cookie_reg("Cookie: ([\\w\\d]*)=([\\w\\d]*)");
	string cookie = "";
	if (regex_search(read_buff, matches, cookie_reg)) {
		DEBUG_PRINTF("[DEBUG] %s\r\n", "MATCH!");
		//try to find the cookie
		for (int i = 0; i < (int)matches.size(); ++i) {
			DEBUG_PRINTF("[DEBUG] match[%i] = %s\r\n", i, matches[i].str().c_str());
			if (!matches[i].str().compare(COOKIE_NAME)) {
				cookie = matches[i + 1].str();
				DEBUG_PRINTF(" [DEBUG] Found cookie:\'%s\'\r\n", cookie.c_str());
				break;
			}
		}
	}

	if (strlen(cookie.c_str()) && clients_by_cookie.find(cookie) == clients_by_cookie.end()) {
		fprintf(stderr, "Magic! Cookie (%s) found but not in client map.\r\n", cookie.c_str());
		exit(1);
	} else if (!strlen(cookie.c_str())) {
		DEBUG_PRINTF("[DEBUG] %s\r\n", "Cookie not found. Generating...");
		cookie = this->generate_cookie(COOKIE_LEN);
		string cookie_name = string(COOKIE_NAME);
		Client *tmp = new Client(COOKIE_NAME, cookie);
		tmp->test_run = this->test_run;
		clients_by_cookie.insert(make_pair(cookie, *tmp));
	}

	Client *client = &clients_by_cookie.at(cookie);

	client->respond_to(read_buff, clientfd);
}
string Server::generate_cookie(int length) {

	auto randchar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    string str(length, 0);
    generate_n(str.begin(), length, randchar);
    return str;
}