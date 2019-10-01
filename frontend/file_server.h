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

// The file server will be organized in the following way
// on the backend database. Each column name will be an 
// absolute path that either represents a folder or a file.



#ifndef _FILE_SERVER_H_
#define _FILE_SERVER_H_

#include <string>
#include <vector>
#include "frontend_defs.h"

using namespace std;
// enum for the return statuses of the FileServer, which 
// should be used to pass back errors and state if something
// went wrong during the function call
typedef enum {
	FS_OK, FS_ERR
} FS_STATUS;

typedef enum {
	STRING
} FS_SUPPORTED_FILETYPE;

class Quorum;

class FileServer {
public:
	FileServer(Quorum *q, string user);
	~FileServer();
	string cwd = "/";

	// change cwd
	// param relative_path: a path string with relation to cwd
	//						for example ".." should change cwd
	//						to the parent directory and return
	//						root if there is no parent. All other
	//						paths should begin with "./" since they
	//						are relative.
	FS_STATUS cd(string relative_path);

	// list the contents of cwd
	// param ret_val:   a pointer to a vacant location in memory where
	//					the caller expects the output list of 
	//					directory and/or filenames.
	FS_STATUS ls(vector<string> *ret_val);

	// makes a directory in the given place
	// param relative_path: see cd
	FS_STATUS mkdir(string relative_path);

	// removes whatever is at this path (whether it is a directory 
	// or a file)
	// param relative_path: see cd
	FS_STATUS rm(string relative_path);

	// moves whatever is at this path (whether it is a directory 
	// or a file)
	// param from_relative_path: see relative_path in cd
	// param to_relative_path:   see relative_path in cd
	FS_STATUS mv(string from_relative_path, string to_relative_path);

	// upload the given data from the blob
	// param relative_path: see cd
	// param data:			this is the content that
	//						the user would like placed at the relative
	//						path. Note that this must be a file.
	FS_STATUS upload(string relative_path, string data);

	// place the data from the relative path into the blob *
	// param relative_path: see cd
	// param data:			this is a pointer to a location in memory where
	//						the user would like placed the data from the
	//						given relative path. Note that this must be a file.
	FS_STATUS download(string relative_path, string *data);
	
private:
	Quorum *quorum;
	string user;

	FS_STATUS remove_from_listing(string path);

	FS_STATUS add_to_listing(string path);
	
	// Correctly modifies the cwd with the relative path 
	// and returns it in string. See cd for the full spec of what that
	// means. 
	// Note: DOES NOT modify the cwd.
	FS_STATUS append_relative_path(string relative_path, string *ret_val);
};

#endif
