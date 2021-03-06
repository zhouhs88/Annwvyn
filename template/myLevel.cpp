#include "stdafx.h"
#include "myLevel.hpp"

using namespace Annwvyn;

MyLevel::MyLevel() : constructLevel()
{
}

void MyLevel::load()
{
	//Create Sinbad the Ogre
	auto Sinbad(addGameObject("Sinbad.mesh"));
	Sinbad->setScale({ 0.3f, 0.3f, 0.3f });
	Sinbad->setPosition({ 0, 1, 0 });
	//Setting a non-null mass and a non static geometric primitive make this object
	//a dynamic physics object (it can move if forces are applied upon him)
	Sinbad->setUpPhysics(250, boxShape);

	//Load Ground:
	auto Ground(addGameObject("Ground.mesh"));
	Ground->setPosition({ 0, 0, 0 });
	Ground->setUpPhysics();

	//Create a light source
	auto Sun(addLightObject());
	Sun->setType(AnnLightObject::ANN_LIGHT_DIRECTIONAL);

	//zenith sunlight
	Sun->setDirection(AnnVect3{ -1, -1.5f, -1 }.normalisedCopy());

	//The ambient light is a scene light that is emitted by every point of the scene. This is for making the shadows not pitch black.
	AnnGetSceneryManager()->setAmbientLight(AnnColor(.25f, .25f, .25f));
}

void MyLevel::runLogic()
{
}