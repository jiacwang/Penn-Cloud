#include "quorum.h"
#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "../../database/database.grpc.pb.h"
#include "../../utils/utils.h"
#include <string>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

int main(int argc, char* argv[]) {
	Quorum *q = new Quorum("databases.txt");

	//Read
	string *buf = new string("");
	q->read("user_1", "PASSWORD", buf);
	printf("initial read: %s\n", (*buf).c_str());

	//Write
	int stat = q->write("user_1", "PASSWORD", "changed");
	printf("write stat: %d\n", stat);

	//Read again
	buf = new string("");
	q->read("user_1", "PASSWORD", buf);
	printf("read after write: %s\n", (*buf).c_str());

	//Delete
	int stat_del = q->erase("user_1", "PASSWORD");
	printf("delete stat: %d\n", stat_del);

	//Read again
	buf = new string("");
	q->read("user_1", "PASSWORD", buf);
	printf("read after delete: %s\n", (*buf).c_str());

	delete buf;
}