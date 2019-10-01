#!/usr/bin/env python
import os
import argparse
from subprocess import *
import signal
import sys

proc = []
def signal_handler(signal, frame):
        print()
        for p in proc:
        	print("KILL")
        	p.kill()
        sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

parser = argparse.ArgumentParser(description='Start the 505 project')
parser.add_argument('-q', action='store_true', help='quiet make')
parser.add_argument('-v', action='store_true', help='verbose logging')
args = parser.parse_args()

FNULL = open(os.devnull, 'w')
def make_clean():
	print("make_clean")
	if args.q:
		call(["make", "clean", "USERNAME=ptrent"], stdout=FNULL, stderr=STDOUT)
	else:
		call(["make", "clean", "USERNAME=ptrent"])

def make_all():
	print("make_all")
	if args.q:
		call(["make", "all", "USERNAME=ptrent"], stdout=FNULL, stderr=STDOUT)
	else:
		call(["make", "all", "USERNAME=ptrent"])

def spin_up_db():
	print("spin_up_db")
	v_arg = "-v" if args.v else ""
	db1 = Popen(["./database/database_server", v_arg, "frontend/quorum/databases.txt", "1", "1"])
	db2 = Popen(["./database/database_server", v_arg, "frontend/quorum/databases.txt", "1", "2"])
	db3 = Popen(["./database/database_server", v_arg, "frontend/quorum/databases.txt", "1", "3"])
	db4 = Popen(["./database/database_server", v_arg, "frontend/quorum/databases.txt", "1", "4"])
	db5 = Popen(["./database/database_server", v_arg, "frontend/quorum/databases.txt", "1", "5"])
	db6 = Popen(["./database/database_server", v_arg, "frontend/quorum/databases.txt", "1", "6"])
	return [db1, db2, db3, db4, db5, db6]

def spin_up_frontend():
	print("spin_up_frontend")
	v_arg = "-v" if args.v else ""
	fe1 = Popen(["./frontend/frontend", v_arg, "frontend/servers.txt", "1"])
	fe2 = Popen(["./frontend/frontend", v_arg, "frontend/servers.txt", "2"])
	fe3 = Popen(["./frontend/frontend", v_arg, "frontend/servers.txt", "3"])
	return [fe1, fe2, fe3]

def spin_up_admin():
	print("spin_up_admin")
	v_arg = "-v" if args.v else ""
	admin = Popen(["./admin/admin", v_arg, "127.0.0.1:7001", "admin/servers.txt"])
	return [admin]


def spin_up_load_balancer():
	balancer = Popen(["./frontend/load_balancer", "8001", "frontend/servers.txt"])
	return [balancer]

def main():
	all_processes = []
	make_clean()
	make_all()
	all_processes += spin_up_db()
	all_processes += spin_up_frontend()
	all_processes += spin_up_admin()
	all_processes += spin_up_load_balancer()
	return all_processes

if __name__ == '__main__':
	proc = main()
	while 1:
		a = 1
