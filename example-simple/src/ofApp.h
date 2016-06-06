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

	ofImage _texture;
	ofBoxPrimitive _box;
	ofMatrix4x4 _translateMatrix;
	ofShader _shader;

	ofBoxPrimitive _controllerBox;
	ofShader _controllersShader;
};
