#include "OgreOculusRender.hpp"

OgreOculusRender::OgreOculusRender(std::string winName)
{
	//Initialize some variables
	name = winName;
	root = NULL;
	window = NULL;
	smgr = NULL;

	for(size_t i(0); i < 2; i++)
	{
		cams[i] = NULL;
		rtts[i] = NULL;
		vpts[i] = NULL;
	}

	//Oc is an OculusInterface object. Communication with the Rift SDK is handeled by that class
	oc = NULL;
	CameraNode = NULL;

	cameraPosition = Ogre::Vector3(0,0,10);
	cameraOrientation = Ogre::Quaternion::IDENTITY;

	this->nearClippingDistance = (float) 0.05;
	this->lastOculusPosition = cameraPosition;
	this->lastOculusOrientation = cameraOrientation;
	this->updateTime = 0;

	fullscreen = true;
	hsDissmissed = false;

	backgroundColor = Ogre::ColourValue(0.3f,0.3f,0.9f);
}

OgreOculusRender::~OgreOculusRender()
{
	//The only thig we dynamicly load is the oculus interface
	delete oc;
}

//I may move this method back to the AnnEngine class... 
void OgreOculusRender::loadReseourceFile(const char path[])
{
	/*from ogre wiki : load the given resource file*/
	Ogre::ConfigFile configFile;
	configFile.load(path);
	Ogre::ConfigFile::SectionIterator seci = configFile.getSectionIterator();
	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
				archName, typeName, secName);
		}
	}
}

//See commant of the loadResourceFile method
void OgreOculusRender::initAllResources()
{
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void OgreOculusRender::initLibraries()
{
	//Create the ogre root with standards Ogre configuration file
	root = new Ogre::Root("plugins.cfg","ogre.cfg","Ogre.log");

	//hard code the loading of the distortions shaders
	//Ogre::ResourceGroupManager::getSingleton().addResourceLocation("./media/CORE.zip", "Zip", "OCULUS_CORE");
	//Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("OCULUS_CORE");

	//Class to get basic information from the Rift. Initialize the RiftSDK
	oc = new OculusInterface();
}

void OgreOculusRender::initialize()
{
	//init libraries;
	initLibraries();

	//Mandatory. If thouse pointers are unitilalized, program have to stop here.
	assert(root != NULL && oc != NULL);

	//Get configuration via ogre.cfg OR via the config dialog.
	getOgreConfig();

	//Create the render window with the given sice from the Oculus
	createWindow();

	//Load resources from the resources.cfg file
	loadReseourceFile("resources.cfg");
	initAllResources();

	//Create scene manager
	initScene();

	//Create cameras and handeling nodes
	initCameras();

	//Create rtts and viewports on them
	initRttRendering();

	//Init the oculus rendering
	initOculus();
}

void OgreOculusRender::getOgreConfig()
{
	//Ogre as to be initialized
	assert(root != NULL);
	//Try to resore the config from an ogre.cfg file
	if(!root->restoreConfig())
		//Open the config dialog of Ogre (even if we're ignoring part of the parameters you can input from it)
		if(!root->showConfigDialog())
			//If the user clicked the "cancel" button or other bad stuff happened during the configuration (like a dragon attack)
			abort();
}

void OgreOculusRender::createWindow()
{
	assert(root != NULL && oc != NULL);
	Ogre::NameValuePairList misc;

	//This one only works on windows : "Borderless = no decoration"
	misc["border"]="none"; //In case the program is not running in fullscreen mode, don't put windwo borders
	misc["vsync"]="true";
	misc["displayFrequency"]="75";
	misc["monitorIndex"]="1"; //Use the 2nd monitor, assuming the Oculus Rift is not the primary. Or is the only screen on the system.

	//Initialize a window ans specify that creation is manual
	window = root->initialise(false, name);
	//Actually create the window
	window = root->createRenderWindow(name, oc->getHmd()->Resolution.w, oc->getHmd()->Resolution.h, fullscreen,&misc);

	//Put the window at the place given by the SDK (usefull on linux system where the X server thinks multiscreen is a single big one...)
	window->reposition(oc->getHmd()->WindowsPos.x,oc->getHmd()->WindowsPos.y);
}

void OgreOculusRender::initCameras()
{
	assert(smgr != NULL);
	cams[left] = smgr->createCamera("lcam");
	cams[right] = smgr->createCamera("rcam");
	for(int i = 0; i < 2; i++)
	{
		cams[i]->setPosition(cameraPosition);
		cams[i]->setAutoAspectRatio(true);
		cams[i]->setNearClipDistance(1);
		cams[i]->setFarClipDistance(1000);
	}

	//do NOT attach camera to this node...
	CameraNode = smgr->getRootSceneNode()->createChildSceneNode();
}

void OgreOculusRender::setCamerasNearClippingDistance(float distance)
{
	nearClippingDistance = distance;
}

void OgreOculusRender::initScene()
{
	assert(root != NULL);
	smgr = root->createSceneManager("OctreeSceneManager","OSM_SMGR");
}

void OgreOculusRender::initRttRendering()
{
	//get texture sice from ovr with default FOV
	texSizeL = ovrHmd_GetFovTextureSize(oc->getHmd(), ovrEye_Left, oc->getHmd()->MaxEyeFov[0], 1.0f);
	texSizeR = ovrHmd_GetFovTextureSize(oc->getHmd(), ovrEye_Right, oc->getHmd()->MaxEyeFov[1], 1.0f);

	std::cerr << "Texure size to create : " << texSizeL.w << " x " <<texSizeL.h  << " px" << std::endl;
	/*
	//Create texture
	Ogre::TexturePtr rtt_textureL = Ogre::TextureManager::getSingleton().createManual("RttTexL", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, texSizeL.w, texSizeL.h, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
	Ogre::TexturePtr rtt_textureR = Ogre::TextureManager::getSingleton().createManual("RttTexR", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, texSizeR.w, texSizeR.h, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

	//Create Render Texture
	Ogre::RenderTexture* rttEyeLeft = rtt_textureL->getBuffer(0,0)->getRenderTarget();
	Ogre::RenderTexture* rttEyeRight = rtt_textureR->getBuffer(0,0)->getRenderTarget();

	//Create and bind a viewport to the texture
	Ogre::Viewport* vptl = rttEyeLeft->addViewport(cams[left]);
	vptl->setBackgroundColour(backgroundColor);
	Ogre::Viewport* vptr = rttEyeRight->addViewport(cams[right]);
	vptr->setBackgroundColour(backgroundColor);

	//Store viewport pointer
	vpts[left] = vptl;
	vpts[right] = vptr;

	//Pupulate textures with an initial render
	rttEyeLeft->update();
	rttEyeRight->update();

	//Store rtt textures pointer
	rtts[left] = rttEyeLeft;
	rtts[right] = rttEyeRight;
	*/

	rift_smgr = root->createSceneManager(Ogre::ST_GENERIC);
	mLeftEyeRenderTexture = Ogre::TextureManager::getSingleton().createManual("RttTexL", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, texSizeL.w, texSizeL.h, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
	mRightEyeRenderTexture = Ogre::TextureManager::getSingleton().createManual("RttTexR", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, texSizeR.w, texSizeR.h, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);


	


}

void OgreOculusRender::initOculus(bool fullscreenState)
{
	setFullScreen(fullscreenState);
/*
	//Get FOV
	EyeFov[left] = oc->getHmd()->MaxEyeFov[left];
	EyeFov[right] = oc->getHmd()->MaxEyeFov[right];

	//Set OpenGL configuration
	ovrGLConfig cfg;
	cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	cfg.OGL.Header.Multisample = 1;
	cfg.OGL.Header.BackBufferSize = oc->getHmd()->Resolution;
	//cfg.OGL.Header.RTSize = oc->getHmd()->Resolution;

	//OpenGL initialization differ between Windows and Linux
#ifdef _WIN32 //If windows

	//Get window
	HWND hwnd;
	window->getCustomAttribute("WINDOW",&hwnd); //potential pointer problem here
	cfg.OGL.Window = hwnd;

	//Get GL Context
	HDC dc;
	window->getCustomAttribute("HDC", &dc);
	cfg.OGL.DC = dc;

#else //Linux; untested code (yet)

	//Get X window id
	size_t wID;
	window->getCustomAttribute("WINDOW", &wID);
	std::cout << "Wid : " << wID << endl;
	cfg.OGL.Win = wID;

	//Get X Display
	Display* display;
	window->getCustomAttribute("DISPLAY", &display);
	cfg.OGL.Disp = display;

#endif

	if(!ovrHmd_ConfigureRendering(
		oc->getHmd(),
		&cfg.Config,
		oc->getHmd()->DistortionCaps,
		EyeFov,
		EyeRenderDesc))
		abort(); //if something goes wrong while configuring the rendering, abort.

	// Direct rendering from a window handle to the Hmd.
	// Not required if ovrHmdCap_ExtendDesktop flag is set.
#ifdef _WIN32
	HWND directHWND;
	window->getCustomAttribute("WINDOW", &directHWND);
	ovrHmd_AttachToWindow(oc->getHmd(), directHWND, NULL, NULL);
#else
	//Not currently available
#endif

	//Send texture data to OVR for rendering
	//->left eye texture :
	EyeTexture[left].OGL.Header.API = ovrRenderAPI_OpenGL;
	EyeTexture[left].OGL.Header.TextureSize = texSizeL;
	EyeTexture[left].OGL.Header.RenderViewport.Pos.x = 0;
	EyeTexture[left].OGL.Header.RenderViewport.Pos.y = 0;
	EyeTexture[left].OGL.Header.RenderViewport.Size = texSizeL;
	Ogre::GLTexture* gl_rtt_l = static_cast<Ogre::GLTexture*>(Ogre::GLTextureManager::getSingleton().getByName("RttTexL").get());
	EyeTexture[left].OGL.TexId = gl_rtt_l->getGLID();

	//right eye texture :
	EyeTexture[right] = EyeTexture[left]; //Basic configuration is shared.
	EyeTexture[right].OGL.Header.TextureSize = texSizeR;
	EyeTexture[right].OGL.Header.RenderViewport.Size = texSizeR;
	Ogre::GLTexture* gl_rtt_r = static_cast<Ogre::GLTexture*>(Ogre::GLTextureManager::getSingleton().getByName("RttTexR").get());
	EyeTexture[right].OGL.TexId = gl_rtt_r->getGLID();

	*/



	mMatLeft = Ogre::MaterialManager::getSingleton().getByName( "Oculus/LeftEye" );
	mMatLeft->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture( mLeftEyeRenderTexture );
	mMatRight = Ogre::MaterialManager::getSingleton().getByName( "Oculus/RightEye" );
	mMatRight->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture( mRightEyeRenderTexture );


	EyeRenderDesc[0] = ovrHmd_GetRenderDesc( oc->getHmd(), ovrEye_Left, oc->getHmd()->MaxEyeFov[0] );
	EyeRenderDesc[1] = ovrHmd_GetRenderDesc( oc->getHmd(), ovrEye_Right, oc->getHmd()->MaxEyeFov[1] );

	ovrVector2f UVScaleOffset[2];
	ovrRecti viewports[2];
	viewports[0].Pos.x = 0;
	viewports[0].Pos.y = 0;
	viewports[0].Size = texSizeL;
	viewports[1].Pos.x = 0;
	viewports[1].Pos.y = 0;
	viewports[1].Size = texSizeR;

	Ogre::SceneNode* meshNode = rift_smgr->getRootSceneNode()->createChildSceneNode();

	for (int eyeNum(0); eyeNum < 2; eyeNum++)
	{
		ovrDistortionMesh meshData;

		ovrHmd_CreateDistortionMesh(oc->getHmd(),
			EyeRenderDesc[eyeNum].Eye,
			EyeRenderDesc[eyeNum].Fov,
			0,
			&meshData);

		Ogre::GpuProgramParametersSharedPtr params;

		if(eyeNum == 0)
		{
			ovrHmd_GetRenderScaleAndOffset( EyeRenderDesc[eyeNum].Fov,
				texSizeL, viewports[eyeNum],
				UVScaleOffset);
			params = mMatLeft->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		}
		else
		{
			ovrHmd_GetRenderScaleAndOffset( EyeRenderDesc[eyeNum].Fov,
				texSizeR, viewports[eyeNum],
				UVScaleOffset);
			params = mMatRight->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		}

		params->setNamedConstant( "eyeToSourceUVScale",
				Ogre::Vector2( UVScaleOffset[0].x, UVScaleOffset[0].y ) );
		params->setNamedConstant( "eyeToSourceUVOffset",
				Ogre::Vector2( UVScaleOffset[1].x, UVScaleOffset[1].y ) );
		Ogre::ManualObject* manual;
		if( eyeNum == 0 )
		{
			manual = rift_smgr->createManualObject("RiftRenderObjectLeft");
			manual->begin("Oculus/LeftEye", Ogre::RenderOperation::OT_TRIANGLE_LIST);
			//manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
		}
		else
		{
			manual = rift_smgr->createManualObject("RiftRenderObjectRight");
			manual->begin("Oculus/RightEye", Ogre::RenderOperation::OT_TRIANGLE_LIST);
			//manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
		}

			for( unsigned int i = 0; i < meshData.VertexCount; i++ )
		{
			ovrDistortionVertex v = meshData.pVertexData[i];
			manual->position( v.ScreenPosNDC.x,
				v.ScreenPosNDC.y, 0 );
			manual->textureCoord( v.TanEyeAnglesR.x,//*UVScaleOffset[0].x + UVScaleOffset[1].x,
				v.TanEyeAnglesR.y);//*UVScaleOffset[0].y + UVScaleOffset[1].y);
			manual->textureCoord( v.TanEyeAnglesG.x,//*UVScaleOffset[0].x + UVScaleOffset[1].x,
				v.TanEyeAnglesG.y);//*UVScaleOffset[0].y + UVScaleOffset[1].y);
			manual->textureCoord( v.TanEyeAnglesB.x,//*UVScaleOffset[0].x + UVScaleOffset[1].x,
				v.TanEyeAnglesB.y);//*UVScaleOffset[0].y + UVScaleOffset[1].y);
			float vig = std::max( v.VignetteFactor, (float)0.0 );
			manual->colour( vig, vig, vig, vig );
		}
		for( unsigned int i = 0; i < meshData.IndexCount; i++ )
		{
			manual->index( meshData.pIndexData[i] );
		}

		
		// tell Ogre, your definition has finished
		manual->end();

		ovrHmd_DestroyDistortionMesh( &meshData );

		meshNode->attachObject( manual );
	}

		// Create a camera in the (new, external) scene so the mesh can be rendered onto it:
	rift_cam = rift_smgr->createCamera("OculusRiftExternalCamera");
	rift_cam->setFarClipDistance( 50 );
	rift_cam->setNearClipDistance( 0.001 );
	rift_cam->setProjectionType( Ogre::PT_ORTHOGRAPHIC );
	rift_cam->setOrthoWindow( 2, 2 );

	rift_smgr->getRootSceneNode()->attachObject(rift_cam);

	meshNode->setPosition( 0, 0, -1 );
	meshNode->setScale( 1, 1, -1 );

	mViewport = window->addViewport( rift_cam );
	mViewport->setBackgroundColour(Ogre::ColourValue::Black);
	mViewport->setOverlaysEnabled(true);

	IPD = ovrHmd_GetFloat(oc->getHmd(), OVR_KEY_IPD,  0.064f);

	Ogre::RenderTexture* renderTexture = mLeftEyeRenderTexture->getBuffer()->getRenderTarget();
	renderTexture->addViewport(cams[0]);
	renderTexture->getViewport(0)->setClearEveryFrame(true);
	renderTexture->getViewport(0)->setBackgroundColour(backgroundColor);
	renderTexture->getViewport(0)->setOverlaysEnabled(true);

	renderTexture = mRightEyeRenderTexture->getBuffer()->getRenderTarget();
	renderTexture->addViewport(cams[1]);
	renderTexture->getViewport(0)->setClearEveryFrame(true);
	renderTexture->getViewport(0)->setBackgroundColour(backgroundColor);
	renderTexture->getViewport(0)->setOverlaysEnabled(true);
	
	ovrFovPort fovLeft = oc->getHmd()->MaxEyeFov[ovrEye_Left];
	ovrFovPort fovRight = oc->getHmd()->MaxEyeFov[ovrEye_Right];

	float combinedTanHalfFovHorizontal = std::max( fovLeft.LeftTan, fovLeft.RightTan );
	float combinedTanHalfFovVertical = std::max( fovLeft.UpTan, fovLeft.DownTan );

	float aspectRatio = combinedTanHalfFovHorizontal / combinedTanHalfFovVertical;
	
	Ogre::Camera* camLeft = cams[0];
	Ogre::Camera* camRight = cams[1];

	camLeft->setAspectRatio( aspectRatio );
	camRight->setAspectRatio( aspectRatio );
	
	ovrMatrix4f projL = ovrMatrix4f_Projection ( fovLeft, 0.001, 50.0, true );
	ovrMatrix4f projR = ovrMatrix4f_Projection ( fovRight, 0.001, 50.0, true );

	camLeft->setCustomProjectionMatrix( true,
		Ogre::Matrix4( projL.M[0][0], projL.M[0][1], projL.M[0][2], projL.M[0][3],
				projL.M[1][0], projL.M[1][1], projL.M[1][2], projL.M[1][3],
				projL.M[2][0], projL.M[2][1], projL.M[2][2], projL.M[2][3],
				projL.M[3][0], projL.M[3][1], projL.M[3][2], projL.M[3][3] ) );
	camRight->setCustomProjectionMatrix( true,
		Ogre::Matrix4( projR.M[0][0], projR.M[0][1], projR.M[0][2], projR.M[0][3],
				projR.M[1][0], projR.M[1][1], projR.M[1][2], projR.M[1][3],
				projR.M[2][0], projR.M[2][1], projR.M[2][2], projR.M[2][3],
				projR.M[3][0], projR.M[3][1], projR.M[3][2], projR.M[3][3] ) );
}

void OgreOculusRender::RenderOneFrame()
{
	//get some info
	cameraPosition = this->CameraNode->getPosition();
	cameraOrientation = this->CameraNode->getOrientation();

	//Begin frame
	ovrFrameTiming hmdFrameTiming = ovrHmd_BeginFrame(oc->getHmd(), 0);
	ovrTrackingState ts = ovrHmd_GetTrackingState(oc->getHmd(), hmdFrameTiming.ScanoutMidpointSeconds);

	//Tells ogre about the rendering
	root->_fireFrameStarted();
	Ogre::WindowEventUtilities::messagePump();
	
	for (Ogre::SceneManagerEnumerator::SceneManagerIterator it = root->getSceneManagerIterator(); it.hasMoreElements(); it.moveNext())
		it.peekNextValue()->_handleLodEvents();

	smgr->_handleLodEvents();


	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		Posef pose = ts.HeadPose.ThePose;

	}

	root->_fireFrameRenderingQueued();


	root->renderOneFrame();
	/*
	//Tell ogre that Frame started
	root->_fireFrameStarted();

	for (Ogre::SceneManagerEnumerator::SceneManagerIterator it = root->getSceneManagerIterator(); it.hasMoreElements(); it.moveNext())
		it.peekNextValue()->_handleLodEvents();

	ovrPosef headPose[2];

	//Message pump events
	Ogre::WindowEventUtilities::messagePump();


	for(int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
	{
		//Get the correct eye to render
		ovrEyeType eye = oc->getHmd()->EyeRenderOrder[eyeIndex];

		//Set the Ogre render target to the texture
		//root->getRenderSystem()->_setRenderTarget(rtts[eye]);
		vpts[eye]->clear();

		//Get the eye pose
		ovrPosef eyePose = ovrHmd_GetHmdPosePerEye(oc->getHmd(), eye);
		headPose[eye] = eyePose;

		//Get the hmd orientation
		OVR::Quatf camOrient = eyePose.Orientation;

		//Get the projection matrix
		OVR::Matrix4f proj = ovrMatrix4f_Projection(EyeRenderDesc[eye].Fov,static_cast<float>(nearClippingDistance), 10000.0f, true);

		//Convert it to Ogre matrix
		Ogre::Matrix4 OgreProj;
		for(int x(0); x < 4; x++)
			for(int y(0); y < 4; y++)
				OgreProj[x][y] = proj.M[x][y];

		//Set the matrix
		cams[eye]->setCustomProjectionMatrix(true, OgreProj);


		//Set the orientation
		cams[eye]->setOrientation(cameraOrientation * Ogre::Quaternion(camOrient.w,camOrient.x,camOrient.y,camOrient.z));

		//Set Position
		cams[eye]->setPosition
			(cameraPosition  //the "gameplay" position of player's avatar head
			+ 
			(cams[eye]->getOrientation() * - Ogre::Vector3( //realword camera orientation + the oposite of the 
			EyeRenderDesc[eye].HmdToEyeViewOffset.x, //view adjust vector. we translate the camera, not the whole world
			EyeRenderDesc[eye].HmdToEyeViewOffset.y, //The translations has to occur in function of the current head orientation.
			EyeRenderDesc[eye].HmdToEyeViewOffset.z) //That's why just multiply by the quaternion we just calculated. 

			+ cameraOrientation * Ogre::Vector3( //cameraOrientation is in fact the direction the avatar is facing expressed as an Ogre::Quaternion
			headPose[eye].Position.x,
			headPose[eye].Position.y,
			headPose[eye].Position.z)));

		root->_fireFrameRenderingQueued();
		rtts[eye]->update();
	}

	//Ogre::Root::getSingleton().getRenderSystem()->_setRenderTarget(window);

	this->updateTime = hmdFrameTiming.DeltaSeconds;
	//Do the rendering then the buffer swap

	ovrHmd_EndFrame(oc->getHmd(), headPose, (ovrTexture*)EyeTexture);
	//Tell Ogre that frame ended

	root->_fireFrameEnded();

	returnPose.position = cameraPosition + 
		Ogre::Vector3
		(headPose[0].Position.x,
		headPose[0].Position.y,
		headPose[0].Position.z);

	returnPose.orientation = cameraOrientation * Ogre::Quaternion
		(headPose[0].Orientation.w,
		headPose[0].Orientation.x,
		headPose[0].Orientation.y,
		headPose[0].Orientation.z);
		*/
	this->updateTime = hmdFrameTiming.DeltaSeconds;
	root->_fireFrameEnded();

}

void OgreOculusRender::dissmissHS()
{
	ovrHmd_DismissHSWDisplay(oc->getHmd());
	hsDissmissed = true;
}

void OgreOculusRender::setFullScreen(bool fs)
{
	fullscreen = fs;
}

bool OgreOculusRender::isFullscreen()
{
	return fullscreen;
}

Ogre::SceneManager* OgreOculusRender::getSceneManager()
{
	return smgr;
}

Ogre::RenderWindow* OgreOculusRender::getWindow()
{
	return window;
}

void OgreOculusRender::debugPrint()
{
	for(int i(0); i < 2; i++)
	{
		cout << "cam " << i << " " << cams[i]->getPosition() << endl;
		cout << cams[i]->getOrientation() << endl;
	}
}

void OgreOculusRender::debugSaveToFile(const char path[])
{
	if(rtts[0]) rtts[0]->writeContentsToFile(path);
}

Ogre::SceneNode* OgreOculusRender::getCameraInformationNode()
{
	return CameraNode;
}

Ogre::Timer* OgreOculusRender::getTimer()
{
	if(root)
		return root->getTimer();
	return NULL;
}

float OgreOculusRender::getUpdateTime()
{
	return updateTime;
}

void OgreOculusRender::recenter()
{
	ovrHmd_RecenterPose(oc->getHmd());
}

bool OgreOculusRender::IsHsDissmissed()
{
	return hsDissmissed;
}