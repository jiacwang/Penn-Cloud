//database.h

//This is the database header file that will return data upon request
//and write data to the appropriate entry in the table. Also has includes
//a shutdown() method to shut the databse down and a ping() method to return 
//the status of the database
#include <string>

#ifndef DATABASE_H
#define DATABASE_H

class Database
{

public:
	Database();
	~Database();

private:
	std::string shutdown();
	std::string ping();
	std::string readSlice();
	std::string read(int row, int column);
	std::string getVersionNumber(int row, int column);
	std::string write(int row, int column, std::string data, int versionNumber);
};

#endif