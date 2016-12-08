#include <iostream>     // for cout, rand
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"

#include "stereo.h"
#include <stdio.h>
#include "graph.h"

typedef Graph<double,double,double> GraphType;

// declarations of global variables
Table2D<RGB> imageL; // image is "loaded" from a BMP file by function "image_load" in "main.cpp"
Table2D<RGB> imageR; // image is "loaded" from a BMP file by function "image_load" in "main.cpp"
Table2D<double> disparityValues;
Table2D<int> disparityMap;
Table2D<int> integralImg;

int w_width = 20;
int w_height = w_width;
int img_width = 0;
int img_height = 0;
const double INFTY = 1.e20;
const int INT_INFTY = 1.e9;

void init() {
	cout << "resetting image" << endl;

	img_width = imageL.getWidth();
	img_height = imageR.getHeight();


}


// Creates an integral image using the left and right images
// Integral image - given pixel p, value should be the sum of all pixels from origin to p
void createIntegralImg(int disp) 
{
	// Set the size of the integral image using disp
	int img_width = imageL.getWidth() - 1;
	int img_height = imageL.getHeight() - 1;

	// Get the difference of the left + right images -- converted to greyscale
	Table2D<int> diffImg = crop(imageR, Point(disp, 0), Point(img_width, img_height)) - crop(imageL, Point(0, 0), Point(img_width - disp, img_height));
	double diff_width = diffImg.getWidth();
	double diff_height = diffImg.getHeight();

	// Create the integral image!
	integralImg.reset(diffImg.getWidth(), diffImg.getHeight(), 0);
	for (int r = 0; r < integralImg.getWidth(); r++) {
		for (int c = 0; c < integralImg.getHeight(); c++) {
			// Set each pixel to the sum of pixels in the box (0,0) to (r,c)
			integralImg[r][c] = diffImg[r][c] * diffImg[r][c]; // ssd for pixel
			if ((r == 0) && (c > 0))
				integralImg[r][c] = integralImg[r][c] + integralImg[r][c - 1];
			else if ((c == 0) && (r > 0))
				integralImg[r][c] = integralImg[r][c] + integralImg[r - 1][c];
			else if ((r > 0) && (c > 0))
				integralImg[r][c] = integralImg[r][c] + integralImg[r][c-1] + integralImg[r-1][c] - integralImg[r-1][c-1];
		}
	}
}

// Get the ssd of the window given the top left corner
int window_ssd(int row, int col) 
{
	// index 0 -> index of window bottom is subtracted by one (e.g. w=10 -- (0,0)->(9,9)
	int width_idx = row + wwin - 1; 
	int height_idx = col + hwin - 1;
	
	// Index (0,0) - just get the value 
	int window_val = integralImg[width_idx][height_idx];

	// Index (0, x) - subtract the column box
	if ((row == 0) && (col > 0))
		window_val -= integralImg[width_idx][col-1];
	// Index (y, 0) - subtract the row box
	else if ((col == 0) && (row > 0))
		window_val -= integralImg[row-1][height_idx];
	// Index (x, y) - subtract column and row box, then add back double-subtracted area
	else if ((row > 0) && (col > 0))
		window_val = window_val - integralImg[width_idx][col-1] - integralImg[row-1][height_idx] + integralImg[row - 1][col - 1];

	return window_val;
}

// 
void windowBasedStereo()
{
	init();

	// TODO: find a way to get the max disparity. Setting to 1/3 image for now
	//int maxDisp = 60;//ceil(double(imageL.getWidth() / 35));

	// Set disparity image size -- (n - (w-1)) x (m - (w-1))
	disparityValues.reset(imageL.getWidth() - (wwin - 1), imageL.getHeight() - (hwin - 1), INFTY);
	disparityMap.reset(imageL.getWidth() - (wwin - 1), imageL.getHeight() - (hwin - 1), 0);

	// For each disparity
	for (int d = 0; d < maxDisp; d++) {
		// Get the integral image
		createIntegralImg(d);

		// For each pixel in disparity image
		// Only shifting to the right - always going through all the rows
		for (int r = d; r < disparityMap.getWidth() - d; r++) {
			// Shifting to the right - start at the disparity level, end at (width - disparity)
			for (int c = 0; c < disparityMap.getHeight(); c++) {
				int window_val = window_ssd(r - d, c);
				int val = disparityValues[r][c];
				if (disparityValues[r][c] >= window_val) {
					disparityValues[r][c] = window_val; // Figure out lowest SSD = disparityValues
					disparityMap[r][c] = d*5;//(maxDisp - d)*5; // Set the disparityMap = lowest disparity
				}
			}
		}
	}
}




// Implement scanline stereo using Viterbi algorithm to optimize the sum of photo-consistensies and smoothness along each scan line.
// Compare three smoothness terms penalizing quadratic, absoulte, and "truncated" absolute differences between neighboring pixels' disparities
// Try two versions with and without "static cues"
void scanlineStereo()
{
	int curDisp = maxDisp;
	//testImg();
	init();
	float std = calculateSD();


	// Set disparity image size
	disparityValues.reset(imageL.getWidth(), imageL.getHeight(), INFTY);
	disparityMap.reset(imageL.getWidth(), imageL.getHeight(), 0);
	
	// For every line...
	for (int y = 0; y < img_height; y++) {
		// Set up container
		Table2D<vitNode> energy; 
		vitNode bestEnergy(INFTY, 0);
		int end_x = 0;
		energy.reset(img_width + 1, maxDisp, vitNode(0,0));
	
		// Iterate over points in a scanline
		for (int x = 0; x < img_width; x++) { 
			int curDisp = (x + maxDisp) >= img_width ? img_width - x : maxDisp;

			// Iterate over all the disparities for RIGHT+1 pixel
			for (int d1 = 0; d1 < curDisp; d1++) {
				vitNode minEnergy(INFTY, 0);
				double min_d2en = INFTY;
				int min_d2idx = 0;

				// Iterate over all disparities for RIGHT pixel
				for (int d2 = 0; d2 < curDisp; d2++) {
					double oldEnergy = energy[x][d2].energy;
					double diff = abs(dI(imageL[x+d2][y], imageR[x+d1][y])); 
					
					double wt = 1; //exp(-pow(dI(imageR[x+d1][y], imageR[x+d2][y]), 2)/(2*std)); 
					double spatial = wt*abs(d1 - d2); // Absolute
					//Quadratic - double spatial = pow(double(d1 - d2), 2);
					// Truncated absolute - spatial = (spatial > 5) ? 5 : spatial;
					spatial = (spatial > 2) ? 2 : spatial;

					double curEnergy = oldEnergy + diff + spatial;

					if (curEnergy <= minEnergy.energy)
						minEnergy = vitNode(curEnergy, d2);
				}

				energy[x+1][d1] = minEnergy;

				// Get the minimum state of the last node
				// img_width - 2 because index off-by-one and looking at +1 pixel
				if ((x >= img_width-1-maxDisp) && (minEnergy.energy/x <= bestEnergy.energy)) {
					bestEnergy.toParent = d1;
					bestEnergy.energy = minEnergy.energy/x;
					end_x = x;
				}
			}
		}


		int curState = bestEnergy.toParent;
		//if (y == 30) {
		for (int i=img_width-1; i > 0; i--) { //for (int i=end_x; i > 0; i--) {
			disparityValues[i][y] = bestEnergy.energy;
			disparityMap[i][y] = bestEnergy.toParent * 10; //nearest(bestEnergy.toParent,10)*5;
			bestEnergy = energy[i][bestEnergy.toParent];
		}
		//}

		//int curState = bestEnergy.toParent;
		//for (int i=end_x; i > 0; i--) { //for (int i=(img_width-1); i > 0; i--) {
		//	disparityValues[i][y] = bestEnergy.energy;
		//	bestEnergy = energy[i][curState];
		//	disparityMap[i][y] = abs(curState - bestEnergy.toParent) * 10; //nearest(bestEnergy.toParent,10)*5;
		//	curState = bestEnergy.toParent;
		//}
	}

	cout << "Hello" << endl;

}

void multilineStereo() {

	//testImg();
	init();
	//int maxDisp = 10;
	int num_edges = (img_width-1)*img_height + (img_height-1)*img_width;
	float std = calculateSD();
	GraphType *g = new GraphType(img_width*img_height*maxDisp, num_edges);

	// Build the graph
	for (int d = 0; d <= maxDisp; d++) {
		// Add the the nodes for one layer
		for (int y = 0; y < img_height; y++) {
			for (int x = 0; x < img_width; x++) {
				// Add node
				g->add_node();

				int n = (img_width*img_height*d) + to_index(x, y);
				double wt = 0; 
				
				// Add the edge connecting each node with the one to the left
				if (x > 0) {
					wt = exp(-pow(dI(imageR[x-1][y], imageR[x][y]), 2)/(2*std)); 
					g->add_edge(n, n-1, wt, wt);
				}

				// Add the edge connecting each node with the one below it
				if (y > 0) {
					wt = exp(-pow(dI(imageR[x][y], imageR[x][y-1]), 2)/(2*std)); 
					g->add_edge(n, n-img_width, wt, wt);
				}
			}
		}

		// Add edges between each layer
		for (int y = 0; y < img_height; y++) {
			for (int x = 0; x < img_width; x++) {
				int n = (img_width*img_height*d) + to_index(x, y);
				
				// Link bottom layer to source
				if (d == 0)
					g->add_tweights(n, INT_INFTY, 0); // Connected to the source - should not be cut, connected to the sink - should be 
				// Link top layer to sink
				else if (d == maxDisp) {
					g->add_tweights(n, 0, INT_INFTY); // Opposite of source connection
					double wt = 0;
					wt = ((x+d) < img_width) ? abs(dI(imageR[x][y], imageL[x+d][y])) : wt;
					g->add_edge(n, n-(img_width*img_height), INT_INFTY, wt);
				} else if (d > 0) {
					double wt = 0;
					wt = ((x+d) < img_width) ? abs(dI(imageR[x][y], imageL[x+d][y])) : wt;
					g->add_edge(n, n-(img_width*img_height), INT_INFTY, wt);
				}
			}
		}
	}

	int flow = g->maxflow();
	printf("Flow=%d\n", flow);

	// Set disparity image size
	disparityValues.reset(imageR.getWidth(), imageR.getHeight(), INFTY);
	disparityMap.reset(imageR.getWidth(), imageR.getHeight(), -1);

	for (int d = 1; d <= maxDisp; d++) {
		// Add the the nodes for one layer
		for (int y = 0; y < img_height; y++) {
			for (int x = 0; x < img_width; x++) {
			// Add node
				int n = (img_width*img_height*d) + to_index(x, y);

				if ((g->what_segment(n) == GraphType::SINK) && (disparityMap[x][y] < 0)) {
					disparityMap[x][y] = (d-1)*10;// (maxDisp - d+1)*10;
				}
			}
		}
	}
	

	delete g;
}


// Test image
void testImg() {
	imageL.reset(4,3,RGB(0,0,0));
	imageR.reset(4,3,RGB(0,0,0));
	imageL[0][0] = RGB(10,10,10);
	imageL[1][0] = RGB(15,15,15);
	imageL[2][0] = RGB(20,20,20);
	imageL[3][0] = RGB(0,0,0);
	imageL[0][1] = RGB(25,25,25);
	imageL[1][1] = RGB(26,26,26);
	imageL[2][1] = RGB(30,30,30);
	imageL[3][1] = RGB(0,0,0);
	imageL[0][2] = RGB(33,33,33);
	imageL[1][2] = RGB(36,36,36);
	imageL[2][2] = RGB(37,37,37);
	imageL[3][2] = RGB(0,0,0);

	imageR[0][0] = RGB(0,0,0);
	imageR[1][0] = RGB(10,10,10);
	imageR[2][0] = RGB(15,15,15);
	imageR[3][0] = RGB(0,0,0);
	imageR[0][1] = RGB(0,0,0);
	imageR[1][1] = RGB(25,25,25);
	imageR[2][1] = RGB(36,36,36);
	imageR[3][1] = RGB(0,0,0);
	imageR[0][2] = RGB(0,0,0);
	imageR[1][2] = RGB(33,33,33);
	imageR[2][2] = RGB(36,36,36);
	imageR[3][2] = RGB(0,0,0);
}