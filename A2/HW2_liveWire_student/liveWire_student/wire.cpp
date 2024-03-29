#include <iostream>     // for cout, rand
#include <vector>       // STL data structures
#include <stack>
#include <queue> 
#include <set>
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"
#include "cs1037lib-time.h"  // for "pausing"  
#include "wire.h"


// GLOBAL PARAMETERS AND SPECIALISED DATA TYPES
const double INFTY=1.e20;
const double PI=3.1415926535897;
const int NUMDIRS=8;
static const Point shift[NUMDIRS]={Point(-1,0),Point(1,0),Point(0,-1),Point(0,1),Point(-1,-1),Point(1,-1),Point(-1,1),Point(1,1)};
enum Direction {LEFT=0, RIGHT=1, TOP=2, BOTTOM=3, TOPLEFT=4, TOPRIGHT=5, BOTTOMLEFT=6, BOTTOMRIGHT=7, NONE=10};
const Direction Reverse[NUMDIRS]={RIGHT,LEFT,BOTTOM,TOP,BOTTOMRIGHT,BOTTOMLEFT,TOPRIGHT,TOPLEFT}; 
//double fn(const double t) {double w=0.1; return 1.0/(1.0+w*t);}  // function used for setting "pixel penalties" ("Sensitivity" w is a tuning parameter)
double fn(const double t) {return (1/(sqrt(2*PI)*sigma))*exp(-(pow(t, 2))/(2*pow(double(sigma),2)));}

// declarations of global variables 
Table2D<RGB> image; // image is "loaded" from a BMP file by function "image_load" in "main.cpp" 
vector<Point> contour; // list of "contour" points
bool closedContour=false; // a flag indicating if contour was closed 
Table2D<int> region;  // 2D array storing binary mask for pixels (1 = "in", 0 = "out", 2 = "active front")
Table2D<double> penalty; // 2d array of pixels' penalties computed in reset_segm() 
                         // Low penalty[x][y] implies high image "contrast" at image pixel p=(x,y).
Table2D<double>    dist;     // 2D table of "distances" for current paths to the last seed
Table2D<Direction> toParent; // 2D table of "directions" along current paths to the last seed. 


// GUI calls this function when button "Clear" is pressed, or when new image is loaded
// THIS FUNCTION IS FULLY IMPLEMENTED, YOU DO NOT NEED TO CHANGE THE CODE IN IT
void reset_segm()
{
	cout << "resetting 'contour' and 'region'" << endl;

	// removing all region markings
	region.reset(image.getWidth(),image.getHeight(),0);

	// remove all points from the "contour"
	while (!contour.empty()) contour.pop_back();
	closedContour=false;

	// resetting 2D tables "dist" and "toParent" (erazing paths) 
	dist.reset(image.getWidth(),image.getHeight(),INFTY);
	toParent.reset(image.getWidth(),image.getHeight(),NONE);

	// recomputing "penalties" from an estimate of image contrast at each pixel p=(x,y)
	if (image.isEmpty()) {penalty.resize(0,0); return;} 
	Table2D<double> contrast = grad2(image);  //(implicit conversion of RGB "image" to "Table2D<double>") 
	// NOTE: function grad2() (see Math2D.h) computes (at each pixel) expression  Ix*Ix+Iy*Iy  where Ix and Iy
	// are "horizontal" and "vertical" derivatives of image intensity I(x,y) - the average of RGB values.
	// This expression describes the "rate of change" of intensity, or local image "contrast". 
	penalty = convert(contrast,&fn); // "&fn" - address of function "fn" (defined at the top of this file) 
	// "convert" (see Math2D.h) sets penalty at each pixel according to formula "penalty[x][y] = fn (contrast[x][y])" 
}


/////////////////////////////////////////////////////////////////////////////////////////
// GUI calls this function as "mouse" moves over the image to any new pixel p. It is also 
// called inside "addToContour" for each mouse click in order to add the current live-wire
// to the contour. Function should allocate, initialize, and return a path (stack) of 
// adjacent points from p to the current "seed" (last contour point) but excluding the seeed. 
// The path should follow directions in 2D table "toParent" computed in "computePaths()".
stack<Point>* liveWire(Point p)
{
	if (contour.empty()) return NULL;  
	
	// Create a new stack of Points dynamically.
	stack<Point> * path = new stack<Point>();

	// Add point 'p' into it. Then (iteratively) add its "parent" according to
	// 2D Table "toParent", then the parent of the parent, e.t.c....    until
	// you get a pixel that has parent "NONE". In case "toParent" table is 
	// computed correctly inside "computePaths(seed)", this pixel should be 
	// the current "seed", so you do not need to add it into the "path".
	// ....
	Direction n = toParent[p];
	while(n!=NONE) {path->push(p); p=p+shift[n]; n = toParent[p];}

	// Return the pointer to the stack.
	return path;
}

// GUI calls this function when a user left-clicks on an image pixel while in "Contour" mode
void addToContour(Point p) 
{
	if (closedContour) return;

	// if contour is empty, append point p to the "contour", 
	// else (if contour is not empty) append to the "contour" 
	// all points returned by "live-wire(p)" (a path from p to last seed). 
	if (contour.empty()) contour.push_back(p);
	else 
	{   
		stack<Point> *path = liveWire(p);
		if (path) while (!path->empty()) {contour.push_back(path->top()); path->pop();}
		delete path;
	}
	// The call to computePaths(p) below should precompute "optimal" paths 
	// to the new seed p and store them in 2D table "toParent".
	computePaths(p);
}

// GUI calls this function when a user right-clicks on an image pixel while in "Contour" mode
void addToContourLast(Point p)
{
	if (closedContour || contour.empty()) return;

	// add your code below to "close" the contour using "live-wire"
	addToContour(p);
	// extracting live-wire from the first contour point to point p
	stack<Point> *path = liveWire(contour[0]);
	if (path) { // note: last point in "path" is already in "contour"
		while (path->size()>1) {contour.push_back(path->top()); path->pop();}
		delete path;
	}
	closedContour = true;
	draw();
	
	cout << "interior region has volume of " << contourInterior() << " pixels" << endl; 

}

// GUI calls this function when a user left-clicks on an image pixel while in "Region" mode
// The method computes a 'region' of pixels connected to 'seed'. The region is grown by adding
// to each pixel p (starting at 'seed') all neighbors q such that intensity difference is small,
// that is, |Ip-Iq|<T where T is some fixed threshold. Use Queue for active front. 
// To compute intensity difference Ip-Iq, use function dI(RGB,RGB) defined in "Image2D.h"   
void regionGrow(Point seed, double T)
{
	if (!image.pointIn(seed)) return;
	int counter = 0;                  
	queue<Point> active;        
	active.push(seed);
	
	// use BREADTH-FIRST_SEARCH (FIFO order) traversal - "Queue"
	while (!active.empty())
	{
		Point p = active.front();
		active.pop();
		region[p]=1;  // pixel p is extracted from the "active_front" and added to "region", but
		counter++;    // then, all "appropriate" neighbors of p are added to "active_front" (below)
		for (int i=0; i<4; i++) // goes over 4 neighbors of pixel p
		{   // uses overloaded operator "+" in Point.h and array of 'shifts' (see the top of file)
			Point q = p + shift[i]; // to compute a "neighbor" of pixel p
			if (image.pointIn(q) && region[q]==0 && abs(dI(image[p],image[q]))<T) 
			{ // we checked if q is inside image range and that the difference in intensity
	          // between p and q  is sufficiently small 	 
				active.push(q); 
				region[q]=2; // "region" value 2 corresponds to pixels in "active front".
			}                 // Eventually, all pixels in "active front" are extracted 
		}                     // and their "region" value is set to 1.
		if (view && counter%60==0) {draw(); Pause(20);} // visualization, skipped if checkbox "view" is off
	}
	cout << "grown region of volume " << counter << " pixels    (threshold " << T << ", BFS-Queue)" << endl;
}

///////////////////////////////////////////////////////////////////////////////////
// computePaths() is a function for "precomputing" live-wire (shortest) paths from 
// any image pixel to the given "seed". This method is called inside "addToContour" 
// for each new mouse click. Optimal path from any pixel to the "seed" should accumulate 
// the least amount of "penalties" along the path (each pixel p=(x,y) on a path contributes 
// penalty[p] precomputed in "reset_segm()"). In this function you should use 2D tables 
// "toParent" and "dist" that store for each pixel its "path-towards-seed" direction and, 
// correspondingly, the sum of penalties on that path (a.k.a. "distance" to the seed). 
// The function iteratively improves paths by traversing pixels from the seed until 
// all nodes have direction "toParent" giving the shortest (cheapest) path to the "seed".
void computePaths(Point seed)
{
	if (!image.pointIn(seed)) return;
	region.reset(0); // resets 2D table "region" for visualization

	// Reset 2D arrays "dist" and "toParent"  (erazing all current paths)
	dist.reset(INFTY);
	toParent.reset(NONE);
	dist[seed] = 0;
	int counter = 0; // just for info printed at the bottom of the function
	
	// STUDENTS: YOU NEED TO REPLACE THE CODE BELOW (which is a copy of BFS algorithm in region growing)
	// Create a queue (priority_queue) for "active" points/pixels and 
	// traverse pixels to improve paths stored in "toParent" (and "dist") 
	//priority_queue<MyPoint> active; 
	queue<MyPoint> active; // TODO: PRIORITY QUEUE??? BUT SO SLOW

	// Initialize queue and set
	active.push(MyPoint(seed, dist[seed]));
	while(!active.empty())
	{
		MyPoint p = active.front(); 
		active.pop();
		region[p] = 1; // 1 = expanded
		counter++;

		// Go over all neighbours of pixel p
		for (int i = 0; i < NUMDIRS; i++) {
			Point q = p + shift[i]; // Compute "neighbour" of pixel p

			// If q is in the image and has not yet been expanded
			if (image.pointIn(q) && region[q] != 1) {

				double w_pq = penalty[q];
				if ((dist[p] + w_pq) < dist[q]) {
					dist[q] = dist[p] + w_pq;
					toParent[q] = Reverse[i];
				}

				// Only add q to active list if it is a free node
				if (region[q] == 0) {
					active.push(MyPoint(q, dist[q]));
					region[q] = 2; // 2 = active front
				}
			}
		}
		if (view && counter%60==0) {draw(); Pause(20);} // visualization, skipped if checkbox "view" is off
	} 

	cout << "paths computed,  number of 'pops' = " << counter 
		 <<  ",  number of pixels = " << (region.getWidth()*region.getHeight()) << endl; 
}

// This function is called at the end of "addToContourLast()". Function 
// "contourInterior()" returns volume of the closed contour's interior region 
// (including points on the contor itself), or -1 if contour is not closed yet.
int contourInterior()
{
	if (!closedContour || image.isEmpty()) return -1;
	int counter = 0;

	region.reset(0);
	for (unsigned i = 0; i < contour.size(); ++i) region[contour[i]] = 3;

	queue<Point> active;
	active.push(Point(0,0)); // note: non-empty image guarantees that pixel p=(0,0) is inside image
	while(!active.empty())
	{
		Point p=active.front();
		active.pop();
		region[p]=3;
		counter++;
		for (int i=0; i<4; i++) 
		{
			Point q = p+shift[i]; //using overloaded operator + (see "Point.h")
			if ( region.pointIn(q) && region[q]==0) 
			{
				region[q]=2; // active front
				active.push(q);
			}
		}
		if (view && counter%60==0) {draw(); Pause(20);} // visualization, skipped if checkbox "view" is off
	}
	return region.getWidth() * region.getHeight() - counter;
}

