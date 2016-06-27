#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofGLWindowSettings settings;
	settings.setGLVersion(4, 1);
	settings.width = 1280;
	settings.height = 720;
	ofCreateWindow(settings);
	ofRunApp(new ofApp());
}
