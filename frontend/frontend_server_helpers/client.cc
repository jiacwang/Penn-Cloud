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
#include "client.h"


#define DEBUG_PRINTF(fmt, ...) \
		do { if (verbose_flag) {printf(fmt, __VA_ARGS__);}} while (0)

volatile static int verbose_flag = true;
using namespace std;
#define QUORUM_PATH_PREFIX "frontend/quorum/"
#define HTML_PATH_PREFIX "frontend/html/"
Client::Client(string cookie_name, string cookie) {
	this->cookie = cookie;
	this->cookie_name = cookie_name;
	this->quorum = new Quorum(string(QUORUM_PATH_PREFIX) + string("databases.txt"));
}

Client::~Client() {
	DEBUG_PRINTF("[DEBUG] Deinit for cookie %s\r\n", this->cookie.c_str());
}

void Client::respond_to(string request, int resp_fd) {
	if (!strlen(request.c_str())) {
		if (resp_fd) close(resp_fd);
		return;
		// silently do nothing
	}
	this->clientfd = resp_fd;
	if (this->test_run) {
		// this->triage_test(request); 
	} else {
		this->triage_http(request);
	}
}

//- - - - - - - - - - - - admin functions - - - - - - - - - - -
// creates a login for the client with the given username and password
void Client::signup(string username, string password) {
	//find out if user exists
	if (this->quorum->write(username, PASSWORD_COL, password) != Q_OK) {
		fprintf(stderr, "%s\n", "[ERROR] Writing the new user failed! Exiting...");
		exit(1);
	}
	if (this->quorum->write(username, MBOX_COL, "") != Q_OK) {
		fprintf(stderr, "%s\n", "[ERROR] Writing the new user inbox failed! Exiting...");
		exit(1);
	}
	if (this->quorum->write(username, TOP_LEVEL, "./\n") != Q_OK) {
		fprintf(stderr, "%s\n", "[ERROR] Writing / failed! Exiting...");
		exit(1);
	}
	this->login(username, password);
}

// logs a previously signed up client into their account
void Client::login(string username, string password) {
	DEBUG_PRINTF("[DEBUG] login(%s, %s)\r\n", username.c_str(), password.c_str());
	string correct_pass;
	QUORUM_STATE qs = this->quorum->read(username, PASSWORD_COL, &correct_pass);
	switch (qs) {
		case Q_ERR_NO_SUCH_USER:
			DEBUG_PRINTF("[DEBUG] ERR: No such user %s\r\n", username.c_str());
		case Q_OK:
			if (password.compare(correct_pass) == 0) {
				this->username = username;
				this->authenticated = true;
				this->fileserver = new FileServer(this->quorum, this->username);
				this->mailserver = new MailServer(this->quorum, this->username);
			}
			break;
		default:
			break;
	}
}

//- - - - - - - - - - - - SMTP functions - - - - - - - - - - -
void Client::send_email(string to, string from, string msg_text) {
	string subject = "";
	this->mailserver->send(from, to, subject, msg_text);
	string response = http_found_redirect("/mail.html");
	write(this->clientfd, response.c_str(), strlen(response.c_str()) * sizeof(char));
	close(this->clientfd);
	// this->http_get("/mail.html");
}

void Client::list_emails() {
	DEBUG_PRINTF("[DEBUG] %s\r\n", "listing emails");
	vector<email_t> emails = this->mailserver->get_inbox();
	string content = html_content_for_mail_list(emails);
	string response = http_ok_response(content, this->cookie_name, this->cookie);
	write(this->clientfd, response.c_str(), strlen(response.c_str()) * sizeof(char));
}

void Client::display_email(string email_id) {
	email_t full_email = this->mailserver->get_email(email_id);
	string content = html_content_for_email(full_email);
	string response = http_ok_response(content, this->cookie_name, this->cookie);
	write(this->clientfd, response.c_str(), strlen(response.c_str()) * sizeof(char));
}

//- - - - - - - - - - - - FILE functions - - - - - - - - - - -
// Creates a directory at the given path if one doesn't exist
void Client::file_make_directory(string local_path) {
	DEBUG_PRINTF("[DEBUG] file_make_directory(%s)\r\n", local_path.c_str());
	string mkpath = local_path;
	regex reg_endslash(".*/$");
	smatch matches;
	if (!regex_search(local_path, matches, reg_endslash)) {
		mkpath += string("/");
	}
	if (this->fileserver->mkdir(mkpath) != FS_OK) {
		fprintf(stderr, "%s\n", "[ERROR] mkdir failed in file_make_directory");
	}
	this->file_change_to_directory(mkpath);
}

// Moves the working directory to the one with the local path
// relative
void Client::file_change_to_directory(string local_path) {
	DEBUG_PRINTF("[DEBUG] change to directory ./%s\r\n", local_path.c_str());
	this->fileserver->cd(local_path);
	vector<string> lst;
	this->fileserver->ls(&lst);
	string content = html_content_for_dir_lst(lst, fileserver->cwd);
	string resp = http_ok_response(content, this->cookie_name, this->cookie);
	write(this->clientfd, resp.c_str(), strlen(resp.c_str()) * sizeof(char));
}

void Client::file_download(string filename) {
	string file;
	if (this->fileserver->download(filename, &file) != FS_OK) {
		fprintf(stderr, "%s\n", "[ERROR] Failed to download");
	}

	string resp = http_ok_post(file, filename, this->cookie_name, this->cookie);
	// DEBUG_PRINTF("[DEBUG] S: \'%s\'\r\n", resp.c_str());
	write(this->clientfd, resp.c_str(), strlen(resp.c_str()) * sizeof(char));
}

// creates a file with the give name (this can also be a path)
void Client::file_upload(string filename) {
	DEBUG_PRINTF("[DEBUG] TODO: file_upload(%s)\r\n", filename.c_str());
	//TODO
}

void Client::triage_http(string request) {
	DEBUG_PRINTF("[DEBUG] Received from client: \r\n\'%s\'\r\n", request.c_str());
	smatch matches;
	regex regx_get("GET\\s([^\\s]*)\\sHTTP.*");
	regex regx_post("POST[\\s\\S]*boundary=(.*)\\r\\n");

	if (regex_search(request, matches, regx_get)) {
		if (matches.size() != 2) {
			fprintf(stderr, "Received a malformatted GET request. Exiting...\r\n");
			exit(1);
		}
		
		string path = matches[1].str();
		this->http_get(path);
	} else if (regex_search(request, matches, regx_post)) {
		if(matches.size() != 2) {
			fprintf(stderr, "%s\n", "ERR! post matched less than 2 items.");
		}
		string boundary = matches[1].str();
		regex regx_boundary(boundary + string("[\\s\\S]*") + boundary + 
			string("([\\s\\S]*)--") + boundary + string(".*\r\n"));
		if (!regex_search(request, matches, regx_boundary)){
			fprintf(stderr, "%s\n", "ERR! Tried to post with nothing.");
			exit(1);
		}
		this->http_post(matches[1]);
	}
}

void Client::http_get(string input_path) {
	DEBUG_PRINTF("[DEBUG] %s\r\n", "HTTP GET Detected...");
	regex mail_id("/read_email:(\\-*\\d*)");
	regex mail_send("/to=([@\\w]*):text=(.*)");
	regex file_cd("/to_dir=(.*)");
	regex file_dow("/download=(.*)");
	regex file_mkdir("/new_folder=(.*)");
	regex regx_login("uname=([\\w\\d]*):pass=([\\S]*)");
	regex regx_signup("new_uname=([\\w\\d]*):new_pass=([\\S]*)");
	smatch matches;

	if (regex_search(input_path, matches, regx_login)) {
		if (matches.size() < 3) {
			fprintf(stderr, "%s\n", "ERR! Login matched less than 3 items.");
			exit(1);
		}
		string uname = matches[1].str();
		string pass = matches[2].str();
		this->login(uname, pass);
		if (this->authenticated) {
			http_get("/choose.html");
		} else {
			http_get("/landing_page_user_not_found.html");
		}
	}

	if(regex_search(input_path, matches, regx_signup)) {
		if (matches.size() < 3) {
			fprintf(stderr, "%s\n", "ERR! Signup matched less than 3 items.");
			exit(1);
		}
		string uname = matches[1].str();
		string pass = matches[2].str();
		this->signup(uname, pass);
		if (this->authenticated) {
			http_get("/choose.html");
			return;
		} else {
			http_get("/landing_page_user_not_found.html");
		}
	}

	if (!this->authenticated) {
		// we are not authenticated, make sure the user is not trying something fishy.
		if (input_path.compare("/landing_page_user_not_found.html") != 0) {
			input_path = "/";
		}
	}
	if (input_path.find(string("mail.html")) != string::npos) {
		//mail
		this->list_emails();
		return;
	}
	if (input_path.find(string("drive.html")) != string::npos) {
		this->file_change_to_directory("/");
		return;
	}

	if (regex_search(input_path, matches, mail_id)) {
		if (matches.size() < 2) {
			fprintf(stderr, "%s\n", "Unknown email format");
			exit(1);
		}
		string email_id = matches[1].str();
		this->display_email(email_id);
		return;
	}

	if (regex_search(input_path, matches, mail_send)) {
		if (matches.size() < 3) {
			fprintf(stderr, "%s\n", "Unknown email format");
			exit(1);
		}
		string to = matches[1].str();
		string msg = matches[2].str();
		regex space_chars("%20");
		regex left_angle("%3C");
		regex right_angle("%3E");
		msg = regex_replace(msg, space_chars, " ");
		msg = regex_replace(msg, left_angle, "<");
		msg = regex_replace(msg, right_angle, ">");
		string from = this->username + string("@localhost");
		this->send_email(to, from, msg);
		return;
	}

	if (regex_search(input_path, matches, file_cd)) {
		if (matches.size() < 2) {
			fprintf(stderr, "%s\n", "File CD did not find a directory.");
			exit(1);
		}
		string local_path = matches[1].str() + string("/");
		this->file_change_to_directory(local_path);
		return;
	}

	if (regex_search(input_path, matches, file_dow)) {
		if (matches.size() < 2) {
			fprintf(stderr, "%s\n", "File CD did not find a directory.");
			exit(1);
		}
		string local_path = matches[1].str() ;
		this->file_download(local_path);
		return;
	}

	if (regex_search(input_path, matches, file_mkdir)) {
		if (matches.size() < 2) {
			fprintf(stderr, "%s\n", "File MKDIR did not find a filename.");
			exit(1);
		}
		string local_path = matches[1].str();
		this->file_make_directory(local_path);
		return;
	}

	string path = string(HTML_PATH_PREFIX) + input_path;
	if (path.compare(string(HTML_PATH_PREFIX) + string("/")) == 0) {
		if (this->authenticated) {
			path = string(HTML_PATH_PREFIX) + string("choose.html");
		} else {
			path = string(HTML_PATH_PREFIX) + string("landing_page.html");
		}
	}

	ifstream http_file;
	http_file.open(path.c_str());
	string send_buff = "";
	if (!http_file) {
		string npath = string(HTML_PATH_PREFIX) + string("404.html");
		http_file.open(npath.c_str());
		string content = "";
		string line;
		while (getline(http_file, line)) {
			content += line;
			content += "\r\n";
		}
		send_buff = http_not_found_response(content, this->cookie_name, this->cookie);
		http_file.close();
	} else {
		string html_content = "";
		string line;
		while (getline(http_file, line)) {
			html_content += line;
			html_content += "\r\n";
		}
		send_buff = http_ok_response(html_content, this->cookie_name, this->cookie);
		http_file.close();
	}

	DEBUG_PRINTF("[DEBUG] %s\r\n", "Webserver responding to Client");

	write(this->clientfd, send_buff.c_str(), strlen(send_buff.c_str()) * sizeof(char));
	close(this->clientfd);
}

void Client::http_post(string content) {
	DEBUG_PRINTF("[DEBUG] %s\r\n", "HTTP POST Detected");
	regex regx_filename("filename=\"(.*)\"");
	smatch matches;
	if (!regex_search(content, matches, regx_filename)) {
		fprintf(stderr, "%s\n", "[ERROR] Could not find filename for posted file!");
		exit(1);
	}
	string filename = matches[1].str();
	this->fileserver->upload(filename, content);
	this->http_get("/drive.html");
}
