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

#include <OgreTerrainGroup.h>

inline Ogre::TerrainGroup* loadLegacyTerrain(const Ogre::String& cfgFileName, Ogre::SceneManager* sceneMgr)
{
    using namespace Ogre;

    if(!TerrainGlobalOptions::getSingletonPtr())
        new TerrainGlobalOptions();

    auto terrainGroup = new TerrainGroup(sceneMgr);
#if OGRE_VERSION >= ((1 << 16) | (11 << 8) | 6)
    terrainGroup->loadLegacyTerrain(cfgFileName);
#endif

    return terrainGroup;
}
