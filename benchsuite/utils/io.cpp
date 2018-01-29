#include "io.hpp"

vector<char>
file_read_binary(const string& path)
{
  vector<char> result;
  try {
    ifstream ifs(path, ios::binary | ios::ate);
    ifs.exceptions(ifstream::failbit | ifstream::badbit);
    ifstream::pos_type pos = ifs.tellg();
    result.reserve(pos);
    result.resize(pos);
    ifs.seekg(0, ios::beg);
    ifs.read(result.data(), pos);
  } catch (ios::failure& e) {
    throw ios::failure("error reading '" + path + "' " + e.what());
  }
  return result;
}

string
file_read(const string& path)
{
  string s;
  try {
    ifstream ifs(path);
    ifs.exceptions(ifstream::failbit | ifstream::badbit);
    stringstream string;
    string << ifs.rdbuf();
    s = move(string.str());
  } catch (ios::failure& e) {
    throw ios::failure("error reading '" + path + "' " + e.what());
  }
  return s;
}
