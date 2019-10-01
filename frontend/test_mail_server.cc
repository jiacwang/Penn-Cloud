#include "mail_server.h"
#include "quorum/quorum.h"

int main(int argc, char* argv[]) {
	Quorum *q = new Quorum("servers.txt");
	MailServer *ms = new MailServer(q, "user_1");

	//Send to user
	ms->send("sally@localhost", "user_1@localhost", "Test", "Hello this is a test okay bye!");
	vector<email_t> email_list = ms->get_inbox();
	printf("%s %d\n", email_list[0].id.c_str(),  std::stoi(email_list[0].id.c_str()));
	email_t email = ms->get_email(std::stoi(email_list[0].id.c_str()));
	
	printf("From: %s, To: %s, Subject: %s, Text: %s\n", email.from.c_str(), email.to.c_str(), email.subject.c_str(), email.text.c_str());
	

}