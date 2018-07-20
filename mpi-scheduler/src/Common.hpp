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
#include <mpi.h>
#include <signal.h>  
#include <iostream>

using namespace std;

namespace MPIScheduler {

class MPISchedulerException: public exception {
public:
  explicit MPISchedulerException(const string &s): msg_(s) {}
  MPISchedulerException(const string &s1, 
      const string &s2): msg_(s1 + s2) {}
private:
  string msg_;
};

using Time = chrono::time_point<chrono::system_clock>;

class Common {
public:
  // todobenoit not portable
  // todobenoit not portable
  static void makedir(const std::string &name) {
    mkdir(name.c_str(), 0755);
  }
  
  static string joinPaths(const std::string &path1, const std::string &path2) {
    return path1 + "/" + path2;
  }

  static string joinPaths(const std::string &path1, 
      const std::string &path2,
      const std::string &path3) {
    return joinPaths(joinPaths(path1, path2), path3);
  }

  static Time getTime() {
    return chrono::system_clock::now();
  }

  static string getIncrementalLogFile(const string &path, 
      const string &name,
      const string &extension);

  static long getElapsedMs(Time begin, Time end) {
    return chrono::duration_cast<chrono::milliseconds>
      (end-begin).count();
  }


  static void check(int mpiError) {
    if (mpiError != MPI_SUCCESS) {
      cout << "MPI ERROR !!!!" << endl;
      throw MPISchedulerException("MPI error !");
    }
  }

  static int getPid() {
    return getpid();
  }
  
  static string getHost() {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return string(hostname);
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

class SchedulerArgumentsParser {
public:
  SchedulerArgumentsParser(int argc, char** argv):
    commandsFilename()
  {
    if (argc != 5) {
      print_help();
      throw MPISchedulerException("Error: invalid syntax");
    }
    unsigned int i = 1;
    implem = string(argv[i++]);
    library = string(argv[i++]);
    commandsFilename = string(argv[i++]);
    outputDir = string(argv[i++]);
  }
  
  void print_help() 
  {
    cout << "Syntax:" << endl;
    cout << "multiraxml --split-scheduler library command_file output_dir" << endl;
  }

  string implem;
  string library;
  string commandsFilename;
  string outputDir;
};

} // namespace MPIScheduler

#endif
