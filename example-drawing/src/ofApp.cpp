#include "ofApp.h"

#define STRINGIFY(A) #A

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetVerticalSync(false);

	bShowHelp = true;
	bUseShader = true;
	bIsLeftTriggerPressed = false;
	bIsRightTriggerPressed = false;

	polylineResolution = .004f;
	lastLeftControllerPosition.set(ofVec3f());
	lastRightControllerPosition.set(ofVec3f());

	// We need to pass the method we want ofxOpenVR to call when rending the scene
	openVR.setup(std::bind(&ofApp::render, this, std::placeholders::_1));
	openVR.setDrawControllers(true);

	ofAddListener(openVR.ofxOpenVRControllerEvent, this, &ofApp::controllerEvent);

	// Vertex shader source
	string vertex;

	vertex = "#version 150\n";
	vertex += STRINGIFY(
						uniform mat4 matrix;

						in vec4 position;

						void main() {
							gl_Position = matrix * position;
						}
						);

	// Fragment shader source
	string fragment = "#version 150\n";
	fragment += STRINGIFY(
						out vec4 outputColor;
						void main() {
							outputColor = vec4(1.0, 1.0, 1.0, 1.0);
						}
						);

	// Shader
	shader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
	shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
	shader.bindDefaults();
	shader.linkProgram();
}

//--------------------------------------------------------------
void ofApp::exit(){
	ofRemoveListener(openVR.ofxOpenVRControllerEvent, this, &ofApp::controllerEvent);

	openVR.exit();
}

//--------------------------------------------------------------
void ofApp::update(){
	openVR.update();

	if (bIsLeftTriggerPressed) {
		if (openVR.isControllerConnected(vr::TrackedControllerRole_LeftHand)) {
			// Getting the translation component of the controller pose matrix
			leftControllerPosition = openVR.getControllerPose(vr::TrackedControllerRole_LeftHand)[3];

			if (lastLeftControllerPosition.distance(leftControllerPosition) >= polylineResolution) {
				leftControllerPolylines[leftControllerPolylines.size() - 1].lineTo(leftControllerPosition);
				lastLeftControllerPosition.set(leftControllerPosition);
			}
		}
	}

	if (bIsRightTriggerPressed) {
		if (openVR.isControllerConnected(vr::TrackedControllerRole_RightHand)) {
			// Getting the translation component of the controller pose matrix
			rightControllerPosition = openVR.getControllerPose(vr::TrackedControllerRole_RightHand)[3];

			if (lastRightControllerPosition.distance(rightControllerPosition) >= polylineResolution) {
				rightControllerPolylines[rightControllerPolylines.size() - 1].lineTo(rightControllerPosition);
				lastRightControllerPosition = rightControllerPosition;
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	openVR.render();
	openVR.renderDistortion();

	openVR.drawDebugInfo(10.0f, 500.0f);

	// Help
	if (bShowHelp) {
		_strHelp.str("");
		_strHelp.clear();
		_strHelp << "HELP (press h to toggle): " << endl;
		_strHelp << "Press the Trigger of a controller to draw a line with that specific controller." << endl;
		_strHelp << "Press the Touchpad to star a new line." << endl;
		_strHelp << "Press the Grip button to clear all the lines drawn with that specific controller." << endl;
		_strHelp << "Drawing resolution " << polylineResolution << " (press: +/-)." << endl;
		_strHelp << "Drawing default 3D models " << openVR.getRenderModelForTrackedDevices() << " (press: m)." << endl;
		ofDrawBitmapStringHighlight(_strHelp.str(), ofPoint(10.0f, 20.0f), ofColor(ofColor::black, 100.0f));
	}
}

//--------------------------------------------------------------
void  ofApp::render(vr::Hmd_Eye nEye){
	// Using a shader
	if (bUseShader) {
		ofMatrix4x4 currentViewProjectionMatrix = openVR.getCurrentViewProjectionMatrix(nEye);

		shader.begin();
		shader.setUniformMatrix4f("matrix", currentViewProjectionMatrix, 1);
		ofSetColor(ofColor::white);
		for (auto pl : leftControllerPolylines) {
			pl.draw();
		}

		for (auto pl : rightControllerPolylines) {
			pl.draw();
		}
		shader.end();
	}
	// Loading matrices
	else {
		ofPushView();
		ofSetMatrixMode(OF_MATRIX_PROJECTION);
		ofLoadMatrix(openVR.getCurrentProjectionMatrix(nEye));
		ofSetMatrixMode(OF_MATRIX_MODELVIEW);
		ofMatrix4x4 currentViewMatrixInvertY = openVR.getCurrentViewMatrix(nEye);
		currentViewMatrixInvertY.scale(1.0f, -1.0f, 1.0f);
		ofLoadMatrix(currentViewMatrixInvertY);

		ofSetColor(ofColor::white);

		for (auto pl : leftControllerPolylines) {
			pl.draw();
		}

		for (auto pl : rightControllerPolylines) {
			pl.draw();
		}
		ofPopView();
	}
}

//--------------------------------------------------------------
void ofApp::controllerEvent(ofxOpenVRControllerEventArgs& args){
	//cout << "ofApp::controllerEvent > role: " << ofToString(args.controllerRole) << " - event type: " << ofToString(args.eventType) << " - button type: " << ofToString(args.buttonType) << " - x: " << args.analogInput_xAxis << " - y: " << args.analogInput_yAxis << endl;
	// Left
	if (args.controllerRole == ControllerRole::Left) {
		// Trigger
		if (args.buttonType == ButtonType::ButtonTrigger) {
			if (args.eventType == EventType::ButtonPress) {
				bIsLeftTriggerPressed = true;

				if (leftControllerPolylines.size() == 0) {
					leftControllerPolylines.push_back(ofPolyline());
					lastLeftControllerPosition.set(ofVec3f());
				}
			}
			else if (args.eventType == EventType::ButtonUnpress) {
				bIsLeftTriggerPressed = false;
			}
		}
		// ButtonTouchpad
		else if (args.buttonType == ButtonType::ButtonTouchpad) {
			if (args.eventType == EventType::ButtonPress) {
				leftControllerPolylines.push_back(ofPolyline());
				lastLeftControllerPosition.set(ofVec3f());
			}
		}
		// Grip
		else if (args.buttonType == ButtonType::ButtonGrip) {
			if (args.eventType == EventType::ButtonPress) {
				for (auto pl : leftControllerPolylines) {
					pl.clear();
				}

				leftControllerPolylines.clear();
			}
		}
	}

	// Right
	else if (args.controllerRole == ControllerRole::Right) {
		// Trigger
		if (args.buttonType == ButtonType::ButtonTrigger) {
			if (args.eventType == EventType::ButtonPress) {
				bIsRightTriggerPressed = true;

				if (rightControllerPolylines.size() == 0) {
					rightControllerPolylines.push_back(ofPolyline());
					lastRightControllerPosition.set(ofVec3f());
				}
			}
			else if (args.eventType == EventType::ButtonUnpress) {
				bIsRightTriggerPressed = false;
			}
		}
		// ButtonTouchpad
		else if (args.buttonType == ButtonType::ButtonTouchpad) {
			if (args.eventType == EventType::ButtonPress) {
				rightControllerPolylines.push_back(ofPolyline());
				lastRightControllerPosition.set(ofVec3f());
			}
		}
		// Grip
		else if (args.buttonType == ButtonType::ButtonGrip) {
			if (args.eventType == EventType::ButtonPress) {
				for (auto pl : rightControllerPolylines) {
					pl.clear();
				}

				rightControllerPolylines.clear();
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
		case '+':
		case '=':
			polylineResolution += .0001f;
			break;
		
		case '-':
		case '_':
			polylineResolution -= .0001f;
			if (polylineResolution < 0) {
				polylineResolution = 0;
			}
			break;

		case 'h':
			bShowHelp = !bShowHelp;
			break;

		case 'm':
			openVR.setRenderModelForTrackedDevices(!openVR.getRenderModelForTrackedDevices());
			break;

		default:
			break;
	}

	cout << polylineResolution  << endl;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
