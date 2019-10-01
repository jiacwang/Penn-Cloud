// load_balancer.cc
//
// Connect to this. Get redirected to another port!
//
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <regex>
#include <stdlib.h>
#include <signal.h>
#include "frontend_server_helpers/http_helper.h"

#define DEBUG_PRINTF(fmt, ...) \
		do { if (verbose_flag) { printf(fmt, __VA_ARGS__);}} while (0)

#define MAX_THREADS 100

volatile static bool verbose_flag = false;
using namespace std;
int next_server_index = 0;

void redirect_client(int fd, vector<string> addr_list) {
	next_server_index ++;
	next_server_index = next_server_index % addr_list.size();
	string content = string("http://") + addr_list.at(next_server_index);
	string response = http_found_redirect(content);

	write(fd, response.c_str(), strlen(response.c_str())*sizeof(char));
	close(fd);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "*** Author: Phillip Trent (ptrent)\r\n");
		exit(1);
	}

	int c;
	int test_run = false;
	int opterr = 0;
	int my_port = atoi(argv[argc - 2]);

	
	// code help from https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
	while ((c = getopt(argc, argv, "vt")) != -1) {
		switch (c) {
			case 'v':
				verbose_flag = true;
				break;
			case 't':
				test_run = true;
				break;
			case '?':
				fprintf(stderr, "It looks like there was a non recognized option. Please try again.\n\r");
				return 1;
			default:
				exit(1);
		}
	}
	char *file_path = argv[argc - 1];
	ifstream server_list_file;
	server_list_file.open(file_path);
	if (!server_list_file) {
		fprintf(stderr, "Oops! Looks like there was no file at %s\n\r", file_path);
	    return 1;
	}

	vector<string> addr_list;
	string line;
	while (getline(server_list_file, line)) addr_list.push_back(line);
	server_list_file.close();


	// bind to the ip that I was given.
	// when a user connects, send a redirect request
	int socketfd;
	int clientfd;
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
		servaddr.sin_port = htons(my_port);	

		DEBUG_PRINTF("%s %i\r\n", "Load balancer started listening on port", my_port);
	
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
		redirect_client(clientfd, addr_list);
	}

	addr_list.size();

	
	return 0;
}

