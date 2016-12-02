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

// declarations of global functions (see code below "main")
void image_load();
void display_image(Table2D<RGB> img);
void image_save();  // call-back function for button "Save Result"
void win_approach();

int main()
{
	cout << "Base Image Name: ";
	cin >> image_name;
	cout << "Loading image" << endl;
	
	int blank = CreateTextLabel(""); // adds grey "control panel" for buttons/dropLists, see "cs1037utils.h"
    SetControlPosition(blank,0,0); SetControlSize(blank,1280,cp_height); // see "cs1037utils.h"
	SetDrawAxis(pad,cp_height+pad,false);
	int button_save = CreateButton("Save",image_save); // the last argument specifies the call-back function, see "cs1037utils.h"

	image_load();
	scanlineStereo();
	//win_approach();
	display_image(disparityMap);

	  // while-loop processing keys/mouse interactions 
	while (!WasWindowClosed()) // WasWindowClosed() returns true when 'X'-box is clicked
	{
		int x = 0;
	}

	
	DeleteControl(button_save);

	return 0;
}

void image_load()
{
	imageL = loadImage<RGB>(to_Cstr(image_name << "_L.bmp")); // global function defined in Image2D.h
	imageR = loadImage<RGB>(to_Cstr(image_name << "_R.bmp"));
}

void display_image(Table2D<RGB> img)
{
	SetWindowVisible(true); // see "cs1037utils.h"
	if (!img.isEmpty()) drawImage(img); // draws image (object defined in wire.cpp) using global function in Image2D.h (1st draw method there)
}

// call-back function for button "Save"
void image_save() 
{
	cout << "saving the results into image file... " << endl;
	Table2D<RGB> tmp = imageL;


	save_counter++;
	// Create a directory if needed
	CreateDirectory(to_Cstr("Results"), NULL);
	string name(to_Cstr("Results/result_" << image_name << "_L" << save_counter << ".bmp"));
	saveImage(tmp, to_Cstr(name));
	cout << "Saved:" << name << endl;
}

void win_approach()
{
	windowBasedStereo();
}