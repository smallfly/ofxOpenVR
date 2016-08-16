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

	// Vertex shader source
	string vertex;

	vertex = "#version 150\n";
	vertex += STRINGIFY(
						uniform mat4 matrix;

						in vec4  position;
						in vec2  texcoord;

						out vec2 texCoordVarying;

						void main()
						{
							texCoordVarying = texcoord;
							gl_Position = matrix * position;

						}
					);

	// Fragment shader source
	string fragment = "#version 150\n";
	fragment += STRINGIFY(
						uniform sampler2DRect baseTex;
						
						in vec2 texCoordVarying;

						out vec4 fragColor;

						vec2 texcoord0 = texCoordVarying;

						void main() {
							vec2 tx = texcoord0;
							tx.y = 256.0 - tx.y;
							vec4 image = texture(baseTex, tx);
							fragColor = image;
						}
					);

	// Shader
	_shader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
	_shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
	_shader.bindDefaults();
	_shader.linkProgram();
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
	
	_cameraPos.x = cos( t ) * 100;
	_cameraPos.z = sin( t ) * 100;
	
	ofMatrix4x4 hmdMatrix = _openVR.getCurrentHMDMatrix();
	ofQuaternion hmdRotation = hmdMatrix.getRotate();

	ofMatrix4x4 newHmdMatrix;
	newHmdMatrix = newHmdMatrix.newTranslationMatrix( -_cameraPos);
	newHmdMatrix.rotate(hmdRotation);

	_openVR.setCurrentHMDMatrix(newHmdMatrix);
}

//--------------------------------------------------------------
void ofApp::draw(){
	
	_openVR.render();

	_openVR.renderDistortion();

	_openVR.drawDebugInfo();
}

//--------------------------------------------------------------
void  ofApp::render(vr::Hmd_Eye nEye)
{
	
	ofMatrix4x4 currentViewMatrix = _openVR.getCurrentViewMatrix(nEye);
	ofMatrix4x4 currentViewProjectionMatrix = _openVR.getCurrentViewProjectionMatrix(nEye);
	ofMatrix4x4 hdmPoseMat = currentViewProjectionMatrix;

	ofPushMatrix();

	ofMatrix4x4 currentProjMatrix = _openVR.getCurrentProjectionMatrix(nEye);
	currentProjMatrix.scale(1.0, -1.0, 1.0);

	ofSetMatrixMode(OF_MATRIX_PROJECTION);
	ofLoadMatrix(currentProjMatrix);

	ofSetMatrixMode(OF_MATRIX_MODELVIEW);
	ofLoadMatrix(_openVR.getCurrentViewMatrix(nEye));
	
	// boxes in space
	
	ofSeedRandom(0);

	for (int i = 0; i < 100; i++){

		ofVec3f pos;
		pos.x = ofRandom(-200, 200);
		pos.y = ofRandom(-200, 200);
		pos.z = ofRandom(-200, 200);

		float size = ofRandom(2, 20);
		_box.set(size);

		ofFloatColor c;
		c.r = ofRandomuf();
		c.g = ofRandomuf();
		c.b = ofRandomuf();
		
		ofSetColor(c);
		_box.setPosition(pos);
		_box.draw();
	}

	ofPopMatrix();
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

