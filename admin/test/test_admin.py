#!/usr/bin/env python

import re
import socket

class bcolors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

SOCKET_TIMEOUT = 5 #seconds
BUFFER_SIZE = 2048 #bytes

command_lst = [
#sign up and log in
"\\signup username=" + username + " password=" + password + "\r\n",
"\\login username=" + username + " password=" + password + "\r\n",
#
# mail tests
#
#send an email
"\\smtp to " + dst_email + "\r\n",
"\\smtp from " + src_email + "\r\n",
"\\smtp data\r\n",
#insert data to send here
"\\smtp end\r\n",
"\\smtp send\r\n",
#cancel an email
"\\smtp to " + dst_email + "\r\n",
"\\smtp from " + src_email + "\r\n",
"\\smtp data\r\n",
#insert data to send here
"\\smtp end\r\n",
"\\smtp cancel\r\n",
#
# file tests
#
"\\file mkdir test_dir\r\n",
"\\file cd test_dir\r\n",
"\\file touch test_file\r\n",
"\\file ls test_dir\r\n"
]

def stat_str(color, mid_str):
	return "[ " + color + "{}".format(mid_str) + bcolors.ENDC + " ]"


def receive_from_socket(s):
	buff = ""
	while True:
		data = s.recv(1024)
		if "\n" in data:
			buff += data
			break
		else:
			buff += data
	return buff

def run_tests():
	print stat_str(bcolors.BLUE, "- - - - Begin Test - - - -")
	print "Reading in ../servers"
	server_file = open("../servers.txt", "r")
	sockets = []

	for line in server_file:
		m = re.search('([^:]*):(\\d*)', line)
		ip = m.group(1)
		port = int(m.group(2))
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		try:
			s.connect((ip, port))
			print stat_str(bcolors.GREEN, "Connected") + " " + ip + ":" + str(port)
			sockets += [s]
		except socket.error, exc:
			print stat_str(bcolors.RED, "Not Connected") + " " + ip + ":" + str(port)
	for ind,s in enumerate(sockets):
		s.settimeout(SOCKET_TIMEOUT)
		for i in range(0,len(command_lst)):
			s.send(command_lst[i])
			out = receive_from_socket(s)
			print((stat_str(bcolors.BLUE, ind)) + (stat_str(bcolors.GREEN, "PASS") \
				if out == "OK\r\n" else stat_str(bcolors.RED, "FAIL")) + " " + command_lst[i][:-2])

	for s in sockets:
		s.close()

	print stat_str(bcolors.BLUE, "- - - -  End Test  - - - -")

run_tests()
