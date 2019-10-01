// file_server.cc
//
// FileServer class implementation
//

#include "file_server.h"
#include "../utils/utils.h"
#include <cstring>
#include <string.h>
#include <regex>

FileServer::FileServer(Quorum *q, string user) {
	this->quorum = q;
	this->user = user;
}

FS_STATUS FileServer::cd(string relative_path) {
	return append_relative_path(relative_path, &this->cwd);
}

FS_STATUS FileServer::ls(vector<string> *ret_val) {
	printf("FileServer::ls\n");
	string content = string();
	if (quorum->read(user, cwd, &content) != Q_OK) {
        fprintf(stderr, "FileServer::ls failed on read!\n");
		return FS_ERR;
	}

	split_string(content, "\n", ret_val);
	
	return FS_OK;
}

FS_STATUS FileServer::mkdir(string relative_path) {
    printf("FileServer::mkdir\n");
	string dir_path;
	if (append_relative_path(relative_path, &dir_path) != FS_OK) {
		return FS_ERR;
	}

    if (quorum->write(user, dir_path, "./\n") != Q_OK) {
        return FS_ERR;
    }
    
    return (add_to_listing(dir_path) == FS_OK) ? FS_OK : FS_ERR;	
}

FS_STATUS FileServer::rm(string relative_path) {
	printf("FileServer::rm\n");
	string dir_path;
	if (append_relative_path(relative_path, &dir_path) != FS_OK) {
		return FS_ERR;
	} else {
		return (quorum->erase(user, dir_path) == Q_OK) ? FS_OK : FS_ERR;
	}
}

FS_STATUS FileServer::mv(string from_relative_path, string to_relative_path) {
    printf("FileServer::mv\n");
	string from_path;
	string to_path;
	if (append_relative_path(from_relative_path, &from_path) != FS_OK) {
		return FS_ERR;
	}

	if (append_relative_path(to_relative_path, &to_path) != FS_OK) {
		return FS_ERR;
	}

	// Read current content
	string content;
	if (quorum->read(user, from_path, &content) != Q_OK) {
        fprintf(stderr, "FileServer::mv failed on read!\n");
		return FS_ERR;
	}
	
	// Write to new location
	if (quorum->write(user, to_path, content) != Q_OK) {
        fprintf(stderr, "FileServer::mv failed on write!\n");
		return FS_ERR;
	}

	// Delete from current location
	if (quorum->erase(user, from_path) != Q_OK) {
        fprintf(stderr, "FileServer::mv failed on erase!\n");
		return FS_ERR;
	}

    if (remove_from_listing(from_path) != FS_OK) {
        return FS_ERR;
    }

    if (add_to_listing(to_path) != FS_OK) {
        return FS_ERR;
    } 

	// Now recursively move every item in subtree
	vector<string> items;
	split_string(content, "\n", &items);
	for(vector<string>::iterator it = items.begin(); it != items.end(); ++it) {
		if (mv(from_path + *it, to_path + *it) != FS_OK) {
			return FS_ERR;
		}
	}

	return FS_OK;
}

FS_STATUS FileServer::upload(string relative_path, string data) {
    printf("FileServer::upload\n");
	// First get the absolute path
	string dir_path;
	if (append_relative_path(relative_path, &dir_path) != FS_OK) {
		return FS_ERR;
	}
	
    // Write the file to the absolute path
	if (quorum->write(user, dir_path, data) != Q_OK) {
        fprintf(stderr, "FileServer::upload failed on write!\n");
		return FS_ERR;
	}

    return add_to_listing(dir_path);
}

FS_STATUS FileServer::download(string relative_path, string *data) {
    printf("FileServer::download\n");
	string dir_path;
	if (append_relative_path(relative_path, &dir_path) != FS_OK) {
		return FS_ERR;
	} else {
		return (quorum->read(user, dir_path, data) == Q_OK) ? FS_OK : FS_ERR;
	}
}

FS_STATUS FileServer::remove_from_listing(string path) {
    printf("FileServer::remove_from_listing\n");
    // Extract the folder from the absolute path
	smatch matches;
	regex regx_dir("(^.*\\/[.+\\/]*)(.+)$");
	if (!regex_search(path, matches, regx_dir) || matches.size() < 3) {
        fprintf(stderr, "FileServer::remove_from_listing failed on regex_search!\n");
        return FS_ERR;
    }

    // Read the current folder listing
    string dir = string(matches[1]);
    string fname = string(matches[2]);
    string content;
    if (quorum->read(user, dir, &content) != Q_OK) {
        fprintf(stderr, "FileServer::remove_from_listing failed on read!\n");
        return FS_ERR;
    }

    content.replace(content.find(fname), fname.length(), "");

    // Overwrite the listing
    if (quorum->write(user, dir, content) != Q_OK) {
            fprintf(stderr, "FileServer::remove_from_listing failed on write!\n");
		    return FS_ERR;
    } else {
        return FS_OK;
    }
}

FS_STATUS FileServer::add_to_listing(string path) {
    printf("FileServer::add_to_listing\n");
    // We need to add it to the listing in its folder,
    // so extract the folder from the absolute path
	smatch matches;
	regex regx_dir("(^.*\\/[.+\\/]*)(.+)$");
	if (!regex_search(path, matches, regx_dir) || matches.size() < 3) {
        fprintf(stderr, "FileServer::add_to_listing failed on regex_search!\n");
        return FS_ERR;
    }

    // Read the current folder listing
    string dir = string(matches[1]);
    string fname = string(matches[2]);
    string content;
    quorum->read(user, dir, &content);

    // Check if the file already exists (i.e. we just
    // overwrote it). If it does, do nothing. Otherwise,
    // add it to the listing.
    if (content.find(fname) == string::npos) {
        content += fname + "\n";
        if (quorum->write(user, dir, content) != Q_OK) {
            fprintf(stderr, "FileServer::add_to_listing failed on write!\n");
		    return FS_ERR;
	    }
    } else {
        return FS_OK;
    }
}

FS_STATUS FileServer::append_relative_path(string relative_path, string *ret_val) {
    printf("FileServer::append_relative_path\n");
	*ret_val = this->cwd;
	if (relative_path.compare("/") == 0 || strlen(relative_path.c_str()) == 0) {
		*ret_val = "/";
		return FS_OK;
	}
	string new_relative_path = relative_path;
	smatch matches;
	regex regx_curr("^\\.\\/(.*)?");
	regex regx_back("^\\.\\.\\/(.*)?");
	regex regx_absp("^\\/.*");
	regex regx_mod_cwd("(.*\\/)?[\\s\\S]*\\/$");
	// see if it's an absolute path
	if (regex_search(relative_path, matches, regx_absp)) {
		*ret_val = relative_path;
		return FS_OK;
	}
	// check if it's ..
	if (regex_search(new_relative_path, matches, regx_back)) {
		// if we have something interesting after ../ match that
		// otherwise, just update the new relative path to nothing
		new_relative_path = (matches.size()==2 ? matches[1].str() : string(""));
		if (regex_search(*ret_val, matches, regx_mod_cwd)) {
			if (matches.size() == 2) {
				*ret_val = matches[1].str();
			}
		}
		fprintf(stderr, "%s\r\n", "BACK");
	//now check if it is a ./
	} else if (regex_search(new_relative_path, matches, regx_curr)) {
		new_relative_path = (matches.size() == 2 ? matches[1].str() : string(""));
		fprintf(stderr, "%s\r\n", "CURR");
	}

	if (strlen(new_relative_path.c_str())) {
		regex end_sl("/$");
		if (regex_search(*ret_val, matches, end_sl)) {
			*ret_val += new_relative_path;
		} else {
			*ret_val += string("/") + new_relative_path;
		}		
	}
	regex double_slash("//");
	*ret_val = regex_replace(*ret_val, double_slash, "/");

	if (strlen(ret_val->c_str()) == 0) *ret_val = "/";

	return FS_OK;
}

FileServer::~FileServer() {
	//deinit
}

