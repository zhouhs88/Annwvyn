#pragma once

//Remove some visual studio stupidity
#pragma warning (disable : 4244)

//C STDLIB for C++
#include <cmath>
#include <cassert>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

//C++ STL and STDLIB
#include <iostream>
#include <ostream>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <array>
#include <thread>
#include <mutex>
#include <random>
#include <valarray>
#include <functional>

//Object-Oriented Graphical Rendering Engine
#include <Ogre.h>
#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreMatrix3.h>
//For the console, we use the font system of the Overlay
#include <Overlay/OgreFont.h>
#include <Overlay/OgreFontManager.h>

//Object-Oriented Input System
#include <OIS.h>

//Bullet
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btVector3.h>

//OpenAl
#include <al.h>
#include <alc.h>

//libsndfile
#include <sndfile.h>

//Oculus VR API
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

//Chaisript interpretor
#pragma warning (disable : 4602)
#include <chaiscript.hpp>
#include <chaiscript_defines.hpp>
#include <chaiscript_stdlib.hpp>
#include <chaiscript_threading.hpp>
#pragma warning (default: 4602)

//OpenVR (Valve/HTC Vive) API
#include <openvr.h>
#include <openvr_capi.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <glew.h>
#endif