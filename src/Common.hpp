#ifndef _MULTIRAXML_COMMON_HPP_
#define _MULTIRAXML_COMMON_HPP_

#include <fstream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
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

using Time = chrono::time_point<chrono::system_clock>;

class Common {
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

  static Time getTime() {
    return chrono::system_clock::now();
  }

  static long getElapsedMs(Time begin, Time end) {
    return chrono::duration_cast<chrono::milliseconds>
      (end-begin).count();
  }

  // todobenoit not portable
  static string getSelfpath() {
    char buff[1000];
    ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    buff[len] = '\0';
    return string(buff);
  }


};


class Timer {
public:
  Timer() {
    reset();
  }

  int getElapsedMs() const {
    auto end = Common::getTime();
    return Common::getElapsedMs(_start, end);
  }

  void reset() {
    _start = chrono::system_clock::now();
  }
private:
  Time _start;
};

class SVGDrawer {
public:
  SVGDrawer(const string &filepath,
      double ratioWidth,
      double ratioHeight);
  ~SVGDrawer();
  void writeSquare(double x, double y, double w, double h, const char *color = 0);
private:
  void writeHeader();
  void writeFooter();
  ofstream _os;
  double _ratioWidth;
  double _ratioHeight;
};

#endif
