#include "ofApp.h"

#define STRINGIFY(A) #A

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetVerticalSync(false);

	// We need to pass the method we want ofxOpenVR to call when rending the scene
	_openVR.setup(std::bind(&ofApp::render, this, std::placeholders::_1));
	_openVR.setDrawControllers(true);

	_texture.load("of.png");
	//_texture.getTextureReference().getTextureData().bFlipTexture = true;
	_texture.getTexture().setTextureWrap(GL_REPEAT, GL_REPEAT);
	
	_box.set(1);
	_box.enableColors();
	_box.mapTexCoordsFromTexture(_texture.getTexture());
	
	// Create a translation matrix to place the box in the space
	_translateMatrix.translate(ofVec3f(0.0, 1.0, -1.0));

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

	ofDrawBitmapStringHighlight(ofToString(ofGetFrameRate()), ofPoint(10.0f, 20.0f));
}

//--------------------------------------------------------------
void  ofApp::render(vr::Hmd_Eye nEye)
{
	ofMatrix4x4 hdmPoseMat = _translateMatrix * _openVR.getCurrentViewProjectionMatrix(nEye);

	_shader.begin();
	_shader.setUniformMatrix4f("matrix", hdmPoseMat, 1);
	_shader.setUniformTexture("baseTex", _texture, 0);
	_box.draw();
	_shader.end();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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

