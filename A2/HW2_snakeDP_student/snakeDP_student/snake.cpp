#include <iostream>     // for cout, rand
#include <vector>       // STL data structures
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"
#include "snake.h"


// GLOBAL PARAMETERS AND SPECIALISED DATA TYPES
int dP=4;    // default value for spacing between control points (in pixels)
const double INFTY=1.e20;

// declarations of global variables 
Table2D<RGB> image; // image is "loaded" from a BMP file by function "image_load" in "main.cpp" 
vector<Point> contour; // list of control points of a "snake"
bool closedContour=false; // a flag indicating if contour was closed 

Table2D<vitNode> energy;
Table2D<vitNode> prevEnergy;
const int NUMDIRS = 5;
static const Point shift[NUMDIRS] = { Point(-1,0),Point(1,0),Point(0,-1),Point(0,1),Point(0,0) };
enum Direction { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, STAY = 8, NONE = 10 };
double endEnergy;

//const int NUMDIRS=9;
//static const Point shift[NUMDIRS]={Point(-1,0),Point(1,0),Point(0,-1),Point(0,1),Point(-1,-1),Point(1,-1),Point(-1,1),Point(1,1),Point(0,0)};
//enum Direction {LEFT=0, RIGHT=1, TOP=2, BOTTOM=3, TOPLEFT=4, TOPRIGHT=5, BOTTOMLEFT=6, BOTTOMRIGHT=7, STAY=8, NONE=10};
//const Direction Reverse[NUMDIRS]={RIGHT,LEFT,BOTTOM,TOP,BOTTOMRIGHT,BOTTOMLEFT,TOPRIGHT,TOPLEFT}; 


// GUI calls this function when button "Clear" is pressed, or when new image is loaded
// THIS FUNCTION IS FULLY IMPLEMENTED, YOU DO NOT NEED TO CHANGE THE CODE IN IT
void reset_segm()
{
	cout << "resetting 'snake'" << endl;

	// remove all points from the "contour"
	while (!contour.empty()) contour.pop_back();
	closedContour=false;

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
	for (i=0; i<n; i++) contour[i] = contour[i] + ((click - contour[i])*0.25) ;
}

void addRepulseNudge(Point click)
{
	cout << "                     ...repulse from pixel p=(" << click.x << "," << click.y << ")" << endl;
	// the code below needs to be replaced, it is for fun only :)
	unsigned i, n = (unsigned) contour.size();
	for (i=0; i<n; i++) contour[i] = contour[i] - ((click - contour[i])*0.25) ;
}

///////////////////////////////////////////////////////////////
// DP_move() is a function that computes one optimal move for 
// the snake's control points using DP (Viterbi algorithm)
void DP_move()
{
	int n = (int) contour.size();

	double alpha = 0.1;
	energy.reset(n-1, NUMDIRS, vitNode(0,0)); // # neighbour states, # contour points nodes
	//Table2D<double> contrast = grad2(image); // describes "rate of change" of intensity
	vitNode checkNode;
	for (int i=0; i<n-1; i++) {
		// Iterate over all neighbours (states) of the next contour
		for (int s1=0; s1<NUMDIRS; s1++) {
			vitNode minEnergy(INFTY, 0);
			Point shiftNext = contour[i+1] + shift[s1];
			// Iterate over all neighbours (states) of the current contour
			for (int s2=0; s2<NUMDIRS; s2++) {
				Point shiftCur = contour[i] + shift[s2];
				double curEnergy = (i > 0) ? energy[i - 1][s2].energy : 0;
				curEnergy = curEnergy + (alpha * (shiftNext - shiftCur).norm());

				if (curEnergy < minEnergy.energy) 
					minEnergy = vitNode(curEnergy, s2);
			}
			energy[i][s1] = minEnergy;
		}
	}

	// Get the minimum state of the last node
	vitNode bestNode(INFTY, 0);
	int curState = 0;
	for (int state=0; state<NUMDIRS; state++) {
		if (energy[n - 2][state].energy < bestNode.energy) {
			bestNode = energy[n - 2][state];
			curState = state;
		}
	}

	// DEBUGGING
	//cout << "Last Energy: " << bestNode.energy << ", State: " << curState << "\n";
	//cout << "CHECK Energy: " << checkNode.energy << ", State: " << checkNode.toParent << "\n";

	//cout << "\nPRINTING PREV ENERGY\n";
	//print_energy(prevEnergy);
	//cout << "PRINTING ENERGY\n";
	//print_energy(energy);

	//endEnergy = bestNode.energy;
	//prevEnergy = energy;

	//contour[n-1] = contour[n-1] + shift[curState];
	//curState = bestNode.toParent;

	// Set the rest of the contour points
	for (int i=(n-1); i > 0; i--) {
		contour[i] = contour[i] + shift[curState];
		curState = energy[i-1][curState].toParent;
	}
	contour[0] = contour[0] + shift[curState];
}

///////////////////////////////////////////////////////////////
// DP_converge() is a function that runs DP moves for a snake
// until convergence to a local optima position
void DP_converge()
{
	for (int i=0; i<100; i++) DP_move();
}


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