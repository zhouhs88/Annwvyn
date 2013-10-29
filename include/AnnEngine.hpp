#ifndef ANN_ENGINE
#define ANN_ENGINE

//windows DLL
#ifdef DLLDIR_EX
   #define DLL  __declspec(dllexport)   // export DLL information
#else
   #define DLL  __declspec(dllimport)   // import DLL information
#endif

//bypass on linux
#ifdef __gnu_linux__
#define DLL
#endif

//C++ STD & STL
#include <vector>
#include <sstream>
//Ogre 3D
#include <Ogre.h>
#include <OIS.h>
//Bullet
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>
//OgreOculus by kojack
#include "OgreOculus.h"
//btOgre
#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"
//Annwvyn
#include "AnnGameObject.hpp"
#include "AnnTriggerObject.hpp"
#include "AnnTypes.h"
#include "AnnTools.h"
#include "AnnAudioEngine.hpp"

#ifdef __gnu_linux__
#include <unistd.h>
#endif



namespace Annwvyn
{
	class DLL AnnEngine
	{
	public:
		//Class constructor. take the name of the window
		AnnEngine(const char title[] = "Annwvyn Game Engine");

		//Class destructor. Do clean up stuff.
		~AnnEngine();

		//display config window and return an ogre root (create a new one by default)
		Ogre::Root* askSetUpOgre(Ogre::Root* root = new Ogre::Root);

		//load data to the ressource group manager
		void loadZip(const char path[]);
		void loadDir(const char path[]);

		//init ressources groups
		void initRessources();

		//init OgreOculus stuff
		void oculusInit();

		//init the BodyParams variable
		static void initBodyParams(Annwvyn::bodyParams* bodyP,
			float eyeHeight = 1.70f,
			float walkSpeed = 1.2f,
			float turnSpeed = 0.003f,
			float mass = 80.0f,
			Ogre::Vector3 Position = Ogre::Vector3(0,0,10),
			Ogre::Quaternion HeadOrientation = Ogre::Quaternion(1,0,0,0),
			btCollisionShape* Shape = NULL,
			btRigidBody* Body = NULL);
		
		//create a bullet shape for player's body
		void createVirtualBodyShape();

		//create a bullet rigid body for the player
		void createPlayerPhysicalVirtualBody();

		//add player rigid body to the bullet world
		void addPlayerPhysicalBodyToDynamicsWorld();

		//do all player related physics stuff for you
		void initPlayerPhysics();

		//update player location/orientation from the bullet body
		void updatePlayerFromPhysics();

		//translate the player (DONT DETECT COLLISIONS HERE !!!)
		void translatePhysicBody(Ogre::Vector3 translation);

		//set player linear speed from a 3D vector
		void setPhysicBodyLinearSpeed(Ogre::Vector3 V);

		//create a game object form the name of an entity loaded on the ressource group manager.
		AnnGameObject* createGameObject(const char entityName[]);

		//set the ambiant light
		void setAmbiantLight(Ogre::ColourValue v);

		//add a light to the scene. return a pointer to the new light
		AnnLightObject* addLight();

		//calculate one frame of the game
		void renderOneFrame();

		//display bullet debuging drawing
		void setDebugPhysicState(bool state);//if state == true, display physics debug


		void runPhysics(); 

		//return true if the game want to terminate the program
		bool requestStop();

		//log something to the console. If flag = true (by default), will print "Annwvyn - " in front of the message
		static void log(std::string message, bool flag = true);

		//update camera position/orientation from rift and virtual body
		void updateCamera();

		//refresh all for you
		void refresh();

		//return a vector depending on WASD keys pressed
		bool processWASD(Ogre::Vector3* translate);//return the translation vector to aply on the body

		//caputre event form keyboard and mouse
		void captureEvents();//keyboard and mouse

		//update program time. retur the delay between the last call of this method
		float updateTime(); //return deltaT
		float getTime();

		void playObjectsAnnimation();


		//set the ground object
		void setGround(AnnGameObject* Ground);

		//return the Annwvyn OpenAL simplified audio engine
		AnnAudioEngine* getAudioEngine();

		//get lowlevel OIS access
		OIS::Mouse* getOISMouse();
		OIS::Keyboard* getOISKeyboard();

		//is key 'key' pressed ? (see OIS headers for KeyCode, generaly 'OIS::KC_X' where X is the key you want.
		bool isKeyDown(OIS::KeyCode key); //this is simplier to use if you are on the OIS namespace ;-)

		//return true if you touch the ground
		bool collisionWithGround();
		
		void processCollisionTesting();

		btDiscreteDynamicsWorld* getDynamicsWorld();

		AnnTriggerObject* createTriggerObject();

		void processTriggersContacts();

		Ogre::SceneManager* getSceneManager();

		void setSkyDomeMaterial(bool activate, const char materialName[], float curvature = 2.0f, float tiling = 1.0f);

		Annwvyn::AnnGameObject* playerLookingAt();


		Annwvyn::bodyParams* getBodyParams();

		Ogre::SceneNode* getCamera();

	private:
		Annwvyn::bodyParams* m_bodyParams;
		
		Ogre::Root* m_Root;
		Ogre::RenderWindow* m_Window;
		Ogre::SceneManager* m_SceneManager;
		Ogre::SceneNode* m_Camera;
		
		bool readyForLoadingRessources;

		Ogre::Entity* m_ent; //only used for creating nodes into the smgr
		
		Oculus oculus;

		//dynamic container for games objects
		std::vector <AnnGameObject*> objects;
		std::vector <AnnTriggerObject*> triggers;

		//Events processing : 
		OIS::InputManager *m_InputManager ;
		OIS::Keyboard *m_Keyboard;
		OIS::Mouse *m_Mouse;
		OIS::ParamList pl;

		size_t windowHnd;
		std::ostringstream windowHndStr;

		bool activateWASD;
		bool activateJump;
		float jumpForce;

		//time
		unsigned long last,now; //Milisec
		float deltaT; //Sec

		 //bullet
		btBroadphaseInterface* m_Broadphase;
		btDefaultCollisionConfiguration* m_CollisionConfiguration;
		btCollisionDispatcher* m_Dispatcher;
		btSequentialImpulseConstraintSolver* m_Solver;
		btGhostPairCallback* m_ghostPairCallback;
		btDiscreteDynamicsWorld* m_DynamicsWorld;

		bool debugPhysics;
		BtOgre::DebugDrawer* m_debugDrawer;

		btQuaternion fixedBodyOrient;

		AnnGameObject* m_Ground;

		//Audio
		AnnAudioEngine* AudioEngine;
	};
}
#endif //ANN_ENGINE
