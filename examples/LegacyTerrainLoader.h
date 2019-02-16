/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2009 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
-----------------------------------------------------------------------------
*/

#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>
#include <OgreTerrainMaterialGeneratorA.h>
#include <OgreConfigFile.h>

inline Ogre::TerrainGroup* loadLegacyTerrain(const Ogre::String& cfgFileName, Ogre::SceneManager* sceneMgr)
{
    using namespace Ogre;

    ConfigFile cfg;
    cfg.loadFromResourceSystem(cfgFileName, RGN_DEFAULT);
    cfg.getSettings();

    float worldSize;
    uint32 terrainSize;
    StringConverter::parse(cfg.getSetting("PageSize"), terrainSize);
    StringConverter::parse(cfg.getSetting("PageWorldX"), worldSize);

    auto terrainGlobals = TerrainGlobalOptions::getSingletonPtr();
    if(!terrainGlobals)
        terrainGlobals = new TerrainGlobalOptions();

    terrainGlobals->setMaxPixelError(StringConverter::parseReal(cfg.getSetting("MaxPixelError")));

    auto profile = static_cast<TerrainMaterialGeneratorA::SM2Profile *>(
            terrainGlobals->getDefaultMaterialGenerator()->getActiveProfile());
    profile->setLayerSpecularMappingEnabled(false);
    profile->setLayerNormalMappingEnabled(false);
    profile->setLightmapEnabled(false); // baked into diffusemap

    auto terrainGroup = new TerrainGroup(sceneMgr, Terrain::ALIGN_X_Z, terrainSize, worldSize);
    terrainGroup->setOrigin(Vector3(worldSize/2, 0, worldSize/2));

    auto &defaultimp = terrainGroup->getDefaultImportSettings();
    defaultimp.terrainSize = terrainSize;

    StringConverter::parse(cfg.getSetting("MaxHeight"), defaultimp.inputScale);
    defaultimp.maxBatchSize = StringConverter::parseInt(cfg.getSetting("TileSize"));
    defaultimp.minBatchSize = (defaultimp.maxBatchSize - 1)/2 + 1;

    const String& worldTexName = cfg.getSetting("WorldTexture");
    defaultimp.layerList.resize(1);
    defaultimp.layerList[0].worldSize = worldSize; // covers whole terrain
    defaultimp.layerList[0].textureNames.push_back(worldTexName);
    // avoid empty texture name - will otherwise not be used
    defaultimp.layerList[0].textureNames.push_back(worldTexName);

    // Load terrain from heightmap
    Image img;
    img.load(cfg.getSetting("Heightmap.image"), terrainGlobals->getDefaultResourceGroup());
    terrainGroup->defineTerrain(0, 0, &img);

    // sync load since we want everything in place when we start
    terrainGroup->loadTerrain(0, 0, true);

    terrainGroup->freeTemporaryResources();

    return terrainGroup;
}
