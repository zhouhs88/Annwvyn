#include "stdafx.h"
#include "OgreOculusRender.hpp"

#include "AnnLogger.hpp"
#include "AnnGetter.hpp"

using namespace Annwvyn;

//Static class members
bool OgreOculusRender::mirrorHMDView{ true };
Ogre::TextureUnitState* OgreOculusRender::debugTexturePlane{ nullptr };
OgreOculusRender* OgreOculusRender::OculusSelf{ nullptr };

OgreOculusRender::OgreOculusRender(std::string winName) : OgreVRRender(winName),
frontierWidth{ 100 },
Oculus{ nullptr },
vpts{ {nullptr, nullptr} },
currentFrameDisplayTime{ 0 },
lastFrameDisplayTime{ 0 },
mirrorTexture{ nullptr },
oculusMirrorTextureGLID{ 0 },
ogreMirrorTextureGLID{ 0 },
oculusRenderTextureGLID{ 0 },
renderTextureGLID{ 0 },
textureSwapChain{ nullptr },
layers{ nullptr },
perfHudMode{ ovrPerfHud_Off },
currentIndex{ 0 },
currentSessionStatusFrameIndex{ 0 },
debugViewport{ nullptr },
debugSmgr{ nullptr },
debugCam{ nullptr },
debugCamNode{ nullptr },
debugPlaneNode{ nullptr },
lastOculusPosition{ feetPosition },
lastOculusOrientation{ bodyOrientation }
{
	rendererName = "OpenGL/Oculus";
	OculusSelf = static_cast<OgreOculusRender*>(self);

	//List of bitmask for each buttons as we will test them
	touchControllersButtons[left][0] = ovrButton_X;
	touchControllersButtons[left][1] = ovrButton_Y;
	touchControllersButtons[left][2] = ovrButton_Enter;
	touchControllersButtons[left][3] = ovrButton_LThumb;
	touchControllersButtons[right][0] = ovrButton_A;
	touchControllersButtons[right][1] = ovrButton_B;
	touchControllersButtons[right][2] = 0; //This button is the Oculus Dashboard button. Be false all the time
	touchControllersButtons[right][3] = ovrButton_RThumb;

	//Initialize the vector<bool>s that will hold the processed button states
	for (const auto side : { left, right })
	{
		currentControllerButtonsPressed[side].resize(touchControllersButtons[side].size(), false);
		lastControllerButtonsPressed[side].resize(touchControllersButtons[side].size(), false);
	}
}

OgreOculusRender::~OgreOculusRender()
{
	//Destroy any Oculus SDK related objects
	ovr_DestroyTextureSwapChain(Oculus->getSession(), textureSwapChain);
	ovr_DestroyMirrorTexture(Oculus->getSession(), mirrorTexture);
	delete Oculus;

	//Clean the Ogre environment
	root->destroySceneManager(debugSmgr);
	root->destroySceneManager(smgr);

	//Release shared pointers from Ogre before deleting Root
	DebugPlaneMaterial.setNull();
	rttTexture.setNull();
}

bool OgreOculusRender::shouldQuit()
{
	return getSessionStatus().ShouldQuit == ovrTrue;
}

bool OgreOculusRender::shouldRecenter()
{
	return getSessionStatus().ShouldRecenter == ovrTrue;
}

bool OgreOculusRender::isVisibleInHmd()
{
	return getSessionStatus().IsVisible == ovrTrue;
}

void OgreOculusRender::cycleDebugHud()
{
	//Loop through the perf HUD mode available
	perfHudMode = (perfHudMode + 1) % ovrPerfHud_Count;

	//Set the current perf hud mode
	ovr_SetInt(Oculus->getSession(), "PerfHudMode", perfHudMode);
}

void OgreOculusRender::changeViewportBackgroundColor(Ogre::ColourValue color)
{
	//save the color then apply it to each viewport
	backgroundColor = color;

	//Render buffers
	for (auto eye : eyeUpdateOrder)
		if (vpts[eye])
			vpts[eye]->setBackgroundColour(backgroundColor);

	//Debug window
	if (debugViewport && !mirrorHMDView)
		debugViewport->setBackgroundColour(backgroundColor);
}

void OgreOculusRender::debugPrint()
{
	for (auto eye : eyeUpdateOrder)
	{
		AnnDebug() << "eyeCamera " << eye << " " << eyeCameras[eye]->getPosition();
		AnnDebug() << eyeCameras[eye]->getOrientation();
	}
}

inline Ogre::Vector3 OgreOculusRender::oculusToOgreVect3(const ovrVector3f & v)
{
	return Ogre::Vector3{ v.x, v.y, v.z };
}

inline Ogre::Quaternion OgreOculusRender::oculusToOgreQuat(const ovrQuatf & q)
{
	return Ogre::Quaternion{ q.w, q.x, q.y, q.z };
}

void OgreOculusRender::recenter()
{
	ovr_RecenterTrackingOrigin(Oculus->getSession());
}

void OgreOculusRender::initVrHmd()
{
	//Class to get basic information from the Rift. Initialize the RiftSDK
	Oculus = new OculusInterface();
	hmdSize = Oculus->getHmdDesc().Resolution;
	updateTime = 1.0 / double(Oculus->getHmdDesc().DisplayRefreshRate);

	ovr_GetSessionStatus(Oculus->getSession(), &sessionStatus);
	ovr_SetTrackingOriginType(Oculus->getSession(), ovrTrackingOrigin_FloorLevel);

	auto playerEyeHeight = ovr_GetFloat(Oculus->getSession(), "EyeHeight", -1.0f);
	if (playerEyeHeight != -1.0f) AnnGetPlayer()->setEyesHeight(playerEyeHeight);

	AnnDebug() << "Player eye height : " << playerEyeHeight << "m";
	AnnDebug() << "Eye leveling translation : " << AnnGetPlayer()->getEyeTranslation();
}

void OgreOculusRender::setMonoFov(float degreeFov) const
{
	if (monoCam) monoCam->setFOVy(Ogre::Degree(degreeFov));
}

void OgreOculusRender::initScene()
{
	//Get if the complied buffer are correct
	if (!debugPlaneSanityCheck())
		throw std::runtime_error("Error : " + std::to_string(ANN_ERR_NOTINIT) + "Sanity check failed, check the static buffer in OgreOculusRender.hpp");

	//Create the scene manager for the engine
	smgr = root->createSceneManager("OctreeSceneManager", "OSM_SMGR");
	smgr->setShadowTechnique(Ogre::ShadowTechnique::SHADOWTYPE_STENCIL_ADDITIVE);

	//We are done with the main scene. The "smgr" Scene Manager will handle the actual VR world.
	//To easily display the debug view, we will create a "debugSmgr" scene just for re-projecting the textures to the window

	//Create the scene manager for the debug output
	debugSmgr = root->createSceneManager(Ogre::ST_GENERIC);
	debugSmgr->setAmbientLight(Ogre::ColourValue::White); //no shadow

	//Create the camera with a 16:9 ratio in Orthographic projection
	debugCam = debugSmgr->createCamera("DebugRender");
	debugCam->setAutoAspectRatio(true);
	//Don't really care about the depth buffer here. The only geometry is at distance = 1.0f
	debugCam->setNearClipDistance(0.1f);
	debugCam->setFarClipDistance(1.1f);

	//Orthographic projection, none of that perspective rubbish here :p
	debugCam->setProjectionType(Ogre::PT_ORTHOGRAPHIC);

	//Set the orthographic window to match the quad we will construct
	debugCam->setOrthoWindow(debugPlaneGeometry[0], debugPlaneGeometry[1]);

	//Attach the camera to a node
	debugCamNode = debugSmgr->getRootSceneNode()->createChildSceneNode();
	debugCamNode->attachObject(debugCam);

	//Base setup inside the scene manager
	debugPlaneNode = debugCamNode->createChildSceneNode();
	debugPlaneNode->setPosition(0, 0, -1);

	//Create a manual object
	auto debugPlane = debugSmgr->createManualObject("DebugPlane");

	//Create a manual material and add a texture unit state to the default pass
	DebugPlaneMaterial = Ogre::MaterialManager::getSingleton().create("DebugPlaneMaterial", "General", true);
	debugTexturePlane = DebugPlaneMaterial.getPointer()->getTechnique(0)->getPass(0)->createTextureUnitState();

	//Describe the manual object
	debugPlane->begin("DebugPlaneMaterial", Ogre::RenderOperation::OT_TRIANGLE_STRIP);

	//Theses buffers are statically declared on the class.
	for (const auto point : debugPlaneIndexBuffer)
	{
		debugPlane->position(AnnVect3{ debugPlaneVertexBuffer[point].data() });
		debugPlane->textureCoord(AnnVect2{ debugPlaneTextureCoord[point].data() });
	}

	//We're done!
	debugPlane->end();

	//Add it to the scene
	debugPlaneNode->attachObject(debugPlane);
	debugPlaneNode->setVisible(true);
}

///This will create the Oculus Textures and the Ogre textures for rendering and mirror display
void OgreOculusRender::initRttRendering()
{
	//Init GLEW here to be able to call OpenGL functions
	AnnDebug() << "Init GL Extension Wrangler";

	loadOpenGLFunctions();

	//Get texture size from ovr with the maximal FOV for each eye
	const auto texSizeL = ovr_GetFovTextureSize(Oculus->getSession(), ovrEye_Left, Oculus->getHmdDesc().DefaultEyeFov[left], 1.f);
	const auto texSizeR = ovr_GetFovTextureSize(Oculus->getSession(), ovrEye_Right, Oculus->getHmdDesc().DefaultEyeFov[right], 1.f);

	//Calculate the render buffer size for both eyes. The width of the frontier is the number of unused pixel between the two eye buffer.
	//Apparently, keeping them glued together make some slight bleeding.
	bufferSize.w = texSizeL.w + texSizeR.w + frontierWidth;
	bufferSize.h = std::max(texSizeL.h, texSizeR.h);

	//To use SSAA, just make the buffer bigger
	if (UseSSAA)
	{
		if (AALevel / 2 > 0)
		{
			bufferSize.w *= AALevel / 2;
			bufferSize.h *= AALevel / 2;
			frontierWidth *= AALevel / 2;
		}
		AALevel = 0;
	}
	AnnDebug() << "Buffer texture size : " << bufferSize.w << " x " << bufferSize.h << " px";

	//Define the creation option of the texture swap chain
	ovrTextureSwapChainDesc textureSwapChainDesc = {};
	textureSwapChainDesc.Type = ovrTexture_2D;
	textureSwapChainDesc.ArraySize = 1;
	textureSwapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureSwapChainDesc.Width = bufferSize.w;
	textureSwapChainDesc.Height = bufferSize.h;
	textureSwapChainDesc.MipLevels = 1;
	textureSwapChainDesc.SampleCount = 1;
	textureSwapChainDesc.StaticImage = ovrFalse;

	//Request the creation of an OpenGL swapChain from the Oculus Library
	if (ovr_CreateTextureSwapChainGL(Oculus->getSession(), &textureSwapChainDesc, &textureSwapChain) != ovrSuccess)
	{
		//If we can't get the textures, there is no point trying more.
		AnnDebug("Cannot create Oculus OpenGL SwapChain");
		throw std::runtime_error("Error : " + std::to_string(ANN_ERR_RENDER) + "Cannot create Oculus OpenGL swapchain");
	}

	//Create the texture within the Ogre Texture Manager
	renderTextureGLID = createRenderTexture(bufferSize.w, bufferSize.h);

	//Calculate the actual width of the desired image on the texture in a % of the width of the buffer (as a float between 0 to 1)
	float proportionalWidth = float((bufferSize.w - frontierWidth / 2) / 2) / float(bufferSize.w);
	AnnDebug() << proportionalWidth;

	//Create viewports on the texture to render the eyeCameras
	vpts[left] = rttEyes->addViewport(eyeCameras[left], 0, 0, 0, proportionalWidth);
	vpts[right] = rttEyes->addViewport(eyeCameras[right], 1, 1.f - proportionalWidth, 0, proportionalWidth);

	//Set the background color of each viewport the 1st time
	changeViewportBackgroundColor(backgroundColor);

	//Fill in MirrorTexture parameters
	ovrMirrorTextureDesc mirrorTextureDesc = {};
	mirrorTextureDesc.Width = hmdSize.w;
	mirrorTextureDesc.Height = hmdSize.h;
	mirrorTextureDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

	//Create the Oculus Mirror Texture
	if (ovr_CreateMirrorTextureGL(Oculus->getSession(), &mirrorTextureDesc, &mirrorTexture) != ovrSuccess)
	{
		//If for some weird reason (stars alignment, dragons, northern gods, reaper invasion) we can't create the mirror texture
		AnnDebug("Cannot create Oculus mirror texture");
		throw std::runtime_error("Error : " + std::to_string(ANN_ERR_RENDER) + "Cannot create Oculus mirror texture");
	}

	auto mirror = createAdditionalRenderBuffer(hmdSize.w, hmdSize.h, "MirrorTex");
	ogreMirrorTextureGLID = std::get<1>(mirror);

	ovr_GetTextureSwapChainBufferGL(Oculus->getSession(), textureSwapChain, 0, &oculusRenderTextureGLID);

	//Attach the camera of the debug render scene to a viewport on the actual application window
	debugViewport = window->addViewport(debugCam);
	debugViewport->setBackgroundColour(Ogre::ColourValue::Black);
	debugTexturePlane->setTextureName("MirrorTex");
	debugTexturePlane->setTextureFiltering(Ogre::FO_POINT, Ogre::FO_POINT, Ogre::FO_NONE);
	debugViewport->setAutoUpdated(false);
}

void OgreOculusRender::showRawView()
{
	mirrorHMDView = false;
	if (!debugTexturePlane) return;
	OculusSelf->debugViewport->setCamera(OculusSelf->debugCam);
	debugTexturePlane->setTextureName("RttTex");
	OculusSelf->debugViewport->setBackgroundColour(OculusSelf->backgroundColor);
	AnnDebug() << "Switched to Raw view";
}

void OgreOculusRender::showMirrorView()
{
	mirrorHMDView = true;
	if (!debugTexturePlane) return;
	OculusSelf->debugViewport->setCamera(OculusSelf->debugCam);
	debugTexturePlane->setTextureName("MirrorTex");
	OculusSelf->debugViewport->setBackgroundColour(Ogre::ColourValue::Black);
	AnnDebug() << "Switched to Oculus Compositor view";
}

void OgreOculusRender::showMonscopicView()
{
	mirrorHMDView = false;
	if (!OculusSelf) return;
	OculusSelf->debugViewport->setCamera(OculusSelf->monoCam);
	OculusSelf->debugViewport->setBackgroundColour(OculusSelf->backgroundColor);
	AnnDebug() << "Switched to Monoscopic view";
}

void OgreOculusRender::initClientHmdRendering()
{
	//Populate OVR structures
	EyeRenderDesc[left] = ovr_GetRenderDesc(Oculus->getSession(), ovrEye_Left, Oculus->getHmdDesc().DefaultEyeFov[left]);
	EyeRenderDesc[right] = ovr_GetRenderDesc(Oculus->getSession(), ovrEye_Right, Oculus->getHmdDesc().DefaultEyeFov[right]);
	offset[left] = EyeRenderDesc[left].HmdToEyeOffset;
	offset[right] = EyeRenderDesc[right].HmdToEyeOffset;

	//Create a layer with our single swaptexture on it. Each side is an eye.
	layer.Header.Type = ovrLayerType_EyeFov;
	layer.Header.Flags = 0;
	layer.ColorTexture[left] = textureSwapChain;
	layer.ColorTexture[right] = textureSwapChain;
	layer.Fov[left] = EyeRenderDesc[left].Fov;
	layer.Fov[right] = EyeRenderDesc[right].Fov;

	//Define the two viewports dimensions :
	ovrRecti leftRect, rightRect;
	leftRect.Size = bufferSize;													//same size than the buffer
	leftRect.Size.w /= 2;
	leftRect.Size.w -= (frontierWidth / 2);										//but half the width
	rightRect = leftRect;														//The two rects are of the same size, but not at the same position

	//Give OVR the position of the 2 viewports
	ovrVector2i leftPos, rightPos;
	leftPos.x = 0;																//The left one start at the bottom left corner
	leftPos.y = 0;
	rightPos = leftPos;
	rightPos.x = bufferSize.w - (bufferSize.w / 2) + (frontierWidth / 2);		//But the right start at half the buffer width
	leftRect.Pos = leftPos;
	rightRect.Pos = rightPos;

	//Assign the defined viewport to the layer
	layer.Viewport[left] = leftRect;
	layer.Viewport[right] = rightRect;

	//Get the projection matrix for the desired near/far clipping from Oculus and apply them to the eyeCameras
	updateProjectionMatrix();

	//Make sure that the perf hud will not show up by himself...
	perfHudMode = ovrPerfHud_Off;
	ovr_SetInt(Oculus->getSession(), "PerfHudMode", perfHudMode);
}

void OgreOculusRender::updateProjectionMatrix()
{
	//Get the matrices from the Oculus library
	const std::array<ovrMatrix4f, ovrEye_Count> oculusProjectionMatrix
	{
		ovrMatrix4f_Projection(EyeRenderDesc[ovrEye_Left].Fov, nearClippingDistance, farClippingDistance, 0),
		ovrMatrix4f_Projection(EyeRenderDesc[ovrEye_Right].Fov, nearClippingDistance, farClippingDistance, 0)
	};

	//Put them in in Ogre's Matrix4 format
	std::array<Ogre::Matrix4, 2> ogreProjectionMatrix{};

	//For each eye
	for (const auto& eye : eyeUpdateOrder)
	{
		//Traverse the 4x4 matrix
		for (const auto x : quadIndexBuffer)
			for (const auto y : quadIndexBuffer)
				//put the number where it should
				ogreProjectionMatrix[eye][x][y] = oculusProjectionMatrix[eye].M[x][y];

		//Set the matrix
		eyeCameras[eye]->setCustomProjectionMatrix(true, ogreProjectionMatrix[eye]);
	}
}

ovrSessionStatus OgreOculusRender::getSessionStatus()
{
	if (currentSessionStatusFrameIndex != frameCounter)
	{
		ovr_GetSessionStatus(Oculus->getSession(), &sessionStatus);
		currentSessionStatusFrameIndex = frameCounter;
	}
	return sessionStatus;
}

bool OgreOculusRender::usesCustomAudioDevice()
{
	return true;
}

std::string OgreOculusRender::getAudioDeviceIdentifierSubString()
{
	return std::string("Rift Audio");
}

void OgreOculusRender::showDebug(DebugMode mode)
{
	switch (mode)
	{
	case RAW_BUFFER:
		return showRawView();
	case HMD_MIRROR:
		return showMirrorView();
	case MONOSCOPIC:
		return showMonscopicView();
	default: break;
	}
}

void OgreOculusRender::handleIPDChange()
{
	for (auto eye : eyeUpdateOrder)
		eyeCameras[eye]->setPosition(oculusToOgreVect3(EyeRenderDesc[eye].HmdToEyeOffset));
}

void OgreOculusRender::getTrackingPoseAndVRTiming()
{
	//Get timing
	lastFrameDisplayTime = currentFrameDisplayTime;
	currentFrameDisplayTime = ovr_GetPredictedDisplayTime(Oculus->getSession(), ++frameCounter);
	updateTime = currentFrameDisplayTime - lastFrameDisplayTime;

	//Reorient the headset if the runtime flags for it
	if (getSessionStatus().ShouldRecenter) recenter();

	//Get the tracking state
	ts = ovr_GetTrackingState(Oculus->getSession(),
		currentFrameDisplayTime,
		ovrTrue);

	//Update pose and controllers
	pose = ts.HeadPose.ThePose;
	ovr_CalcEyePoses(pose, offset.data(), layer.RenderPose);
	updateTouchControllers();
	handleIPDChange();

	//Apply pose to the two cameras
	trackedHeadPose.orientation = bodyOrientation * oculusToOgreQuat(pose.Orientation);
	trackedHeadPose.position = feetPosition + bodyOrientation*oculusToOgreVect3(pose.Position);
}

void OgreOculusRender::renderAndSubmitFrame()
{
	//Process window's message queue
	Ogre::WindowEventUtilities::messagePump();

	//Select the current render texture
	ovr_GetTextureSwapChainCurrentIndex(Oculus->getSession(), textureSwapChain, &currentIndex);

	//Update the relevant OpenGL IDs
	ovr_GetTextureSwapChainBufferGL(Oculus->getSession(), textureSwapChain, currentIndex, &oculusRenderTextureGLID);
	ovr_GetMirrorTextureBufferGL(Oculus->getSession(), mirrorTexture, &oculusMirrorTextureGLID);

	//Render
	root->_fireFrameRenderingQueued();
	vpts[left]->update();
	vpts[right]->update();
	rttEyes->update(); //This will resolve the sampling for the anti-aliasing of the texture

	//Copy the rendered image to the Oculus Swap Texture
	glCopyImageSubData(renderTextureGLID,
		GL_TEXTURE_2D,
		0, 0, 0, 0,
		oculusRenderTextureGLID,
		GL_TEXTURE_2D,
		0, 0, 0, 0,
		bufferSize.w, bufferSize.h, 1);

	//Get the rendering layer
	layers = &layer.Header;

	//Submit the frame
	ovr_CommitTextureSwapChain(Oculus->getSession(), textureSwapChain);
	ovr_SubmitFrame(Oculus->getSession(), frameCounter, nullptr, &layers, 1);

	//Update the render debug view if the window is visible
	if (window->isVisible())
	{
		//Put the mirrored view available for Ogre if asked for
		if (mirrorHMDView) glCopyImageSubData(oculusMirrorTextureGLID,
			GL_TEXTURE_2D,
			0, 0, 0, 0,
			ogreMirrorTextureGLID,
			GL_TEXTURE_2D,
			0, 0, 0, 0,
			hmdSize.w, hmdSize.h, 1);

		//Update the window
		debugViewport->update();
		window->update();
	}
}

void OgreOculusRender::initializeHandObjects(const oorEyeType side)
{
	//If it's the first time we have access data on this hand controller, instantiate the object
	if (!handControllers[side])
	{
		handControllers[side] = std::make_shared<AnnOculusTouchController>
			(Oculus->getSession(), smgr->getRootSceneNode()->createChildSceneNode(), size_t(side), AnnHandController::AnnHandControllerSide(side));
	}
}

void OgreOculusRender::initializeControllerAxes(const oorEyeType side, std::vector<AnnHandControllerAxis>& axesVector)
{
	axesVector.push_back(AnnHandControllerAxis{ "Thumbstick X", inputState.Thumbstick[side].x });
	axesVector.push_back(AnnHandControllerAxis{ "Thumbstick Y", inputState.Thumbstick[side].y });
	axesVector.push_back(AnnHandControllerAxis{ "Trigger X", inputState.IndexTrigger[side] });
	axesVector.push_back(AnnHandControllerAxis{ "GripTrigger X", inputState.HandTrigger[side] });
}

void OgreOculusRender::ProcessButtonStates(const oorEyeType side) {
	//Extract button states and deduce press/released events
	pressed.clear(); released.clear();
	for (auto i(0); i < currentControllerButtonsPressed[side].size(); i++)
	{
		//Save the current polled state as the last one
		lastControllerButtonsPressed[side][i] = currentControllerButtonsPressed[side][i];
		//Get the current state of the button
		currentControllerButtonsPressed[side][i] = (inputState.Buttons & touchControllersButtons[side][i]) != 0;

		//Detect pressed/released event and add it to the list
		if (!lastControllerButtonsPressed[side][i] && currentControllerButtonsPressed[side][i])
			pressed.push_back(uint8_t(i));
		else if (lastControllerButtonsPressed[side][i] && !currentControllerButtonsPressed[side][i])
			released.push_back(uint8_t(i));
	}
}

void OgreOculusRender::updateTouchControllers()
{
	//Get the controller state
	if (OVR_FAILURE(ovr_GetInputState(Oculus->getSession(), ovrControllerType_Active, &inputState))) return;
	//Check if there's Oculus Touch Data on this thing
	if (!(inputState.ControllerType & ovrControllerType_Touch)) return;

	for (const auto side : { left, right })
	{
		initializeHandObjects(side);

		//Extract the hand pose from the tracking state
		handPoses[side] = ts.HandPoses[side];

		//Get the controller
		auto handController = handControllers[side];

		//Set axis informations
		auto& axesVector = handController->getAxesVector();
		if (axesVector.size() == 0)
			initializeControllerAxes(side, axesVector);

		//Update the values of the axes
		axesVector[0].updateValue(inputState.Thumbstick[side].x);
		axesVector[1].updateValue(inputState.Thumbstick[side].y);
		axesVector[2].updateValue(inputState.IndexTrigger[side]);
		axesVector[3].updateValue(inputState.HandTrigger[side]);

		ProcessButtonStates(side);

		//Set all the data on the controller
		handController->getButtonStateVector() = currentControllerButtonsPressed[side];
		handController->getPressedButtonsVector() = pressed;
		handController->getReleasedButtonsVector() = released;
		handController->setTrackedPosition(feetPosition + bodyOrientation * oculusToOgreVect3(handPoses[side].ThePose.Position));
		handController->setTrackedOrientation(bodyOrientation * oculusToOgreQuat(handPoses[side].ThePose.Orientation));
		handController->setTrackedAngularSpeed(oculusToOgreVect3(handPoses[side].AngularVelocity));
		handController->setTrackedLinearSpeed(oculusToOgreVect3(handPoses[side].LinearVelocity));
	}
}

void AnnOculusTouchController::rumbleStart(float factor)
{
	ovrControllerType myType = {};
	if (side == leftHandController) myType = ovrControllerType_LTouch;
	else if (side == rightHandController) myType = ovrControllerType_RTouch;

	ovr_SetControllerVibration(currentSession, myType, 0, factor);
}

void AnnOculusTouchController::rumbleStop()
{
	ovrControllerType myType = {};
	if (side == leftHandController) myType = ovrControllerType_LTouch;
	else if (side == rightHandController) myType = ovrControllerType_RTouch;

	ovr_SetControllerVibration(currentSession, myType, 0, 0);
}

AnnOculusTouchController::AnnOculusTouchController(ovrSession session, Ogre::SceneNode* handNode, AnnHandControllerID controllerID, AnnHandControllerSide controllerSide) : AnnHandController("Oculus Touch", handNode, controllerID, controllerSide),
currentSession(session)
{
	if (side == leftHandController) myControllerType = ovrControllerType_LTouch;
	else if (side == rightHandController) myControllerType = ovrControllerType_RTouch;
}