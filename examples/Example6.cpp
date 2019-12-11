//===============================================================================================================
//Example 6 - Custom PageLoader
//---------------------------------------------------------------------------------------------------------------
//	This example demonstrates the use of a custom PageLoader by creating a procedural tree loader (advanced).
//  Instructions: Move around with the arrow/WASD keys, hold SHIFT to move faster, and hold SPACE to fly.
//	HINT: Search this source for "[NOTE]" to find important code and comments related to PagedGeometry.
//===============================================================================================================
#define AppTitle "PagedGeometry Example 6 - Custom PageLoader"


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
#include "TreeLoader3D.h"

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
	PagedGeometry *trees;
};


//--------------------------------------- Custom PageLoader class -----------------------------------------------
//[NOTE] This is where the custom PageLoader class, "ProceduralLoader", is defined. Making a custom
//PageLoader class is as simple as implementing a PageLoader-derived class. The API reference contains
//detailed instructions on doing this (see the PageLoader documentation), but basically you just
//implement a loadPage() function which adds trees to the scene when the PagedGeometry engine requests
//a certain region to be loaded.
class ProceduralLoader: public PageLoader
{
public:
	ProceduralLoader(SceneManager *sceneMgr);
	~ProceduralLoader();
	void loadPage(PageInfo &page);

private:
	Entity *myTree;
	SceneManager *sceneMgr;
};

ProceduralLoader::ProceduralLoader(SceneManager *sceneMgr)
{
	//Load a tree entity
	this->sceneMgr = sceneMgr;
	myTree = sceneMgr->createEntity("Tree", "tree2.mesh");
}

ProceduralLoader::~ProceduralLoader()
{
	//Delete the tree entity
	sceneMgr->destroyEntity("Tree");
}

void ProceduralLoader::loadPage(PageInfo &page)
{
	//[NOTE] When this function (loadPage) is called, the PagedGeometry engine needs a certain region of
	//geometry to be loaded immediately. You can implement this function any way you like, loading your
	//trees from RAM, a hard drive, the internet, or even procedurally. In this example, a very simple
	//procedural implementation is used which basically places trees randomly on the terrain.

	//This may appear similar to the code seen in other examples where trees are randomly added to
	//TreeLoader3D or TreeLoader2D, but this method is much more direct. While the TreeLoader classes
	//store the tree positions in memory and retrieve them later when loadPage() is called, this directly
	//generates the random tree positions of requested areas. The primary difference from the user's point
	//of view is that this demo ProceduralLoader class uses no memory, and can produce an infinite amount of
	//trees in infinite mode (you can test this by commenting out the trees->setBounds() line below in
	//World::load() - the trees should extend infinitely, although the terrain won't).

	Ogre::Vector3 position;
	Ogre::Quaternion rotation;
	Ogre::Vector3 scale;
	Ogre::ColourValue color;
	for (int i = 0; i < 100; i++){
		//Calculate a random rotation around the Y axis
		rotation = Ogre::Quaternion(Degree(Math::RangeRandom(0, 360)), Ogre::Vector3::UNIT_Y);

		//Note that the position is within the page.bounds boundaries. "page.bounds" specifies
		//the area of the world that needs to be loaded.
		position.x = Math::RangeRandom(page.bounds.left, page.bounds.right);
		position.z = Math::RangeRandom(page.bounds.top, page.bounds.bottom);
		position.y = HeightFunction::getTerrainHeight(position.x, position.z);

		//Calculate a scale value (uniformly scaled in all dimensions)
		float uniformScale = Math::RangeRandom(0.5f, 0.6f);
		scale.x = uniformScale;
		scale.y = uniformScale;
		scale.z = uniformScale;

		//All trees will be fully lit in this demo
		color = ColourValue::White;

		//[NOTE] addEntity() is used to add trees to the scene from PageLoader::loadPage().
		addEntity(myTree, position, rotation, scale, color);
	}
}
//---------------------------------------------------------------------------------------------------------------


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
	sceneMgr->setAmbientLight(Ogre::ColourValue(1, 1, 1));
}

//[NOTE] In addition to some Ogre setup, this function configures PagedGeometry in the scene.
void World::load()
{
	//-------------------------------------- LOAD TERRAIN --------------------------------------
	//Setup the fog up to 500 units away
	sceneMgr->setFog(FOG_LINEAR, viewport->getBackgroundColour(), 0, 100, 700);

	//Load the terrain
	auto terrain = loadLegacyTerrain("terrain.cfg", sceneMgr);

    //Setup the height function (so the Y values of trees can be calculated when they are placed on the terrain)
    HeightFunction::initialize(terrain);

	//Start off with the camera at the center of the terrain
	cameraNode->setPosition(700, 100, 700);

	//-------------------------------------- LOAD TREES --------------------------------------
	//Create and configure a new PagedGeometry instance
	trees = new PagedGeometry();
	trees->setCamera(camera);	//Set the camera so PagedGeometry knows how to calculate LODs
	trees->setPageSize(100);	//Set the size of each page of geometry
	trees->setBounds(TBounds(0, 0, 1500, 1500));	//Force a boundary on the trees, since ProceduralLoader is infinite
	trees->addDetailLevel<BatchPage>(150, 50);		//Use batches up to 150 units away, and fade for 30 more units
	trees->addDetailLevel<ImpostorPage>(500, 50);	//Use impostors up to 400 units, and for for 50 more units

	//Create an instance of the custom PageLoader class, called "ProceduralLoader"
	ProceduralLoader *treeLoader = new ProceduralLoader(sceneMgr);
	trees->setPageLoader(treeLoader);

	//[NOTE] The ProceduralLoader will now randomly place trees on the terrain when PagedGeometry needs them.
	//The actual implementation of the ProceduralLoader class can be found above.
}

void World::unload()
{
	//[NOTE] Always remember to delete any PageLoader(s) and PagedGeometry instances in order to avoid memory leaks.

	//Delete the TreeLoader3D instance
	delete trees->getPageLoader();

	//Delete the PagedGeometry instance
	delete trees;
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

	//Render the scene with Ogre
	root->renderOneFrame();
}
