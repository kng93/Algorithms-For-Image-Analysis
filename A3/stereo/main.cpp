#include "cs1037lib-window.h" // for basic drawing and keyboard/mouse input operations 
#include "cs1037lib-button.h" // for basic buttons and other GUI controls 
#include "Cstr.h"		// convenient macros for generating C-style text strings
#include <iostream>		// for cout
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include <cmath>
#include "Image2D.h"
#include "stereo.h"
#include <string>

#include <windows.h>

// declarations of global variables used for GUI controls/buttons
int save_counter;
string image_name;
const int cp_height = 34; // height of "control panel" (area for buttons)
const int pad = 10; // width of extra "padding" around image (inside window)
const char* mode_names[]  = { "WINDOW", "SCANLINE", "MULTILINE"}; // an array of mode names 
enum Mode  {WINDOW=0, SCANLINE=1, MULTILINE=2}; 
Mode mode = WINDOW;
int modeVal = 0;
int d_box; // handle for d-box
int wwin_box; // handle for width window-box
int hwin_box; // handle for height window-box
int maxDisp = 10;
int wwin = 5;
int hwin = 5;
char c='\0'; // stores last pressed key


// declarations of global functions (see code below "main")
void image_load();
void display_image(Table2D<RGB> img);
void image_save();  // call-back function for button "Save Result"
void mode_set(int index);   // call-back function for dropList selecting mode 
void d_set(const char* d_string);  // call-back function for setting parameter "d" (max disparity) 
void wwin_set(const char* wwin_string);  // call-back function for setting parameter "wwin" (size of width window) 
void hwin_set(const char* hwin_string);  // call-back function for setting parameter "hwin" (size of height window) 
void run_algo();

int main()
{
	cout << "Base Image Name: ";
	cin >> image_name;
	cout << "Loading image" << endl;
	
	int blank = CreateTextLabel(""); // adds grey "control panel" for buttons/dropLists, see "cs1037utils.h"
    SetControlPosition(blank,0,0); SetControlSize(blank,1280,cp_height); // see "cs1037utils.h"
	int label1 = CreateTextLabel("Mode:"); // see "cs1037utils.h"
	int dropList_modes = CreateDropList(3, mode_names, mode, mode_set); // the last argument specifies the call-back function, see "cs1037utils.h"
	int button_save = CreateButton("Save",image_save); // the last argument specifies the call-back function, see "cs1037utils.h"
	d_box = CreateTextBox(to_Cstr("d=" << maxDisp), d_set); // variable dP is declared in "snake.cpp" 
	wwin_box = CreateTextBox(to_Cstr("wwin=" << wwin), wwin_set); // variable dP is declared in "snake.cpp" 
	hwin_box = CreateTextBox(to_Cstr("hwin=" << hwin), hwin_set); // variable dP is declared in "snake.cpp" 
	SetDrawAxis(pad,cp_height+pad,false);

	image_load();
	////multilineStereo();
	////scanlineStereo();
	//windowBasedStereo();
	//display_image(disparityMap);

	  // while-loop processing keys/mouse interactions 
	while (!WasWindowClosed()) // WasWindowClosed() returns true when 'X'-box is clicked
	{
		if (GetKeyboardInput(&c)) // check keyboard
		{ 
			int x = 0;
		}
	}
	
	DeleteControl(dropList_modes);  
	DeleteControl(d_box); 
	DeleteControl(wwin_box); 
	DeleteControl(hwin_box);
	DeleteControl(button_save);

	return 0;
}

// call-back function for the 'mode' selection dropList 
// 'int' argument specifies the index of the 'mode' selected by the user among 'mode_names'
void mode_set(int index)
{
	mode = (Mode) index;
	modeVal = index;
	cout << "drawing mode is set to " << mode_names[index] << endl;
	run_algo();
}

void image_load()
{
	imageL = loadImage<RGB>(to_Cstr(image_name << "_L.bmp")); // global function defined in Image2D.h
	imageR = loadImage<RGB>(to_Cstr(image_name << "_R.bmp"));
	int width  = max(400,(int)imageL.getWidth()) + 2*pad + 80;
	int height = max(100,(int)imageL.getHeight())+ 2*pad + cp_height;

	SetWindowSize(width,height); // window height includes control panel ("cp")
    SetControlPosition(   d_box,     imageL.getWidth()+pad+5, cp_height+pad);
    SetControlPosition(   wwin_box,     imageL.getWidth()+pad+5, cp_height+pad+25);
    SetControlPosition(   hwin_box,     imageL.getWidth()+pad+5, cp_height+pad+50);

	run_algo();
}

void display_image(Table2D<RGB> img)
{
	SetWindowVisible(true); // see "cs1037utils.h"
	if (!img.isEmpty()) drawImage(img); // draws image (object defined in wire.cpp) using global function in Image2D.h (1st draw method there)
}

// call-back function for setting parameter "d" 
// (interval between control points) 
void d_set(const char* d_string) {
	sscanf_s(d_string,"d=%d",&maxDisp);
	cout << "parameter d is set to " << maxDisp << endl;
	run_algo();
}

// call-back function for setting parameter "wwin" 
// (interval between control points) 
void wwin_set(const char* wwin_string) {
	sscanf_s(wwin_string,"wwin=%d",&wwin);
	cout << "parameter wwin is set to " << wwin << endl;
	run_algo();
}

// call-back function for setting parameter "d" 
// (interval between control points) 
void hwin_set(const char* hwin_string) {
	sscanf_s(hwin_string,"d=%d",&hwin);
	cout << "parameter dP is set to " << hwin << endl;
	run_algo();
}

void run_algo()
{
	switch (mode) {
	case WINDOW:
		windowBasedStereo();
		break;
	case SCANLINE:
		scanlineStereo();
		break;
	case MULTILINE:
		multilineStereo();
		break;
	}
	display_image(disparityMap);
}

// call-back function for button "Save"
void image_save() 
{
	cout << "saving the results into image file... " << endl;
	//Table2D<RGB> tmp = imageL;


	save_counter++;
	// Create a directory if needed
	CreateDirectory(to_Cstr("Results"), NULL);
	string name(to_Cstr("Results/result_" << image_name << "_L" << save_counter << ".bmp"));
	saveImage(disparityMap, to_Cstr(name));
	cout << "Saved:" << name << endl;
}