#pragma once

using namespace std;

extern Table2D<RGB> imageL; 
extern Table2D<RGB> imageR;
extern Table2D<double> disparityValues;
extern Table2D<int> disparityMap;

extern int img_width; // set in stereo.cpp
extern int img_height; // set in stereo.cpp
extern int maxDisp; // set in main.cpp
extern int wwin; // set in main.cpp
extern int hwin; // set in main.cpp
typedef unsigned pix_id; // internal (private) indices of features 

void createIntegralImg(int disp);
int window_ssd(int row, int col);
void windowBasedStereo();

void testImg();
void scanlineStereo();
void multilineStereo();
void init();


// functions for converting node indeces to Points and vice versa.
inline double sqr_rgb(RGB &a) {return (a.r * a.r) + (a.g * a.g) + (a.b * a.b);}
inline double nearest(int num, int multiple) {return (num + (multiple / 2)) / multiple * multiple;}
inline Point to_point(pix_id index) { return Point(index%img_width,index/img_width);}  
inline pix_id to_index(Point p) {return p.x+p.y*img_width;}
inline pix_id to_index(int x, int y) {return x+y*img_width;}

class vitNode {
public:
	double energy;
	int toParent;
	vitNode(const double energy1, const int toParent1) : energy(energy1), toParent(toParent1) {}
	vitNode() : energy(0), toParent(0) {};
	bool operator<(const vitNode& c) const {return energy < c.energy;}
    bool operator>(const vitNode& c) const {return energy > c.energy;}
};

inline float calculateSD()
{
    float sum = 0.0, mean, standardDeviation = 0.0;
	int num_vals = img_width*img_height;

    for(int x = 0; x < img_width; x++) 
		for (int y = 0; y < img_height; y++)
			sum += float(imageL[x][y]);

    mean = sum/num_vals;

    for(int x = 0; x < img_width; x++) 
		for (int y = 0; y < img_height; y++)
			standardDeviation += pow(float(imageL[x][y]) - mean, 2);

    return sqrt(standardDeviation / num_vals);
}