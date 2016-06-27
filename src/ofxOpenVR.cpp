#include "ofxOpenVR.h"

#define STRINGIFY(A) #A

//--------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device 
//          property and turn it into a std::string
//--------------------------------------------------------------
std::string getTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}


//--------------------------------------------------------------
//--------------------------------------------------------------
void ofxOpenVR::setup(std::function< void(vr::Hmd_Eye) > f)
{
	
	// Store the user's callable render function 
	_callableRenderFunction = f;

	// Initialize vars
	_bIsGLInit = false;
	_pHMD = NULL;
	_pRenderModels = NULL;
	_bGlFinishHack = true;
	_unLensVAO = 0;
	_iTrackedControllerCount = 0;
	_leftControllerDeviceID = -1;
	_rightControllerDeviceID = -1;
	_iTrackedControllerCount_Last = -1;
	_iValidPoseCount = 0;
	_iValidPoseCount_Last = -1;
	_bDrawControllers = false;
	_bIsGridVisible = false;
	_clearColor.set(.08f, .08f, .08f, 1.0f);

	_controllersVbo.setMode(OF_PRIMITIVE_LINES);
	_controllersVbo.disableTextures();

	init();
}

//--------------------------------------------------------------
void ofxOpenVR::exit()
{
	if (vr::VRCompositor()->IsMirrorWindowVisible()) {
		hideMirrorWindow();
	}

	if (_pHMD)
	{
		vr::VR_Shutdown();
		_pHMD = NULL;
	}

	if (_bIsGLInit)
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(nullptr, nullptr);
		glDeleteBuffers(1, &_glIDVertBuffer);
		glDeleteBuffers(1, &_glIDIndexBuffer);

		glDeleteRenderbuffers(1, &leftEyeDesc._nDepthBufferId);
		glDeleteTextures(1, &leftEyeDesc._nRenderTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc._nRenderFramebufferId);
		glDeleteTextures(1, &leftEyeDesc._nResolveTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc._nResolveFramebufferId);

		glDeleteRenderbuffers(1, &rightEyeDesc._nDepthBufferId);
		glDeleteTextures(1, &rightEyeDesc._nRenderTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc._nRenderFramebufferId);
		glDeleteTextures(1, &rightEyeDesc._nResolveTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc._nResolveFramebufferId);

		if (_unLensVAO != 0)
		{
			glDeleteVertexArrays(1, &_unLensVAO);
		}
	}

}


//--------------------------------------------------------------
void ofxOpenVR::update()
{
	// for now as fast as possible
	if (_pHMD)
	{
		handleInput();
		drawControllers();
	}

	// Spew out the controller and pose count whenever they change.
	if (_iTrackedControllerCount != _iTrackedControllerCount_Last || _iValidPoseCount != _iValidPoseCount_Last)
	{
		_iValidPoseCount_Last = _iValidPoseCount;
		_iTrackedControllerCount_Last = _iTrackedControllerCount;

		//printf("PoseCount:%d(%s) Controllers:%d\n", _iValidPoseCount, _strPoseClasses.c_str(), _iTrackedControllerCount);
		printf("PoseCount:%d Controllers:%d\n", _iValidPoseCount, _iTrackedControllerCount);
	}

	updateDevicesMatrixPose();
}

//--------------------------------------------------------------
void ofxOpenVR::render()
{
	// for now as fast as possible
	if (_pHMD)
	{
		renderStereoTargets(); 

		vr::Texture_t leftEyeTexture = { (void*)leftEyeDesc._nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)rightEyeDesc._nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	if (_bGlFinishHack)
	{
		//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		glFinish();
	}
}

//--------------------------------------------------------------
Matrix4 ofxOpenVR::getHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!_pHMD)
		return Matrix4();

	vr::HmdMatrix44_t mat = _pHMD->GetProjectionMatrix(nEye, _fNearClip, _fFarClip, vr::API_OpenGL);

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

//--------------------------------------------------------------
Matrix4 ofxOpenVR::getHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!_pHMD)
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = _pHMD->GetEyeToHeadTransform(nEye);
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return matrixObj.invert();
}

//--------------------------------------------------------------
ofMatrix4x4 ofxOpenVR::getCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
	Matrix4 matMVP;
	if (nEye == vr::Eye_Left)
	{
		matMVP = _mat4ProjectionLeft * _mat4eyePosLeft * _mat4HMDPose;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = _mat4ProjectionRight * _mat4eyePosRight *  _mat4HMDPose;
	}

	ofMatrix4x4 matrix(matMVP.get());
	return matrix;
}

//--------------------------------------------------------------
ofMatrix4x4 ofxOpenVR::getCurrentProjectionMatrix(vr::Hmd_Eye nEye)
{
	Matrix4 matP;
	if (nEye == vr::Eye_Left)
	{
		matP = _mat4ProjectionLeft;
	}
	else if (nEye == vr::Eye_Right)
	{
		matP = _mat4ProjectionRight;
	}

	ofMatrix4x4 matrix(matP.get());
	return matrix;
}

//--------------------------------------------------------------
ofMatrix4x4 ofxOpenVR::getCurrentViewMatrix(vr::Hmd_Eye nEye)
{
	Matrix4 matV;
	if (nEye == vr::Eye_Left)
	{
		matV = _mat4eyePosLeft * _mat4HMDPose;
	}
	else if (nEye == vr::Eye_Right)
	{
		matV = _mat4eyePosRight *  _mat4HMDPose;
	}

	ofMatrix4x4 matrix(matV.get());
	return matrix;
}

//--------------------------------------------------------------
ofMatrix4x4 ofxOpenVR::getControllerPose(vr::ETrackedControllerRole nController)
{
	ofMatrix4x4 matrix;

	if (nController == vr::TrackedControllerRole_LeftHand) {
		matrix.set(_mat4LeftControllerPose.get());
	}
	else if (nController == vr::TrackedControllerRole_RightHand) {
		matrix.set(_mat4RightControllerPose.get());
	}

	return matrix;
}

//--------------------------------------------------------------
bool ofxOpenVR::isControllerConnected(vr::ETrackedControllerRole nController)
{
	if (_pHMD) {
		if (_iTrackedControllerCount > 0) {
			if (nController == vr::TrackedControllerRole_LeftHand) {
				return _pHMD->IsTrackedDeviceConnected(_leftControllerDeviceID);
			}
			else if (nController == vr::TrackedControllerRole_RightHand) {
				return _pHMD->IsTrackedDeviceConnected(_rightControllerDeviceID);
			}
		}
	}

	return false;
}

//--------------------------------------------------------------
void ofxOpenVR::setDrawControllers(bool bDrawControllers)
{
	_bDrawControllers = bDrawControllers;
}

//--------------------------------------------------------------
void ofxOpenVR::setClearColor(ofFloatColor color)
{
	_clearColor.set(color);
}

//--------------------------------------------------------------
void ofxOpenVR::showMirrorWindow()
{
	vr::VRCompositor()->ShowMirrorWindow();
}

//--------------------------------------------------------------
void ofxOpenVR::hideMirrorWindow()
{
	vr::VRCompositor()->HideMirrorWindow();
}

//--------------------------------------------------------------
void ofxOpenVR::toggleGrid(float transitionDuration)
{
	_bIsGridVisible = !_bIsGridVisible;
	vr::VRCompositor()->FadeGrid(transitionDuration, _bIsGridVisible);
}

//--------------------------------------------------------------
void ofxOpenVR::showGrid(float transitionDuration)
{
	if (!_bIsGridVisible) {
		_bIsGridVisible = true;
		vr::VRCompositor()->FadeGrid(transitionDuration, _bIsGridVisible);
	}
}

//--------------------------------------------------------------
void ofxOpenVR::hideGrid(float transitionDuration)
{
	if (_bIsGridVisible) {
		_bIsGridVisible = false;
		vr::VRCompositor()->FadeGrid(transitionDuration, _bIsGridVisible);
	}
}

//--------------------------------------------------------------
bool ofxOpenVR::init()
{
	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		return false;
	}

	// This is not used for now
	_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
	if (!_pRenderModels)
	{
		_pHMD = NULL;
		vr::VR_Shutdown();

		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		return false;
	}
	_strTrackingSystemName = "No Driver";
	_strTrackingSystemModelNumber = "No Display";

	_strTrackingSystemName = getTrackedDeviceString(_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	_strTrackingSystemModelNumber = getTrackedDeviceString(_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);

	_fNearClip = 0.1f;
	_fFarClip = 30.0f;

	_bIsGLInit = initGL();
	if (!_bIsGLInit)
	{
		printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
		return false;
	}

	if (!initCompositor())
	{
		printf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
		return false;
	}

	return true;
}

//--------------------------------------------------------------
bool ofxOpenVR::initGL()
{
	if (!createAllShaders())
		return false;

	setupCameras();

	if (!setupStereoRenderTargets())
		return false;

	setupDistortion();

	return true;
}

//--------------------------------------------------------------
bool ofxOpenVR::initCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}

//--------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR SDL
//--------------------------------------------------------------
bool ofxOpenVR::createAllShaders()
{
	// Controller transform shader
	string vertex = "#version 410\n";
	vertex += STRINGIFY(
						uniform mat4 matrix;
						layout(location = 0) in vec4 position;
						layout(location = 1) in vec3 v3ColorIn;
						out vec4 v4Color;
						void main()
						{
							v4Color.xyz = v3ColorIn; v4Color.a = 1.0;
							gl_Position = matrix * position;
						}
					);

	string fragment = "#version 410\n";
	fragment += STRINGIFY(
						in vec4 v4Color;
						out vec4 outputColor;
						void main()
						{
							outputColor = v4Color;
						}
					);

	_controllersTransformShader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
	_controllersTransformShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
	_controllersTransformShader.bindDefaults();
	_controllersTransformShader.linkProgram();

	// Lens shader - render distortion
	vertex = "#version 410\n";
	vertex += STRINGIFY(
						layout(location = 0) in vec4 position;
						layout(location = 1) in vec2 v2UVredIn;
						layout(location = 2) in vec2 v2UVGreenIn;
						layout(location = 3) in vec2 v2UVblueIn;
						noperspective  out vec2 v2UVred;
						noperspective  out vec2 v2UVgreen;
						noperspective  out vec2 v2UVblue;
						void main()
						{
							v2UVred = v2UVredIn;
							v2UVgreen = v2UVGreenIn;
							v2UVblue = v2UVblueIn;
							gl_Position = position;
						}
					);

	fragment = "#version 410\n";
	fragment += STRINGIFY(
						uniform sampler2D mytexture;

						noperspective  in vec2 v2UVred;
						noperspective  in vec2 v2UVgreen;
						noperspective  in vec2 v2UVblue;

						out vec4 outputColor;

						void main()
						{
							float fBoundsCheck = ((dot(vec2(lessThan(v2UVgreen.xy, vec2(0.05, 0.05))), vec2(1.0, 1.0)) + dot(vec2(greaterThan(v2UVgreen.xy, vec2(0.95, 0.95))), vec2(1.0, 1.0))));
							if (fBoundsCheck > 1.0)
							{
								outputColor = vec4(0, 0, 0, 1.0);
							}
							else
							{
								float red = texture(mytexture, v2UVred).x;
								float green = texture(mytexture, v2UVgreen).y;
								float blue = texture(mytexture, v2UVblue).z;
								outputColor = vec4(red, green, blue, 1.0);
							}
						}
					);

	_lensShader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
	_lensShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
	_lensShader.bindDefaults();
	_lensShader.linkProgram();

	return true;
}

//--------------------------------------------------------------
bool ofxOpenVR::createFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc)
{
	// Still using direct OpenGL calls to create the FBO as OF does not allow the create of GL_TEXTURE_2D_MULTISAMPLE texture.
	
	glGenFramebuffers(1, &framebufferDesc._nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc._nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc._nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc._nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc._nDepthBufferId);

	glGenTextures(1, &framebufferDesc._nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc._nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc._nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc._nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc._nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc._nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc._nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc._nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

//--------------------------------------------------------------
bool ofxOpenVR::setupStereoRenderTargets()
{
	if (!_pHMD)
		return false;

	_pHMD->GetRecommendedRenderTargetSize(&_nRenderWidth, &_nRenderHeight);

	if (!createFrameBuffer(_nRenderWidth, _nRenderHeight, leftEyeDesc))
		return false;

	if (!createFrameBuffer(_nRenderWidth, _nRenderHeight, rightEyeDesc))
		return false;

	return true;
}

//--------------------------------------------------------------
void ofxOpenVR::setupDistortion()
{
	if (!_pHMD)
		return;

	GLushort _iLensGridSegmentCountH = 43;
	GLushort _iLensGridSegmentCountV = 43;

	float w = (float)(1.0 / float(_iLensGridSegmentCountH - 1));
	float h = (float)(1.0 / float(_iLensGridSegmentCountV - 1));

	float u, v = 0;

	std::vector<VertexDataLens> vVerts(0);
	VertexDataLens vert;

	//left eye distortion verts
	float Xoffset = -1;
	for (int y = 0; y<_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x<_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = _pHMD->ComputeDistortion(vr::Eye_Left, u, v);

			vert.texCoordRed = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	//right eye distortion verts
	Xoffset = 0;
	for (int y = 0; y<_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x<_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = _pHMD->ComputeDistortion(vr::Eye_Right, u, v);

			vert.texCoordRed = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	std::vector<GLushort> vIndices;
	GLushort a, b, c, d;

	GLushort offset = 0;
	for (GLushort y = 0; y<_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x<_iLensGridSegmentCountH - 1; x++)
		{
			a = _iLensGridSegmentCountH*y + x + offset;
			b = _iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}

	offset = (_iLensGridSegmentCountH)*(_iLensGridSegmentCountV);
	for (GLushort y = 0; y<_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x<_iLensGridSegmentCountH - 1; x++)
		{
			a = _iLensGridSegmentCountH*y + x + offset;
			b = _iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}
	_uiIndexSize = vIndices.size();

	glGenVertexArrays(1, &_unLensVAO);
	glBindVertexArray(_unLensVAO);

	glGenBuffers(1, &_glIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, _glIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataLens), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &_glIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _glIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//--------------------------------------------------------------
void ofxOpenVR::setupCameras()
{
	_mat4ProjectionLeft = getHMDMatrixProjectionEye(vr::Eye_Left);
	_mat4ProjectionRight = getHMDMatrixProjectionEye(vr::Eye_Right);
	_mat4eyePosLeft = getHMDMatrixPoseEye(vr::Eye_Left);
	_mat4eyePosRight = getHMDMatrixPoseEye(vr::Eye_Right);
}

//--------------------------------------------------------------
void ofxOpenVR::updateDevicesMatrixPose()
{
	if (!_pHMD)
		return;
	
	// Reset some vars.
	_iValidPoseCount = 0;
	_iTrackedControllerCount = 0;
	_leftControllerDeviceID = -1;
	_rightControllerDeviceID = -1;

	_strPoseClassesOSS.str("");
	_strPoseClassesOSS.clear();
	_strPoseClassesOSS << "FPS " << ofToString(ofGetFrameRate()) << endl;
	_strPoseClassesOSS << "Frame #" << ofToString(ofGetFrameNum()) << endl;
	_strPoseClassesOSS << endl;
	_strPoseClassesOSS << "Connected Device(s): " << endl;

	// Retrieve all tracked devices' matrix/pose.
	vr::VRCompositor()->WaitGetPoses(_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	// Go through all the tracked devices.
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			_iValidPoseCount++;

			// Keep all valid matrices.
			_rmat4DevicePose[nDevice] = convertSteamVRMatrixToMatrix4(_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);

			// Add info to the debug panel.
			switch (_pHMD->GetTrackedDeviceClass(nDevice))
			{
				case vr::TrackedDeviceClass_Controller:
					if (_pHMD->GetControllerRoleForTrackedDeviceIndex(nDevice) == vr::TrackedControllerRole_LeftHand) {
						_strPoseClassesOSS << "Controller Left" << endl;
					}
					else if (_pHMD->GetControllerRoleForTrackedDeviceIndex(nDevice) == vr::TrackedControllerRole_RightHand) {
						_strPoseClassesOSS << "Controller Right" << endl;
					}
					else {
						_strPoseClassesOSS << "Controller" << endl;
					}
					break;

				case vr::TrackedDeviceClass_HMD:
					_strPoseClassesOSS << "HMD" << endl;
					break;

				case vr::TrackedDeviceClass_Invalid:
					_strPoseClassesOSS << "Invalid Device Class" << endl;
					break;

				case vr::TrackedDeviceClass_Other:
					_strPoseClassesOSS << "Other Device Class" << endl;
					break;

				case vr::TrackedDeviceClass_TrackingReference:
					_strPoseClassesOSS << "Tracking Reference - Camera" << endl;
					break;

				default:
					_strPoseClassesOSS << "Unknown Device Class" << endl;
					break;
			}

			// Store controllers' ID and matrix. 
			if (_pHMD->GetTrackedDeviceClass(nDevice) == vr::TrackedDeviceClass_Controller) {
				_iTrackedControllerCount += 1;

				if (_pHMD->GetControllerRoleForTrackedDeviceIndex(nDevice) == vr::TrackedControllerRole_LeftHand) {
					_leftControllerDeviceID = nDevice;
					_mat4LeftControllerPose = _rmat4DevicePose[nDevice];
				}
				else if (_pHMD->GetControllerRoleForTrackedDeviceIndex(nDevice) == vr::TrackedControllerRole_RightHand) {
					_rightControllerDeviceID = nDevice;
					_mat4RightControllerPose = _rmat4DevicePose[nDevice];
				}
			}
		}
	}

	// Store HDM's matrix
	if (_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		_mat4HMDPose = _rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd].invert();
	}

	// Aad more info to the debug panel.
	_strPoseClassesOSS << endl;
	_strPoseClassesOSS << "Pose Count: " << _iValidPoseCount << endl;
    _strPoseClassesOSS << "Controller Count: " << _iTrackedControllerCount << endl;

}

//--------------------------------------------------------------
void ofxOpenVR::handleInput()
{
	// Process SteamVR events
	vr::VREvent_t event;
	while (_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		processVREvent(event);
	}

	// TODO - Check this.
	// Process SteamVR controller state
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		vr::VRControllerState_t state;
		if (_pHMD->GetControllerState(unDevice, &state))
		{
			_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;
		}
	}
}


//--------------------------------------------------------------
// Purpose: Processes a single VR event
//--------------------------------------------------------------
void ofxOpenVR::processVREvent(const vr::VREvent_t & event)
{	
	// Check device's class.
	switch (_pHMD->GetTrackedDeviceClass(event.trackedDeviceIndex))
	{
		case vr::TrackedDeviceClass_Controller:
			ofxOpenVRControllerEventArgs _args;

			// Controller's role.
			if (_pHMD->GetControllerRoleForTrackedDeviceIndex(event.trackedDeviceIndex) == vr::TrackedControllerRole_LeftHand) {
				_args.controllerRole = ControllerRole::Left;
			}
			else if (_pHMD->GetControllerRoleForTrackedDeviceIndex(event.trackedDeviceIndex) == vr::TrackedControllerRole_RightHand) {
				_args.controllerRole = ControllerRole::Right;
			}
			else {
				_args.controllerRole = ControllerRole::Unknown;
			}

			// Get extra data about the controller.
			vr::VRControllerState_t pControllerState;
			vr::VRSystem()->GetControllerState(event.trackedDeviceIndex, &pControllerState);

			_args.analogInput_xAxis = -1;
			_args.analogInput_yAxis = -1;

			// Button type
			switch (event.data.controller.button) {
				case vr::k_EButton_System:
				{
					_args.buttonType = ButtonType::ButtonSystem;
				}
				break;

				case vr::k_EButton_ApplicationMenu:
				{
					_args.buttonType = ButtonType::ButtonApplicationMenu;
				}
				break;

				case vr::k_EButton_Grip:
				{
					_args.buttonType = ButtonType::ButtonGrip;
				}
				break;

				case vr::k_EButton_SteamVR_Touchpad:
				{
					_args.buttonType = ButtonType::ButtonTouchpad;

					_args.analogInput_xAxis = pControllerState.rAxis->x;
					_args.analogInput_yAxis = pControllerState.rAxis->y;
				}
				break;

				case vr::k_EButton_SteamVR_Trigger:
				{
					_args.buttonType = ButtonType::ButtonTrigger;
				}
				break;
			}

			// Check event's type.
			switch (event.eventType) {
				case vr::VREvent_ButtonPress:
				{
					_args.eventType = EventType::ButtonPress;
				}
				break;

				case vr::VREvent_ButtonUnpress:
				{
					_args.eventType = EventType::ButtonUnpress;
				}
				break;

				case vr::VREvent_ButtonTouch:
				{
					_args.eventType = EventType::ButtonTouch;
				}
				break;

				case vr::VREvent_ButtonUntouch:
				{
					_args.eventType = EventType::ButtonUntouch;
				}
				break;
			}

			ofNotifyEvent(ofxOpenVRControllerEvent, _args);
			break;

		case vr::TrackedDeviceClass_HMD:
			break;

		case vr::TrackedDeviceClass_Invalid:
			break;

		case vr::TrackedDeviceClass_Other:
			break;

		case vr::TrackedDeviceClass_TrackingReference:
			break;

		default:
			break;
	}
	
	// TODO - Continue from here...
	// Check event's type.
	switch (event.eventType) {
		case vr::VREvent_TrackedDeviceActivated:
		{
			//SetupRenderModelForTrackedDevice(event.trackedDeviceIndex);
			printf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
		}
		break;

		case vr::VREvent_TrackedDeviceDeactivated:
		{
			printf("Device %u detached.\n", event.trackedDeviceIndex);
		}
		break;	

		case vr::VREvent_TrackedDeviceUpdated:
		{
			printf("Device %u updated.\n", event.trackedDeviceIndex);
		}
		break;

		case vr::VREvent_ButtonPress:
		{
			//_leftControllerDeviceID
			vr::VRControllerState_t pControllerState;
			vr::VRSystem()->GetControllerState(event.trackedDeviceIndex, &pControllerState);

			if (_pHMD->GetControllerRoleForTrackedDeviceIndex(event.trackedDeviceIndex) == vr::TrackedControllerRole_LeftHand) {
			//if (event.trackedDeviceIndex == _leftControllerDeviceID) {
				printf("L Device %u button press %s - x:%4.2f - y:%4.2f.\n", event.trackedDeviceIndex, vr::VRSystem()->GetButtonIdNameFromEnum((vr::EVRButtonId)event.data.controller.button), pControllerState.rAxis->x, pControllerState.rAxis->y);
				//GetControllerAxisTypeNameFromEnum(EVRControllerAxisType eAxisType)	
			}
			else {
				printf("R Device %u button press %s - x:%4.2f - y:%4.2f.\n", event.trackedDeviceIndex, vr::VRSystem()->GetButtonIdNameFromEnum((vr::EVRButtonId)event.data.controller.button), pControllerState.rAxis->x, pControllerState.rAxis->y);
			}
		}
		break;

		case vr::VREvent_ButtonUnpress:
		{
			printf("Device %u button unpress %s.\n", event.trackedDeviceIndex, vr::VRSystem()->GetButtonIdNameFromEnum((vr::EVRButtonId)event.data.controller.button));
		}
		break;

		case vr::VREvent_ButtonTouch:
		{
			printf("Device %u button touch %s.\n", event.trackedDeviceIndex, vr::VRSystem()->GetButtonIdNameFromEnum((vr::EVRButtonId)event.data.controller.button));
		}
		break;

		case vr::VREvent_ButtonUntouch:
		{
			printf("Device %ubutton untouch %s.\n", event.trackedDeviceIndex, vr::VRSystem()->GetButtonIdNameFromEnum((vr::EVRButtonId)event.data.controller.button));
		}
		break;
	}			
}

//--------------------------------------------------------------
void ofxOpenVR::renderStereoTargets()
{
	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc._nRenderFramebufferId);
	glViewport(0, 0, _nRenderWidth, _nRenderHeight);
	renderScene(vr::Eye_Left);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc._nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc._nResolveFramebufferId);

	glBlitFramebuffer(0, 0, _nRenderWidth, _nRenderHeight, 0, 0, _nRenderWidth, _nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);


	glEnable(GL_MULTISAMPLE);

	// Right Eye
	glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc._nRenderFramebufferId);
	glViewport(0, 0, _nRenderWidth, _nRenderHeight);
	renderScene(vr::Eye_Right);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc._nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc._nResolveFramebufferId);

	glBlitFramebuffer(0, 0, _nRenderWidth, _nRenderHeight, 0, 0, _nRenderWidth, _nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

//--------------------------------------------------------------
// Purpose: Draw all of the controllers as X/Y/Z lines
//--------------------------------------------------------------
void ofxOpenVR::drawControllers()
{
	// Don't draw controllers if somebody else has input focus
	if (_pHMD->IsInputFocusCapturedByAnotherProcess())
		return;

	_controllersVbo.clear();
	
	// Left controller
	if (_pHMD->IsTrackedDeviceConnected(_leftControllerDeviceID)) {
		Vector4 center = _mat4LeftControllerPose * Vector4(0, 0, 0, 1);

		for (int i = 0; i < 3; ++i)
		{
			Vector3 color(0, 0, 0);
			Vector4 point(0, 0, 0, 1);
			point[i] += 0.05f;  // offset in X, Y, Z
			color[i] = 1.0;  // R, G, B
			point = _mat4LeftControllerPose * point;

			_controllersVbo.addVertex(ofVec3f(center.x, center.y, center.z));
			_controllersVbo.addColor(ofFloatColor(color.x, color.y, color.z));

			_controllersVbo.addVertex(ofVec3f(point.x, point.y, point.z));
			_controllersVbo.addColor(ofFloatColor(color.x, color.y, color.z));
		}
	}
	
	// Right controller
	if (_pHMD->IsTrackedDeviceConnected(_rightControllerDeviceID)) {
		Vector4 center = _mat4RightControllerPose * Vector4(0, 0, 0, 1);

		for (int i = 0; i < 3; ++i)
		{
			Vector3 color(0, 0, 0);
			Vector4 point(0, 0, 0, 1);
			point[i] += 0.05f;  // offset in X, Y, Z
			color[i] = 1.0;  // R, G, B
			point = _mat4RightControllerPose * point;

			_controllersVbo.addVertex(ofVec3f(center.x, center.y, center.z));
			_controllersVbo.addColor(ofFloatColor(color.x, color.y, color.z));

			_controllersVbo.addVertex(ofVec3f(point.x, point.y, point.z));
			_controllersVbo.addColor(ofFloatColor(color.x, color.y, color.z));
		}
	}

}

//--------------------------------------------------------------
void ofxOpenVR::renderScene(vr::Hmd_Eye nEye)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Don't continue if somebody else has input focus
	if (_pHMD->IsInputFocusCapturedByAnotherProcess())
	{
		return;
	}

	// Draw the controllers
	if (_bDrawControllers) {
		_controllersTransformShader.begin();
		_controllersTransformShader.setUniformMatrix4f("matrix", getCurrentViewProjectionMatrix(nEye), 1);
		_controllersVbo.draw();
		_controllersTransformShader.end();
	}

	// User's render function
	_callableRenderFunction(nEye);
}

//--------------------------------------------------------------
void ofxOpenVR::renderDistortion()
{
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, ofGetWidth(), ofGetHeight());

	glBindVertexArray(_unLensVAO);
	_lensShader.begin();

	//render left lens (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEyeDesc._nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glDrawElements(GL_TRIANGLES, _uiIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	//render right lens (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEyeDesc._nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glDrawElements(GL_TRIANGLES, _uiIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(_uiIndexSize));

	glBindVertexArray(0);
	_lensShader.end();
}

//--------------------------------------------------------------
void ofxOpenVR::drawDebugInfo(float x, float y)
{

	_strPoseClassesOSS << endl;
	_strPoseClassesOSS << "System Name: " << _strTrackingSystemName << endl;
	_strPoseClassesOSS << "System S/N: " << _strTrackingSystemModelNumber << endl;

	ofDrawBitmapStringHighlight(_strPoseClassesOSS.str(), ofPoint(x, y), ofColor(ofColor::black, 100.0f));
}

//--------------------------------------------------------------
Matrix4 ofxOpenVR::convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}

