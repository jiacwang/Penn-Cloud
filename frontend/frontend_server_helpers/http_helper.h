#ifndef _HTTP_HELPER_H_
#define _HTTP_HELPER_H_

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
using namespace std;



inline string http_ok_response(string html_content, string cookie_name, string cookie) {
	string resp = "HTTP/1.1 200 OK\r\n";
	resp += "Content-Type: text/html\r\n";
	resp += "Connection: close\r\n";
	resp += "Set-Cookie: ";
	resp += cookie_name + string("=") + cookie + string("\r\n");
	resp += "Content-Length: " ;
	resp += to_string((int)strlen(html_content.c_str()));
	resp += "\r\n";
	resp += "\r\n";
	resp += html_content;
	resp += "\r\n";
	return resp;
}

inline string http_not_found_response(string html_content, string cookie_name, string cookie) {
	string resp = "HTTP/1.1 404 Not Found\r\n";
	resp += "Content-Type: text/html\r\n";
	resp += "Connection: close\r\n";
	resp += "Set-Cookie: ";
	resp += cookie_name + string("=") + cookie + string("\r\n");
	resp += "Content-Length: " ;
	resp += to_string((int)strlen(html_content.c_str()));
	resp += "\r\n";
	resp += "\r\n";
	resp += html_content;
	resp += "\r\n";
	return resp;
}

inline string http_found_response(string new_location, string cookie_name, string cookie) {
	string resp = "HTTP/1.1 302 Found\r\n";
	resp += "Set-Cookie: ";
	resp += cookie_name + string("=") + cookie + string("\r\n");
	resp += "Location: ";
	resp += new_location;
	resp += "\r\n";
	resp += "Connection: close\r\n";
	resp += "\r\n";
	return resp;
}

inline string http_found_redirect(string new_location) {
	string resp = "HTTP/1.1 302 Found\r\n";
	resp += "Location: ";
	resp += new_location;
	resp += "\r\n";
	resp += "Connection: close\r\n";
	resp += "\r\n";
	return resp;
}

inline string http_ok_post(string content, string filename, string cookie_name, string cookie) {
	string resp = "HTTP/1.1 200 OK\r\n";
	resp += "Content-Type: application/octet-stream\r\n";
	resp += string("Content-Disposition: attachment; filename=\"") + filename + string("\"\r\n");
	resp += "Connection: close\r\n";
	resp += "Set-Cookie: ";
	resp += cookie_name + string("=") + cookie + string("\r\n");
	resp += "Content-Length: " ;
	resp += to_string((int)strlen(content.c_str()));
	resp += "\r\n";
	resp += "\r\n";
	resp += content;
	resp += "\r\n";
	return resp;
}

#endif
