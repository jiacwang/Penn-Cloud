# Makefile for cis505 final project
#
# 
#

#
# To use, make sure to give your usename
#  for example,	`make push USERNAME=ptrent`
#

all:
	cd utils && $(MAKE) all
	cd database && $(MAKE) all
	cd admin && $(MAKE) all
	cd frontend && $(MAKE) all

clean:
	cd utils && $(MAKE) clean
	cd database && $(MAKE) clean
	cd admin && $(MAKE) clean
	cd frontend && $(MAKE) clean

ifndef USERNAME
$(error USERNAME is not set)
endif

BRANCH?=$(USERNAME)_working_branch


push:
	git pull github master
	git pull origin master
	git merge origin/$(BRANCH)
	git push github $(BRANCH)
	git push origin $(BRANCH)

