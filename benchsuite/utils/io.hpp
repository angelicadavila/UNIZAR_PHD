#ifndef CLBALANCER_UTILS_IO_HPP
#define CLBALANCER_UTILS_IO_HPP

#include <string>
#include <vector>

#include <fstream>
#include <iostream>
#include <sstream>

using std::cout;
using std::ifstream;
using std::ios;
using std::move;
using std::string;
using std::stringstream;
using std::vector;

vector<char>
file_read_binary(const string& path);
string
file_read(const string& path);

#endif /* CLBALANCER_UTILS_IO_HPP */
