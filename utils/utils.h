//utils.h

// This is the header file for general utility functions
#include <string>
#include <vector>
#include <map>
using namespace std;

#ifndef UTILS_H
#define UTILS_H

// Splits a string into tokens given a delimiter
void split_string(string s, string delimiter, vector<string> *ret_val);

#endif