//===============================================================================================================
//Example 6 - Custom PageLoader
//---------------------------------------------------------------------------------------------------------------
//	This example demonstrates the use of a custom PageLoader by creating a procedural tree loader (advanced).
//  Instructions: Move around with the arrow/WASD keys, hold SHIFT to move faster, and hold SPACE to fly.
//	HINT: Search this source for "[NOTE]" to find important code and comments related to PagedGeometry.
//===============================================================================================================
#define AppTitle "PagedGeometry Example 6 - Custom PageLoader"

//Include windows/Ogre/OIS headers
#include "PagedGeometryConfig.h"
#include <Ogre.h>
#ifdef OIS_USING_DIR
# include "OIS/OIS.h"
#else
# include "OIS.h"
#endif //OIS_USING_DIR
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif
using namespace Ogre;


//Include PagedGeometry headers that will be needed
#include "PagedGeometry.h"
#include "BatchPage.h"
#include "ImpostorPage.h"
#include "TreeLoader3D.h"

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
	World();
	~World();

	void load();	//Loads the 3D scene
	void unload();	//Unloads the 3D scene cleanly
	void run();		//Runs the simulation

private:
	void render();			//Renders a single frame, updating PagedGeometry and Ogre
	void processInput();	//Accepts keyboard and mouse input, allowing you to move around in the world

	bool running;	//A flag which, when set to false, will terminate a simulation started with run()

	//Various pointers to Ogre objects are stored here:
	Root *root;
	RenderWindow *window;
	Viewport *viewport;
	SceneManager *sceneMgr;
	Camera *camera;

	//OIS input objects
	OIS::InputManager *inputManager;
	OIS::Keyboard *keyboard;
	OIS::Mouse *mouse;

	//Variables used to keep track of the camera's rotation/etc.
	Radian camPitch, camYaw;

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

	//Setup the height function (so the Y values of trees can be calculated when they are placed on the terrain)
	HeightFunction::initialize(sceneMgr);
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
	Root *root = new Ogre::Root("");

	//Load appropriate plugins
	//[NOTE] PagedGeometry needs the CgProgramManager plugin to compile shaders
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifdef _DEBUG
	root->loadPlugin("Plugin_CgProgramManager_d");
	root->loadPlugin("Plugin_OctreeSceneManager_d");
	root->loadPlugin("RenderSystem_Direct3D9_d");
	root->loadPlugin("RenderSystem_GL_d");
#else
	root->loadPlugin("Plugin_CgProgramManager");
	root->loadPlugin("Plugin_OctreeSceneManager");
	root->loadPlugin("RenderSystem_Direct3D9");
	root->loadPlugin("RenderSystem_GL");
#endif
#else
	root->loadPlugin("Plugin_CgProgramManager");
	root->loadPlugin("Plugin_OctreeSceneManager");
	root->loadPlugin("RenderSystem_GL");
#endif

	//Show Ogre's default config dialog to let the user setup resolution, etc.
	bool result = root->showConfigDialog();

	//If the user clicks OK, continue
	if (result)	{
		World myWorld;
		myWorld.load();		//Load world
		myWorld.run();		//Display world
	}

	//Shut down Ogre
	delete root;

	return 0;
}

World::World()
{
	//Setup Ogre::Root and the scene manager
	root = Root::getSingletonPtr();
	window = root->initialise(true, AppTitle);
	sceneMgr = root->createSceneManager(ST_EXTERIOR_CLOSE);

	//Initialize the camera and viewport
	camera = sceneMgr->createCamera("MainCamera");
	viewport = window->addViewport(camera);
	viewport->setBackgroundColour(ColourValue(0.47f, 0.67f, 0.96f));	//Blue sky background color
	camera->setAspectRatio(Real(viewport->getActualWidth()) / Real(viewport->getActualHeight()));
	camera->setNearClipDistance(1.0f);
	camera->setFarClipDistance(2000.0f);

	//Set up lighting
	Light *light = sceneMgr->createLight("Sun");
	light->setType(Light::LT_DIRECTIONAL);
	light->setDirection(Ogre::Vector3(0.0f, -0.5f, 1.0f));
	sceneMgr->setAmbientLight(Ogre::ColourValue(1, 1, 1));

	//Load media (trees, grass, etc.)
	ResourceGroupManager::getSingleton().addResourceLocation("media/trees", "FileSystem");
	ResourceGroupManager::getSingleton().addResourceLocation("media/terrains", "FileSystem");
	ResourceGroupManager::getSingleton().addResourceLocation("media/grass", "FileSystem");
	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

	//Initialize OIS
	size_t windowHnd;
	window->getCustomAttribute("WINDOW", &windowHnd);
	inputManager = OIS::InputManager::createInputSystem(windowHnd);

	keyboard = (OIS::Keyboard*)inputManager->createInputObject(OIS::OISKeyboard, false);
	mouse = (OIS::Mouse*)inputManager->createInputObject(OIS::OISMouse, false);

	//Reset camera orientation
	camPitch = 0;
	camYaw = 0;
}

World::~World()
{
	//Shut down OIS
	inputManager->destroyInputObject(keyboard);
	inputManager->destroyInputObject(mouse);
	OIS::InputManager::destroyInputSystem(inputManager);

	unload();
}


//[NOTE] In addition to some Ogre setup, this function configures PagedGeometry in the scene.
void World::load()
{
	//-------------------------------------- LOAD TERRAIN --------------------------------------
	//Setup the fog up to 500 units away
	sceneMgr->setFog(FOG_LINEAR, viewport->getBackgroundColour(), 0, 100, 700);

	//Load the terrain
	sceneMgr->setWorldGeometry("terrain.cfg");

	//Start off with the camera at the center of the terrain
	camera->setPosition(700, 100, 700);

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
	running = true;
	while(running)
	{
		//Handle windows events
		WindowEventUtilities::messagePump();

		//Update frame
		processInput();
		render();

		//Exit immediately if the window is closed
		if (window->isClosed())
			break;
	}
}

void World::render()
{
	//[NOTE] PagedGeometry::update() is called every frame to keep LODs, etc. up-to-date
	trees->update();

	//Render the scene with Ogre
	root->renderOneFrame();
}

void World::processInput()
{
	using namespace OIS;
	static Ogre::Timer timer;
	static unsigned long lastTime = 0;
	unsigned long currentTime = timer.getMilliseconds();

	//Calculate the amount of time passed since the last frame
	Real timeScale = (currentTime - lastTime) * 0.001f;
	if (timeScale < 0.001f)
		timeScale = 0.001f;
	lastTime = currentTime;

	//Get the current state of the keyboard and mouse
	keyboard->capture();
	mouse->capture();

	//Always exit if ESC is pressed
	if (keyboard->isKeyDown(KC_ESCAPE))
		running = false;

	//Reload the scene if R is pressed
	static bool reloadedLast = false;
	if (keyboard->isKeyDown(KC_R) && !reloadedLast){
		unload();
		load();
		reloadedLast = true;
	}
	else {
		reloadedLast = false;
	}

	//Get mouse movement
	const OIS::MouseState &ms = mouse->getMouseState();

	//Update camera rotation based on the mouse
	camYaw += Radian(-ms.X.rel / 200.0f);
	camPitch += Radian(-ms.Y.rel / 200.0f);
	camera->setOrientation(Quaternion::IDENTITY);
	camera->pitch(camPitch);
	camera->yaw(camYaw);

	//Allow the camera to move around with the arrow/WASD keys
	Ogre::Vector3 trans(0, 0, 0);
	if (keyboard->isKeyDown(KC_UP) || keyboard->isKeyDown(KC_W))
		trans.z = -1;
	if (keyboard->isKeyDown(KC_DOWN) || keyboard->isKeyDown(KC_S))
		trans.z = 1;
	if (keyboard->isKeyDown(KC_RIGHT) || keyboard->isKeyDown(KC_D))
		trans.x = 1;
	if (keyboard->isKeyDown(KC_LEFT) || keyboard->isKeyDown(KC_A))
		trans.x = -1;
	if (keyboard->isKeyDown(KC_PGUP) || keyboard->isKeyDown(KC_E))
		trans.y = 1;
	if (keyboard->isKeyDown(KC_PGDOWN) || keyboard->isKeyDown(KC_Q))
		trans.y = -1;

	//Shift = speed boost
	if (keyboard->isKeyDown(KC_LSHIFT) || keyboard->isKeyDown(KC_RSHIFT))
		trans *= 2;

	trans *= 100;
	camera->moveRelative(trans * timeScale);

	//Make sure the camera doesn't go under the terrain
	Ogre::Vector3 camPos = camera->getPosition();
	float terrY = HeightFunction::getTerrainHeight(camPos.x, camPos.z);
	if (camPos.y < terrY + 2 || !keyboard->isKeyDown(KC_SPACE)){		//Space = fly
		camPos.y = terrY + 2;
		camera->setPosition(camPos);
	}
}
