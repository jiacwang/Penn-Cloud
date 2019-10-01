#ifndef _SERVER_H_
#define _SERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include "../frontend_defs.h"
using namespace std;

class Client;

typedef struct{
	char bind_ip[16];
	int bind_port;
} SERVER_ADDRESS;

class Server {
public:
	bool test_run = false;
	vector<SERVER_ADDRESS> server_addr_list;
	map<string, Client> clients_by_cookie;
	Server(vector<SERVER_ADDRESS> server_addr_list, int index);
	~Server();
	void set_address(SERVER_ADDRESS sa);
	void run();
	void remove_client(int fd);

private:
	int server_addr_index = -1;
	int socketfd = 0;
	SERVER_ADDRESS server_addr;
	void triage_new_client_connection(int clientfd);
	string generate_cookie(int length);
};

#endif