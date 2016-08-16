#pragma once

#include "ofMain.h"
#include <openvr.h>
#include "Matrices.h"
#include "CGLRenderModel.h"

//--------------------------------------------------------------
//--------------------------------------------------------------
enum class ControllerRole
{
	Left = 0,
	Right = 1,
	Unknown = 3
};

//--------------------------------------------------------------
enum class EventType
{
	ButtonPress = 0,
	ButtonUnpress = 1,
	ButtonTouch = 2,
	ButtonUntouch = 3
};

//--------------------------------------------------------------
enum class ButtonType
{
	ButtonSystem = 0,
	ButtonApplicationMenu = 1,
	ButtonGrip = 2,
	ButtonTouchpad = 3,
	ButtonTrigger = 4
};

//--------------------------------------------------------------
//--------------------------------------------------------------
class ofxOpenVRControllerEventArgs : public ofEventArgs
{
public:
	ControllerRole controllerRole;
	ButtonType buttonType;
	EventType eventType;
	float analogInput_xAxis;
	float analogInput_yAxis;
};

//--------------------------------------------------------------
//--------------------------------------------------------------
class ofxOpenVR {

public:
	void setup(std::function< void(vr::Hmd_Eye) > f);
	void exit();

	void update();
	void render();
	void renderDistortion();

	void drawDebugInfo(float x = 10.0f, float y = 20.0f);

	Matrix4 getHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 getHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	ofMatrix4x4 getCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
	ofMatrix4x4 getCurrentProjectionMatrix(vr::Hmd_Eye nEye);
	ofMatrix4x4 getCurrentViewMatrix(vr::Hmd_Eye nEye);

	ofMatrix4x4 getCurrentHMDMatrix(); 
	void setCurrentHMDMatrix(ofMatrix4x4 & mat); 

	ofMatrix4x4 getControllerPose(vr::ETrackedControllerRole nController);
	bool isControllerConnected(vr::ETrackedControllerRole nController);

	void setDrawControllers(bool bDrawControllers);
	void setClearColor(ofFloatColor color);

	void showMirrorWindow();
	void hideMirrorWindow();

	void setRenderModelForTrackedDevices(bool bRender);
	bool getRenderModelForTrackedDevices();

	void toggleGrid(float transitionDuration = 2.0f);
	void showGrid(float transitionDuration = 2.0f);
	void hideGrid(float transitionDuration = 2.0f);

	void setNearClip(float near);
	void setFarClip(float far);
	
	ofEvent<ofxOpenVRControllerEventArgs> ofxOpenVRControllerEvent;

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
	
	ofFloatColor _clearColor;

	uint32_t _nRenderWidth;
	uint32_t _nRenderHeight;

	float _fNearClip;
	float _fFarClip;

	vr::IVRSystem *_pHMD;

	vr::IVRRenderModels *_pRenderModels;
	std::string _strTrackingSystemName;
	std::string _strTrackingSystemModelNumber;
	vr::TrackedDevicePose_t _rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 _rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

	int _iTrackedControllerCount;
	int _iTrackedControllerCount_Last;
	int _iValidPoseCount;
	int _iValidPoseCount_Last;

	std::ostringstream _strPoseClassesOSS;

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

	bool _bRenderModelForTrackedDevices;
	ofShader _renderModelsShader;
	CGLRenderModel* findOrLoadRenderModel(const char *pchRenderModelName);
	void setupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex);
	void setupRenderModels();

	std::vector< CGLRenderModel * > _vecRenderModels;
	CGLRenderModel *_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
};
