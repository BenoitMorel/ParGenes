#ifndef _SVG_DRAWER_HPP_
#define _SVG_DRAWER_HPP_

#include <string>
#include <fstream>

using namespace std;


namespace MPIScheduler {

class SVGDrawer {
public:
  SVGDrawer(const string &filepath,
      double maxXValue,
      double maxYValue);
  ~SVGDrawer();
  void writeSquare(double x, double y, double w, double h, const char *color = 0);
  void writeSquareAbsolute(double x, double y, double w, double h, const char *color = 0);
  static string getRandomHex();
  void writeHeader(const string &caption);
  void writeFooter();

private:
  void writeCaption(const string &text);
  ofstream _os;
  double _width;
  double _height;
  double _ratioWidth;
  double _ratioHeight;
  double _additionalHeight;
};

} // namespace MPIScheduler

#endif

