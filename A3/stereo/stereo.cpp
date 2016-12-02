#include <iostream>     // for cout, rand
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"

#include "stereo.h"


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


void init() {
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
	// TODO: check RGB??
	Table2D<int> diffImg = crop(imageL, Point(disp, 0), Point(img_width, img_height)) - crop(imageR, Point(0, 0), Point(img_width - disp, img_height));
	double diff_width = diffImg.getWidth();
	double diff_height = diffImg.getHeight();

	// Create the integral image!
	integralImg.reset(diffImg.getWidth(), diffImg.getHeight(), 0);
	for (int r = 0; r < integralImg.getWidth(); r++) {
		for (int c = 0; c < integralImg.getHeight(); c++) {
			// Set each pixel to the sum of pixels in the box (0,0) to (r,c)
			double check1 = diffImg[r][c];
			integralImg[r][c] = diffImg[r][c] * diffImg[r][c]; // ssd for pixel
			double check2 = integralImg[r][c];
			if ((r == 0) && (c > 0))
				integralImg[r][c] = integralImg[r][c] + integralImg[r][c - 1];
			else if ((c == 0) && (r > 0))
				integralImg[r][c] = integralImg[r][c] + integralImg[r - 1][c];
			else if ((r > 0) && (c > 0))
				integralImg[r][c] = integralImg[r][c] + integralImg[r][c-1] + integralImg[r-1][c] - integralImg[r-1][c-1];
			double check3 = integralImg[r][c];
			double more = 0;
		}
	}
}

// Get the ssd of the window given the top left corner
int window_ssd(int row, int col) 
{
	init();

	// index 0 -> index of window bottom is subtracted by one (e.g. w=10 -- (0,0)->(9,9)
	int width_idx = row + w_width - 1; 
	int height_idx = col + w_height - 1;
	
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
	int maxDisp = 60;//ceil(double(imageL.getWidth() / 35));

	// Set disparity image size -- (n - (w-1)) x (m - (w-1))
	disparityValues.reset(imageL.getWidth() - (w_width - 1), imageL.getHeight() - (w_height - 1), INFTY);
	disparityMap.reset(imageL.getWidth() - (w_width - 1), imageL.getHeight() - (w_height - 1), 0);

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
				if (disparityValues[r][c] > window_val) {
					disparityValues[r][c] = window_val; // Figure out lowest SSD = disparityValues
					disparityMap[r][c] = nearest(d,10)*5; // Set the disparityMap = lowest disparity
				}
			}
		}
	}
}



void testImg() {
	imageL.reset(3,3,RGB(0,0,0));
	imageR.reset(3,3,RGB(0,0,0));
	imageL[0][0] = RGB(10,10,10);
	imageL[1][0] = RGB(15,15,15);
	imageL[2][0] = RGB(20,20,20);
	imageL[0][1] = RGB(25,25,25);
	imageL[1][1] = RGB(30,30,30);
	imageL[2][1] = RGB(35,35,35);
	imageL[0][2] = RGB(40,40,40);
	imageL[1][2] = RGB(45,45,45);
	imageL[2][2] = RGB(50,50,50);

	imageR[0][0] = RGB(0,0,0);
	imageR[1][0] = RGB(10,10,10);
	imageR[2][0] = RGB(15,15,15);
	imageR[0][1] = RGB(0,0,0);
	imageR[1][1] = RGB(25,25,25);
	imageR[2][1] = RGB(30,30,30);
	imageR[0][2] = RGB(0,0,0);
	imageR[1][2] = RGB(40,40,40);
	imageR[2][2] = RGB(45,45,45);
}

// Implement scanline stereo using Viterbi algorithm to optimize the sum of photo-consistensies and smoothness along each scan line.
// Compare three smoothness terms penalizing quadratic, absoulte, and "truncated" absolute differences between neighboring pixels' disparities
// Try two versions with and without "static cues"
void scanlineStereo()
{
	// TODO: find a way to get the max disparity. Setting to 1/3 image for now
	int topDisp = 30; //ceil(double(imageL.getWidth() / 35));
	//testImg();
	init();


	// Set disparity image size
	disparityValues.reset(imageL.getWidth(), imageL.getHeight(), INFTY);
	disparityMap.reset(imageL.getWidth(), imageL.getHeight(), 0);
	
	// For every line...
	for (int y = 0; y < img_height; y++) {
		// Set up container
		Table2D<vitNode> energy; 
		vitNode bestEnergy(INFTY, 0);
		int end_x = 0;
		energy.reset(img_width + 1, topDisp, vitNode(0,0));

		// Iterate over points in a scanline
		for (int x = 0; x < img_width; x++) { 
			int maxDisp = (x + topDisp) >= img_width ? img_width - x : topDisp;

			// Iterate over all the disparities for RIGHT+1 pixel
			for (int d1 = 0; d1 < maxDisp; d1++) {
				vitNode minEnergy(INFTY, 0);
				double min_d2en = INFTY;
				int min_d2idx = 0;

				// Iterate over all disparities for RIGHT pixel
				for (int d2 = 0; d2 < maxDisp; d2++) {
					double oldEnergy = energy[x][d2].energy;
					double diff = abs(dI(imageL[x+d1][y], imageR[x+d2][y]));
					double curEnergy = oldEnergy + diff + abs(d1 - d2);

					if (curEnergy < minEnergy.energy)
						minEnergy = vitNode(curEnergy, d2);
				}

				energy[x+1][d1] = minEnergy;
				//if (y == 10)
				//	cout << energy[x+1][d1].energy << " ";

				// Get the minimum state of the last node
				// img_width - 2 because index off-by-one and looking at +1 pixel
				if ((x >= img_width-1-topDisp) && (minEnergy.energy/x < bestEnergy.energy)) {
					bestEnergy.toParent = d1;
					bestEnergy.energy = minEnergy.energy/x;
					end_x = x;
				}
			}
			//if (y == 10)
			//	cout << endl;
		}


		//int curState = bestEnergy.toParent;
		//for (int i=end_x; i > 0; i--) { //for (int i=(img_width-1); i > 0; i--) {
		//	disparityValues[i][y] = bestEnergy.energy;
		//	disparityMap[i][y] = bestEnergy.toParent * 10; //nearest(bestEnergy.toParent,10)*5;
		//	bestEnergy = energy[i][bestEnergy.toParent];
		//}

		int curState = bestEnergy.toParent;
		for (int i=end_x; i > 0; i--) { //for (int i=(img_width-1); i > 0; i--) {
			disparityValues[i][y] = bestEnergy.energy;
			curState = bestEnergy.toParent;
			bestEnergy = energy[i][curState];
			disparityMap[i][y] = abs(curState - bestEnergy.toParent) * 10; //nearest(bestEnergy.toParent,10)*5;
		}
	}

	cout << "Hello" << endl;

}