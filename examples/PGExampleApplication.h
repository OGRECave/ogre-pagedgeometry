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
/*
-----------------------------------------------------------------------------
Filename:    ExampleApplication.h
Description: Base class for all the OGRE examples
-----------------------------------------------------------------------------
*/

#ifndef __PGExampleApplication_H__
#define __PGExampleApplication_H__

#include "Ogre.h"
#include "OgreConfigFile.h"
#include <vector>
#include "PagedGeometry.h"

#include <OgreApplicationContext.h>
#include <OgreCameraMan.h>
#include <OgreAdvancedRenderControls.h>

using namespace Ogre;

/** Base class which manages the standard startup of an Ogre application.
    Designed to be subclassed for specific examples if required.
*/
class ExampleApplication : public OgreBites::ApplicationContext
{
public:
    /// Standard constructor
    ExampleApplication() : OgreBites::ApplicationContext()
    {
        mCameraMan = 0;
        mRenderControls = 0;

    }
    /// Standard destructor
    virtual ~ExampleApplication()
    {
        if (mCameraMan)
            delete mCameraMan;
        if (mRenderControls)
            delete mRenderControls;
    }

	virtual void addPG(Forests::PagedGeometry *pg)
	{
		pgs.push_back(pg);
	}

    /// Start the example
    virtual void go(void)
    {
        initApp();
        setWindowGrab();
        mRoot->startRendering();

        // clean up
        destroyScene();	
        closeApp();
    }

protected:
    Camera* mCamera;
    SceneNode* mCameraNode;
    SceneManager* mSceneMgr;
    OgreBites::CameraMan* mCameraMan;
    OgreBites::AdvancedRenderControls* mRenderControls;

	Ogre::String mResourcePath;
	Ogre::String mConfigPath;
	std::vector<Forests::PagedGeometry *> pgs;


    // These internal methods package up the stages in the startup process
    /** Sets up the application - returns false if the user chooses to abandon configuration. */
    virtual void setup(void)
    {
        OgreBites::ApplicationContext::setup();

        chooseSceneManager();
        createCamera();
        createViewports();


        // Set default mipmap level (NB some APIs ignore this)
        TextureManager::getSingleton().setDefaultNumMipmaps(5);

		// Create the scene
        createScene();
		
		createPGDemo();
    }

    virtual void chooseSceneManager(void)
    {
        mSceneMgr = mRoot->createSceneManager();
    }
	
    virtual void createCamera(void)
    {
        // Create the camera
        mCamera = mSceneMgr->createCamera("PlayerCam");

        mCameraNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        mCameraNode->attachObject(mCamera);

        mCameraMan = new OgreBites::CameraMan(mCameraNode);
        addInputListener(mCameraMan);
        mCameraMan->setStyle(OgreBites::CS_FREELOOK);

        // Position it at 500 in Z direction
        mCameraNode->setPosition(Vector3(0,50,500));
        // Look back along -Z
        mCameraNode->lookAt(Vector3(0,0,-300), Node::TS_WORLD);
        mCamera->setNearClipDistance(5);

    }

    bool frameRenderingQueued(const Ogre::FrameEvent& evt) {
        OgreBites::ApplicationContext::frameRenderingQueued(evt);

        for(std::vector<Forests::PagedGeometry *>::iterator it=pgs.begin(); it!=pgs.end(); it++)
        {
            (*it)->update();
        }

        return true;
    }

	virtual void createPGDemo(void) = 0; 
	
    virtual void createScene(void)
	{

		//Setup the fog up to 1500 units away
		mSceneMgr->setFog(FOG_LINEAR, getRenderWindow()->getViewport(0)->getBackgroundColour(), 0, 100, 900);

		//Load the terrain
		mSceneMgr->setWorldGeometry("terrain2.cfg");

		//Start off with the camera at the center of the terrain
		mCameraNode->setPosition(700, 100, 700);

		//Setup a skybox
		mSceneMgr->setSkyBox(true, "3D-Diggers/SkyBox", 2000);

		// setup some useful defaults
		MaterialManager::getSingleton().setDefaultTextureFiltering(TFO_ANISOTROPIC);
		MaterialManager::getSingleton().setDefaultAnisotropy(7);

		LogManager::getSingleton().setLogDetail(LL_BOREME);

		Light *light = mSceneMgr->createLight("Sun");
		light->setType(Light::LT_DIRECTIONAL);
		light->setDirection(Ogre::Vector3(0.0f, -0.5f, 1.0f));
		mSceneMgr->setAmbientLight(Ogre::ColourValue(1, 1, 1));

		mCamera->setPosition(Vector3(100, 50, 1000));
		mCamera->lookAt(Vector3(150, 50, 1000));
		mCamera->setNearClipDistance(0.1);
		mCamera->setFarClipDistance(50000);

		if (mRoot->getRenderSystem()->getCapabilities()->hasCapability(RSC_INFINITE_FAR_PLANE))
		{
			mCamera->setFarClipDistance(0);   // enable infinite far clip distance if we can
		}
	}

    virtual void destroyScene(void){
        for(std::vector<Forests::PagedGeometry *>::iterator it=pgs.begin(); it!=pgs.end(); it++)
        {
            (*it)->removeDetailLevels();
        }
    }

    virtual void createViewports(void)
    {
        // Create one viewport, entire window
        Viewport* vp = getRenderWindow()->addViewport(mCamera);
        vp->setBackgroundColour(ColourValue(0.47f, 0.67f, 0.96f));	//Blue sky background color

        // Alter the camera aspect ratio to match the viewport
        mCamera->setAspectRatio(
            Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
    }

};

#endif
