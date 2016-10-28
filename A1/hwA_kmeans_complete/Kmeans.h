/* Kmeans.h */

//	class FeaturesGrid suits the following goals:
//   - stores N-dimentional image features that can be queried as pixels (Points) in 2D image
//	 - can compute clusters using K-means allowing partial labeling of points via seeds

#pragma once
#include "Basics2D.h"
#include "Table2D.h"
#include "Image2D.h"
#include <vector>

using namespace std;

enum FEATURE_TYPE {color=0, colorXY=1};  // this is for features activated by constructor and reset functions)
                                         // should correspond to "enum Mode" in main

const unsigned NUM_COLS = 255;
const unsigned MAX_ITER = 100;
const unsigned NO_LABEL = -1; // note: k <= 12 (number of clusters)
typedef unsigned Label;  // index of cluster (0,1,2,...,k-1), NO_LABEL = 12 is a special value (see above)
typedef unsigned pix_id; // internal (private) indices of features 


class FeaturesGrid
{
public:
	// constructors/reset
	FeaturesGrid();
	void   reset(Table2D<RGB> & im, FEATURE_TYPE ft = color, float w=0);

	void addSeed(Point& p, Label seed_type); 

	void addFeature(Table2D<double> & im); // allows to add any feature type (e.g. im could be output of any filter)
	                                      // image im should have the same width and height and the original image
	                                      // used to create FeaturesGrid (inside constructor or reset function)

	inline const Table2D<Label>& getSeeds() const {return m_seeds;}  // returns the current set of seeds 
	inline const Table2D<Label>& getLabeling() const {return m_labeling;} // returns current labeling (clustering)
	inline const vector<vector<double>>& getMeans() const {return m_means;} // returns current cluster means)

	// "what_label" returns current Label of the node (cluster index)
	inline Label what_label(Point& p);

	inline unsigned getDim() {return m_dim;}

	// function that computes (or updates) clustering (labeling) of features points respecting seeds (if any). 
	int Kmeans(unsigned k); // ignores seed points with  Label>k or Label=NO_LABEL
	                        // returns the number of iterations to convergence
	int AIC(double alpha);

	int MeanShift(unsigned winSize, unsigned xyWinSize);


	int clusterAlgo(bool kmeans, unsigned k, unsigned rgbWinSize=100, unsigned xyWinSize=200);

private:

	// functions for converting node indeces to Points and vice versa.
	inline Point to_point(pix_id index) { return Point(index%m_labeling.getWidth(),index/m_labeling.getWidth());}  
	inline pix_id to_index(Point p) {return p.x+p.y*m_labeling.getWidth();}
	inline pix_id to_index(int x, int y) {return x+y*m_labeling.getWidth();}
	void init_means(unsigned k); // used to compute initial "means" in kmeans
	int get_seed_val(int val, int init_winSize, unsigned max_size); // helper function for init_meanshift
	void init_meanshift(unsigned winSize, unsigned xyWinSize);

	Table2D<Label> m_seeds;  // look-up table for storing seeds (partial labeling constraints)
	Table2D<Label> m_labeling; // labeling of image pixels
	vector<vector<double>> m_features; // internal array of N-dimentional features created from pixels intensities, colors, location, etc.
	unsigned m_dim; // stores current dimensionality of feature vectors
	vector<vector<double>> m_means; // cluster means
	vector<double> setSeedMean(unsigned k); // Getting all the seed locations

	// squared distance between two vectors of dimension m_dim
	inline double dist2(vector<double> &a, vector<double> &b) {
		double d=0.0; 
		for (unsigned i=0; i<m_dim; i++) d+=((a[i]-b[i])*(a[i]-b[i]));
		return d;
	}

	inline double norm(vector<double> &a) {
		double d=0.0;
		for (unsigned i = 0; i < m_dim; i++) d += (a[i]*a[i]);
		return sqrt(d);
	}

	// comparing doubles
	inline bool double_equal(double a, double b, double epsilon=0.001) { return std::abs(a - b) < epsilon; }

	inline double sub_dist(vector<double> &a, vector<double>& b, int start, int end) {
		vector<double> vec1(a.begin() + start, a.begin() + end);
		vector<double> vec2(b.begin() + start, b.begin() + end);

		int size = end-start;
		double d = 0.0;
		for (unsigned i=0; i<size; i++) d+=((vec1[i]-vec2[i])*(vec1[i]-vec2[i]));

		return d;
	}


	void assignLabels(bool kmeans, int rgbWinSize, int xyWinSize, 
						vector<int>& lb_count, vector<vector<double>>& lb_sum);
	bool clusterSame(bool kmeans, vector<int>& lb_count, vector<vector<double>>& lb_sum);
	void mergeClusters(int rgbWinSize, int xyWinSize, vector<int>& lb_count);
	void meanShiftClean();
};