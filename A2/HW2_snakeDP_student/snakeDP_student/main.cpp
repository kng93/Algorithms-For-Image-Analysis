#include "cs1037lib-window.h" // for basic keyboard/mouse interface 
#include "cs1037lib-button.h" // for basic buttons, check-boxes, etc... 
#include <iostream>           // for cout
#include "Cstr.h"             // convenient methods for generating C-style text strings
#include <vector>
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"
#include "snake.h" // declarations of global variables and functions shared with wire.cpp


// declarations of global variables used for GUI controls/buttons/dropLists
const char* image_names[] = { "logo" , "uwocampus", "canada", "original" , "model" , "liver" }; // an array of image file names
int im_index = 1;    // index of currently opened image (inital value)
int save_counter[] = {0,0,0,0,0,0}; // counter of saved results for each image above 
const char* mode_names[]  = { "ELASTIC", "KEEP LENGTH", "IMG GRAD", "DIST XFORM"}; // an array of mode names 
enum Mode  {ELASTIC=0, KEEP_LENGTH=1, IMG_GRAD=2, DIST_XFORM=3}; 
Mode mode = ELASTIC; // index of the current mode (in the array 'mode_names')
int modeVal = 0;
bool flagGD = false; // flag set by the check box
const int cp_height = 34; // height of "control panel" (area for buttons)
const int pad = 10; // width of extra "padding" around image (inside window)
int dP_box; // handle for dP-box
int check_box_flag; // handle for 'flag' check-box
char c='\0'; // stores last pressed key

// declarations of global functions used in GUI (see code below "main")
void left_click(Point click);   // call-back function for left mouse-clicks
void right_click(Point click);  // call-back function for right mouse-clicks
void image_load(int index); // call-back function for dropList selecting image file
void image_save();  // call-back function for button "Save Result"
void mode_set(int index);   // call-back function for dropList selecting mode 
void clear();  // call-back function for button "Clear"
void flag_set(bool v);  // call-back function for check box for "flag" 
void dP_set(const char* dP_string);  // call-back function for setting parameter "dP" (interval between control points) 
void my_line(Table2D<RGB>& im, Point a, Point b);
void my_ellipse(Table2D<RGB>& im, Point c);


int main()
{
 	cout << " left/right clicks - enter contour points " << endl; 
 	cout << " hit 'n'-key followed by left/right mouse click to add attract/repulse nudge" << endl;
	cout << " click 'Clear' to delete current 'contour' " << endl; 
	cout << " click 'X'-box to close the application " << endl; 
 	cout << " hit 'c'-key to converge snake to a local optima " << endl; 
 	cout << " hit 's'-key to make one DP move for the snake \n\n " << endl; 

	  // initializing buttons/dropLists and the window (using cs1037utils methods)
	int blank = CreateTextLabel(""); // adds grey "control panel" for buttons/dropLists, see "cs1037utils.h"
    SetControlPosition(blank,0,0); SetControlSize(blank,1500,cp_height); // see "cs1037utils.h"
	int dropList_files = CreateDropList(6, image_names, im_index, image_load); // the last argument specifies the call-back function, see "cs1037utils.h"
	int label1 = CreateTextLabel("Mode:"); // see "cs1037utils.h"
	int dropList_modes = CreateDropList(4, mode_names, mode, mode_set); // the last argument specifies the call-back function, see "cs1037utils.h"
	int button_clear = CreateButton("Clear",clear); // the last argument specifies the call-back function, see "cs1037utils.h"
	int button_save = CreateButton("Save",image_save); // the last argument specifies the call-back function, see "cs1037utils.h"
	dP_box = CreateTextBox(to_Cstr("dP=" << dP), dP_set); // variable dP is declared in "snake.cpp" 
	check_box_flag = CreateCheckBox("GD flag" , flagGD, flag_set); // see "cs1037utils.h"
	SetWindowTitle("DP snake");      // see "cs1037utils.h"
    SetDrawAxis(pad,cp_height+pad,false); // sets window's "coordinate center" for GetMouseInput(x,y) and for all DrawXXX(x,y) functions in "cs1037utils" 
	                                      // we set it in the left corner below the "control panel" with buttons
	  // initializing the application
	image_load(im_index);
	SetWindowVisible(true); // see "cs1037utils.h"

	  // while-loop processing keys/mouse interactions 
	while (!WasWindowClosed()) // WasWindowClosed() returns true when 'X'-box is clicked
	{
		if (GetKeyboardInput(&c)) // check keyboard
		{ 
			if      ( c == 's') {DP_move();     draw(); }
			else if ( c == 'c') {DP_converge(); draw(); }
			else if ( c == 'n') {cout << "click a nudge point... " << endl; draw();}
		}

		int x, y, button;
		if (GetMouseInput(&x, &y, &button)) // checks if there are mouse clicks or mouse moves
		{
			Point mouse(x,y); // STORES PIXEL OF MOUSE CLICK
			if      (c == 'n' && button == 1) {addAttractNudge(mouse); c='\0';}
			else if (c == 'n' && button == 2) {addRepulseNudge(mouse); c='\0';}
			else if (button == 1) left_click(mouse);   // button 1 means the left mouse button
			else if (button == 2) right_click(mouse);  // button 2 means the right mouse button
			draw(mouse);
		}
	}

	  // deleting the controls
	DeleteControl(button_clear);    // see "cs1037utils.h"
	DeleteControl(dropList_files);
	DeleteControl(dropList_modes);     
	DeleteControl(label1);
	DeleteControl(dP_box);
	DeleteControl(check_box_flag);
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
	reset_segm();
	draw();
}

// call-back function for the 'image file' selection dropList
// 'int' argument specifies the index of the 'file' selected by the user among 'image_names'
void image_load(int index) 
{
	im_index = index;
	cout << "loading image file " << image_names[index] << ".bmp" << endl;
	image = loadImage<RGB>(to_Cstr(image_names[index] << ".bmp")); // global function defined in Image2D.h
	int width  = max(400,(int)image.getWidth()) + 2*pad + 80;
	int height = max(100,(int)image.getHeight())+ 2*pad + cp_height;
	SetWindowSize(width,height); // window height includes control panel ("cp")
    SetControlPosition(   dP_box,     image.getWidth()+pad+5, cp_height+pad);
    SetControlPosition(check_box_flag,image.getWidth()+pad+5, cp_height+pad+25);
	reset_segm();  // clears current "contour" and "region" objects - function in a4.cpp
	draw();
}

// call-back function for button "Clear"
void clear() { 
	reset_segm(); // clears current "contour" and "region" objects - function in snake.cpp
	draw();
}

// call-back function for check box for "view" check box 
void flag_set(bool f) {flagGD=f; draw();}  

// call-back function for setting parameter "dP" 
// (interval between control points) 
void dP_set(const char* dP_string) {
	sscanf_s(dP_string,"dP=%d",&dP);
	cout << "parameter dP is set to " << dP << endl;
}

// call-back function for left mouse-click
void left_click(Point click)
{
	if (!image.pointIn(click)) return;
	addToContour(click); // function in snake.cpp
}

// call-back function for right mouse-click
void right_click(Point click)
{
	if (!image.pointIn(click)) return;
	addToContourLast(click); // function in snake.cpp
}

// window drawing function
void draw(Point mouse)
{ 
	unsigned i;
	// Clear the window to white
	SetDrawColour(255,255,255); DrawRectangleFilled(-pad,-pad,1280,1280);
	
	if (!image.isEmpty()) drawImage(image); // draws image (object defined in snake.cpp) using global function in Image2D.h (1st draw method there)
	else {SetDrawColour(255, 0, 0); DrawText(2,2,"image was not found"); return;}

	if (!contour.empty()) 
	{	// Draws "contour" - object declared in snake.cpp
 		for (i = 0; i<(contour.size()-1); i++)  {
			SetDrawColour(255,0,0); 
			DrawLine(contour[i].x,   contour[i].y, 
					 contour[i+1].x, contour[i+1].y);
			SetDrawColour(0,0,255); 
            DrawEllipseFilled(contour[i].x, contour[i].y, 2, 2);
		}
		SetDrawColour(0,0,255);
        DrawEllipseFilled(contour[i].x, contour[i].y, 2, 2);
		SetDrawColour(255,0,0); 
		if (closedContour) // links two end points
			DrawLine(contour[i].x, contour[i].y, 
					 contour[0].x, contour[0].y);
		// If contour is 'open' and mouse is over the image, draw extra 'preview' line segment to current mouse pos.
		if (image.pointIn(mouse) && !closedContour && c!='n') 
		{
			DrawLine(contour[i].x, contour[i].y, mouse.x, mouse.y);
		}
	}
}

// call-back function for button "Save"
void image_save() 
{
	// FEEL FREE TO MODIFY THE CODE BELOW TO SAVE WHATEVER IMAGE YOU WOULD LIKE!!!
	cout << "saving the results into image file... ";
	Table2D<RGB> tmp = image;
	unsigned i;

	if (!contour.empty()) {  // SAVES RED CONTOUR (LIVE-WIRE) OVER THE ORIGINAL IMAGE 
 		for (i = 0; i<(contour.size()-1); i++)  {
			my_line(tmp,contour[i],contour[i+1]);
            my_ellipse(tmp,contour[i]);
		}
        my_ellipse(tmp,contour[i]);
		if (closedContour) my_line(tmp,contour[i],contour[0]);
	}
	
	save_counter[im_index]++;
	string name(to_Cstr("Results/result_" << image_names[im_index] << save_counter[im_index] << ".bmp"));
	saveImage(tmp, to_Cstr(name)); // saves to BMP file
	cout << name << endl; // prints the name of the saved .bmp file on the console
}

void my_line(Table2D<RGB>& im, Point a, Point b) 
{
	int x,y,x0,y0,x1,y1;
	if (a.x <= b.x) {x0 = a.x; y0 = a.y; x1 = b.x; y1 = b.y;}
	else            {x0 = b.x; y0 = b.y; x1 = a.x; y1 = a.y;}
	for (x=x0; x<x1; x++) { y = y0+(x-x0)*(y1-y0)/(x1-x0); if (im.pointIn(x,y)) im[x][y]=red;}
	if (a.y <= b.y) {x0 = a.x; y0 = a.y; x1 = b.x; y1 = b.y;}
	else            {x0 = b.x; y0 = b.y; x1 = a.x; y1 = a.y;}
	for (y=y0; y<y1; y++) { x = x0+(y-y0)*(x1-x0)/(y1-y0); if (im.pointIn(x,y)) im[x][y]=red;}
}

void my_ellipse(Table2D<RGB>& im, Point c)
{
	int x,y;
	Point p;
    for (x=-1; x<=1; x++) for (y=-1; y<=1; y++)
		if ((x*x+y*y)<=1) { p = c + Point(x,y); if (im.pointIn(p)) im[p]=blue; }
}
