#pragma once

#include <Annwvyn.h>
#include <memory>

#include "DemoUtils.hpp"

using namespace Annwvyn;

//Hub to select Demos
class DemoHub : LEVEL, LISTENER
{
public:

	DemoHub() : constructLevel(), constructListener()
	{
	}

	~DemoHub()
	{
		AnnDebug() << "Demo hub destructed!";
	}

	void load()
	{
		//Register ourselves as event listener
		AnnGetEventManager()->addListener(getSharedListener());

		//Add static geometry
		auto Ground = addGameObject("Ground.mesh");
		Ground->setUpPhysics();

		auto StoneDemo0 = addGameObject("DemoStone.mesh");
		StoneDemo0->setPosition({ -3, 0, -5 });
		StoneDemo0->setOrientation(AnnQuaternion(AnnDegree(45), AnnVect3::UNIT_Y));
		StoneDemo0->setUpPhysics();

		auto TextPane = make_shared<Ann3DTextPlane>(2.f, 1.f, "Demo 0\nDemo the loading of a demo... xD", 512, 18.f);
		TextPane->setTextAlign(Ann3DTextPlane::ALIGN_CENTER);
		TextPane->setTextColor(AnnColor{ 0, 0, 0 });
		TextPane->setPosition(StoneDemo0->getPosition() + StoneDemo0->getOrientation()*  AnnVect3 { 0, 2, -0.35 });
		TextPane->setOrientation(StoneDemo0->getOrientation());
		TextPane->update();
		addManualMovableObject(TextPane);

		auto Demo0Trigger = static_pointer_cast<AnnSphericalTriggerObject>(addTrggerObject());
		Demo0Trigger->setThreshold(1.5f);
		Demo0Trigger->setPosition(StoneDemo0->getPosition() + AnnVect3(0, 0.5f, 0));
		demo0trig = Demo0Trigger;

		auto StoneTestLevel = addGameObject("DemoStone.mesh");
		StoneTestLevel->setPosition({ +3, 0, -5 });
		StoneTestLevel->setOrientation(AnnQuaternion(AnnDegree(-45), AnnVect3::UNIT_Y));
		StoneTestLevel->setUpPhysics();

		auto TestLevelText = make_shared<Ann3DTextPlane>(2.f, 1.f, "TestLevel\nA simple test level", 512, 18.f);
		TestLevelText->setTextAlign(Ann3DTextPlane::ALIGN_CENTER);
		TestLevelText->setTextColor(AnnColor{ 0, 0, 0 });
		TestLevelText->setPosition(StoneTestLevel->getPosition() + StoneTestLevel->getOrientation()*  AnnVect3 { 0, 2, -0.35 });
		TestLevelText->setOrientation(StoneTestLevel->getOrientation());
		TestLevelText->update();
		addManualMovableObject(TestLevelText);

		auto TestLevelTrigger = static_pointer_cast<AnnSphericalTriggerObject>(addTrggerObject());
		TestLevelTrigger->setThreshold(1.5f);
		TestLevelTrigger->setPosition(StoneTestLevel->getPosition() + AnnVect3(0, 0.5f, 0));
		testLevelTrig = TestLevelTrigger;

		auto Sun = addLightObject();
		Sun->setType(AnnLightObject::ANN_LIGHT_DIRECTIONAL);
		Sun->setDirection({ 0, -1, -0.5 });

		AnnGetSceneryManager()->setAmbiantLight(AnnColor(0.15f, 0.15f, 0.15f));

		AnnGetPlayer()->teleport({ 0,1,0 }, 0);
	}

	//Called at each frame
	void runLogic()
	{
	}

	void unload()
	{
		//Unregister the listener
		AnnGetEventManager()->removeListener(getSharedListener());
		AnnLevel::unload();
	}

	void TriggerEvent(AnnTriggerEvent e)
	{
		if (e.getContactStatus())
			jumpToLevelTriggeredBy(e.getSender());
	}

	void jumpToLevelTriggeredBy(std::shared_ptr<AnnTriggerObject> trigger)
	{
		if (demo0trig == trigger)
		{
			AnnGetLevelManager()->jump(getDemo(0));
			return;
		}
		if (testLevelTrig == trigger)
		{
			AnnGetLevelManager()->jump(getDemo(1));
		}
	}

	level_id getDemo(level_id id)
	{
		return 1 + id;
	}

private:
	std::shared_ptr<AnnTriggerObject> demo0trig;
	std::shared_ptr<AnnTriggerObject> testLevelTrig;
};

class Demo0 : LEVEL
{
public:

	Demo0() : constructLevel()
	{
	}

	void load()
	{
		AnnGetEventManager()->addListener(goBackListener = make_shared<GoBackToDemoHub>());
		auto Ground = addGameObject("Ground.mesh");
		Ground->setUpPhysics();

		auto MyObject = addGameObject("MyObject.mesh");
		MyObject->setPosition({ 0, 1,-5 });
		MyObject->setUpPhysics(200, convexShape);

		auto Sun = addLightObject();
		Sun->setType(AnnLightObject::ANN_LIGHT_DIRECTIONAL);
		Sun->setDirection({ 0,1,-0.75 });

		AnnGetPlayer()->teleport({ 0,1,0 }, 0);
	}

	void unload()
	{
		AnnGetEventManager()->removeListener(goBackListener);
		goBackListener.reset();

		AnnLevel::unload();
	}

	void triggerEventCallback(AnnTriggerEvent e)
	{
	}

	void runLogic()
	{
	}

private:
	shared_ptr<GoBackToDemoHub> goBackListener;
};