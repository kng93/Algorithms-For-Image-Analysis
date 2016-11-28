#pragma once

using namespace std;

extern Table2D<RGB> imageL; 
extern Table2D<RGB> imageR;
extern Table2D<int> disparityValues;
extern Table2D<int> disparityMap;

void createIntegralImg(int disp);
int window_ssd(int row, int col);
void windowBasedStereo();

inline double sqr_rgb(RGB &a) {return (a.r * a.r) + (a.g * a.g) + (a.b * a.b);}