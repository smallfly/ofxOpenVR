#include "ofApp.h"

#define STRINGIFY(A) #A

//--------------------------------------------------------------
void ofApp::setup(){
	
	ofSetVerticalSync(false);

	// We need to pass the method we want ofxOpenVR to call when rending the scene
	_openVR.setup(std::bind(&ofApp::render, this, std::placeholders::_1));
	_openVR.setDrawControllers(true);

	ofAddListener(_openVR.ofxOpenVRControllerEvent, this, &ofApp::controllerEvent);
	
	_light.setup();
	_light.setPosition(0, 0, 0);
	_light.enable();

	for (int i = 0; i < 50; i++) {
		Cube* cube = new Cube();

		float size = 2;

		ofVec3f pos;
		pos.x = ofRandom(-20, 20);
		pos.y = ofRandom(-20, 20);
		pos.z = ofRandom(-20, 20);
		
		cube->box.set(size);
		cube->box.setPosition(pos);

		ofFloatColor c;
		c.r = ofRandomuf();
		c.g = ofRandomuf();
		c.b = ofRandomuf();

		cube->color.set(c);
				
		_cubes.push_back(cube);
	}
}

//--------------------------------------------------------------
void ofApp::exit() {
	
	ofRemoveListener(_openVR.ofxOpenVRControllerEvent, this, &ofApp::controllerEvent);

	_openVR.exit();
}

//--------------------------------------------------------------
void ofApp::update(){
	
	_openVR.update();
	
	float t = ofGetElapsedTimef() * 0.3;
	
	_cameraPos.x = cos( t ) * 6;
	_cameraPos.z = sin( t ) * 6;
	
	_openVR.applyTranslation(_cameraPos);
}

//--------------------------------------------------------------
void ofApp::draw(){
	
	ofEnableLighting();
	ofEnableDepthTest();
	
	_openVR.render();
	_openVR.renderDistortion();

	ofDisableDepthTest();
	_openVR.drawDebugInfo();
}

//--------------------------------------------------------------
void  ofApp::render(vr::Hmd_Eye nEye)
{
	ofPushView();
	ofSetMatrixMode(OF_MATRIX_PROJECTION);
	ofLoadMatrix(_openVR.getCurrentProjectionMatrix(nEye));
	ofSetMatrixMode(OF_MATRIX_MODELVIEW);
	ofMatrix4x4 currentViewMatrixInvertY = _openVR.getCurrentViewMatrix(nEye);
	currentViewMatrixInvertY.scale(1.0f, -1.0f, 1.0f);
	ofLoadMatrix(currentViewMatrixInvertY);
	
	// boxes in space
	for (int i = 0; i < 50; i++){
		ofSetColor(_cubes[i]->color);
		_cubes[i]->box.draw();
	}

	ofPopView();
}

//--------------------------------------------------------------
void ofApp::controllerEvent(ofxOpenVRControllerEventArgs& args)
{
	cout << "ofApp::controllerEvent > role: " << (int)args.controllerRole << " - event type: " << (int)args.eventType << " - button type: " << (int)args.buttonType << " - x: " << args.analogInput_xAxis << " - y: " << args.analogInput_yAxis << endl;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	
	_openVR.toggleGrid();
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

