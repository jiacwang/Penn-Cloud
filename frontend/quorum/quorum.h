//quorum.h

//This is the quorum header file that will return data to the front-end server, 
//either data from the database upon a read request or a confirmation message
//that data has been successfully written to the database upon a write request
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <string>


#ifndef QUORUM_H
#define QUORUM_H

typedef enum {
	Q_ERR_NO_SUCH_USER, Q_OK, Q_ERR_COULD_NOT_WRITE, Q_ERR_COULD_NOT_DELETE, Q_ERR_COULD_NOT_READ
} QUORUM_STATE;

#define TOP_LEVEL "/"
#define PASSWORD_COL "PASSWORD"
#define MBOX_COL "MBOX"

class Quorum {
	
public:
	Quorum(std::string config);
	~Quorum();
	void printLookUpTable();
	// This function is called from the frontend server and reads from the database using
	// a quorum coherency strategy
	//
	// param user:    the username of the currently logged-in client (used for the row)
	// param column:  the column from which data should be read
	// param ret_val: the data that is read from the column should be placed into this pointer
	//
	// returns the state of the quorum upon reading, including any errors
	QUORUM_STATE read(std::string user, std::string column, std::string *ret_val);

	// This function is called from the frontend server and writes to the database using
	// a quorum coherency strategy
	//
	// param user:    the username of the currently logged-in client (used for the row)
	// param column:  the column to which data should be written
	// param ret_val: the data that should be written to the (row, column)
	//
	// returns the state of the quorum upon writing, including any errors
	QUORUM_STATE write(std::string user, std::string column, std::string data);

	// This function is called from the frontend server and deletes from the database using
	// a quorum coherency strategy
	//
	// param user:    the username of the currently logged-in client (used for the row)
	// param column:  the column from which data should be deleted
	//
	// returns the state of the quorum upon writing, including any errors
	QUORUM_STATE erase(std::string user, std::string column);
};

#endif