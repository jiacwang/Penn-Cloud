#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "../frontend_defs.h"
using namespace std;

class Server;
class Quorum;
class FileServer;
class MailServer;

typedef struct email_t email_t;

typedef enum {
	GEN_READY, SMTP_RECEIVE_DATA
} CLIENT_STATE;

class Client {
public:
	bool test_run = false;
	Client(string cookie_name, string cookie);
	~Client();
	void respond_to(string request, int resp_fd);
private:
	string cookie = "";
	string cookie_name = "";
	FileServer *fileserver;
	MailServer *mailserver;
	Quorum *quorum;
	CLIENT_STATE state = GEN_READY;
	int clientfd = 0;
	bool authenticated = false;
	string username = "";
	void triage_http(string request);
	void http_get(string input_path);
	void http_post(string content);
	void triage_test(string request);
	//- - - - - - - - - - - - admin functions - - - - - - - - - - -
	// creates a login for the client with the given username and password
	void signup(string username, string password);

	// logs a previously signed up client into their account
	void login(string username, string password);
	//- - - - - - - - - - - - SMTP functions - - - - - - - - - - -
	void list_emails();
	void send_email(string to, string from, string msg_text);
	void display_email(string email_id);

	//- - - - - - - - - - - - FILE functions - - - - - - - - - - -
	// Creates a directory at the given path if one doesn't exist
	void file_make_directory(string local_path);

	// Moves the working directory to the one with the local path
	// relative
	void file_change_to_directory(string local_path);

	void file_download(string filename);
	// creates a file with the give name (this can also be a path)
	void file_upload(string filename);

};

#endif