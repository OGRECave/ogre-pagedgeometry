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
    StringConverter::parse(cfg.getSettings().find("PageSize")->second, terrainSize);
    StringConverter::parse(cfg.getSettings().find("PageWorldX")->second, worldSize);

    auto terrainGlobals = TerrainGlobalOptions::getSingletonPtr();
    if(!terrainGlobals)
        terrainGlobals = new TerrainGlobalOptions();

    terrainGlobals->setMaxPixelError(StringConverter::parseReal(cfg.getSettings().find("MaxPixelError")->second));

    auto profile = static_cast<TerrainMaterialGeneratorA::SM2Profile *>(
            terrainGlobals->getDefaultMaterialGenerator()->getActiveProfile());
    profile->setLayerSpecularMappingEnabled(false);
    profile->setLayerNormalMappingEnabled(false);

    auto terrainGroup = new TerrainGroup(sceneMgr, Terrain::ALIGN_X_Z, terrainSize, worldSize);
    terrainGroup->setOrigin(Vector3(worldSize/2, 0, worldSize/2));

    auto &defaultimp = terrainGroup->getDefaultImportSettings();
    defaultimp.terrainSize = terrainSize;

    StringConverter::parse(cfg.getSettings().find("MaxHeight")->second, defaultimp.inputScale);
    defaultimp.maxBatchSize = StringConverter::parseInt(cfg.getSettings().find("TileSize")->second);
    defaultimp.minBatchSize = defaultimp.maxBatchSize;

    const String& worldTexName = cfg.getSettings().find("WorldTexture")->second;
    defaultimp.layerList.resize(1);
    defaultimp.layerList[0].worldSize = worldSize; // covers whole terrain
    defaultimp.layerList[0].textureNames.push_back(worldTexName);
    // avoid empty texture name - will otherwise not be used
    defaultimp.layerList[0].textureNames.push_back(worldTexName);

    // Load terrain from heightmap
    Image img;
    img.load(cfg.getSettings().find("Heightmap.image")->second, RGN_DEFAULT);
    terrainGroup->defineTerrain(0, 0, &img);

    // sync load since we want everything in place when we start
    terrainGroup->loadTerrain(0, 0, true);
    img.load("terrain_lightmap.jpg", RGN_DEFAULT);
    terrainGroup->getTerrain(0, 0)->getLightmap()->loadImage(img);

    terrainGroup->freeTemporaryResources();

    return terrainGroup;
}
