#pragma once

// THIS FILE CONTAINS DECLARATIONS OF SOME GLOBAL VARIABLES AND FUNCTIONS DEFINED EITHER IN "snake.cpp" OR IN "main.cpp"
// THIS FILE MUST BE INCLUDED IN "main.cpp" and "snake.cpp" AS BOTH USE THESE GLOBAL VARIABLES AND FUNCTIONS 
using namespace std;

extern Table2D<RGB> image;  // loaded by GUI in main.cpp
extern bool closedContour;    // a flag indicating if contour was closed - set in snake.cpp
extern vector<Point> contour;   // a list of control points for a "snake" - set in snake.cpp
extern int dP;   // spacing between control points (in pixels) - set in snake.cpp
extern int modeVal; // set in main.cpp
extern float keep; // minimum length for elasticity - set in snake.cpp
extern float alpha; // constant for internal energy - set in snake.cpp
extern float beta; // constant for distance transform - set in snake.cpp
extern float r; // constant for nudging - set in snake.cpp

void addToContour(Point click);
void addToContourLast(Point click);
void addRepulseNudge(Point click);
void addAttractNudge(Point click);
void reset_segm();
void DP_move();
void DP_converge();

extern bool view; // defined in main.cpp (boolean flag set by a check box)
void draw(Point mouse = Point(-1,-1)); // defined in main.cpp, but it is also called in snake.cpp for visualisation 

class vitNode {
public:
	double energy;
	int toParent;
	vitNode(const double energy1, const int toParent1) : energy(energy1), toParent(toParent1) {}
	vitNode() : energy(0), toParent(0) {};
	bool operator<(const vitNode& c) const {return energy < c.energy;}
    bool operator>(const vitNode& c) const {return energy > c.energy;}
};


vitNode run_viterbi(Table2D<vitNode>& energy, int freezeIdx, double alpha, int n);
void print_energy(Table2D<vitNode> energy);

void DTFwd(vector<int> kernel, double beta);
void DTBwd(vector<int> kernel, double beta);
inline double pDist(Point p) { return abs(p.x + p.y); }