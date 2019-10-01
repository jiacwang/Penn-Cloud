#include "utils.h"

void split_string(string s, string delimiter, vector<string> *ret_val) {
  size_t pos = 0;
	string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
			token = s.substr(0, pos);
			(*ret_val).push_back(token);
			s.erase(0, pos + delimiter.length());
	}
	(*ret_val).push_back(s);
}