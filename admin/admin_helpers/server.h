// This is for the admin console and is spun up to manage
// communication with all of the databases

#ifndef _SERVER_H_
#define _SERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <vector>
using namespace std;

typedef struct{
	char bind_ip[16];
	int bind_port;
} SERVER_ADDRESS;

class Server {
public:
	bool test_run = false;
	vector<SERVER_ADDRESS> db_addr_list;
	Server(vector<SERVER_ADDRESS> database_addr_list, SERVER_ADDRESS admin_ip);
	~Server();
	void set_address(SERVER_ADDRESS sa);
	void run();
	map<string, bool> servers();
	bool shutdown(string bind_addr);
	vector<string> read_slice(string bind_addr, int slice_no);

private:
	int socketfd = 0;
	int clientfd = 0;
	int curr_slice = 0;
	SERVER_ADDRESS server_addr;
	void display_data_for_db(string db_id);
	void display_server_stati();
	void respond_to_client(int clientfd);
};

#endif