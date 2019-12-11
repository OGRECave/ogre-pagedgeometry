//===============================================================================================================
//Example 4 - GrassLoader
//---------------------------------------------------------------------------------------------------------------
//	This example demonstrates the basic use of PagedGeometry to display grass with GrassLoader.
//  Instructions: Move around with the arrow/WASD keys, hold SHIFT to move faster, and hold SPACE to fly.
//	HINT: Search this source for "[NOTE]" to find important code and comments related to PagedGeometry.
//===============================================================================================================
#define AppTitle "PagedGeometry Example 4 - GrassLoader"

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
#include "GrassLoader.h"

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
	PagedGeometry *grass;
	GrassLoader *grassLoader;
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
    sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(light);
    light->getParentSceneNode()->setDirection(Vector3(0.0f, -0.5f, 1.0f));
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

	//-------------------------------------- LOAD GRASS --------------------------------------
	//Create and configure a new PagedGeometry instance for grass
	grass = new PagedGeometry(camera, 50);
	grass->addDetailLevel<GrassPage>(150);

	//Create a GrassLoader object
	grassLoader = new GrassLoader(grass);
	grass->setPageLoader(grassLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to GrassLoader so it can calculate grass Y values
	HeightFunction::initialize(terrain);
	grassLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	//Add some grass to the scene with GrassLoader::addLayer()
	GrassLayer *l = grassLoader->addLayer("grass");

	//Configure the grass layer properties (size, density, animation properties, fade settings, etc.)
	l->setMinimumSize(2.0f, 2.0f);
	l->setMaximumSize(2.5f, 2.5f);
	l->setAnimationEnabled(true);		//Enable animations
	l->setSwayDistribution(10.0f);		//Sway fairly unsynchronized
	l->setSwayLength(0.5f);				//Sway back and forth 0.5 units in length
	l->setSwaySpeed(0.5f);				//Sway 1/2 a cycle every second
	l->setDensity(1.5f);				//Relatively dense grass
	l->setFadeTechnique(FADETECH_GROW);	//Distant grass should slowly raise out of the ground when coming in range
	l->setRenderTechnique(GRASSTECH_QUAD);	//Draw grass as scattered quads

	//This sets a color map to be used when determining the color of each grass quad. setMapBounds()
	//is used to set the area which is affected by the color map. Here, "terrain_texture.jpg" is used
	//to color the grass to match the terrain under it.
	l->setColorMap("terrain_texture.jpg");
	l->setMapBounds(TBounds(0, 0, 1500, 1500));	//(0,0)-(1500,1500) is the full boundaries of the terrain

}

void World::unload()
{
	//[NOTE] Always remember to delete any PageLoader(s) and PagedGeometry instances in order to avoid memory leaks.

	//Delete the GrassLoader instance
	delete grass->getPageLoader();

	//Delete the PagedGeometry instance
	delete grass;
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
	grass->update();

	//Render the scene with Ogre
	root->renderOneFrame();
}
