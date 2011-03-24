
#include "PGExampleApplication.h"
#include "PGExampleFrameListener.h"

#include "PagedGeometry.h"
#include "BatchPage.h"
#include "WindBatchPage.h"
#include "TreeLoader3D.h"
#include "TreeLoader2D.h"
#include "ImpostorPage.h"
#include "GrassLoader.h"

#include "OgreTerrain.h"
#include "OgreTerrainGroup.h"
#include "OgreTerrainQuadTreeNode.h"
#include "OgreTerrainMaterialGeneratorA.h"
#include "OgreTerrainPaging.h"


#define TERRAIN_PAGE_MIN_X 0
#define TERRAIN_PAGE_MIN_Y 0
#define TERRAIN_PAGE_MAX_X 0
#define TERRAIN_PAGE_MAX_Y 0

#define TERRAIN_FILE_PREFIX String("testTerrain")
#define TERRAIN_FILE_SUFFIX String("dat")
#define TERRAIN_WORLD_SIZE 12000.0f
#define TERRAIN_SIZE 513


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
	TerrainGlobalOptions* mTerrainGlobals;
	static TerrainGroup* mTerrainGroup;
	bool mPaging;
	TerrainPaging* mTerrainPaging;
	PageManager* mPageManager;
	PagedGeometry *trees, *grass, *bushes;
	GrassLoader *grassLoader;
	Vector3 mTerrainPos;
	bool mTerrainsImported;
	typedef std::list<Entity*> EntityList;
	EntityList mHouseList;
	ShadowCameraSetupPtr mPSSMSetup;
	static float getTerrainHeight(const float x, const float z, void *userData = NULL);


	void configureTerrainDefaults(Light* l);
	void defineTerrain(long x, long y, bool flat = false);
	void getTerrainImage(bool flipX, bool flipY, Image& img);
	void initBlendMaps(Terrain* terrain);
	void configureShadows(bool enabled, bool depthShadows);
	MaterialPtr buildDepthShadowMaterial(const String& textureName);
};

TerrainGroup* PGSampleApp::mTerrainGroup = 0;

// SAMPLE IMPLEMENTATION
PGSampleApp::PGSampleApp() : ExampleApplication()
{
	mTerrainPos = Vector3::ZERO;
	mTerrainsImported=false;
}

void PGSampleApp::configureTerrainDefaults(Light* l)
{
	// Configure global
	mTerrainGlobals->setMaxPixelError(8);
	// testing composite map
	mTerrainGlobals->setCompositeMapDistance(3000);
	//mTerrainGlobals->setUseRayBoxDistanceCalculation(true);
	//mTerrainGlobals->getDefaultMaterialGenerator()->setDebugLevel(1);
	//mTerrainGlobals->setLightMapSize(256);

	//matProfile->setLightmapEnabled(false);
	// Important to set these so that the terrain knows what to use for derived (non-realtime) data
	mTerrainGlobals->setLightMapDirection(l->getDerivedDirection());
	mTerrainGlobals->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
	//mTerrainGlobals->setCompositeMapAmbient(ColourValue::Red);
	mTerrainGlobals->setCompositeMapDiffuse(l->getDiffuseColour());

	// Configure default import settings for if we use imported image
	Terrain::ImportData& defaultimp = mTerrainGroup->getDefaultImportSettings();
	defaultimp.terrainSize = TERRAIN_SIZE;
	defaultimp.worldSize = TERRAIN_WORLD_SIZE;
	defaultimp.inputScale = 600;
	defaultimp.minBatchSize = 33;
	defaultimp.maxBatchSize = 65;
	// textures
	defaultimp.layerList.resize(3);
	defaultimp.layerList[0].worldSize = 100;
	defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_diffusespecular.dds");
	defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_normalheight.dds");
	defaultimp.layerList[1].worldSize = 30;
	defaultimp.layerList[1].textureNames.push_back("grass_green-01_diffusespecular.dds");
	defaultimp.layerList[1].textureNames.push_back("grass_green-01_normalheight.dds");
	defaultimp.layerList[2].worldSize = 200;
	defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_diffusespecular.dds");
	defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_normalheight.dds");
}

void PGSampleApp::createScene(void)
{
	bool blankTerrain = false;
	mTerrainGlobals = OGRE_NEW TerrainGlobalOptions();

	Vector3 lightdir(0.55, -0.3, 0.75);
	lightdir.normalise();


	Light* l = mSceneMgr->createLight("tstLight");
	l->setType(Light::LT_DIRECTIONAL);
	l->setDirection(lightdir);
	l->setDiffuseColour(ColourValue::White);
	l->setSpecularColour(ColourValue(0.4, 0.4, 0.4));

	mSceneMgr->setAmbientLight(ColourValue(0.2, 0.2, 0.2));


	mTerrainGroup = OGRE_NEW TerrainGroup(mSceneMgr, Terrain::ALIGN_X_Z, TERRAIN_SIZE, TERRAIN_WORLD_SIZE);
	mTerrainGroup->setFilenameConvention(TERRAIN_FILE_PREFIX, TERRAIN_FILE_SUFFIX);
	mTerrainGroup->setOrigin(mTerrainPos + Vector3(TERRAIN_WORLD_SIZE/2, 0, TERRAIN_WORLD_SIZE/2));

	configureTerrainDefaults(l);
#ifdef PAGING
	// Paging setup
	mPageManager = OGRE_NEW PageManager();
	// Since we're not loading any pages from .page files, we need a way just 
	// to say we've loaded them without them actually being loaded
	mPageManager->setPageProvider(&mDummyPageProvider);
	mPageManager->addCamera(mCamera);
	mTerrainPaging = OGRE_NEW TerrainPaging(mPageManager);
	PagedWorld* world = mPageManager->createWorld();
	mTerrainPaging->createWorldSection(world, mTerrainGroup, 2000, 3000, 
		TERRAIN_PAGE_MIN_X, TERRAIN_PAGE_MIN_Y, 
		TERRAIN_PAGE_MAX_X, TERRAIN_PAGE_MAX_Y);
#else
	for (long x = TERRAIN_PAGE_MIN_X; x <= TERRAIN_PAGE_MAX_X; ++x)
		for (long y = TERRAIN_PAGE_MIN_Y; y <= TERRAIN_PAGE_MAX_Y; ++y)
			defineTerrain(x, y, blankTerrain);
	// sync load since we want everything in place when we start
	mTerrainGroup->loadAllTerrains(true);
#endif

	if (mTerrainsImported)
	{
		TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
		while(ti.hasMoreElements())
		{
			Terrain* t = ti.getNext()->instance;
			initBlendMaps(t);
		}
	}

	mTerrainGroup->freeTemporaryResources();



	// create a few entities on the terrain
	Entity* e = mSceneMgr->createEntity("tudorhouse.mesh");
	Vector3 entPos(mTerrainPos.x + 2043, 0, mTerrainPos.z + 1715);
	Quaternion rot;
	entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + 65.5 + mTerrainPos.y;
	rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
	SceneNode* sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
	sn->setScale(Vector3(0.12, 0.12, 0.12));
	sn->attachObject(e);
	mHouseList.push_back(e);

	e = mSceneMgr->createEntity("tudorhouse.mesh");
	entPos = Vector3(mTerrainPos.x + 1850, 0, mTerrainPos.z + 1478);
	entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + 65.5 + mTerrainPos.y;
	rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
	sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
	sn->setScale(Vector3(0.12, 0.12, 0.12));
	sn->attachObject(e);
	mHouseList.push_back(e);

	e = mSceneMgr->createEntity("tudorhouse.mesh");
	entPos = Vector3(mTerrainPos.x + 1970, 0, mTerrainPos.z + 2180);
	entPos.y = mTerrainGroup->getHeightAtWorldPosition(entPos) + 65.5 + mTerrainPos.y;
	rot.FromAngleAxis(Degree(Math::RangeRandom(-180, 180)), Vector3::UNIT_Y);
	sn = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos, rot);
	sn->setScale(Vector3(0.12, 0.12, 0.12));
	sn->attachObject(e);
	mHouseList.push_back(e);

	mSceneMgr->setSkyBox(true, "Examples/CloudyNoonSkyBox");


	configureShadows(true, false);

	mCamera->setPosition(mTerrainPos + Vector3(1683, 50, 2116));
	mCamera->lookAt(Vector3(1963, 50, 1660));
	mCamera->setNearClipDistance(0.1);
	mCamera->setFarClipDistance(50000);

	if (mRoot->getRenderSystem()->getCapabilities()->hasCapability(RSC_INFINITE_FAR_PLANE))
    {
        mCamera->setFarClipDistance(0);   // enable infinite far clip distance if we can
    }

}

void PGSampleApp::configureShadows(bool enabled, bool depthShadows)
{
	TerrainMaterialGeneratorA::SM2Profile* matProfile = 
		static_cast<TerrainMaterialGeneratorA::SM2Profile*>(mTerrainGlobals->getDefaultMaterialGenerator()->getActiveProfile());
	matProfile->setReceiveDynamicShadowsEnabled(enabled);
#ifdef SHADOWS_IN_LOW_LOD_MATERIAL
	matProfile->setReceiveDynamicShadowsLowLod(true);
#else
	matProfile->setReceiveDynamicShadowsLowLod(false);
#endif

	// Default materials
	for (EntityList::iterator i = mHouseList.begin(); i != mHouseList.end(); ++i)
	{
		(*i)->setMaterialName("Examples/TudorHouse");
	}

	if (enabled)
	{
		// General scene setup
		mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
		mSceneMgr->setShadowFarDistance(3000);

		// 3 textures per directional light (PSSM)
		mSceneMgr->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, 3);

		if (mPSSMSetup.isNull())
		{
			// shadow camera setup
			PSSMShadowCameraSetup* pssmSetup = new PSSMShadowCameraSetup();
			pssmSetup->setSplitPadding(mCamera->getNearClipDistance());
			pssmSetup->calculateSplitPoints(3, mCamera->getNearClipDistance(), mSceneMgr->getShadowFarDistance());
			pssmSetup->setOptimalAdjustFactor(0, 2);
			pssmSetup->setOptimalAdjustFactor(1, 1);
			pssmSetup->setOptimalAdjustFactor(2, 0.5);

			mPSSMSetup.bind(pssmSetup);

		}
		mSceneMgr->setShadowCameraSetup(mPSSMSetup);

		if (depthShadows)
		{
			mSceneMgr->setShadowTextureCount(3);
			mSceneMgr->setShadowTextureConfig(0, 2048, 2048, PF_FLOAT32_R);
			mSceneMgr->setShadowTextureConfig(1, 1024, 1024, PF_FLOAT32_R);
			mSceneMgr->setShadowTextureConfig(2, 1024, 1024, PF_FLOAT32_R);
			mSceneMgr->setShadowTextureSelfShadow(true);
			mSceneMgr->setShadowCasterRenderBackFaces(true);
			mSceneMgr->setShadowTextureCasterMaterial("PSSM/shadow_caster");

			MaterialPtr houseMat = buildDepthShadowMaterial("fw12b.jpg");
			for (EntityList::iterator i = mHouseList.begin(); i != mHouseList.end(); ++i)
			{
				(*i)->setMaterial(houseMat);
			}

		}
		else
		{
			mSceneMgr->setShadowTextureCount(3);
			mSceneMgr->setShadowTextureConfig(0, 2048, 2048, PF_X8B8G8R8);
			mSceneMgr->setShadowTextureConfig(1, 1024, 1024, PF_X8B8G8R8);
			mSceneMgr->setShadowTextureConfig(2, 1024, 1024, PF_X8B8G8R8);
			mSceneMgr->setShadowTextureSelfShadow(false);
			mSceneMgr->setShadowCasterRenderBackFaces(false);
			mSceneMgr->setShadowTextureCasterMaterial(StringUtil::BLANK);
		}

		matProfile->setReceiveDynamicShadowsDepth(depthShadows);
		matProfile->setReceiveDynamicShadowsPSSM(static_cast<PSSMShadowCameraSetup*>(mPSSMSetup.get()));

		//addTextureShadowDebugOverlay(TL_RIGHT, 3);


	}
	else
	{
		mSceneMgr->setShadowTechnique(SHADOWTYPE_NONE);
	}


}

MaterialPtr PGSampleApp::buildDepthShadowMaterial(const String& textureName)
{
	String matName = "DepthShadows/" + textureName;

	MaterialPtr ret = MaterialManager::getSingleton().getByName(matName);
	if (ret.isNull())
	{
		MaterialPtr baseMat = MaterialManager::getSingleton().getByName("Ogre/shadow/depth/integrated/pssm");
		ret = baseMat->clone(matName);
		Pass* p = ret->getTechnique(0)->getPass(0);
		p->getTextureUnitState("diffuse")->setTextureName(textureName);

		Vector4 splitPoints;
		const PSSMShadowCameraSetup::SplitPointList& splitPointList = 
			static_cast<PSSMShadowCameraSetup*>(mPSSMSetup.get())->getSplitPoints();
		for (int i = 0; i < 3; ++i)
		{
			splitPoints[i] = splitPointList[i];
		}
		p->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);


	}

	return ret;
}

void  PGSampleApp::defineTerrain(long x, long y, bool flat)
{
	// if a file is available, use it
	// if not, generate file from import

	// Usually in a real project you'll know whether the compact terrain data is
	// available or not; I'm doing it this way to save distribution size

	if (flat)
	{
		mTerrainGroup->defineTerrain(x, y, 0.0f);
	}
	else
	{
		String filename = mTerrainGroup->generateFilename(x, y);
		if (ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
		{
			mTerrainGroup->defineTerrain(x, y);
		}
		else
		{
			Image img;
			getTerrainImage(x % 2 != 0, y % 2 != 0, img);
			mTerrainGroup->defineTerrain(x, y, &img);
			mTerrainsImported = true;
		}

	}
}

void PGSampleApp::getTerrainImage(bool flipX, bool flipY, Image& img)
{
	img.load("terrain.png", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	if (flipX)
		img.flipAroundY();
	if (flipY)
		img.flipAroundX();

}

void PGSampleApp::initBlendMaps(Terrain* terrain)
{
	TerrainLayerBlendMap* blendMap0 = terrain->getLayerBlendMap(1);
	TerrainLayerBlendMap* blendMap1 = terrain->getLayerBlendMap(2);
	Real minHeight0 = 70;
	Real fadeDist0 = 40;
	Real minHeight1 = 70;
	Real fadeDist1 = 15;
	float* pBlend1 = blendMap1->getBlendPointer();
	for (Ogre::uint16 y = 0; y < terrain->getLayerBlendMapSize(); ++y)
	{
		for (Ogre::uint16 x = 0; x < terrain->getLayerBlendMapSize(); ++x)
		{
			Real tx, ty;

			blendMap0->convertImageToTerrainSpace(x, y, &tx, &ty);
			Real height = terrain->getHeightAtTerrainPosition(tx, ty);
			Real val = (height - minHeight0) / fadeDist0;
			val = Math::Clamp(val, (Real)0, (Real)1);
			//*pBlend0++ = val;

			val = (height - minHeight1) / fadeDist1;
			val = Math::Clamp(val, (Real)0, (Real)1);
			*pBlend1++ = val;


		}
	}
	blendMap0->dirty();
	blendMap1->dirty();
	//blendMap0->loadImage("blendmap1.png", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	blendMap0->update();
	blendMap1->update();

	// set up a colour map
	/*
	if (!terrain->getGlobalColourMapEnabled())
	{
		terrain->setGlobalColourMapEnabled(true);
		Image colourMap;
		colourMap.load("testcolourmap.jpg", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		terrain->getGlobalColourMap()->loadImage(colourMap);
	}
	*/

}

float PGSampleApp::getTerrainHeight(const float x, const float z, void *userData)
{
	return mTerrainGroup->getHeightAtWorldPosition(x, 1000, z);
}

void PGSampleApp::createPGDemo(void)
{
	//-------------------------------------- LOAD GRASS --------------------------------------
	//Create and configure a new PagedGeometry instance for grass
	grass = new PagedGeometry(mCamera, 30);
	grass->addDetailLevel<GrassPage>(1000);

	//Create a GrassLoader object
	grassLoader = new GrassLoader(grass);
	grass->setPageLoader(grassLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to GrassLoader so it can calculate grass Y values
	grassLoader->setHeightFunction(&getTerrainHeight);

	//Add some grass to the scene with GrassLoader::addLayer()
	GrassLayer *l = grassLoader->addLayer("grass"); //"3D-Diggers/plant1sprite");

	//Configure the grass layer properties (size, density, animation properties, fade settings, etc.)
	l->setMinimumSize(2.7f, 2.7f);
	l->setMaximumSize(4.9f, 4.9f);
	l->setAnimationEnabled(true);		//Enable animations
	l->setSwayDistribution(7.0f);		//Sway fairly unsynchronized
	l->setSwayLength(0.5f);				//Sway back and forth 0.5 units in length
	l->setSwaySpeed(0.4f);				//Sway 1/2 a cycle every second
	l->setDensity(0.2f);				//Relatively dense grass
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
	l->setMapBounds(TBounds(mTerrainPos.x, mTerrainPos.z, mTerrainPos.x+TERRAIN_WORLD_SIZE, mTerrainPos.z+TERRAIN_WORLD_SIZE));	//(0,0)-(1500,1500) is the full boundaries of the terrain


	//-------------------------------------- LOAD TREES --------------------------------------
	//Create and configure a new PagedGeometry instance
	trees = new PagedGeometry();
	trees->setCamera(mCamera);	//Set the camera so PagedGeometry knows how to calculate LODs
	trees->setPageSize(200);	//Set the size of each page of geometry
	trees->setInfinite();		//Use infinite paging mode

#ifdef WIND
	//WindBatchPage is a variation of BatchPage which includes a wind animation shader
	trees->addDetailLevel<WindBatchPage>(2000, 30);		//Use batches up to 150 units away, and fade for 30 more units
#else
	trees->addDetailLevel<BatchPage>(2000, 30);		//Use batches up to 150 units away, and fade for 30 more units
#endif
	trees->addDetailLevel<ImpostorPage>(TERRAIN_WORLD_SIZE, 50);	//Use impostors up to 400 units, and for for 50 more units

	//Create a new TreeLoader2D object
	TreeLoader2D *treeLoader = new TreeLoader2D(trees, TBounds(mTerrainPos.x, mTerrainPos.z, mTerrainPos.x+TERRAIN_WORLD_SIZE, mTerrainPos.z+TERRAIN_WORLD_SIZE));
	trees->setPageLoader(treeLoader);	//Assign the "treeLoader" to be used to load geometry for the PagedGeometry instance

	//Supply a height function to TreeLoader2D so it can calculate tree Y values
	treeLoader->setHeightFunction(&getTerrainHeight);

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
	trees->setCustomParam(tree1->getName(), "windFactorX", 1);
	trees->setCustomParam(tree1->getName(), "windFactorY", 0.001);
	trees->setCustomParam(tree2->getName(), "windFactorX", 2);
	trees->setCustomParam(tree2->getName(), "windFactorY", 0.0013);
#endif

	//Randomly place 10000 copies of the tree on the terrain
	Ogre::Vector3 position = Ogre::Vector3::ZERO;
	Radian yaw;
	Real scale;
	for (int i = 0; i < 1000; i++)
	{
		yaw = Degree(Math::RangeRandom(0, 360));

		position.x = Math::RangeRandom(mTerrainPos.x, mTerrainPos.x + TERRAIN_WORLD_SIZE);
		position.z = Math::RangeRandom(mTerrainPos.z, mTerrainPos.z + TERRAIN_WORLD_SIZE);

		scale = Math::RangeRandom(0.5f, 1.5f);

		//[NOTE] Unlike TreeLoader3D, TreeLoader2D's addTree() function accepts a Vector2D position (x/z)
		//The Y value is calculated during runtime (to save memory) from the height function supplied (above)
		if (Math::UnitRandom() < 0.5f)
			treeLoader->addTree(tree1, position, yaw, scale);
		else
			treeLoader->addTree(tree2, position, yaw, scale);
	}

	//-------------------------------------- LOAD BUSHES --------------------------------------
	//Create and configure a new PagedGeometry instance for bushes
	bushes = new PagedGeometry(mCamera, 50);

#ifdef WIND
	bushes->addDetailLevel<WindBatchPage>(1000, 50);
#else
	bushes->addDetailLevel<BatchPage>(1000, 50);
#endif

	//Create a new TreeLoader2D object for the bushes
	TreeLoader2D *bushLoader = new TreeLoader2D(bushes, TBounds(mTerrainPos.x, mTerrainPos.z, mTerrainPos.x+TERRAIN_WORLD_SIZE, mTerrainPos.z+TERRAIN_WORLD_SIZE));
	bushes->setPageLoader(bushLoader);

	//Supply the height function to TreeLoader2D so it can calculate tree Y values
	bushLoader->setHeightFunction(&getTerrainHeight);

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
		position.x = Math::RangeRandom(mTerrainPos.x, mTerrainPos.x+TERRAIN_WORLD_SIZE);
		position.z = Math::RangeRandom(mTerrainPos.z, mTerrainPos.z+TERRAIN_WORLD_SIZE);

		float rnd = Math::UnitRandom();
		if (rnd < 0.8f) {
			scale = Math::RangeRandom(0.6f, 1.4f);
			bushLoader->addTree(fern, position, yaw, scale);
		} else if (rnd < 0.9) {
			scale = Math::RangeRandom(0.6f, 1.6f);
			bushLoader->addTree(mushroom, position, yaw, scale);
		} else {
			scale = Math::RangeRandom(0.5f, 1.5f);
			bushLoader->addTree(plant, position, yaw, scale);
		}
	}
	addPG(trees);
	addPG(grass);
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

