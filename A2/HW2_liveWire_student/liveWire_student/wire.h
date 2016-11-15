#pragma once

// THIS FILE CONTAINS DECLARATIONS OF SOME GLOBAL VARIABLES AND FUNCTIONS DEFINED EITHER IN "wire.cpp" OR IN "main.cpp"
// THIS FILE MUST BE INCLUDED IN "main.cpp" and "wire.cpp" AS BOTH USE THESE GLOBAL VARIABLES AND FUNCTIONS 
using namespace std;

extern Table2D<RGB> image;  // loaded by GUI in main.cpp
extern bool closedContour;    // a flag indicating if contour was closed - set in wire.cpp
extern vector<Point> contour;   // a list of 'contour' points - set in wire.cpp
extern Table2D<int> region;   // a binary 2D mask of 'region' points - set in wire.cpp
extern int sigma; // sensitivity parameter to weight the edges - set in main.cpp

void addToContour(Point click);
void addToContourLast(Point click);
void regionGrow(Point seed, double T);
void reset_segm();
stack<Point>* liveWire(Point p);
void computePaths(Point seed);
int contourInterior();

extern bool view; // defined in main.cpp (boolean flag set by a check box)
void draw(Point mouse = Point(-1,-1)); // defined in main.cpp, but it is also called in wire.cpp for visualisation 


////////////////////////////////////////////////////////////
// MyPoint objects extend Points with extra variable "m_priority". 
// MyPoint have an overloaded comparison operators based on their priority.
class MyPoint : public Point {
	double m_priority;
public:
	MyPoint(Point p, double priority) : Point(p), m_priority(priority) {}
	double getPriority() const {return m_priority;}	
    bool operator<(const MyPoint& b) const   {return getPriority() > b.getPriority();}
    bool operator>(const MyPoint& b) const   {return getPriority() < b.getPriority();}
};
