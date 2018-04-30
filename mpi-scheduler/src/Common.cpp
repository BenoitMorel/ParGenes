#include "Common.hpp"
#include<iostream>

#include <stdio.h>
#include <ftw.h>
#include <unistd.h>

namespace MPIScheduler {

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
  
string Common::getIncrementalLogFile(const string &path, 
      const string &name,
      const string &extension)
{
  string file = joinPaths(path, name + "." + extension);
  int index = 1;
  while (ifstream(file).good()) {
    file = joinPaths(path, name + "_" + to_string(index) + "." + extension);
    index++;
  }
  return file;
}


SVGDrawer::SVGDrawer(const string &filepath,
    double maxXValue,
    double maxYValue):
  _os(filepath),
  _width(500.0),
  _height(500.0),
  _ratioWidth(_width / maxXValue),
  _ratioHeight(_height / maxYValue),
  _additionalHeight(50.0)
{
  if (!_os) {
    cerr << "Warning: cannot open  " << filepath << ". Skipping svg export." << endl;
    return;
  }
  writeHeader();
  writeSquareAbsolute(0, 0, _width, _height + _additionalHeight, "#ffffff");
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
  
void SVGDrawer::writeHorizontalLine(double y, int lineWidth)
{
  if (!_os)
    return;
  _os <<  "<line x1=\"0\" y1=\"" << y * _ratioHeight << "\" x2=\"" <<  _width   
      << "\" y2=\""<< y * _ratioHeight<<  "\" style=\"stroke:rgb(0,0,255);stroke-width:"
      << lineWidth << "\" />" << endl;
}

void SVGDrawer::writeSquare(double x, double y, double w, double h, const char *color)
{
  writeSquareAbsolute(x * _ratioWidth, y * _ratioHeight, w * _ratioWidth, h * _ratioHeight, color);
}

void SVGDrawer::writeSquareAbsolute(double x, double y, double w, double h, const char *color)
{
  if (!_os)
    return;
  string colorStr(color ? string(color) : getRandomHex());
  _os << "  <svg x=\"" << x << "\" y=\"" << y << "\" ";
  _os << "width=\"" << w  << "\" "  << "height=\""  << h << "\" >" << endl;
  _os << "    <rect x=\"0%\" y=\"0%\" height=\"100%\" width=\"100%\" style=\"fill: ";
  _os << colorStr  <<  "\"/>" << endl;
  _os << "  </svg>" << endl;
}
  
void SVGDrawer::writeCaption(const string &text)
{
  if (!_os)
    return;
  _os << "  <svg x =\"0\" y=\"500\" width=\"500\" height=\"40\">" << endl;
  _os << "    <text x=\"0%\" y=\"100%\" fill=\"blue\" font-size=\"35\">" << text << "</text>" << endl;
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


} // namespace MPIScheduler
