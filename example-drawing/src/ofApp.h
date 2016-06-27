#pragma once

#include "ofMain.h"
#include "ofxOpenVR.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void exit();

		void update();
		void draw();

		void render(vr::Hmd_Eye nEye);
		void controllerEvent(ofxOpenVRControllerEventArgs& args);

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		ofxOpenVR openVR;

		bool bUseShader;
		ofShader shader;

		float polylineResolution;

		vector<ofPolyline> leftControllerPolylines;
		vector<ofPolyline> rightControllerPolylines;
		bool bIsLeftTriggerPressed;
		bool bIsRightTriggerPressed;
		ofVec3f leftControllerPosition;
		ofVec3f rightControllerPosition;
		ofVec3f lastLeftControllerPosition;
		ofVec3f lastRightControllerPosition;

		bool bShowHelp;
		std::ostringstream _strHelp;
};
