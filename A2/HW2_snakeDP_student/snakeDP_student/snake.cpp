#include <iostream>     // for cout, rand
#include <vector>       // STL data structures
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"
#include "snake.h"


// GLOBAL PARAMETERS AND SPECIALISED DATA TYPES
int dP=4;    // default value for spacing between control points (in pixels)
float keep = 1; // minimum length for elasticity - set in snake.cpp
float alpha = 0.1; // constant for internal energy - set in snake.cpp
float beta = 0.1; // constant for distance transform - set in snake.cpp
float r = 50; 
const double INFTY=1.e20;

// declarations of global variables 
Table2D<RGB> image; // image is "loaded" from a BMP file by function "image_load" in "main.cpp" 
vector<Point> contour; // list of control points of a "snake"
bool closedContour=false; // a flag indicating if contour was closed 
bool quad = true;
vector<double> nudgeEn;

Table2D<vitNode> energy;
Table2D<vitNode> energy2;
Table2D<double> contrast; // describes "rate of change" of intensity
Table2D<double> distTransform; 
//const int NUMDIRS = 5;
//static const Point shift[NUMDIRS] = { Point(-1,0),Point(1,0),Point(0,-1),Point(0,1),Point(0,0) };
//enum Direction { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, STAY = 8, NONE = 10 };

const int NUMDIRS=9;
static const Point shift[NUMDIRS]={Point(-1,0),Point(1,0),Point(0,-1),Point(0,1),Point(-1,-1),Point(1,-1),Point(-1,1),Point(1,1),Point(0,0)};
enum Direction {LEFT=0, RIGHT=1, TOP=2, BOTTOM=3, TOPLEFT=4, TOPRIGHT=5, BOTTOMLEFT=6, BOTTOMRIGHT=7, STAY=8, NONE=10};

// GUI calls this function when button "Clear" is pressed, or when new image is loaded
// THIS FUNCTION IS FULLY IMPLEMENTED, YOU DO NOT NEED TO CHANGE THE CODE IN IT
void reset_segm()
{
	cout << "resetting 'snake'" << endl;

	// remove all points from the "contour"
	while (!contour.empty()) contour.pop_back();
	closedContour=false;

	// Added..
	contrast = grad2(image); // describes "rate of change" of intensity
	
	vector<int> kernel; kernel.push_back(0); kernel.push_back(1); kernel.push_back(1); kernel.push_back(0);
	distTransform.reset(image.getWidth(), image.getHeight(), 0);
	DTFwd(kernel, beta);
	DTBwd(kernel, beta);
}

// GUI calls this function when a user left-clicks on an image pixel while in "Contour" mode
void addToContour(Point p) 
{
	if (closedContour || dP<=0) return;

	// if contour is empty, append control point p to the "contour", 
	// else (if contour is not empty) append to the "contour" 
	// control points on a straignt line segment from 
	// the end of the current contour to point 'p'
	// The interval between control points is conttroled by parameter 'dP'
	if (contour.empty()) contour.push_back(p);
	else if (!(p==contour.back()))
	{ 
		Point last = contour.back();
		Point v = (p - last); 
		int n = 1 + (int) (v.norm()/dP);
		for (int i=1; i<=n; i++) contour.push_back(last + v*(i/((double)n)));
	}
	cout << "contour size = " << contour.size() << endl;
}

// GUI calls this function when a user right-clicks on an image pixel while in "Contour" mode
void addToContourLast(Point p)
{
	if (closedContour || contour.empty() || dP<=0) return;

	addToContour(p);
	// adding the "closing" interval connecting to the first control point
	addToContour(contour[0]);
	contour.pop_back(); // removes unnecessary copy of point contour[0] at the back!!!!

	closedContour = true;
	draw();
	
}

void addAttractNudge(Point click)
{
	cout << "                     ...attract to pixel p=(" << click.x << "," << click.y << ")" << endl;
	// the code below needs to be replaced, it is for fun only :)
	unsigned i, n = (unsigned) contour.size();
	nudgeEn.clear();

	for (i = 0; i < n; i++) nudgeEn.push_back(-pow(r,2)/(click- contour[i]).norm());
	DP_move();
	nudgeEn.clear();
	//for (i = 0; i<n; i++) contour[i] = contour[i] + ((click - contour[i])*0.25);
}

void addRepulseNudge(Point click)
{
	cout << "                     ...repulse from pixel p=(" << click.x << "," << click.y << ")" << endl;
	// the code below needs to be replaced, it is for fun only :)
	unsigned i, n = (unsigned) contour.size();
	nudgeEn.clear();

	for (i = 0; i < n; i++) nudgeEn.push_back(pow(r, 2) / (click - contour[i]).norm());
	DP_move();
	nudgeEn.clear();
	//for (i=0; i<n; i++) contour[i] = contour[i] - ((click - contour[i])*0.25) ;
}

///////////////////////////////////////////////////////////////
// DP_move() is a function that computes one optimal move for 
// the snake's control points using DP (Viterbi algorithm)
void DP_move()
{
	int n = (int)contour.size();

	if (closedContour) {
		energy.reset(n, NUMDIRS, vitNode(0, 0)); // # neighbour states, # contour points nodes
		energy2.reset(n, NUMDIRS, vitNode(0, 0));
		vitNode first_energy = run_viterbi(energy, 0, alpha, n); // freeze the first point
		vitNode half_energy = run_viterbi(energy2, int(n / 2), alpha, n); // freeze the point

		// Pick the run that yielded the smallest energy
		int curState = 0;
		if (half_energy.energy < first_energy.energy) {
			energy = energy2;
			curState = half_energy.toParent;
		}

		// Set the contour points
		int startIdx = (half_energy.energy < first_energy.energy) ? int(n / 2) : 0;
		for (int i = startIdx + n; i > startIdx; i--) {
			int idx = i % n;
			contour[idx] = contour[idx] + shift[curState];
			curState = energy[(i - 1) % n][curState].toParent;
		}
	}
	else {
		// TODO: Compact this code a bit more... (contour drawing)
		energy.reset(n-1, NUMDIRS, vitNode(0, 0)); // # neighbour states, # contour points nodes
		vitNode first_energy = run_viterbi(energy, 0, alpha, n);
		int curState = first_energy.toParent;

		// Set the rest of the contour points
		for (int i=(n-1); i > 0; i--) {
			contour[i] = contour[i] + shift[curState];
			curState = energy[i-1][curState].toParent;
		}
		contour[0] = contour[0] + shift[curState];
	}
}

// Helper function for DP_move
vitNode run_viterbi(Table2D<vitNode>& energy, int freezeIdx, double alpha, int n) {
	vitNode bestEnergy(INFTY, 0);
	int end = (closedContour) ? n : n - 1;
	Point keepPoint = (modeVal == 0) ? Point(0,0) : Point(keep,keep); // Defines space that should be kept between points
	
	// TODO: CHECK IF THE POINT IS IN THE IMAGE
	for (int i = freezeIdx; i<end + freezeIdx; i++) {
		int idx = i % n;
		// Iterate over all neighbours (states) of the next contour
		for (int s1 = 0; s1<NUMDIRS; s1++) {
			vitNode minEnergy(INFTY, 0);
			Point shiftNext = contour[(i + 1) % n] + shift[s1];
			// Iterate over all neighbours (states) of the current contour
			for (int s2 = 0; s2 < NUMDIRS; s2++) {
				Point shiftCur = contour[idx] + shift[s2];
				double curEnergy = (idx > 0) ? energy[idx - 1][s2].energy : energy[end - 1][s2].energy;
				double extEnergy = (modeVal == 2) ? pow(contrast[shiftCur],2) : 0; // External energy using gradient
				// TODO: CHECK DISTANCE NOT SUBTRACTED BUT ADDED
				extEnergy = (modeVal == 3) ? pow(distTransform[shiftCur], 2) : extEnergy; // External energy using distance transform
				extEnergy = (nudgeEn.size() > 0) ? extEnergy + nudgeEn[idx] : extEnergy;
				double intEnergy = (quad) ? (shiftNext - shiftCur - keepPoint).norm() : pDist(shiftNext - shiftCur - keepPoint);

				curEnergy = curEnergy + (alpha * intEnergy) - extEnergy;

				if (curEnergy < minEnergy.energy)
					minEnergy = vitNode(curEnergy, s2);
			}
			energy[idx][s1] = minEnergy;
			// Get the minimum state of the last node
			if ((idx == (freezeIdx - 1) % n) && (minEnergy.energy < bestEnergy.energy)) {
				bestEnergy.toParent = s1;
				bestEnergy.energy = minEnergy.energy;
			}
		}
	}

	return bestEnergy;
}


///////////////////////////////////////////////////////////////
// DP_converge() is a function that runs DP moves for a snake
// until convergence to a local optima position
void DP_converge()
{
	for (int i=0; i<100; i++) DP_move();
}

///////////////////////////////////////////////////////////////
// Distance Transform functions
// kernel - 0 = top left, 1 = top right, 2 = bottom left, 3 = bottom right
void DTFwd(vector<int> kernel, double beta)
{
	for (int x = 0; x < distTransform.getWidth(); x++) {
		for (int y = 0; y < distTransform.getHeight(); y++) {
			double cmp1, cmp2; //cmp1 = top right, cmp2 = bottom left
			cmp1 = (x > 0) ? contrast[x-1][y] : INFTY;
			cmp2 = (y > 0) ? contrast[x][y - 1] : INFTY;
			double res = std::min(std::min(beta*kernel[1] + cmp1, beta*kernel[2] + cmp2), contrast[x][y]);
			distTransform[x][y] = res;
		}
	}
}

// kernel - 0 = top left, 1 = top right, 2 = bottom left, 3 = bottom right
void DTBwd(vector<int> kernel, double beta)
{
	int endX = distTransform.getWidth();
	int endY = distTransform.getHeight();
	for (int x = 0; x < endX; x++) {
		for (int y = 0; y < endY; y++) {
			double cmp1, cmp2; //cmp1 = top right, cmp2 = bottom left
			cmp1 = (x < endX-1) ? contrast[x + 1][y] : INFTY;
			cmp2 = (y < endY-1) ? contrast[x][y + 1] : INFTY;
			distTransform[x][y] = std::min(std::min(beta*kernel[1] + cmp1, beta*kernel[2] + cmp2), contrast[x][y]);
		}
	}
}



///////////////////////////////////////////////////////////////
// DEBUGGING FUNCTION
void print_energy(Table2D<vitNode> energy)
{
	for (int i = 0; i < energy.getHeight(); i++)
	{
		for (int j = 0; j < energy.getWidth(); j++)
		{
			cout << energy[j][i].energy << "," << energy[j][i].toParent << "\t";
		}
		cout << "\n\n";
	}
}

