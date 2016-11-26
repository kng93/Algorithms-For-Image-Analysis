#include <iostream>     // for cout, rand
#include "Basics2D.h"
#include "Table2D.h"
#include "Math2D.h"
#include "Image2D.h"

#include "stereo.h"


// declarations of global variables
Table2D<RGB> imageL; // image is "loaded" from a BMP file by function "image_load" in "main.cpp"
Table2D<RGB> imageR; // image is "loaded" from a BMP file by function "image_load" in "main.cpp"