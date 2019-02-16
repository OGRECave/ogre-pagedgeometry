#include "PGExampleApplication.h"

#include "PagedGeometry.h"
#include "BatchPage.h"
#include "WindBatchPage.h"
#include "TreeLoader3D.h"
#include "TreeLoader2D.h"
#include "ImpostorPage.h"
#include "GrassLoader.h"

#include "HeightFunction.h"

using namespace Forests;

// we use wind pages
#define WIND

// SAMPLE CLASS
class PGSampleApp : public ExampleApplication
{
public:
	PGSampleApp();
	void createPGDemo(void);
protected:
};

// SAMPLE IMPLEMENTATION
PGSampleApp::PGSampleApp() : ExampleApplication()
{
}

void PGSampleApp::createPGDemo(void)
{
	//-------------------------------------- LOAD GRASS --------------------------------------
	//Create and configure a new PagedGeometry instance for grass
	PagedGeometry *grass = new PagedGeometry(mCamera, 30);
	grass->addDetailLevel<GrassPage>(60);

	//Create a GrassLoader object
	GrassLoader *grassLoader = new GrassLoader(grass);
	grass->setPageLoader(grassLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to GrassLoader so it can calculate grass Y values
	HeightFunction::initialize(mTerrainGroup);
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
	addPG(grass);

	//-------------------------------------- LOAD TREES --------------------------------------
	//Create and configure a new PagedGeometry instance
	PagedGeometry *trees = new PagedGeometry();
	trees->setCamera(mCamera);	//Set the camera so PagedGeometry knows how to calculate LODs
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
	Entity *tree1 = mSceneMgr->createEntity("Tree1", "fir05_30.mesh");

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
	for (int i = 0; i < 10000; i++){
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

	//-------------------------------------- LOAD BUSHES --------------------------------------
	//Create and configure a new PagedGeometry instance for bushes
	PagedGeometry *bushes = new PagedGeometry(mCamera, 50);

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
	Entity *fern = mSceneMgr->createEntity("Fern", "farn1.mesh");

	Entity *plant = mSceneMgr->createEntity("Plant", "plant2.mesh");

	Entity *mushroom = mSceneMgr->createEntity("Mushroom", "shroom1_1.mesh");

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

	addPG(bushes);

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
