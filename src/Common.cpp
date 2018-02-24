#include "Common.hpp"
#include<iostream>

#include <stdio.h>
#include <ftw.h>
#include <unistd.h>


int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
  int rv = remove(fpath);
  if (rv)
    perror(fpath);
  return rv;
}

void Common::removedir(const std::string &name)
{
  nftw(name.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

SVGDrawer::SVGDrawer(const string &filepath,
    double ratioWidth,
    double ratioHeight):
  _os(filepath),
  _ratioWidth(ratioWidth),
  _ratioHeight(ratioHeight)
{
  if (!_os) {
    cerr << "Warning: cannot open  " << filepath << ". Skipping svg export." << endl;
    return;
  }
  writeHeader();
}

SVGDrawer::~SVGDrawer()
{
  writeFooter();
}

string SVGDrawer::getRandomHex()
{
  static const char *buff = "0123456789abcdef";
  char res[8];
  res[0] = '#';
  res[7] = 0;
  for (int i = 0; i < 6; ++i) {
    res[i+1] = buff[rand() % 16];
  }
  return string(res);
}

void SVGDrawer::writeSquare(double x, double y, double w, double h, const char *color)
{
  if (!_os)
    return;
  string colorStr(color ? string(color) : getRandomHex());
  _os << "  <svg x=\"" << x * _ratioWidth << "\" y=\"" << y * _ratioHeight << "\" ";
  _os << "width=\"" << w * _ratioWidth << "\" "  << "height=\""  << h * _ratioHeight << "\" >" << endl;
  _os << "    <rect x=\"0%\" y=\"0%\" height=\"100%\" width=\"100%\" style=\"fill: ";
  _os << colorStr  <<  "\"/>" << endl;
  _os << "  </svg>" << endl;
}


void SVGDrawer::writeHeader()
{
  if (!_os)
    return;
  _os << "<svg  xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">" << endl;

}

void SVGDrawer::writeFooter()
{
  if (!_os)
    return;
  _os << "</svg>" << endl;

}

