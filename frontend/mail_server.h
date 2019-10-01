// file_server.h
//
// This is the file frontend server
// the class held within should implement functions
// that are called by the frontend server (after 
// creating an instance of this class) and those functions
// should interface with the backend data store to 
// serve the correct data for the client in question
//
// note that each client will have its own instance of
// a file server
//

#ifndef _MAIL_SERVER_H_
#define _MAIL_SERVER_H_

#include <string>
#include "frontend_defs.h"

using namespace std;

typedef struct email_t {
	string id = string(""); //this
	string subject = string("");
	string text = string("");
	string to = string("");
	string from = string(""); //this
	string timestamp = string("");
} email_t;

class Quorum;

class MailServer {
public:
	MailServer(Quorum *q, string user);
	~MailServer();
	vector<email_t> get_inbox();
	void send(string from, string to, string subject, string msg_text);
	email_t get_email(string email_id);

private:
	string user;
	Quorum *q;
};

#endif
