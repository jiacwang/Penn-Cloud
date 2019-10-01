#ifndef _HTML_HELPER_H_
#define _HTML_HELPER_H_

#include <string>
#include <string.h>
#include <vector>
#include <iterator>
#include "../frontend_defs.h"
#include <fstream>

std::string html_content_for_mail_list(std::vector<email_t> mail_info);

std::string html_content_for_email(email_t email);

std::string html_content_for_dir_lst(std::vector<std::string> lst, std::string cwd);

std::string html_content_for_post();
#endif