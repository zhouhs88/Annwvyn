/**
 * \file AnnPlayer.hpp
 * \brief class that represent the player
 * \author A. Brainville (Ybalrid)
 */

#ifndef ANN_PLAYER
#define ANN_PLAYER

#include "systemMacro.h"
#include <Ogre.h>
#include "euler.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

namespace Annwvyn
{
	class AnnEngine; //predeclaration of AnnEngine

	///Parameters of the VirtualBody		
	class DLL bodyParams
	{
		/**
		* That class was a structure. It is keeped as a class with public attributes to keep compatibility with legacy code.
		*/
	public:

		///Constructor that handle the default body parameters. 
		bodyParams();

		float eyeHeight;
		float walkSpeed;
		float turnSpeed;
		float mass;
		Ogre::Vector3 Position;
		Ogre::Euler Orientation;
		Ogre::Quaternion HeadOrientation;

		//bullet
		btCollisionShape* Shape;
		btRigidBody* Body;
	};

	///class that represent the player
	class DLL AnnPlayer
	{
	public:

		///Construct the player object
		AnnPlayer();
		
		///Destroy the player object
		~AnnPlayer();

		///Prevent modification of physical parameter
		void lockParameters();

		///Permit to know if parameters are locked
		bool isLocked();

		///Set the position of the player.
		/// \param Position 3D vector representing the position of the player (refernced by the point between his eyes)
		void setPosition(Ogre::Vector3 Position);
		///Set body orientation
		/// \param Orientation Euler angle that represent the orientation of the player's BODY. That orientation is used as a "zero point" for the head orientation.
		void setOrientation(Ogre::Euler Orientation);
		///Set the head orientation
		/// \param HeadOrientation A quaternion representing the orientation of the head
		void setHeadOrientation(Ogre::Quaternion HeadOrientation);
		
		///distance between footplane and eyes
		/// \param eyeHeight floating point number in metter
		void setEyesHeight(float eyeHeight);
		///WalkSpeed, metters by second
		/// \param walkSpeed The speed a the user is walking
		void setWalkSpeed(float walkSpeed);
		///Turnspeed
		/// \param turnSpeed Angular speed that the user can turn his body
		void setTurnSpeed(float turnSpeed);
		///Mass in Kg
		/// \param mass Mass of the player in Kg 
		void setMass(float mass);

		///Bullet shape
		/// \param Shape the Bullet collision shape used to simulate player physics
		void setShape(btCollisionShape* Shape);
		///Bullet RIGID body
		/// \param The bullet rigid body used for simulating player's physics
		void setBody(btRigidBody* Body);

		///Get the distance between footplane and eyes in metters
		float getEyesHeight();
		///Get walkspeed in metter/seconds
		float getWalkSpeed();
		///Get turnspeed in rad/seconds
		float getTurnSpeed();
		///Get mass in Kg
		float getMass();

		///Get position vector
		Ogre::Vector3 getPosition();
		///Get body orientation (euler vector)
		Ogre::Euler getOrientation();

		///Get rigid body
		btRigidBody* getBody();
		///Get Shape
		btCollisionShape* getShape();

		///Apply a relative yaw transform to the player. Usefull to bind it to the mouse X axis for FPS-like gameplay.
		/// \param angle Radian angle of the transformaton.
		void applyRelativeBodyYaw(Ogre::Radian angle);

		///manual speed management
		void setLinearSpeed(Ogre::Vector3 v);		
		void killLinearSpeed();
		void addLinearSpeed(Ogre::Vector3 v);


	protected:

		///Object that keep body parameters (= legacy structure)
		bodyParams* playerBody;

	private:

		///Give back the right to
		void unlockParameters();

		///Give Annwvyn::AnnEngine the rght to access private members
		friend class AnnEngine;

		///Engine update call for each frame
		void engineUpdate();

		///Get the pointer to bodyParams (compatibility with legacy code here. Highly dangerous, do not mess with whatever is pointed by that vector. Seriously.
		bodyParams* getLowLevelBodyParams();

		///The famous boolean that permit to prevent YOU for breaking my work! :D
		bool locked;


		Ogre::Vector3 getTranslation();


	public:
		enum walkDirection{forward, backward, left, right};
		bool walking[4];
	};

}

#endif //ANN_PLAYER

