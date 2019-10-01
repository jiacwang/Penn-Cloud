#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <cstdint>
#include <pthread.h>
#include <unistd.h>
#include <mutex>
#include <map>
#include <sys/time.h>
#include <sstream>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include "database.grpc.pb.h"
#include "../utils/utils.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using namespace std;

using map_t = map<string, map<string, Cell>>;

// string delimiter = "-  493c739bb422fa10170f954ae2da542392a1dc78eeac1ac05489bcc6ee7f6241  -";
string elt_delimiter = "-  493c739bb422fa10170f954ae2da542392a1dc78eeac1ac05489bcc6ee7f6241  -";
string map_delimiter = "-  493c739bb422fa10ienbk54ae2da542392a1dc78eeac1ac05489bcc6ee7f6241  -";
string cell_delimiter = "-  493c739bb422fa10170f95i2kdmbiaa392a1dc78eeac1ac05489bcc6ee7f6241  -";
string nl_delimiter = "-  493c739bb422fa10170f954ae2da582892a1dc78eeac1ac05489bcc6ee7f6241  -";
string BACKUP_FNAME;
vector<string> group_servers;
string ip;

struct {
    bool debug;          /* -v option */
    string file;
    int row;
    int idx;
} globalArgs;
static const char *optString = "v";

//
// For debugging
//
string get_prefix() {
    // Generate prefix
    struct timeval tval;
    gettimeofday(&tval, NULL);
    tm *now = localtime(&tval.tv_sec);
    char buf1[20];
    strftime(buf1, 20, "%H:%M:%S.", now);
    char buf2[20];
    // sprintf(buf2, "%06ld S(%02d,%02d)", (long int) tval.tv_usec, globalArgs.row, globalArgs.idx);
    sprintf(buf2, "%06ld S<%s>", (long int) tval.tv_usec, ip.c_str());
    return string(buf1) + string(buf2);
}

void print_debug(string s) {
    if (globalArgs.debug) {
        // Add prefix and print
        string output = get_prefix() + " " + s;
        printf("%s\n", output.c_str());
    }
}

//
// For parsing command-line arguments
//
int parse_args(int argc, char *argv[]) {
    int opt = 0;
    globalArgs.debug = false;
    globalArgs.file = "";
    globalArgs.idx = 0;

    opt = getopt(argc, argv, optString);
    while (opt != -1) {
        switch (opt) {
            case 'v':
                globalArgs.debug = true;
                break;
            default:
                /* Shouldn't get here. */
                fprintf(stderr, "Invalid argument\n");
                exit(1);
        }
        opt = getopt(argc, argv, optString);
    }

    globalArgs.file = argv[argc - 3];
    globalArgs.row = atoi(argv[argc - 2]);
    globalArgs.idx = atoi(argv[argc - 1]);
    return 0;
}

void parse_servers() {
    // Format we're expecting:
    // ./database_server servers.txt 2 3
    string line;
    ifstream f(globalArgs.file);
    printf("Opening Database File: %s\r\n", globalArgs.file.c_str());
    if (f.is_open()) {
        for (int currline = 0; currline < globalArgs.row; ++currline) {
            getline(f, line);
            print_debug("Line " + to_string(currline) + " " + line);
        }

        split_string(line, ",", &group_servers);
        int idx = globalArgs.idx;
        ip = group_servers[idx - 1];
        BACKUP_FNAME = "backups/" + ip + "-backup.ckpt";
        group_servers.erase(group_servers.begin() + idx - 1);

        f.close();
    } else {
        fprintf(stderr, "%s\n", "[ERROR] Could not open file");
        // exit(1);
    }
}

bool operator!=(const Cell& lhs, const Cell& rhs) {
    return (lhs.contents() != rhs.contents() || lhs.version() != rhs.version());
}

class DatabaseImpl final : public Database::Service {
  private:
    int frequency = 60;
    mutex mtx;
    map_t db;
    map_t checkpoint_db;

    map_t map_difference(map_t& new_map, map_t& old_map) {
        map_t ret_val;
        for (auto const& t : new_map) {
            if ((old_map).find(t.first) == (old_map).end()) {
                ret_val[t.first] = t.second;   
            } else {
                for (auto const& q : t.second) {
                    if ((old_map)[t.first].find(q.first) == (old_map)[t.first].end() ||
                        (old_map)[t.first][q.first] != (new_map)[t.first][q.first]) {
                        ret_val[t.first][q.first] = q.second;
                    }
                }
            }
        }

        return ret_val;  
    }

    void serialize_map(string fname, map_t& m) {
        string output;

        // Format of a line:
        // username elt_delimiter elt1 map_delimiter cell_contents cell_delimiter cell_version elt_delimiter ... 
        // so for example (spaces added for readability):
        // gandhit \t mbox : blah blah , 12 \t file1 : asdf , 5 \n
        for (auto const& row : m) {
            // printf("Row name: %s\n", row.first.c_str());
            output += row.first;

            for (auto const& elt : row.second) {
                output += elt_delimiter;
                output += elt.first;
                output += map_delimiter;
                output += elt.second.contents();
                output += cell_delimiter;
                output += to_string(elt.second.version());    
            }

            output += nl_delimiter;
        }
        output = output.substr(0, output.length() - nl_delimiter.length());

        // Write to the file
        // printf("Here's what I'm about to write: \n%s \n", output.c_str());
        ofstream out_file(fname);
        out_file << output;
    };

    void deserialize_map(string fname, map_t& m) {
        ifstream file_stream{fname};

        // Failed to open
        if (file_stream.fail()) { return; }

        ostringstream ss{};
        file_stream >> ss.rdbuf(); 

        // Failed to read
        if (file_stream.fail() && !file_stream.eof()) { return; }

        string s = ss.str();

        // First we get all the rows into a vector
        vector<string> rows;
        split_string(s, nl_delimiter, &rows);

        for (auto const& row : rows) {
            // Now we get all elements in the row
            vector<string> elts;
            split_string(row, elt_delimiter, &elts);

            map<string, Cell> row_map;
            // Skip the first element, as that's the row name (username)
            for (vector<string>::iterator elt = elts.begin() + 1; elt != elts.end(); ++elt) {
                // Separate out the map: col : contents , version
                vector<string> kv;
                split_string(*elt, map_delimiter, &kv);
                vector<string> cell_vec;
                split_string(kv[1], cell_delimiter, &cell_vec);
                Cell cell;
                cell.set_contents(cell_vec[0]);
                cell.set_version(atoi(cell_vec[1].c_str()));
                row_map[kv[0]] = cell;
            }

            // printf("Just parsed %s \n", elts[0].c_str());
            m[elts[0]] = row_map;
        }

        // return str_stream.str();
    }

    static void *backup_worker(void *arg) {
        DatabaseImpl* pThis = (DatabaseImpl*) (arg);

        while (1) {
            sleep(pThis->frequency);
            print_debug("Database backup service running!");
            pThis->mtx.lock();
            print_debug("Got the lock");
            // Get just the forward difference for now (so don't count deletions)
            map_t curr_map;
            curr_map.insert(pThis->db.begin(), pThis->db.end());
            pThis->mtx.unlock(); // We don't want to hold the lock longer than necessary
            print_debug("Released the lock");
            // map_t diff_map = pThis->map_difference(curr_map, pThis->checkpoint_db);

            // Write diff_map to checkpoint file
            // Also calculate hash

            pThis->serialize_map(BACKUP_FNAME, curr_map);
        }
    }

  public:
    explicit DatabaseImpl() {
        // Initialize db with some values
        vector<string> usernames = {"user_1", "user_2", "user_3"};
        vector<string> passwords = {"hello", "hi", "potato"};
        vector<string> listings = {"./\n", "./\n", "./\n"};
        vector<string> mboxes = {"", "", ""};
        vector<int> versions = {1, 1, 12};
        for (int i = 0; i < usernames.size(); ++i) {
            Cell cell1, cell2, cell3;
            // Set content fields
            cell1.set_contents(passwords[i]);
            cell2.set_contents(listings[i]);
            cell3.set_contents(mboxes[i]);

            // Set version numbers
            cell1.set_version(versions[i]);
            cell1.set_version(versions[i]);
            cell1.set_version(versions[i]);

            // Set in database
            db[usernames[i]]["PASSWORD"] = cell1;
            db[usernames[i]]["/"] = cell2;
            db[usernames[i]]["MBOX"] = cell3;
        }

        deserialize_map(BACKUP_FNAME, db);

        pthread_t backup_thread;
        pthread_create(&backup_thread, NULL, backup_worker, this);
    }

    Status shutdown(ServerContext* context, const Empty* empty, Success* success) override {
        exit(0);
        success->set_success(false);
        return Status::CANCELLED;
    }

    Status ping(ServerContext* context, const Empty* empty, Success* success) override {
        success->set_success(true);
        return Status::OK;
    }

    Status read(ServerContext* context, const Location* loc, Cell* cell) override {
        mtx.lock();
        if (db.find(loc->row()) == db.end() || db[loc->row()].find(loc->col()) == db[loc->row()].end()) {
            // Entry doesn't exist
            mtx.unlock();
            return Status::CANCELLED;
        }
        cell->set_version(db[loc->row()][loc->col()].version());
        cell->set_contents(db[loc->row()][loc->col()].contents());
        mtx.unlock();
        return Status::OK;
    }

    Status getVersionNumber(ServerContext* context, const Location* loc, Cell* cell) override {
        mtx.lock();
        cell->set_version(db[loc->row()][loc->col()].version());
        mtx.unlock();
        return Status::OK;
    }

    Status write(ServerContext* context, const WriteRequest* wr, Success* success) override {
        mtx.lock();
        Cell* cell = &db[wr->loc().row()][wr->loc().col()];
        cell->set_version(wr->cell().version());
        cell->set_contents(wr->cell().contents());
        mtx.unlock();
        success->set_success(true);
        return Status::OK;
    }

    Status cput(ServerContext* context, const CPutRequest* cr, Success* success) override {
        mtx.lock();
        Cell* cell = &db[cr->loc().row()][cr->loc().col()];
        if (cell->contents() == cr->curr().contents()) {
            cell->set_version(cr->new_val().version());
            cell->set_contents(cr->new_val().contents());
            success->set_success(true);
        } else {
            success->set_success(false);
        }

        return success->success() ? Status::OK : Status::CANCELLED;
    }

    Status erase(ServerContext* context, const Location* loc, Success* success) override {
        mtx.lock();
        db[loc->row()].erase(loc->col());
        if (db[loc->row()].size() == 0) {
            db.erase(loc->row());
        }
        mtx.unlock();
        success->set_success(true);
        return Status::OK;
    }

    Status getRows(ServerContext* context, const RowRange* rr, Tablet* tablet) override {
        mtx.lock();
        if (rr->start_row() >= db.size()) {
            fprintf(stderr, "DatabaseServer::getRows - start_row out of range!\n");
            mtx.unlock();
            return Status::CANCELLED;
        }

        auto& tablet_map = *tablet->mutable_tablet();
        map_t::iterator it = next(db.begin(), int(rr->start_row()));
        map_t::iterator range_end = next(db.begin(), int(rr->end_row()) + 1);
        map<string, Row> output_map;

        for (; it != db.end() && it != range_end; ++it) {
            auto row = Row::default_instance();
            auto& map = *row.mutable_row();
            for (auto const& elt : it->second) {
                map[elt.first] = elt.second;
            }
            tablet_map[it->first] = row;
        }

        mtx.unlock();
        return Status::OK;
    }
};

void RunServer() {
    string server_address(ip);
    DatabaseImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    print_debug("Server listening on " + server_address);
    server->Wait();
}

int main(int argc, char** argv) {
    // Expect only arg: --db_path=path/to/route_guide_db.json.
    // std::string db = routeguide::GetDbFileContent(argc, argv);

    if (argc < 4) {
        fprintf(stderr, "Incorrect number of arguments!\n");
        exit(-1);
    }

    parse_args(argc, argv);
    parse_servers();
    RunServer();

    return 0;
}


