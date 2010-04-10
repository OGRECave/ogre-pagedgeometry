//This provides functions that can be used to easily get the height of Ogre's terrain at any x/z point.
//Simply call HeightFunction::initialize(), then use HeightFunction::getTerrainHeight() as needed.

//This file is used by the PagedGeometry examples to place trees on the terrain.

#include "Ogre.h"
using namespace Ogre;

namespace HeightFunction
{
	class MyRaySceneQueryListener: public RaySceneQueryListener
	{
	public:
		inline bool queryResult(SceneQuery::WorldFragment *fragment, Real distance)
		{
			resultDistance = distance;
			return false;
		}
		inline bool queryResult(MovableObject *obj, Real distance)
		{
			resultDistance = distance;
			return false;
		}

		float resultDistance;
	};

	bool initialized = false;
	RaySceneQuery* raySceneQuery;
	Ray updateRay;
	MyRaySceneQueryListener *raySceneQueryListener;

	//Initializes the height function. Call this before calling getTerrainHeight()
	void initialize(SceneManager *sceneMgr){
		if (!initialized){
			initialized = true;
			updateRay.setOrigin(Vector3::ZERO);
			updateRay.setDirection(Vector3::NEGATIVE_UNIT_Y);
			raySceneQuery = sceneMgr->createRayQuery(updateRay);
			raySceneQuery->setQueryTypeMask(Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK);
			raySceneQuery->setWorldFragmentType(Ogre::SceneQuery::WFT_SINGLE_INTERSECTION);
			raySceneQueryListener = new MyRaySceneQueryListener;
		}
	}

	//Gets the height of the terrain at the specified x/z coordinate
	//The userData parameter isn't used in this implementation of a height function, since
	//there's no need for extra data other than the x/z coordinates.
	inline float getTerrainHeight(const float x, const float z, void *userData = NULL){
		updateRay.setOrigin(Vector3(x, 0.0f, z));
		updateRay.setDirection(Vector3::UNIT_Y);
		raySceneQuery->setRay(updateRay);
		raySceneQuery->execute(raySceneQueryListener);

		return raySceneQueryListener->resultDistance;
	}
}