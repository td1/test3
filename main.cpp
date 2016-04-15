#include "ofMain.h"
#include "picoApp.h"

int main(int argc, char *argv[])
{
    if (argc != 1) {
    	ofLog(OF_LOG_ERROR, "Usage: ./selfadjust\n");
        return 1;
    }
    ofSetLogLevel(OF_LOG_WARNING);
    ofSetupOpenGL(640, 480, OF_WINDOW); // HUNG set 1280x720 for camera
    ofRunApp( new picoApp());
}
