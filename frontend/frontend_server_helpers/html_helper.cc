#include "html_helper.h"
#include <regex>

#define PATH_PREFIX "frontend/html/"

string read_in_file(string path) {
	string content = string("");
	ifstream mail_header;
	string n_path = string(PATH_PREFIX) + path;
	mail_header.open(n_path.c_str());
	if (!mail_header) {
		fprintf(stderr, "No %s. Exiting...\r\n", path.c_str());
		exit(1);
	}
	string line;
	while(getline(mail_header, line)) {
		content += line;
		content += string("\r\n");
	}
	mail_header.close();
	return content;
}

string html_content_for_mail_list(vector<email_t> mail_info) {
	string content = string("");
	content += read_in_file("mail_header.txt");
	for (auto it = begin(mail_info); it != end(mail_info); ++it) {
		string dest = string("<!-- Begin Email -->\r\n"
			"    <a>\r\n"
			"        <b>\r\n"
			"            <input type=\"button\" name=\"mail\" id=\"") + it->id + 
		string("\" value=\"READ\" onclick=\"redirect_to_email_id(this)\">\r\n"
			"        </b>\r\n"
			"        <b>\r\n"
			"            <h2>") + string("from") + string("</h2>\r\n"
			"        </b>\r\n"
			"        <b>\r\n"
			"            <p>") + it->from + string("</p>\r\n"
			"        </b>\r\n"
			"    </a>\r\n"
			"    <hr>\r\n"
			"<!-- End Email -->\r\n");
		content += dest;
	}
	content += read_in_file("mail_footer.txt");
	return content;
}

string html_content_for_email(email_t email) {
	string content = string("");
	content += read_in_file("single_email_header.txt");
	content += string("<h1>Email #") + email.id + string("</h1>\r\n");
    content += string("<h2> from: ") + email.from + string("</h2>\r\n");
    content += string("<hr>\r\n");
    content += string("<p>") + email.text + string("</p>");
	content += read_in_file("single_email_footer.txt");
	return content;
}

bool is_directory(string name) {
	regex dir(".*/$");
	smatch matches;
	return regex_search(name, matches, dir);
}

string html_content_for_dir_lst(vector<string> lst, string cwd) {
	string content = string();
	content += read_in_file("drive_header.txt");

	//create navbar
	content += string("<div class=\"navbar\"><h1>") + cwd + string("</h1></div>\r\n");

	//start div classes
	content += string("<div class=\"main\">\r\n<div class=\"directory\">\r\n");
	content += string("<input type=\"button\" name=\"directory\" id=\"..\" "
		"value=\"../\" onclick=\"go_to_dir(this)\"><hr>\r\n");
	
	for (auto it = begin(lst); it != end(lst); ++it) {
		if (strlen(it->c_str()) == 0) continue;
		string name = string(is_directory(*it) ? "directory" : "file");
		string onclick_ins = string(is_directory(*it) ? "go_to_dir" : "download");
		content += string("<input type=\"button\" ");
		content += string("name=\"") + name;
		content += string("\" id=\"") + *it;
		content += string("\" value=\"") + *it;
		content += string("\" onclick=\"") + onclick_ins + "(this)\"><hr>";
	}
	
	content += read_in_file("drive_footer.txt");
	return content;
}

string html_content_for_post() {
	string content = string();
	content += read_in_file("example_post.txt");
	return content;
}
