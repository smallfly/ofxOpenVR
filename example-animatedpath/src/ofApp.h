#pragma once

#include "ofMain.h"
#include "ofxOpenVR.h"

//--------------------------------------------------------------
class ofApp : public ofBaseApp {

public:
	void setup();
	void exit();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	void render(vr::Hmd_Eye nEye);

	void controllerEvent(ofxOpenVRControllerEventArgs& args);

private:

	ofxOpenVR _openVR;
	
	struct Cube
	{
		ofBoxPrimitive box;
		ofFloatColor color;
	};

	vector<Cube*> _cubes;
	
	ofLight _light;
	glm::vec3 _cameraPos;
};
