//===============================================================================================================
//Example 2 - TreeLoader2D
//---------------------------------------------------------------------------------------------------------------
//	This example demonstrates the basic use of PagedGeometry to display trees with TreeLoader2D.
//  Instructions: Move around with the arrow/WASD keys, hold SHIFT to move faster, and hold SPACE to fly.
//	HINT: Search this source for "[NOTE]" to find important code and comments related to PagedGeometry.
//===============================================================================================================
#define AppTitle "PagedGeometry Example 2 - TreeLoader2D"

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
#include "TreeLoader2D.h"

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
	light->setDirection(Vector3(0.0f, -0.5f, 1.0f));
	sceneMgr->setAmbientLight(ColourValue(1, 1, 1));

	//Load media (trees, grass, etc.)
	ResourceGroupManager::getSingleton().addResourceLocation("media/trees", "FileSystem");
	ResourceGroupManager::getSingleton().addResourceLocation("media/terrains", "FileSystem");
	ResourceGroupManager::getSingleton().addResourceLocation("media/grass", "FileSystem");
	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

	//Initialize OIS
	using namespace OIS;
	size_t windowHnd;
	window->getCustomAttribute("WINDOW", &windowHnd);
	inputManager = InputManager::createInputSystem(windowHnd);

	keyboard = (Keyboard*)inputManager->createInputObject(OISKeyboard, false);
	mouse = (Mouse*)inputManager->createInputObject(OISMouse, false);

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
	trees->setPageSize(80);	//Set the size of each page of geometry
	trees->setInfinite();		//Use infinite paging mode
	trees->addDetailLevel<BatchPage>(150, 50);		//Use batches up to 150 units away, and fade for 30 more units
	trees->addDetailLevel<ImpostorPage>(500, 50);	//Use impostors up to 400 units, and for for 50 more units

	//Create a new TreeLoader2D object
	TreeLoader2D *treeLoader = new TreeLoader2D(trees, TBounds(0, 0, 1500, 1500));
	trees->setPageLoader(treeLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to TreeLoader2D so it can calculate tree Y values
	HeightFunction::initialize(sceneMgr);
	treeLoader->setHeightFunction(&HeightFunction::getTerrainHeight);

	//Load a tree entity
	Entity *myEntity = sceneMgr->createEntity("Tree", "tree2.mesh");

	//Randomly place 20,000 copies of the tree on the terrain
	Vector3 position = Vector3::ZERO;
	Radian yaw;
	Real scale;
	for (int i = 0; i < 20000; i++){
		yaw = Degree(Math::RangeRandom(0, 360));

		position.x = Math::RangeRandom(0, 1500);
		position.z = Math::RangeRandom(0, 1500);

		scale = Math::RangeRandom(0.5f, 0.6f);

		//[NOTE] Unlike TreeLoader3D, TreeLoader2D's addTree() function accepts a Vector2D position (x/z)
		//The Y value is calculated during runtime (to save memory) from the height function supplied (above)
		treeLoader->addTree(myEntity, position, yaw, scale);
	}
}

void World::unload()
{
	//[NOTE] Always remember to delete any PageLoader(s) and PagedGeometry instances in order to avoid memory leaks.

	//Delete the TreeLoader2D instance
	delete trees->getPageLoader();

	//Delete the PagedGeometry instance
	delete trees;

	//Also delete the tree entity
	sceneMgr->destroyEntity("Tree");
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
