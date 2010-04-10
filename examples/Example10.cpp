#include "HeightFunction.h"

#include "PGExampleApplication.h"
#include "PGExampleFrameListener.h"

#include "PagedGeometry.h"
#include "BatchPage.h"
#include "WindBatchPage.h"
#include "TreeLoader3D.h"
#include "TreeLoader2D.h"
#include "ImpostorPage.h"
#include "GrassLoader.h"

using namespace Forests;

// we use wind pages
//#define WIND

// SAMPLE CLASS
class PGSampleApp : public ExampleApplication
{
public:
	PGSampleApp();
	void createPGDemo(void);
	void createScene(void);
protected:
};

// SAMPLE IMPLEMENTATION
PGSampleApp::PGSampleApp() : ExampleApplication()
{
}

void PGSampleApp::createScene(void)
{

	//Setup the fog up to 1500 units away
	mSceneMgr->setFog(FOG_NONE);

	//Load the terrain
	mSceneMgr->setWorldGeometry("terrain3.cfg");

	//Start off with the camera at the center of the terrain
	mCamera->setPosition(700, 100, 700);

	//Setup a skybox
	mSceneMgr->setSkyBox(true, "3D-Diggers/SkyBox", 2000);

	// setup some useful defaults
	//MaterialManager::getSingleton().setDefaultTextureFiltering(TFO_ANISOTROPIC);
	//MaterialManager::getSingleton().setDefaultAnisotropy(7);

	LogManager::getSingleton().setLogDetail(LL_BOREME);

	Light *light = mSceneMgr->createLight("Sun");
	light->setType(Light::LT_DIRECTIONAL);
	light->setDirection(Ogre::Vector3(0.0f, -0.5f, 1.0f));
	mSceneMgr->setAmbientLight(Ogre::ColourValue(1, 1, 1));

	mCamera->setPosition(Vector3(100, 50, 1000));
	mCamera->lookAt(Vector3(150, 50, 1000));
	mCamera->setNearClipDistance(0.1);
	mCamera->setFarClipDistance(50000);

	if (mRoot->getRenderSystem()->getCapabilities()->hasCapability(RSC_INFINITE_FAR_PLANE))
	{
		mCamera->setFarClipDistance(0);   // enable infinite far clip distance if we can
	}
}
void PGSampleApp::createPGDemo(void)
{
	//-------------------------------------- LOAD TREES --------------------------------------
	//Create and configure a new PagedGeometry instance
	PagedGeometry *trees = new PagedGeometry();
	trees->setCamera(mCamera);	//Set the camera so PagedGeometry knows how to calculate LODs
	trees->setPageSize(50);	//Set the size of each page of geometry
	trees->setInfinite();		//Use infinite paging mode

#ifdef WIND
	//WindBatchPage is a variation of BatchPage which includes a wind animation shader
	trees->addDetailLevel<WindBatchPage>(70, 30);		//Use batches up to 70 units away, and fade for 30 more units
#else
	trees->addDetailLevel<BatchPage>(70, 30);		//Use batches up to 70 units away, and fade for 30 more units
#endif
	trees->addDetailLevel<ImpostorPage>(5000, 50);	//Use impostors up to 400 units, and for for 50 more units

	//Create a new TreeLoader2D object
	TreeLoader2D *treeLoader = new TreeLoader2D(trees, TBounds(0, 0, 1500, 1500));
	trees->setPageLoader(treeLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to TreeLoader2D so it can calculate tree Y values
	HeightFunction::initialize(mSceneMgr);
	treeLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	//[NOTE] This sets the color map, or lightmap to be used for trees. All trees will be colored according
	//to this texture. In this case, the shading of the terrain is used so trees will be shadowed
	//just as the terrain is (this should appear like the terrain is casting shadows on the trees).
	//You may notice that TreeLoader2D / TreeLoader3D doesn't have a setMapBounds() function as GrassLoader
	//does. This is because the bounds you specify in the TreeLoader2D constructor are used to apply
	//the color map.
	treeLoader->setColorMap("terrain_lightmap.jpg");

	//Load a tree entity
	Entity *tree1 = mSceneMgr->createEntity("Tree1", "fir06_30.mesh");

	Entity *tree2 = mSceneMgr->createEntity("Tree2", "fir14_25.mesh");

#ifdef WIND
	trees->setCustomParam(tree1->getName(), "windFactorX", 15);
	trees->setCustomParam(tree1->getName(), "windFactorY", 0.01);
	trees->setCustomParam(tree2->getName(), "windFactorX", 22);
	trees->setCustomParam(tree2->getName(), "windFactorY", 0.013);
#endif

	//Randomly place 10000 copies of the tree on the terrain
	Ogre::Vector3 position = Ogre::Vector3::ZERO;
	Radian yaw;
	Real scale;
	for (int i = 0; i < 60000; i++){
		yaw = Degree(Math::RangeRandom(0, 360));

		position.x = Math::RangeRandom(0, 1500);
		position.z = Math::RangeRandom(0, 1500);

		scale = Math::RangeRandom(0.07f, 0.12f);

		//[NOTE] Unlike TreeLoader3D, TreeLoader2D's addTree() function accepts a Vector2D position (x/z)
		//The Y value is calculated during runtime (to save memory) from the height function supplied (above)
		if (Math::UnitRandom() < 0.5f)
			treeLoader->addTree(tree1, position, yaw, scale);
		else
			treeLoader->addTree(tree2, position, yaw, scale);
	}
	addPG(trees);

}

// MAIN BELOW
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{

	PGSampleApp *app = new PGSampleApp();
	app->go();

	return 0;
}
