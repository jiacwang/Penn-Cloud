#include "html_helper.h"
#include <regex>
using namespace std;

#define PATH_PREFIX "admin/html/"
string read_in_file(string path) {
	string content = string("");
	string n_path = string(PATH_PREFIX) + path;
	ifstream mail_header;
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


std::string html_content_for_data(std::vector<string> data) {
	string content = string("");
	content += read_in_file("slice_data_header.txt");
	for (auto it = begin(data); it != end(data); ++it) {
		content += string("\r\n<p>");
		content += string(*it);
		content += string("</p>\r\n<hr>\r\n");
		printf("Appending: %s\r\n", it->c_str());
	}
	content += read_in_file("slice_data_footer.txt");
	return content;
}

std::string html_content_for_status(std::map<std::string,bool> status) {
	string content = string("");
	content += read_in_file("system_status_header.txt");
	for (auto it = begin(status); it != end(status); ++it) {
		content += string("<a><p style=\"color:");
		content += string((it->second) ? "#26A65B" : "#F03434");
		content += string("\">O</p></a>");
		content += string("<a><input type=\"button\" name=\"database\" ");
		content += string("id=\"") + (it->first);
		content += string("\" value=\"database ") + (it->first);
		content += string("\" onclick=\"go_to_db(this)\"></a>");
		content += string("<a><input type=\"button\" "
			"value=\"                                                          \"></a>");
		content += string("<a><input type=\"button\" style=\"color:#F03434\" name=\"kill\"");
		content += string(" id=\"") + (it->first);
		content += string("\" value=\"shutdown\" onclick=\"kill_db(this)\"></a><hr>");
	}
	content += read_in_file("system_status_footer.txt");
	return content;
}
