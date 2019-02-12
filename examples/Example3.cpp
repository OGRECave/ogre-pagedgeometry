//===============================================================================================================
//Example 3 - Trees and Bushes
//---------------------------------------------------------------------------------------------------------------
//	This example demonstrates the use of PagedGeometry to display trees and bushes with different view ranges.
//  Instructions: Move around with the arrow/WASD keys, hold SHIFT to move faster, and hold SPACE to fly.
//	HINT: Search this source for "[NOTE]" to find important code and comments related to PagedGeometry.
//===============================================================================================================
#define AppTitle "PagedGeometry Example 3 - Trees and Bushes"

#include "PagedGeometryConfig.h"
#include <Ogre.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif

#include <OgreApplicationContext.h>
#include <OgreCameraMan.h>

using namespace Ogre;


//Include PagedGeometry headers that will be needed
#include "PagedGeometry.h"
#include "BatchPage.h"
#include "ImpostorPage.h"
#include "TreeLoader2D.h"

//Include "LegacyTerrainLoader.h", a header that allows loading Ogre 1.7 style terrain
#include "LegacyTerrainLoader.h"

//Include "HeightFunction.h", a header that provides some useful functions for quickly and easily
//getting the height of the terrain at a given point.
#include "HeightFunction.h"
//[NOTE] Remember that this "HeightFunction.h" file is not related to the PagedGeometry library itself
//in any way. It's simply a utility that's included with all these examples to make getting the terrain
//height easy. You can use it in your games/applications if you want, although if you're using a
//collision/physics library with a faster alternate, you may use that instead.

//PagedGeometry's classes and functions are under the "Forests" namespace
using namespace Forests;

//Demo world class
//[NOTE] The main PagedGeometry-related sections of this class are load() and
//render. These functions setup and use PagedGeometry in the scene.
class World
{
public:
	World(RenderWindow* win);

	void load();	//Loads the 3D scene
	void unload();	//Unloads the 3D scene cleanly
	void run();		//Runs the simulation

	void render();			//Renders a single frame, updating PagedGeometry and Ogre

	//Various pointers to Ogre objects are stored here:
	Root *root;
	RenderWindow *window;
	Viewport *viewport;
	SceneManager *sceneMgr;
	Camera *camera;
	SceneNode* cameraNode;

	//Pointers to PagedGeometry class instances:
	PagedGeometry *trees, *bushes;
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
    //Initialize Ogre
    OgreBites::ApplicationContext ctx;
    ctx.initApp();
    ctx.setWindowGrab(true);

    World myWorld(ctx.getRenderWindow());
    myWorld.load();     //Load world

    OgreBites::CameraMan camman(myWorld.cameraNode);
    ctx.addInputListener(&camman);

    myWorld.run();      //Display world

    myWorld.unload();

    //Shut down Ogre
    ctx.closeApp();

    return 0;
}

World::World(RenderWindow* win)
{
    //Setup Ogre::Root and the scene manager
    root = Root::getSingletonPtr();
    window = win;
    sceneMgr = root->createSceneManager();

	//Initialize the camera and viewport
	camera = sceneMgr->createCamera("MainCamera");
	viewport = window->addViewport(camera);
	viewport->setBackgroundColour(ColourValue(0.47f, 0.67f, 0.96f));	//Blue sky background color
	camera->setAspectRatio(Real(viewport->getActualWidth()) / Real(viewport->getActualHeight()));
	camera->setNearClipDistance(1.0f);
	camera->setFarClipDistance(2000.0f);

    cameraNode = sceneMgr->getRootSceneNode()->createChildSceneNode();
    cameraNode->attachObject(camera);

	//Set up lighting
	Light *light = sceneMgr->createLight("Sun");
	light->setType(Light::LT_DIRECTIONAL);
	light->setDirection(Vector3(0.0f, -0.5f, 1.0f));
	sceneMgr->setAmbientLight(ColourValue(1, 1, 1));
}

//[NOTE] In addition to some Ogre setup, this function configures PagedGeometry in the scene.
void World::load()
{
	//-------------------------------------- LOAD TERRAIN --------------------------------------
	//Setup the fog up to 500 units away
	sceneMgr->setFog(FOG_LINEAR, viewport->getBackgroundColour(), 0, 100, 700);

	//Load the terrain
	auto terrain = loadLegacyTerrain("terrain.cfg", sceneMgr);

	//Start off with the camera at the center of the terrain
	cameraNode->setPosition(700, 100, 700);

	//-------------------------------------- LOAD TREES --------------------------------------
	//Create and configure a new PagedGeometry instance for trees
	trees = new PagedGeometry(camera, 80);
	trees->addDetailLevel<BatchPage>(150, 50);
	trees->addDetailLevel<ImpostorPage>(500, 50);

	//Create a new TreeLoader2D object
	TreeLoader2D *treeLoader = new TreeLoader2D(trees, TBounds(0, 0, 1500, 1500));
	trees->setPageLoader(treeLoader);

	//Supply the height function to TreeLoader2D so it can calculate tree Y values
	HeightFunction::initialize(terrain);
	treeLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	//Load a tree entity
	Entity *myTree = sceneMgr->createEntity("Tree", "tree2.mesh");

	//Randomly place 10,000 copies of the tree on the terrain
	Vector3 position = Vector3::ZERO;
	Radian yaw;
	Real scale;
	for (int i = 0; i < 10000; i++){
		yaw = Degree(Math::RangeRandom(0, 360));
		position.x = Math::RangeRandom(0, 1500);
		position.z = Math::RangeRandom(0, 1500);
		scale = Math::RangeRandom(0.5f, 0.6f);

		treeLoader->addTree(myTree, position, yaw, scale);
	}

	//-------------------------------------- LOAD BUSHES --------------------------------------
	//Create and configure a new PagedGeometry instance for bushes
	bushes = new PagedGeometry(camera, 50);
	bushes->addDetailLevel<BatchPage>(80, 50);

	//Create a new TreeLoader2D object for the bushes
	TreeLoader2D *bushLoader = new TreeLoader2D(bushes, TBounds(0, 0, 1500, 1500));
	bushes->setPageLoader(bushLoader);

	//Supply the height function to TreeLoader2D so it can calculate tree Y values
	bushLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	//Load a bush entity
	Entity *myBush = sceneMgr->createEntity("Bush", "Bush.mesh");

	//Randomly place 30,000 copies of the bush on the terrain
	for (int i = 0; i < 30000; i++){
		yaw = Degree(Math::RangeRandom(0, 360));
		position.x = Math::RangeRandom(0, 1500);
		position.z = Math::RangeRandom(0, 1500);
		scale = Math::RangeRandom(0.7f, 0.8f);

		bushLoader->addTree(myBush, position, yaw, scale);
	}
}

void World::unload()
{
	//[NOTE] Always remember to delete any PageLoader(s) and PagedGeometry instances in order to avoid memory leaks.

	//Delete the TreeLoader2D instances
	delete trees->getPageLoader();
	delete bushes->getPageLoader();

	//Delete the PagedGeometry instances
	delete trees;
	delete bushes;

	//Also delete the tree/bush entities
	sceneMgr->destroyEntity("Tree");
	sceneMgr->destroyEntity("Bush");
}

void World::run()
{
    //Render loop
    while(!Root::getSingleton().endRenderingQueued())
    {
        //Update frame
        render();
    }
}

void World::render()
{
	//[NOTE] PagedGeometry::update() is called every frame to keep LODs, etc. up-to-date
	trees->update();
	bushes->update();

	//Render the scene with Ogre
	root->renderOneFrame();
}
