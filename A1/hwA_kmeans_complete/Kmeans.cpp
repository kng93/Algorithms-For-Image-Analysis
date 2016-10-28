#include <iostream>     // for cout
#include "cs1037lib-time.h" // for basic timing/pausing
#include <vector>
#include "Kmeans.h"
#include <time.h>       /* time */
#include <complex>
using namespace std;

FeaturesGrid::FeaturesGrid()
: m_seeds()
, m_labeling()
, m_features()
, m_dim(0)
{}

void FeaturesGrid::reset(Table2D<RGB> & im, FEATURE_TYPE ft, float w) 
{
	cout << "resetting data... " << endl;
	int height = im.getHeight(), width =  im.getWidth();
	Point p;
	m_seeds.reset(width,height,NO_LABEL); 
	m_labeling.reset(width,height,NO_LABEL); // NOTE: width and height of m_labeling table 
	                                         // are used in functions to_index() and to_Point(), 
	m_dim=0;
	m_means.clear();
	m_features.clear();      // deleting old features
	m_features.resize(width*height); // creating a new array of feature vectors

	if (ft == color || ft == colorXY) {
		m_dim+=3;  // adding color features
		for (int y=0; y<height; y++) for (int x=0; x<width; x++) { 
			pix_id n = to_index(x,y);
			m_features[n].push_back((double) im[x][y].r);
			m_features[n].push_back((double) im[x][y].g);
			m_features[n].push_back((double) im[x][y].b);
		}
	}
	if (ft == colorXY) {
		m_dim+=2;  // adding location features (XY)
		for (int y=0; y<height; y++) for (int x=0; x<width; x++) { 
			pix_id n = to_index(x,y);
			m_features[n].push_back((double) w*x);
			m_features[n].push_back((double) w*y);
		}
	}

	cout << "done" << endl;
}

void FeaturesGrid::addFeature(Table2D<double> & im)
{
	m_dim++; // adding extra feature (increasing dimensionality of feature vectors
	int height = im.getHeight(), width =  im.getWidth();
	for (int y=0; y<height; y++) for (int x=0; x<width; x++) { 
		pix_id n = to_index(x,y);
		m_features[n].push_back(im[x][y]);
	}

}

// GUI calls this function when a user left- or right-clicks on an image pixel
void FeaturesGrid::addSeed(Point& p, Label seed_type) 
{
	if (!m_seeds.pointIn(p)) return;
	Label current_constraint = m_seeds[p];
	if (current_constraint==seed_type) return;
	m_seeds[p]=seed_type;
}

inline Label FeaturesGrid::what_label(Point& p)
{
	return m_labeling[p];
}

// HELPER FUNCTION FOR clusterAlgo
void FeaturesGrid::assignLabels(bool kmeans, int rgbWinSize, int xyWinSize,
								vector<int>& lb_count, vector<vector<double>>& lb_sum) {

	int num_lbs = m_means.size();
	// Loop over all the pixels
	for (int x=0; x<m_labeling.getWidth(); x++) { 
		for (int y=0; y<m_labeling.getHeight(); y++) {	
			// Set the minimum distance
			double min_dist = -1; 
			pix_id n = to_index(x,y);

			// Try all the seed values
			for (Label lb = 0; lb < num_lbs; lb++) {
				if (m_means[lb].size() > 0) {
					double lb_dist = sqrt(dist2(m_features[n], m_means[lb]));
					
					// Get the minimum distance
					bool check_win = (kmeans) || (lb_dist < rgbWinSize);
					if ((min_dist == -1) || ((lb_dist < min_dist) && (check_win))) {
						min_dist = lb_dist;
						m_labeling[x][y] = lb;
					// Increase the count and add value to the label that is closest
					} 
					
					if ((!kmeans) && (lb_dist < rgbWinSize)) {
						lb_count[lb] += 1;
						for (unsigned i = 0; i < m_dim; i++) lb_sum[lb][i] += m_features[n][i];
					}
				}
			}

			if (kmeans) {
				int min_lb = m_labeling[x][y];
				lb_count[min_lb] += 1;
				for (unsigned i = 0; i < m_dim; i++) lb_sum[min_lb][i] += m_features[n][i];
			}
		}
	}
}

// HELPER FUNCTION FOR clusterAlgo
bool FeaturesGrid::clusterSame(bool kmeans, vector<int>& lb_count, vector<vector<double>>& lb_sum) {
	bool same = true;
	int num_lbs = m_means.size();

	// Get the mean for all the labels
	for (Label lb = 0; lb < num_lbs; lb++) {
		
		for (unsigned i = 0; i < m_dim; i++) {
			if (lb_count[lb] > 0) {
				double new_mean = lb_sum[lb][i] / lb_count[lb];
				// Keep track if the mean has changed for any of the labels
				same = same && (double_equal(new_mean, m_means[lb][i]));
				m_means[lb][i] = new_mean;
			// If meanshift - don't clear it yet
			} else if (kmeans)
				m_means[lb][i] = 0;
		}
	}

	return same;
}

// HELPER FUNCTION FOR clusterAlgo -- meanshift merge clusters
void FeaturesGrid::mergeClusters(int rgbWinSize, int xyWinSize, vector<int>& lb_count) {
	int num_lbs = m_means.size();

	for (int lb = 0; lb < num_lbs; lb++) {
		int closest_cluster = -1;
		double min_clust_dist = -1;

		for (int lb2 = 0; lb2 < num_lbs; lb2++) {
			// Only check against "alive" clusters (or clusters that are not fully "dead" yet)
			if ((lb != lb2) && (m_means[lb].size() > 0) && (m_means[lb2].size() > 0)) {
				double pt_dist = sqrt(dist2(m_means[lb], m_means[lb2]));
				if ((closest_cluster == -1) || (pt_dist < min_clust_dist)) {
					min_clust_dist = pt_dist;
					closest_cluster = lb2;
				}
			}
		}

		// If the cluster is about to die or the cluster is within a threshold distance (0.5*winSize) to another cluster
		if (((lb_count[lb] == 0) && (m_means[lb].size() > 0)) || ((min_clust_dist > -1) && (min_clust_dist < 0.1*rgbWinSize))) {
			// Set the mean to zero now
			m_means[lb].clear();

			// Relabel any points to the closest label
			for (int x = 0; x < m_labeling.getWidth(); x++) {
				for (int y = 0; y < m_labeling.getHeight(); y++) {
					if (m_labeling[x][y] == lb) {
						m_labeling[x][y] = closest_cluster;
					}
				}
			}
		}
	}
}


// HELPER FUNCTION FOR clusterAlgo -- relabel all the points
void FeaturesGrid::meanShiftClean() {
	int num_lbs = m_means.size();
	vector<int> old_idxs;

	// Get the old labels (before erasing all the empty labels)
	for (int pt = num_lbs - 1; pt >= 0; pt--) {
		if (m_means[pt].size() == 0)
			m_means.erase(m_means.begin() + pt);
		else
			old_idxs.push_back(pt);
	}

	num_lbs = m_means.size();
	// Relabel all the points
	for (int x = 0; x < m_labeling.getWidth(); x++) {
		for (int y = 0; y < m_labeling.getHeight(); y++) {
			double min_dist = -1;
			pix_id n = to_index(x,y);
			// Go through the labels
			for (int pt = 0; pt < num_lbs; pt++) {
				int pt_label = m_labeling[x][y];
				if (pt_label == old_idxs[pt])
					m_labeling[x][y] = num_lbs - pt - 1;
				// Any points without a label assigned to closest cluster
				else if (pt_label == NO_LABEL) {
					if (m_means[pt].size() > 0) {
						double cur_dist = dist2(m_features[n], m_means[pt]);
						if ((min_dist == -1) || (cur_dist < min_dist)) {
							min_dist = cur_dist;
							m_labeling[x][y] = pt;
						}
					}
				}
			}
		}
	}
}


int FeaturesGrid::clusterAlgo(bool kmeans, unsigned k, unsigned rgbWinSize, unsigned xyWinSize) {
	// Initialize seeds
	if (kmeans)
		init_means(k);
	else
		init_meanshift(rgbWinSize, xyWinSize);

	// Initialize variables
	int iter = 0; // iteration counter
	bool changed = true;
	int num_init_points = m_means.size(); // kmeans - will be k
	vector<int> lb_count;
	vector<vector<double>> lb_sum;

	cout << "computing " << (kmeans ? "kmeans" : "mean shift") << " for data..." << endl;

	while (changed && iter < MAX_ITER) {
		// Initialize label variables
		lb_count.clear(); 
		lb_sum.clear(); lb_sum.resize(num_init_points);
		for (int lb = 0; lb < num_init_points; lb++) lb_count.push_back(0);
		for (int lb = 0; lb < num_init_points; lb++) for (unsigned i = 0; i < m_dim; i++) lb_sum[lb].push_back(0.0);

		assignLabels(kmeans, rgbWinSize, xyWinSize, lb_count, lb_sum);

		bool same = clusterSame(kmeans, lb_count, lb_sum);

		// Merge Clusters (mean shift)
		if (!kmeans)
			mergeClusters(rgbWinSize, xyWinSize, lb_count);

		changed = !same;
		iter++;
		cout << " Iteration #: " << iter << endl;
	}

	if (!kmeans)
		meanShiftClean();
	cout << " number of clusters is " << m_means.size() << endl;

	if (kmeans)
		return iter;
	else
		return m_means.size();
}


int FeaturesGrid::AIC(double sparsity)
{
	const int k_size = 4;
	int k_vals[k_size] = {2, 4, 8, 11};
	cout << "computing AIC for data..." << endl;	

	// Keep track of - m_means, m_labeling, m_seeds
	Table2D<Label> min_m_seeds;
	Table2D<Label> min_m_labeling;
	vector<vector<double>> min_m_means;
	double min_dist = 0;
	unsigned min_k = 0;

	for (int i = 0; i < k_size; i++) {
		double total_dist = 0;
		clusterAlgo(true, k_vals[i]);

		// Calculate the AIC for the current k
		for (int x=0; x<m_labeling.getWidth(); x++) { 
			for (int y=0; y<m_labeling.getHeight(); y++) {
				pix_id n = to_index(x,y);
				double lb = m_labeling[x][y];
				total_dist += dist2(m_features[n], m_means[lb]);
			}
		}
		total_dist += sparsity*k_vals[i];
		cout << "AIC for " << k_vals[i] << "is: " << total_dist << endl;	


		// Set the minimum values to be called at the end
		if ((i == 0) || (min_dist > total_dist))
		{
			min_m_seeds = m_seeds;
			min_m_labeling = m_labeling;
			min_m_means = m_means;
			min_dist = total_dist;
			min_k = k_vals[i];
		}
	}

	m_seeds = min_m_seeds;
	m_labeling = min_m_labeling;
	m_means = min_m_means;

	return min_k;
}


void FeaturesGrid::init_means(unsigned k) {
	m_means.clear();
	m_means.resize(k);
	
	for (Label ln = 0; ln<k; ln++) for (unsigned i=0; i<m_dim; i++) m_means[ln].push_back(0.0); // setting to zero all mean vector components

	// WRITE YOUR CODE FOR INITIALIZING SEEDS (RANDOMLY OR FROM SEEDS IF ANY)	
	const vector<double> seed_count = setSeedMean(k);
	srand(time(NULL)); // initialize random seed

	unsigned int num_features = m_features.size();
	int loc;
	Point p;

	for (Label lb = 0; lb < k; lb++)
	{
		// If there are no seeds - randomly set seeds
		if (seed_count[lb] == 0) {
			// Get a random location in the image
			loc = rand() % num_features;

			// mean is set to the values of the seeded locations
			for (unsigned i = 0; i < m_dim; i++)
				m_means[lb][i] = m_features[loc][i];
		}
	}
}


// Helper function for init_meanshift
int FeaturesGrid::get_seed_val(int val, int init_winSize, unsigned max_size) {
	int top_val = (val + init_winSize) > max_size ? max_size : (val + init_winSize);
	int seed_val = rand() % init_winSize + val;
	seed_val = (seed_val > max_size) ? max_size : seed_val;

	return seed_val;
}


void FeaturesGrid::init_meanshift(unsigned winSize, unsigned xyWinSize) {
	
	int init_winSize = ceil((double)NUM_COLS/ ceil((double)NUM_COLS/winSize));

	// Variables for x + y
	int X_WIDTH = m_labeling.getWidth();
	int Y_HEIGHT = m_labeling.getHeight();
	int x_winSize = ceil((double)X_WIDTH / ceil((double)X_WIDTH / xyWinSize));
	int y_winSize = ceil((double)Y_HEIGHT / ceil((double)Y_HEIGHT / xyWinSize));

	srand(time(NULL)); // initialize random seed
	m_means.clear();

	int seedr, seedg, seedb, seedx, seedy;

	// Initialize the means
	for (int r = 0; r < NUM_COLS; r+=init_winSize) {
		seedr = get_seed_val(r, init_winSize, NUM_COLS);

		for (int g = 0; g < NUM_COLS; g+=init_winSize) {
			seedg = get_seed_val(g, init_winSize, NUM_COLS);

			for (int b = 0; b < NUM_COLS; b+=init_winSize) {
				seedb = get_seed_val(b, init_winSize, NUM_COLS);

				// If RGBXY
				if (m_dim > 3) {
					for (int x = 0; x < X_WIDTH; x+=x_winSize) {
						seedx = get_seed_val(x, x_winSize, X_WIDTH);

						for (int y = 0; y < Y_HEIGHT; y+=y_winSize) {
							seedy = get_seed_val(y, y_winSize, Y_HEIGHT);

							vector<double> rgbxy; rgbxy.push_back(seedr); rgbxy.push_back(seedg); 
							rgbxy.push_back(seedb); rgbxy.push_back(seedx); rgbxy.push_back(seedy);
							m_means.push_back(rgbxy);
						}
					}
				} else {
					vector<double> rgb; rgb.push_back(seedr); rgb.push_back(seedg); rgb.push_back(seedb);
					m_means.push_back(rgb);
				}

			}
		}
	}

	
	cout << "initialized mean shift" << endl;
}


// Set the mean using the seeds (if any)
// Returns a vector of the counts of seeds (if zero, no seed)
vector<double> FeaturesGrid::setSeedMean(unsigned k) {

	// Initialize the seed sum and count
	vector<vector<double>> seed_sum; seed_sum.clear(); seed_sum.resize(k);
	vector<double> seed_count; seed_count.clear(); seed_count.resize(k);
	for (Label lb = 0; lb < k; lb++) seed_count.push_back(0);
	for (Label lb = 0; lb < k; lb++) for (unsigned i = 0; i < m_dim; i++) seed_sum[lb].push_back(0.0);

	// Get the sum of all the seed values
	for (int x = 0; x < m_labeling.getWidth(); x++) {
		for (int y = 0; y < m_labeling.getHeight(); y++) {
			Label lb = m_seeds[x][y];
			pix_id n = to_index(x, y);

			if (lb < NO_LABEL) {
				seed_count[lb] += 1;
				for (unsigned i = 0; i < m_dim; i++) seed_sum[lb][i] += m_features[n][i];
			}
		}
	}

	// Get the mean for all the labels
	for (Label lb = 0; lb < k; lb++)
		for (unsigned i = 0; i < m_dim; i++)
			m_means[lb][i] = (seed_count[lb] > 0) ? seed_sum[lb][i] / seed_count[lb] : 0;

	return seed_count;
}