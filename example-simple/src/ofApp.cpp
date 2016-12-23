#include "ofApp.h"

#define STRINGIFY(A) #A

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetVerticalSync(false);

	// We need to pass the method we want ofxOpenVR to call when rending the scene
	_openVR.setup(std::bind(&ofApp::render, this, std::placeholders::_1));
	_openVR.setDrawControllers(true);

	ofAddListener(_openVR.ofxOpenVRControllerEvent, this, &ofApp::controllerEvent);

	_texture.load("of.png");
	_texture.getTexture().setTextureWrap(GL_REPEAT, GL_REPEAT);
	
	_box.set(1);
	_box.enableColors();
	_box.mapTexCoordsFromTexture(_texture.getTexture());
	
	// Create a translation matrix to place the box in the space
	_translateMatrix.translate(ofVec3f(0.0, .0, -2.0));

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

	// Controllers
	_controllerBox.set(.1);
	_controllerBox.enableColors();

	// Vertex shader source
	vertex = "#version 150\n";
	vertex += STRINGIFY(
						uniform mat4 matrix;

						in vec4 position;
						in vec3 v3ColorIn;

						out vec4 v4Color;

						void main() {
							v4Color.xyz = v3ColorIn; v4Color.a = 1.0;
							gl_Position = matrix * position;
						}
					);

	// Fragment shader source
	fragment = "#version 150\n";
	fragment += STRINGIFY(
						in vec4 v4Color;
						out vec4 outputColor;
						void main() {
							outputColor = v4Color;
						}
					);

	// Shader
	_controllersShader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
	_controllersShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
	_controllersShader.bindDefaults();
	_controllersShader.linkProgram();

}

//--------------------------------------------------------------
void ofApp::exit() {
	ofRemoveListener(_openVR.ofxOpenVRControllerEvent, this, &ofApp::controllerEvent);

	_openVR.exit();
}

//--------------------------------------------------------------
void ofApp::update(){
	_openVR.update();
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
	// OF textured cube
	glm::mat4x4 currentViewProjectionMatrix = _openVR.getCurrentViewProjectionMatrix(nEye);
	glm::mat4x4 hdmPoseMat = _translateMatrix * currentViewProjectionMatrix;

	_shader.begin();
	_shader.setUniformMatrix4f("matrix", hdmPoseMat, 1);
	_shader.setUniformTexture("baseTex", _texture, 0);
	_box.draw();
	_shader.end();

	// Left controller
	if (_openVR.isControllerConnected(vr::TrackedControllerRole_LeftHand)) {
		glm::mat4x4 leftControllerPoseMat = currentViewProjectionMatrix * _openVR.getControllerPose(vr::TrackedControllerRole_LeftHand);

		_controllersShader.begin();
		_controllersShader.setUniformMatrix4f("matrix", leftControllerPoseMat, 1);
		_controllerBox.drawWireframe();
		_controllersShader.end();
	}

	// Right controller
	if (_openVR.isControllerConnected(vr::TrackedControllerRole_RightHand)) {
		glm::mat4x4 rightControllerPoseMat = currentViewProjectionMatrix * _openVR.getControllerPose(vr::TrackedControllerRole_RightHand);

		_controllersShader.begin();
		_controllersShader.setUniformMatrix4f("matrix", rightControllerPoseMat, 1);
		_controllerBox.drawWireframe();
		_controllersShader.end();
	}
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

