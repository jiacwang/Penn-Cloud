#ifndef _HTML_HELPER_H_
#define _HTML_HELPER_H_

#include <string>
#include <string.h>
#include <vector>
#include <iterator>
#include <fstream>
#include <map>
using namespace std;

std::string html_content_for_data(std::vector<string> data);

std::string html_content_for_status(std::map<std::string,bool> status);

#endif