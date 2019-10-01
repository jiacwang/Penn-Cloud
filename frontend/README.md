# Frontend
The frontend is what the client connects to. It serves HTTP to the client and interfaces with the backend via a set of classes as defined below.

## FileServer (file_server.h/.cc)
The file server is in charge of writing and reading informaiton, including files and folders to the data store.

## MailServer (mail_server.h/.cc)
The mail server is in charge of handling mail requests via translating requests for specific messages, a listing of messages, etc. It communicates with the data store to this end. 

## FrontendServer (frontend_server.cc)
The frontend server handles the triage of what the customer wants and responds to requests over HTTP. This server creates a new thread for each client connection and for each thread creates an instance of MailServer and an instance of FileServer. 

