//===============================================================================================================
//Example 8 - Forest Demo
//---------------------------------------------------------------------------------------------------------------
//	Description
//===============================================================================================================
#define AppTitle "PagedGeometry Example 8 - Forest Demo"

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
#include "BatchPage.h"
#include "WindBatchPage.h"
#include "ImpostorPage.h"
#include "TreeLoader2D.h"

#define WIND

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
	PagedGeometry *trees, *grass, *bushes;
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
	camera->setNearClipDistance(0.1f);
	camera->setFarClipDistance(5000.0f);

	//Set up lighting
	Light *light = sceneMgr->createLight("Sun");
	light->setType(Light::LT_DIRECTIONAL);
    sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(light);
    light->getParentSceneNode()->setDirection(Vector3(0.0f, -0.5f, 1.0f));
	sceneMgr->setAmbientLight(Ogre::ColourValue(1, 1, 1));

    cameraNode = sceneMgr->getRootSceneNode()->createChildSceneNode();
    cameraNode->attachObject(camera);
}

//[NOTE] In addition to some Ogre setup, this function configures PagedGeometry in the scene.
void World::load()
{
	//-------------------------------------- LOAD TERRAIN --------------------------------------
	//Setup the fog up to 1500 units away
	sceneMgr->setFog(FOG_LINEAR, viewport->getBackgroundColour(), 0, 100, 900);

	//Load the terrain
	auto terrain = loadLegacyTerrain("terrain2.cfg", sceneMgr);

	//Start off with the camera at the center of the terrain
	cameraNode->setPosition(700, 100, 700);

	//Setup a skybox
	sceneMgr->setSkyBox(true, "3D-Diggers/SkyBox", 2000);

	//-------------------------------------- LOAD GRASS --------------------------------------
	//Create and configure a new PagedGeometry instance for grass
	grass = new PagedGeometry(camera, 30);
	grass->addDetailLevel<GrassPage>(60);

	//Create a GrassLoader object
	grassLoader = new GrassLoader(grass);
	grass->setPageLoader(grassLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to GrassLoader so it can calculate grass Y values
	HeightFunction::initialize(terrain);
	grassLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	//Add some grass to the scene with GrassLoader::addLayer()
	GrassLayer *l = grassLoader->addLayer("3D-Diggers/plant1sprite");

	//Configure the grass layer properties (size, density, animation properties, fade settings, etc.)
	l->setMinimumSize(0.7f, 0.7f);
	l->setMaximumSize(0.9f, 0.9f);
	l->setAnimationEnabled(true);		//Enable animations
	l->setSwayDistribution(7.0f);		//Sway fairly unsynchronized
	l->setSwayLength(0.1f);				//Sway back and forth 0.5 units in length
	l->setSwaySpeed(0.4f);				//Sway 1/2 a cycle every second
	l->setDensity(3.0f);				//Relatively dense grass
	l->setRenderTechnique(GRASSTECH_SPRITE);
	l->setFadeTechnique(FADETECH_GROW);	//Distant grass should slowly raise out of the ground when coming in range

	//[NOTE] This sets the color map, or lightmap to be used for grass. All grass will be colored according
	//to this texture. In this case, the colors of the terrain is used so grass will be shadowed/colored
	//just as the terrain is (this usually makes the grass fit in very well).
	l->setColorMap("terrain_texture2.jpg");

	//This sets the density map that will be used to determine the density levels of grass all over the
	//terrain. This can be used to make grass grow anywhere you want to; in this case it's used to make
	//grass grow only on fairly level ground (see densitymap.png to see how this works).
	l->setDensityMap("densitymap.png");

	//setMapBounds() must be called for the density and color maps to work (otherwise GrassLoader wouldn't
	//have any knowledge of where you want the maps to be applied). In this case, the maps are applied
	//to the same boundaries as the terrain.
	l->setMapBounds(TBounds(0, 0, 1500, 1500));	//(0,0)-(1500,1500) is the full boundaries of the terrain

	//-------------------------------------- LOAD TREES --------------------------------------
	//Create and configure a new PagedGeometry instance
	trees = new PagedGeometry();
	trees->setCamera(camera);	//Set the camera so PagedGeometry knows how to calculate LODs
	trees->setPageSize(50);	//Set the size of each page of geometry
	trees->setInfinite();		//Use infinite paging mode

#ifdef WIND
	//WindBatchPage is a variation of BatchPage which includes a wind animation shader
	trees->addDetailLevel<WindBatchPage>(90, 30);		//Use batches up to 150 units away, and fade for 30 more units
#else
	trees->addDetailLevel<BatchPage>(90, 30);		//Use batches up to 150 units away, and fade for 30 more units
#endif
	trees->addDetailLevel<ImpostorPage>(700, 50);	//Use impostors up to 400 units, and for for 50 more units

	//Create a new TreeLoader2D object
	TreeLoader2D *treeLoader = new TreeLoader2D(trees, TBounds(0, 0, 1500, 1500));
	trees->setPageLoader(treeLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to TreeLoader2D so it can calculate tree Y values
	treeLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	//[NOTE] This sets the color map, or lightmap to be used for trees. All trees will be colored according
	//to this texture. In this case, the shading of the terrain is used so trees will be shadowed
	//just as the terrain is (this should appear like the terrain is casting shadows on the trees).
	//You may notice that TreeLoader2D / TreeLoader3D doesn't have a setMapBounds() function as GrassLoader
	//does. This is because the bounds you specify in the TreeLoader2D constructor are used to apply
	//the color map.
	treeLoader->setColorMap("terrain_lightmap.jpg");

	//Load a tree entity
	Entity *tree1 = sceneMgr->createEntity("Tree1", "fir05_30.mesh");

	Entity *tree2 = sceneMgr->createEntity("Tree2", "fir14_25.mesh");

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
	for (int i = 0; i < 10000; i++){
		yaw = Degree(Math::RangeRandom(0, 360));

		position.x = Math::RangeRandom(0, 1500);
		position.z = Math::RangeRandom(0, 1500);

		scale = Math::RangeRandom(0.07f, 0.12f);

		float rnd = Math::UnitRandom();
		if (rnd < 0.5f)
		{
		//[NOTE] Unlike TreeLoader3D, TreeLoader2D's addTree() function accepts a Vector2D position (x/z)
		//The Y value is calculated during runtime (to save memory) from the height function supplied (above)
		if (Math::UnitRandom() < 0.5f)
			treeLoader->addTree(tree1, position, yaw, scale);
		//else
		//	treeLoader->addTree(tree2, position, yaw, scale);
		}
		else
			treeLoader->addTree(tree2, position, yaw, scale);
	}

	//-------------------------------------- LOAD BUSHES --------------------------------------
	//Create and configure a new PagedGeometry instance for bushes
	bushes = new PagedGeometry(camera, 50);

#ifdef WIND
	bushes->addDetailLevel<WindBatchPage>(80, 50);
#else
	bushes->addDetailLevel<BatchPage>(80, 50);
#endif

	//Create a new TreeLoader2D object for the bushes
	TreeLoader2D *bushLoader = new TreeLoader2D(bushes, TBounds(0, 0, 1500, 1500));
	bushes->setPageLoader(bushLoader);

	//Supply the height function to TreeLoader2D so it can calculate tree Y values
	bushLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	bushLoader->setColorMap("terrain_lightmap.jpg");

	//Load a bush entity
	Entity *fern = sceneMgr->createEntity("Fern", "farn1.mesh");

	Entity *plant = sceneMgr->createEntity("Plant", "plant2.mesh");

	Entity *mushroom = sceneMgr->createEntity("Mushroom", "shroom1_1.mesh");

#ifdef WIND
	bushes->setCustomParam(fern->getName(), "factorX", 1);
	bushes->setCustomParam(fern->getName(), "factorY", 0.01);

	bushes->setCustomParam(plant->getName(), "factorX", 0.6);
	bushes->setCustomParam(plant->getName(), "factorY", 0.02);
#endif

	//Randomly place 20,000 bushes on the terrain
	for (int i = 0; i < 20000; i++){
		yaw = Degree(Math::RangeRandom(0, 360));
		position.x = Math::RangeRandom(0, 1500);
		position.z = Math::RangeRandom(0, 1500);

		float rnd = Math::UnitRandom();
		if (rnd < 0.8f) {
			scale = Math::RangeRandom(0.3f, 0.4f);
			bushLoader->addTree(fern, position, yaw, scale);
		} else if (rnd < 0.9) {
			scale = Math::RangeRandom(0.2f, 0.6f);
			bushLoader->addTree(mushroom, position, yaw, scale);
		} else {
			scale = Math::RangeRandom(0.3f, 0.5f);
			bushLoader->addTree(plant, position, yaw, scale);
		}
	}

}

void World::unload()
{
	//[NOTE] Always remember to delete any PageLoader(s) and PagedGeometry instances to avoid memory leaks.

	//Delete the PageLoader's
	//Delete the PagedGeometry instances
	if(grass) {delete grass->getPageLoader(); delete grass; grass=0;};
	if(trees) {delete trees->getPageLoader(); delete trees; trees=0;};
	if(bushes) {delete bushes->getPageLoader(); delete bushes; bushes=0;};

	//Also delete the entities
	sceneMgr->destroyEntity("Tree1");
	sceneMgr->destroyEntity("Fern");
	sceneMgr->destroyEntity("Plant");
	sceneMgr->destroyEntity("Mushroom");
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
	trees->update();
	bushes->update();

	//Render the scene with Ogre
	root->renderOneFrame();
}
