#ifndef _MULTIRAXML_COMMON_HPP_
#define _MULTIRAXML_COMMON_HPP_

#include <chrono>
#include <thread>
#include <cstdio>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
using namespace std;

class MultiRaxmlException: public exception {
public:
  MultiRaxmlException(const string &s): msg_(s) {}
  MultiRaxmlException(const string &s1, 
      const string s2): msg_(s1 + s2) {}
  virtual const char* what() const throw() { return msg_.c_str(); }
  void append(const string &str) {msg_ += str;}

private:
  string msg_;
};

class MultiRaxmlCommon {
public:
  static void sleep(unsigned int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
  }

  // todobenoit not portable
  static void readDirectory(const std::string& name, 
      vector<string> &v)
  {
    DIR* dirp = opendir(name.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
      v.push_back(dp->d_name);
    }
    closedir(dirp);
  }

  // todobenoit not portable
  static void makedir(const std::string &name) {
    mkdir(name.c_str(), 0755);
  }

  static void removefile(const std::string &name) {
    remove(name.c_str());
  }

};

class Timer {
public:
  Timer() {
    reset();
  }

  int getElapsedMs() const {
    auto end = chrono::system_clock::now();
    return chrono::duration_cast<chrono::milliseconds>
      (end-_start).count();
  }

  void reset() {
    _start = chrono::system_clock::now();
  }
private:
  chrono::time_point<chrono::system_clock> _start;
};

#endif
