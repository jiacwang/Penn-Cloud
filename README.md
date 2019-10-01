# CIS 505 Project by T24

# About: 
## Load Balancer (`frontend/load_balancer.h,.cc`)

## Frontend (`frontend/`)
The frontend server's responsibility is responding to HTTP and HTML requests from web clients. It consists of `frontend.cc`, which has the main function. This spawns a `Server` instance (`frontend_server_helpers/server.h,.cc`), which will bind to the address passed in, and each time a web client connects, it will spawn a `Client` (`frontend_server_helpers/client.h,.cc`). This client is stored in the `Server` instance with a map from the cookie, which is randomly generated, to the `Client` instance. This allows state to be persistent when the same client connects multiple times in a single session. Each `Client` creates instances of `MailServer` (`mail_server.h,.cc`) and `FileServer` (`file_server.h,.cc`), which it uses for communicating with the database about anything that pertains specifically to those functions. 

## Mail Server
The mail server is located at `frontend/mail_server.cc`. It has the functions for maintaining the state of the mail server in the database (using the quorum as the abstraction layer to the database). The frontend hooks into it to perform mail server actions.

### File Server
The file server is located at `frontend/file_server.cc`. It has the functions for maintaining the state of the file system in the database (using the quorum as the abstraction layer to the database). The frontend hooks into it to perform file system actions.

### Quorum
The quorum, located at `frontend/quorum/quorum.cc`, is responsible for being the middleware between things like the login portal, file server, and mail server and the databases. It is responsible for deciding which replication group to read/write to for a given request and maintaing quorum replication. 

## Database (`database/`)
The database server (located at `database/database_server.cc`) sets up a server and listens for RPC calls to read/write to its database state. It makes local backups to a file in the `./backups/` folder, with its file using the name `ip:port-backup.ckpt`.

## Admin
The admin server connects to the databases. 

# Launching: 
To launch the system, navigate to the top level directory and run `python launch_system.py` this will run the following commands: 

### The make commands
`make clean`  
`make all`  
### Launching the databases
`database/database_server <database servers file> <row> <column>`  
The `<database servers file>` should be a file in the format:  
`<port>:<ip>,<port>:<ip>,<port>:<ip>...`  
`<port>:<ip>,<port>:<ip>,<port>:<ip>...`  
`.`    
`.`    
`.`    
`<port>:<ip>,<port>:<ip>,<port>:<ip>...`  
`<port>:<ip>,<port>:<ip>,<port>:<ip>...`  
Where each line corresponds to a replication group and each bind address corresponds to a single database. These also directly correlate to `<row> <column>` when launching the database.  

### Launching the admin server
`admin/admin <bind address> <database servers file>`  
`<bind address>`: the ip:port that the admin server will bind to
`<database servers file>`: a line separated list of all database bind addresses

### Launching the frontend servers
`frontend/frontend <frontend servers txt> <index>`  
`<frontend server file>`: a line separated list of all database bind addresses
`<index>`: the index of this frontend server's bind address within the `<frontend server file>`

### Launching the load balancer
`frontend/load_balancer <port> <frontend servers file>`  
`<port>`: the port number that the load balancer server will bind to
`<frontend servers file>`: a line separated list of all frontend bind addresses

You can use the `-q` flag with `launch_system.py` to supress make output and the `-v` flag to run
the servers with verbose logging.

