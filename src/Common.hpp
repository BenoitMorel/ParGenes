#ifndef _MULTIRAXML_COMMON_HPP_
#define _MULTIRAXML_COMMON_HPP_

class MultiRaxmlException: public std::exception {
public:
  MultiRaxmlException(const std::string &s): msg_(s) {}
  MultiRaxmlException(const std::string &s1, 
      const std::string s2): msg_(s1 + s2) {}
  virtual const char* what() const throw() { return msg_.c_str(); }
  void append(const std::string &str) {msg_ += str;}

private:
  std::string msg_;
};



#endif
