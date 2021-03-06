/**
 * \file AnnPhysicsEngine.hpp
 * \brief The Physics Engine handle the Bullet "dynamics world" where the simulation occurs.
 *		  It also handle collision feedback and triggers
 * \author A. Brainville (Ybalrid)
 */
#ifndef ANN_PHYSICS_ENGINE
#define ANN_PHYSICS_ENGINE

#include "systemMacro.h"

#include <memory>

//Bullet
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

//btOgre
#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

#include "AnnEngine.hpp"

//#include "AnnSubsytem.hpp"

namespace Annwvyn
{
	///Class that abstract the physics engine initialization and collision information queries for Annwvyn.
	class DLL AnnPhysicsEngine : public AnnSubSystem
	{
	public:

		///Create the physics engine
		AnnPhysicsEngine(Ogre::SceneNode* rootNode, std::shared_ptr<AnnPlayer> player, AnnGameObjectList& objects, AnnTriggerObjectList& triggers);

		///Destroy the physics engine
		~AnnPhysicsEngine();

		///Add the player body to the dynamics world
		void addPlayerPhysicalBodyToDynamicsWorld() const;

		///Create player's rigid-body
		void createPlayerPhysicalVirtualBody(Ogre::SceneNode* node);

		///Create player's body shape (a capsule)
		void createVirtualBodyShape(float radius = 0.125f) const;

		///Pointer to the bullet's dynamics world
		btDiscreteDynamicsWorld* getWorld() const;

		///Step the simulation
		/// \param delta Interval in seconds that time has to be simulated
		void step(float delta) const;

		///Process the collision query system
		void processCollisionTesting() const;

		///Process triggers contact event
		void processTriggersContacts() const;

		///Remove a body from simulation
		void removeRigidBody(btRigidBody* body) const;

		///Do not use anymore, choose to call initPlayerStandingPhysics, or initPlayerRoomscalePhysics.
		DEPRECATED	void initPlayerPhysics(Ogre::SceneNode* cameraNode) { initPlayerStandingPhysics(cameraNode); }
		void initPlayerStandingPhysics(Ogre::SceneNode* playerAnchorNode);
		void initPlayerRoomscalePhysics(Ogre::SceneNode* playerAnchorNode) const;

		///Set the debug drawer state
		void setDebugPhysics(bool state);

		///Step the debug drawing for the physical representation
		void stepDebugDrawer() const;

		///Change the gravity vector
		void changeGravity(AnnVect3 gravity) const;

		///Restore the default gravity
		void resetGravity() const;

		///Toggle the debug physics overlay
		void toggleDebugPhysics();

	private:

		friend class AnnEngine;
		///Update by steeping simulation by one frame time. Should be called only once, and only by AnnEngine
		void update() override;

		std::unique_ptr<btBroadphaseInterface> Broadphase;
		std::unique_ptr<btDefaultCollisionConfiguration> CollisionConfiguration;
		std::unique_ptr<btCollisionDispatcher> Dispatcher;
		std::unique_ptr<btSequentialImpulseConstraintSolver> Solver;

		///Bullet Dynamics World
		std::unique_ptr<btDiscreteDynamicsWorld> DynamicsWorld;

		///Should use the debug drawer
		bool debugPhysics;

		///Debug drawer object from BtOgre
		BtOgre::DebugDrawer* debugDrawer;
		BtOgre::RigidBodyState* playerRigidBodyState;

		AnnGameObjectList& gameObjects;
		AnnTriggerObjectList& triggerObjects;
		std::shared_ptr<AnnPlayer> playerObject;

		AnnVect3 defaultGravity;
	};
}

#endif //ANN_PHYSICS_ENGINE