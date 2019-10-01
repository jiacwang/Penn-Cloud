// mail_server.cc
//
// MailServer class implementation
//

#include "mail_server.h"
#include "../utils/utils.h"
#include "./quorum/quorum.h"
#include <sstream>
#include <regex>
#include <time.h>

using namespace std;
vector<email_t> inbox;
MailServer::MailServer(Quorum *q, string user) {
	this->q = q;
	this->user = user;
}

MailServer::~MailServer() {
	//deinit
}

vector<email_t> MailServer::get_inbox() {
	vector<email_t> v;
	string emails = "";

	q->read(this->user, MBOX_COL, &emails);
	vector<string> list_of_emails;
	split_string(emails, "\n", &list_of_emails);

	for (auto const& it : list_of_emails) {
		if (!it.empty()) {
			vector<string> email_data;
			split_string(it, ":", &email_data);

			email_t em;
			em.from = email_data[1];
			em.id = email_data[0];
			v.push_back(em);
		}
	} 

	return v;

}

void MailServer::send(string from, string to, string subject, string msg_text) {
	time_t t = time(NULL);
	struct tm time = *localtime(&t);

	ostringstream oss_date;
	oss_date << std::to_string(time.tm_year + 1900) << " " << std::to_string(time.tm_mon + 1) << " " << std::to_string(time.tm_mday) << " " << std::to_string(time.tm_hour) << std::to_string(time.tm_min) << std::to_string(time.tm_sec);
	string time_str = oss_date.str();

	ostringstream oss_msg;
	oss_msg << "From:" << from << "\n" << "Subject:" << subject << "\n" << "Time:" << time_str << "\n" << msg_text;
	string email_entry = oss_msg.str();

	std::hash<std::string> str_hash;
    int email_id = str_hash(email_entry);

    ostringstream oss_mbox;
    oss_mbox << email_id << ":" << from << ":" << time_str;
    string mbox_list = oss_mbox.str();

    string emails;
    string user_to;
    regex reg("(.*)@localhost");
	smatch matches;

	if (regex_search(to, matches, reg)) {
		user_to = matches[1];
	}

    q->read(user_to, MBOX_COL, &emails);
    emails += mbox_list + "\n";

    q->write(user_to, MBOX_COL, emails);
    q->write(user_to, std::to_string(email_id), email_entry);
}


email_t MailServer::get_email(string email_id) {
	email_t em;
	em.id = email_id;
	em.to = this->user;

	string email_content = "";
	int stat = q->read(this->user, email_id, &email_content);
	printf("email id: %s\n", email_id.c_str());
	printf("email content: %s\n", email_content.c_str());
	regex reg("From:(.*)[\\S\\s]*Subject:(.*)[\\S\\s]*Time:(.*)[\\s]*([\\S\\s]*)");
	smatch matches;

	if (regex_search(email_content, matches, reg)) {
		em.from = matches[1];
		em.subject = matches[2];
		em.timestamp = matches[3];
		em.text = matches[4];
	}

	return em;
}

void print_email(email_t e) {
	printf("\r\nEmail #%s\r\n", e.id.c_str());
	printf("- - - - - - - - - - \r\n");
	printf("From:    %s\r\n", e.from.c_str());
	printf("To:      %s\r\n", e.to.c_str());
	printf("Subject: %s\r\n", e.subject.c_str());
	printf("Msg:     %s\r\n", e.text.c_str());
} 


