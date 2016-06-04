#pragma once

#include "ofMain.h"
#include <openvr.h>
#include "Matrices.h"

//--------------------------------------------------------------
class ofxOpenVR {

public:
	void setup(std::function< void(vr::Hmd_Eye) > f);
	void exit();

	void update();
	void render();
	void renderDistortion();

	Matrix4 getHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 getHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	ofMatrix4x4 getCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);

	ofMatrix4x4 getControllerPose(vr::ETrackedControllerRole nController);

	void setDrawControllers(bool bDrawControllers);

	void showMirrorWindow();
	void hideMirrorWindow();

	void toggleGrid(float transitionDuration = 2.0f);
	void showGrid(float transitionDuration = 2.0f);
	void hideGrid(float transitionDuration = 2.0f);

private:

	struct VertexDataScene
	{
		Vector3 position;
		Vector2 texCoord;
	};

	struct VertexDataLens
	{
		Vector2 position;
		Vector2 texCoordRed;
		Vector2 texCoordGreen;
		Vector2 texCoordBlue;
	};

	struct FramebufferDesc
	{
		GLuint _nDepthBufferId;
		GLuint _nRenderTextureId;
		GLuint _nRenderFramebufferId;
		GLuint _nResolveTextureId;
		GLuint _nResolveFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	std::function< void(vr::Hmd_Eye) > _callableRenderFunction;

	bool _bGlFinishHack;
	bool _bIsGLInit;
	bool _bIsGridVisible;

	uint32_t _nRenderWidth;
	uint32_t _nRenderHeight;

	float _fNearClip;
	float _fFarClip;

	vr::IVRSystem *_pHMD;
	vr::IVRRenderModels *_pRenderModels;
	std::string _strDriver;
	std::string _strDisplay;
	vr::TrackedDevicePose_t _rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 _rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool _rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];

	int _iTrackedControllerCount;
	int _iTrackedControllerCount_Last;
	int _iValidPoseCount;
	int _iValidPoseCount_Last;

	std::string _strPoseClasses;                            // what classes we saw poses for this frame
	char _rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	ofShader _lensShader;
	GLuint _unLensVAO;
	GLuint _glIDVertBuffer;
	GLuint _glIDIndexBuffer;
	unsigned int _uiIndexSize;

	Matrix4 _mat4HMDPose;
	Matrix4 _mat4eyePosLeft;
	Matrix4 _mat4eyePosRight;

	Matrix4 _mat4ProjectionCenter;
	Matrix4 _mat4ProjectionLeft;
	Matrix4 _mat4ProjectionRight;

	int _leftControllerDeviceID;
	int _rightControllerDeviceID;
	Matrix4 _mat4LeftControllerPose;
	Matrix4 _mat4RightControllerPose;

	bool _bDrawControllers;
	ofVboMesh _controllersVbo;
	ofShader _controllersTransformShader;

	bool init();
	bool initGL();
	bool initCompositor();

	bool createAllShaders();
	bool createFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc);

	bool setupStereoRenderTargets();
	void setupDistortion();
	void setupCameras();

	void updateDevicesMatrixPose();
	void handleInput();
	void processVREvent(const vr::VREvent_t & event);

	void renderStereoTargets();
	
	void drawControllers();
	void renderScene(vr::Hmd_Eye nEye);

	Matrix4 convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);	
};
