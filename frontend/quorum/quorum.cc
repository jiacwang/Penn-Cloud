#include "quorum.h"
#include <fstream>
#include <sstream>
#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "../../database/database.grpc.pb.h"
#include "../../utils/utils.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using namespace std;

int numDatabases;
map<int, vector<string>> databaseLookUpTable;
map<int, vector<unique_ptr<Database::Stub>>> databaseStubs;
int num_groups = 0;

Quorum::Quorum(string config) {
	fstream inFile;
	string line;
	string token;
	char delim = ',';
	int group_number = 0;

	inFile.open(config);
	if (!inFile) {
		cerr << "Unable to open file " << config << endl;
		exit(1);
	} else {
		while (!inFile.eof()) {
			getline(inFile, line);
			stringstream ss(line);
			vector<string> ip_list;
			while (getline(ss, token, delim)) {
				ip_list.push_back(token);
				// Right now database is on "localhost:50051"
				shared_ptr<Channel> channel = grpc::CreateChannel(token, grpc::InsecureChannelCredentials());
				databaseStubs[group_number].push_back(Database::NewStub(channel));
			}

			databaseLookUpTable.insert(pair<int, vector<string>>(group_number, ip_list));
			group_number++;
		}
		num_groups = group_number; 
		inFile.close();
	}
} 

Quorum::~Quorum() {}

QUORUM_STATE Quorum::erase(string user, string column) {
	std::hash<std::string> str_hash;
    int group = str_hash(user) % num_groups;
	vector<string> ip = databaseLookUpTable[group];

	int ack = 0;
	for (int i = 0; i < ip.size(); i++) {
		ClientContext context;
		Success success;
		Location location;

		location.set_row(user);
		location.set_col(column);
		databaseStubs[group][i]->erase(&context, location, &success);
		if (success.success()) {
			ack++;
		}
	}

	if (ack == ip.size()) {
		return Q_OK;
	} else {
		return Q_ERR_COULD_NOT_DELETE; 
	}
}

QUORUM_STATE Quorum::read(string user, string column, string *ret_val) {
    ClientContext context;
    std::hash<std::string> str_hash;
    int group = str_hash(user) % num_groups;
	vector<string> ip = databaseLookUpTable[group];
	int db_read = rand() % ip.size();

	Cell cell;
	Location location;
	location.set_row(user);
	location.set_col(column);
	Status s = databaseStubs[group][db_read]->read(&context, location, &cell);

	if (s.ok()) {
		string content = cell.contents();
		//if (column.compare(PASSWORD_COL) == 0) *ret_val = string("password");
		//printf("content: %s\n", content.c_str());
		*ret_val = string(content);
		return Q_OK;
	} else {	
    	return Q_ERR_COULD_NOT_READ;
    }
}

QUORUM_STATE Quorum::write(string user, string column, string data) {
    std::hash<std::string> str_hash;
    int group = str_hash(user) % num_groups;
	vector<string> ip = databaseLookUpTable[group];
	int max_version = 0;


	for (int i = 0; i < ip.size(); i++) {
		ClientContext context;
		Cell cell;
		Location location;
		location.set_row(user);
		location.set_col(column);

		databaseStubs[group][i]->getVersionNumber(&context, location, &cell);
		int version = cell.version();
		if (version > max_version) {
			max_version = version;
		}
	}

	int new_version = max_version + 1;
	int ack = 0;
	for (int i = 0; i < ip.size(); i++) {
		ClientContext context;
		Success success;
		WriteRequest wr;
		Cell cell;
		Location location;

		location.set_row(user);
		location.set_col(column);
		cell.set_version(new_version);
		cell.set_contents(data);
        
		*wr.mutable_loc() = location;
		*wr.mutable_cell() = cell;

		databaseStubs[group][i]->write(&context, wr, &success);
		if (success.success()) {
			ack++;
		}
	}

	if (ack > (ip.size() / 2)) {
		return Q_OK;
	} else {
		return Q_ERR_COULD_NOT_WRITE; 
	}
}

void Quorum::printLookUpTable() {
	map<int, vector<string>>::iterator it;
	for (it = databaseLookUpTable.begin(); it != databaseLookUpTable.end(); it++) {
		cout << it->first << ":";
		for (int i = 0; i < it->second.size(); i++) {
			cout << it->second.at(i);
		}
		cout << endl;
	}
}
