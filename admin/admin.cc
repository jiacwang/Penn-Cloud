//
// This is the implementation for the admin console
//
// author: ptrent
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
#include "./admin_helpers/server.h"

#define DEBUG_PRINTF(fmt, ...) \
		do { if (verbose_flag) { printf(fmt, __VA_ARGS__);}} while (0)

#define MAX_THREADS 100

volatile static bool verbose_flag = false;
volatile static int my_server_instance = 0;
using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "*** Author: Phillip Trent (ptrent)\r\n");
		exit(1);
	}

	int c;
	int test_run = false;
	int opterr = 0;

	
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
	ifstream db_list_file;
	db_list_file.open(file_path);
	if (!db_list_file) {
		fprintf(stderr, "Oops! Looks like there was no file at %s\n\r", file_path);
	    return 1;
	}

	vector<SERVER_ADDRESS> addr_list;
	string line;
	regex addr_reg("([^:]*):(\\d*)");
	smatch matches;
	while (getline(db_list_file, line)) {
		SERVER_ADDRESS sa;
		if (regex_search(line, matches, addr_reg)) {
			if (matches.size() == 3) {
				strcpy(sa.bind_ip, matches[1].str().c_str());
				sa.bind_port = atoi(matches[2].str().c_str());
			} else {
				fprintf(stderr, "Malformatted input file. Exiting...");
				db_list_file.close();
				exit(1);
			}
		} else {
			fprintf(stderr, "Malformatted input file. Exiting...");
			db_list_file.close();
			exit(1);
		}
		addr_list.push_back(sa);
	}
	db_list_file.close();

	if (my_server_instance > addr_list.size()) {
		fprintf(stderr, "Can not find server forward,bind addresses "
			"for index %i in file %s\n\r", my_server_instance, file_path);
	    return 1;
	}

	SERVER_ADDRESS admin_sa;
	string admin_info = argv[argc - 2];
	if (regex_search(admin_info, matches, addr_reg)) {
		strcpy(admin_sa.bind_ip, matches[1].str().c_str());
		admin_sa.bind_port = atoi(matches[2].str().c_str());
	} else {
		fprintf(stderr, "Malformatted admin ip argument. Exiting...");
		exit(1);
	}
	
	Server s(addr_list, admin_sa);
	s.test_run = test_run;
	s.run();
	return 0;
}

