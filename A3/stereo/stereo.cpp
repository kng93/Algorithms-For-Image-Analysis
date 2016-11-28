#include <iostream>     // for cout, rand
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"

#include "stereo.h"


// declarations of global variables
Table2D<RGB> imageL; // image is "loaded" from a BMP file by function "image_load" in "main.cpp"
Table2D<RGB> imageR; // image is "loaded" from a BMP file by function "image_load" in "main.cpp"
Table2D<int> disparityValues;
Table2D<int> disparityMap;
Table2D<int> integralImg;

int w_width = 40;
int w_height = 40;
const double INFTY = 1.e20;

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
	int width_idx = row + w_width - 1; 
	int height_idx = col + w_height - 1;
	
	// Index (0,0) - just get the value 
	int window_val = integralImg[width_idx][height_idx];

	// Index (0, x) - subtract the column box
	if ((row == 0) && (col > 0))
		window_val -= integralImg[width_idx][col];
	// Index (y, 0) - subtract the row box
	else if ((col == 0) && (row > 0))
		window_val -= integralImg[row][height_idx];
	// Index (x, y) - subtract column and row box, then add back double-subtracted area
	else if ((row > 0) && (col > 0))
		window_val = window_val - integralImg[width_idx][col] - integralImg[row][height_idx] + integralImg[row - 1][col - 1];

	return window_val;
}

// 
void windowBasedStereo()
{
	// TODO: find a way to get the max disparity. Setting to 1/3 image for now
	int maxDisp = ceil(imageL.getWidth() / 3);

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
				if (disparityValues[r][c] < window_val) {
					disparityValues[r][c] = window_val; // Figure out lowest SSD = disparityValues
					disparityMap[r][c] = d; // Set the disparityMap = lowest disparity
				}
			}
		}
	}
}