/*-------------------------------------------------------------------------------------
Copyright (c) 2006 John Judnich
Modified 2008 by Erik Hjortsberg (erik.hjortsberg@iteam.se)

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------------*/

//ImpostorPage.cpp
//ImposterPage is an extension to PagedGeometry which displays entities as imposters.
//-------------------------------------------------------------------------------------

#include <sstream>
#include <OgreRoot.h>
#include <OgreTimer.h>
#include <OgreCamera.h>
#include <OgreQuaternion.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreViewport.h>
#include <OgreTechnique.h>

#include "ImpostorPage.h"
#include "StaticBillboardSet.h"

using namespace Ogre;
using namespace Forests;

// static members initialization
Ogre::uint  Forests::ImpostorPage::s_nImpostorResolution    = 128;
Ogre::uint  Forests::ImpostorPage::s_nSelfInstances         = 0;
Ogre::uint  Forests::ImpostorPage::s_nUpdateInstanceID      = 0;
ColourValue Forests::ImpostorPage::s_clrImpostorBackground  = ColourValue(0.0f, 0.3f, 0.0f, 0.0f);
BillboardOrigin Forests::ImpostorPage::s_impostorPivot      = BBO_CENTER;



//-----------------------------------------------------------------------------
/// Default constructor
ImpostorPage::ImpostorPage() :
m_pSceneMgr    (NULL),
m_pPagedGeom   (NULL),
m_blendMode    (ALPHA_REJECT_IMPOSTOR),
m_nInstanceID  (0),
m_nAveCount    (0),
m_vecCenter    (0, 0, 0)
{
   ++s_nSelfInstances;
}


//-----------------------------------------------------------------------------
/// Destructor
ImpostorPage::~ImpostorPage()
{
   TImpostorBatchs::iterator iter = m_mapImpostorBatches.begin(), iend = m_mapImpostorBatches.end();
   while (iter != iend)
   {
      delete iter->second;
      ++iter;
   }

   if (--s_nSelfInstances == 0 && m_pPagedGeom)
   {
      if (m_pPagedGeom->getSceneNode())
      {
         m_pPagedGeom->getSceneNode()->removeAndDestroyChild("ImpostorPage::renderNode");
         m_pPagedGeom->getSceneNode()->removeAndDestroyChild("ImpostorPage::cameraNode");
      }
      else if (m_pSceneMgr)
      {
         m_pSceneMgr->destroySceneNode("ImpostorPage::renderNode");
         m_pSceneMgr->destroySceneNode("ImpostorPage::cameraNode");
      }
      else
      {
         assert(false && "Who must delete scene node???");
      }

      ResourceGroupManager::getSingleton().destroyResourceGroup("Impostors");
   }
}


//-----------------------------------------------------------------------------
///
void ImpostorPage::init(PagedGeometry *geom, const Ogre::Any &data)
{
   assert(geom && "Null pointer to PagedGeometry");
	m_pSceneMgr    = geom->getSceneManager();
	m_pPagedGeom   = geom;
		
	if (s_nSelfInstances == 1)  // first instance
   {
		// Set up a single instance of a scene node which will be used when rendering impostor textures
		geom->getSceneNode()->createChildSceneNode("ImpostorPage::renderNode");
		geom->getSceneNode()->createChildSceneNode("ImpostorPage::cameraNode");
      ResourceGroupManager::getSingleton().createResourceGroup("Impostors");
	}
}


//-----------------------------------------------------------------------------
///
void ImpostorPage::setRegion(Ogre::Real left, Ogre::Real top, Ogre::Real right, Ogre::Real bottom)
{
	// Calculate center of region
	m_vecCenter.x  = (left + right) * 0.5f;
	m_vecCenter.z  = (top + bottom) * 0.5f;
	m_vecCenter.y  = 0.0f; // The center.y value is calculated when the entities are added
	m_nAveCount    = 0;
}


//-----------------------------------------------------------------------------
///
void ImpostorPage::addEntity(Entity *ent, const Vector3 &position, const Quaternion &rotation, const Vector3 &scale, const Ogre::ColourValue &color)
{
	//Get the impostor batch that this impostor will be added to
	ImpostorBatch *ibatch = ImpostorBatch::getBatch(this, ent);

	//Then add the impostor to the batch
	ibatch->addBillboard(position, rotation, scale, color);

	//Add the Y position to the center.y value (to be averaged later)
	m_vecCenter.y += position.y + ent->getBoundingBox().getCenter().y * scale.y;
	++m_nAveCount;
}


//-----------------------------------------------------------------------------
///
void ImpostorPage::build()
{
   if (m_mapImpostorBatches.empty())
      return;

	// Calculate the average Y value of all the added entities
   if (m_nAveCount > 0)
      m_vecCenter.y /= m_nAveCount;
   else
      m_vecCenter.y = 0;

	//Build all batches
	TImpostorBatchs::iterator it = m_mapImpostorBatches.begin(), iend = m_mapImpostorBatches.end();
   while (it != iend)
   {
      it->second->build();
      ++it;
   }
}

//-----------------------------------------------------------------------------
///
void ImpostorPage::setVisible(bool visible)
{
	// Update visibility status of all batches
	TImpostorBatchs::iterator it = m_mapImpostorBatches.begin(), iend = m_mapImpostorBatches.end();
   while (it != iend)
   {
      it->second->setVisible(visible);
      ++it;
   }
}


//-----------------------------------------------------------------------------
///
void ImpostorPage::setFade(bool enabled, Real visibleDist, Real invisibleDist)
{
	// Update fade status of all batches
	TImpostorBatchs::iterator it = m_mapImpostorBatches.begin(), iend = m_mapImpostorBatches.end();
   while (it != iend)
   {
      it->second->setFade(enabled, visibleDist, invisibleDist);
      ++it;
   }
}


//-----------------------------------------------------------------------------
///
void ImpostorPage::removeEntities()
{
	TImpostorBatchs::iterator iter = m_mapImpostorBatches.begin(), iend = m_mapImpostorBatches.end();
   while (iter != iend)
   {
      iter->second->clear();
      ++iter;
	}

	//Reset y center
	m_vecCenter.y  = 0;
	m_nAveCount    = 0;
}


void ImpostorPage::regenerate(Entity *ent)
{
	ImpostorTexture *tex = ImpostorTexture::getTexture(NULL, ent);
	if (tex != NULL)
		tex->regenerate();
}

void ImpostorPage::regenerateAll()
{
	ImpostorTexture::regenerateAll();
}


//-------------------------------------------------------------------------------------

unsigned long ImpostorBatch::GUID = 0;

ImpostorBatch::ImpostorBatch(ImpostorPage *group, Entity *entity) :
m_pTexture  (NULL)
{
	// Render impostor texture for this entity
	m_pTexture = ImpostorTexture::getTexture(group, entity);
	
	//Create billboard set
   PagedGeometry *pg = group->getParentPagedGeometry();
   bbset = new StaticBillboardSet(pg->getSceneManager(), pg->getSceneNode());
	bbset->setVisibilityFlags(pg->getVisibilityFlags());
	bbset->setTextureStacksAndSlices(IMPOSTOR_PITCH_ANGLES, IMPOSTOR_YAW_ANGLES);

	setBillboardOrigin(ImpostorPage::getImpostorPivot());
	bbset->setMaterial(m_pTexture->material->getName(), m_pTexture->material->getGroup());

	//Init. variables
	igroup = group;
}

ImpostorBatch::~ImpostorBatch()
{
	//Delete billboard set
	delete bbset;

	// Delete texture
	ImpostorTexture::removeTexture(m_pTexture);
}

//Returns a pointer to an ImpostorBatch for the specified entity in the specified
//ImpostorPage. If one does not already exist, one will automatically be created.
ImpostorBatch *ImpostorBatch::getBatch(ImpostorPage *group, Entity *entity)
{
	//Search for an existing impostor batch for this entity
	String entityKey = ImpostorBatch::generateEntityKey(entity);
   ImpostorBatch *batch = group->getImpostorBatch(entityKey);
   if (batch)  // If found return it
      return batch;

   // Otherwise, create a new batch
   batch = new ImpostorBatch(group, entity);
   group->injectImpostorBatch(entityKey, batch);   // warning! function can return false
   return batch;
}

void ImpostorBatch::setBillboardOrigin(BillboardOrigin origin)
{
	bbset->setBillboardOrigin(origin);

	if (bbset->getBillboardOrigin() == BBO_CENTER)
		entityBBCenter = m_pTexture->entityCenter;
	else if (bbset->getBillboardOrigin() == BBO_BOTTOM_CENTER)
		entityBBCenter = Vector3(m_pTexture->entityCenter.x, m_pTexture->entityCenter.y - m_pTexture->entityRadius, m_pTexture->entityCenter.z);
}

String ImpostorBatch::generateEntityKey(Entity *entity)
{
	Ogre::StringStream entityKey;
	entityKey << entity->getMesh()->getName();
	for (unsigned int i = 0; i < entity->getNumSubEntities(); ++i)
   {
		entityKey << "-" << entity->getSubEntity(i)->getMaterialName();
	}
	entityKey << "-" << IMPOSTOR_YAW_ANGLES << "_" << IMPOSTOR_PITCH_ANGLES;
#ifdef IMPOSTOR_RENDER_ABOVE_ONLY
	entityKey << "_RAO";
#endif
	return entityKey.str();
}

//-------------------------------------------------------------------------------------

void ImpostorTexture::loadResource (Ogre::Resource *resource)
{
	if (resource->getLoadingState() == Ogre::Resource::LOADSTATE_UNLOADED) {
		regenerate();
	}
}

//-------------------------------------------------------------------------------------


std::map<String, ImpostorTexture *> ImpostorTexture::selfList;
unsigned long ImpostorTexture::GUID = 0;

//Do not use this constructor yourself - instead, call getTexture()
//to get/create an ImpostorTexture for an Entity.
ImpostorTexture::ImpostorTexture(ImpostorPage *group, Entity *entity)
{
	//Store scene manager and entity
   ImpostorTexture::sceneMgr = group->getParentPagedGeometry()->getSceneManager();
	ImpostorTexture::entity = entity;
	ImpostorTexture::group = group;

	//Add self to list of ImpostorTexture's
	entityKey = ImpostorBatch::generateEntityKey(entity);
	typedef std::pair<String, ImpostorTexture *> ListItem;
	selfList.insert(ListItem(entityKey, this));
	
	//Calculate the entity's bounding box and it's diameter
	boundingBox = entity->getBoundingBox();

	entityRadius = Math::boundingRadiusFromAABB(boundingBox);
	entityDiameter = 2.0f * entityRadius;
	entityCenter = boundingBox.getCenter();
	
	//Render impostor textures
	renderTextures(false);
	
	//Set up the material
	material = MaterialManager::getSingleton().create(getUniqueID("ImpostorMaterial"), "Impostors");

	Pass *p = material->getTechnique(0)->getPass(0);
	p->createTextureUnitState(texture->getName());
	p->setLightingEnabled(false);
	material->setReceiveShadows(false);

	if (group->getBlendMode() == ALPHA_REJECT_IMPOSTOR){
		p->setAlphaRejectSettings(CMPF_GREATER_EQUAL, 128);
	} else if (group->getBlendMode() == ALPHA_BLEND_IMPOSTOR){
		p->setSceneBlending(SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
		p->setDepthWriteEnabled(false);
	}

	//The vertex shader picks the atlas image per impostor, matching the angles used in renderTextures()
	p->setVertexProgram("SpriteImpostor_vp");
	p->setFragmentProgram(group->getParentPagedGeometry()->getFragmentProgramName());
#ifdef IMPOSTOR_RENDER_ABOVE_ONLY
	const Real minPitch = 0;
	const Real maxPitch = Math::HALF_PI * (IMPOSTOR_PITCH_ANGLES - 1) / IMPOSTOR_PITCH_ANGLES;
#else
	const Real minPitch = -Math::HALF_PI;
	const Real maxPitch = Math::PI * (IMPOSTOR_PITCH_ANGLES - 1) / IMPOSTOR_PITCH_ANGLES - Math::HALF_PI;
#endif
	p->getVertexProgramParameters()->setNamedConstant("impostorParams",
		Vector4(IMPOSTOR_YAW_ANGLES, IMPOSTOR_PITCH_ANGLES, minPitch, IMPOSTOR_PITCH_ANGLES / (maxPitch - minPitch)));
}

void ImpostorTexture::updateMaterials()
{
	material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(texture->getName());
}

ImpostorTexture::~ImpostorTexture()
{
	//Delete textures
	if (TextureManager::getSingletonPtr())
		TextureManager::getSingleton().remove(texture);
	texture.reset();

	//Delete material
	if (MaterialManager::getSingletonPtr())
		MaterialManager::getSingleton().remove(material);
	material.reset();
	
	//Remove self from list of ImpostorTexture's
	selfList.erase(entityKey);
}

void ImpostorTexture::regenerate()
{
	if (TextureManager::getSingletonPtr())
		TextureManager::getSingleton().remove(texture);
	texture.reset();
	renderTextures(true);
	updateMaterials();
}

void ImpostorTexture::regenerateAll()
{
	std::map<String, ImpostorTexture *>::iterator iter;
	for (iter = selfList.begin(); iter != selfList.end(); ++iter){
		iter->second->regenerate();
	}
}

void ImpostorTexture::renderTextures(bool force)
{
	Ogre::ManualResourceLoader* loader = 0;
#ifdef IMPOSTOR_FILE_SAVE
	TexturePtr renderTexture;
#else
	TexturePtr renderTexture(texture);
	//if we're not using a file image we need to set up a resource loader, so that the texture is regenerated if it's ever unloaded (such as switching between fullscreen and the desktop in win32)
	loader = this;
#endif
	RenderTexture *renderTarget;
	Camera *renderCamera;
	Viewport *renderViewport;
	SceneNode *camNode;

	//Set up RTT texture
   Ogre::uint textureSize = ImpostorPage::getImpostorResolution();
	if (!renderTexture)
   {
	renderTexture = TextureManager::getSingleton().createManual(getUniqueID("ImpostorTexture"), "Impostors",
				TEX_TYPE_2D, textureSize * IMPOSTOR_YAW_ANGLES, textureSize * IMPOSTOR_PITCH_ANGLES, 0, PF_BYTE_RGBA, TU_RENDERTARGET, loader);
	}
	renderTexture->setNumMipmaps(MIP_UNLIMITED);
	
	//Set up render target
	renderTarget = renderTexture->getBuffer()->getRenderTarget(); 
	renderTarget->setAutoUpdated(false);
	
	//Set up camera
	camNode = sceneMgr->getSceneNode("ImpostorPage::cameraNode");
	renderCamera = sceneMgr->createCamera(getUniqueID("ImpostorCam"));
	camNode->attachObject(renderCamera);
	renderCamera->setLodBias(1000.0f);
	renderViewport = renderTarget->addViewport(renderCamera);
	renderViewport->setOverlaysEnabled(false);
	renderViewport->setClearEveryFrame(true);
	renderViewport->setShadowsEnabled(false);
	renderViewport->setBackgroundColour(ImpostorPage::getImpostorBackgroundColor());
	
	//Set up scene node
	SceneNode* node = sceneMgr->getSceneNode("ImpostorPage::renderNode");
	
	Ogre::SceneNode* oldSceneNode = entity->getParentSceneNode();
	if (oldSceneNode) {
		oldSceneNode->detachObject(entity);
	}

	node->attachObject(entity);
	node->setPosition(-entityCenter);

	//Set up camera FOV
	const Real objDist = entityRadius * 100;
	const Real nearDist = objDist - (entityRadius + 1); 
	const Real farDist = objDist + (entityRadius + 1);
	
	renderCamera->setAspectRatio(1.0f);
	renderCamera->setFOVy(Math::ATan(entityDiameter / objDist));
	renderCamera->setNearClipDistance(nearDist);
	renderCamera->setFarClipDistance(farDist);
	
	//Disable mipmapping (without this, masked textures look bad)
	MaterialManager *mm = MaterialManager::getSingletonPtr();
	FilterOptions oldMinFilter = mm->getDefaultTextureFiltering(FT_MIN);
	FilterOptions oldMagFilter = mm->getDefaultTextureFiltering(FT_MAG);
	FilterOptions oldMipFilter = mm->getDefaultTextureFiltering(FT_MIP);
	mm->setDefaultTextureFiltering(FO_POINT, FO_LINEAR, FO_NONE);

	//Disable fog
	FogMode oldFogMode = sceneMgr->getFogMode();
	ColourValue oldFogColor = sceneMgr->getFogColour();
	Real oldFogDensity = sceneMgr->getFogDensity();
	Real oldFogStart = sceneMgr->getFogStart();
	Real oldFogEnd = sceneMgr->getFogEnd();
	sceneMgr->setFog(FOG_NONE);
	
	// Get current status of the queue mode
	Ogre::SceneManager::SpecialCaseRenderQueueMode OldSpecialCaseRenderQueueMode = sceneMgr->getSpecialCaseRenderQueueMode();
	//Only render the entity
	sceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_INCLUDE); 
   sceneMgr->addSpecialCaseRenderQueue(group->getParentPagedGeometry()->getRenderQueue() + 1);

	uint8 oldRenderQueueGroup = entity->getRenderQueueGroup();
	entity->setRenderQueueGroup(group->getParentPagedGeometry()->getRenderQueue() + 1);
	bool oldVisible = entity->getVisible();
	entity->setVisible(true);
    Ogre::Real oldMaxDistance = entity->getRenderingDistance();
	entity->setRenderingDistance(0);

	bool needsRegen = true;
#ifdef IMPOSTOR_FILE_SAVE
	//Calculate the filename hash used to uniquely identity this render
	uint32 key = FastHash(entityKey.c_str(), entityKey.size(), 0);
	String tempdir = this->group->getParentPagedGeometry()->getTempdir();
	ResourceGroupManager::getSingleton().addResourceLocation(tempdir, "FileSystem", "Impostors");

	String fileName = StringUtil::format("Impostor.%08X.%d", key, textureSize);

	//Attempt to load the pre-render file if allowed
	needsRegen = force;
	if (!needsRegen){
		try{
			texture = TextureManager::getSingleton().load(fileName+".dds", "Impostors", TEX_TYPE_2D, MIP_UNLIMITED);
		}
		catch (Exception&){
			try{
				texture = TextureManager::getSingleton().load(fileName+".png", "Impostors", TEX_TYPE_2D, MIP_UNLIMITED);
			}
			catch (Exception&){
				needsRegen = true;
			}
		}
	}
#endif

	if (needsRegen){
		//If this has not been pre-rendered, do so now
		const float xDivFactor = 1.0f / IMPOSTOR_YAW_ANGLES;
		const float yDivFactor = 1.0f / IMPOSTOR_PITCH_ANGLES;
		for (int o = 0; o < IMPOSTOR_PITCH_ANGLES; ++o){ //4 pitch angle renders
#ifdef IMPOSTOR_RENDER_ABOVE_ONLY
			Radian pitch = Degree((90.0f * o) * yDivFactor); //0, 22.5, 45, 67.5
#else
			Radian pitch = Degree((180.0f * o) * yDivFactor - 90.0f);
#endif

			for (int i = 0; i < IMPOSTOR_YAW_ANGLES; ++i){ //8 yaw angle renders
				Radian yaw = Degree((360.0f * i) * xDivFactor); //0, 45, 90, 135, 180, 225, 270, 315
					
				//Position camera
				camNode->setPosition(0, 0, 0);
                camNode->setOrientation(Quaternion(yaw, Vector3::UNIT_Y) * Quaternion(-pitch, Vector3::UNIT_X));
                camNode->translate(Vector3(0, 0, objDist), Node::TS_LOCAL);
						
				//Render the impostor
				renderViewport->setDimensions((float)(i) * xDivFactor, (float)(o) * yDivFactor, xDivFactor, yDivFactor);
				renderTarget->update();
			}
		}
	
#ifdef IMPOSTOR_FILE_SAVE
		//Save RTT to file with respecting the temp dir
		renderTarget->writeContentsToFile(tempdir + fileName+".png");
        // force re-index
        ResourceGroupManager::getSingleton().removeResourceLocation(tempdir, "Impostors");
        ResourceGroupManager::getSingleton().addResourceLocation(tempdir, "FileSystem", "Impostors");

		//Load the render into the appropriate texture view
		texture = TextureManager::getSingleton().load(fileName+".png", "Impostors", TEX_TYPE_2D, MIP_UNLIMITED);
#else
		texture = renderTexture;
#endif
	}
	

	entity->setVisible(oldVisible);
	entity->setRenderQueueGroup(oldRenderQueueGroup);
	entity->setRenderingDistance(oldMaxDistance);

	sceneMgr->removeSpecialCaseRenderQueue(group->getParentPagedGeometry()->getRenderQueue() + 1);
	// Restore original state
	sceneMgr->setSpecialCaseRenderQueueMode(OldSpecialCaseRenderQueueMode); 

	//Re-enable mipmapping
	mm->setDefaultTextureFiltering(oldMinFilter, oldMagFilter, oldMipFilter);

	//Re-enable fog
	sceneMgr->setFog(oldFogMode, oldFogColor, oldFogDensity, oldFogStart, oldFogEnd);

	//Delete camera
	renderTarget->removeViewport(0);
	renderCamera->getSceneManager()->destroyCamera(renderCamera);
	
	//Delete scene node
	node->detachAllObjects();
	if (oldSceneNode) {
		oldSceneNode->attachObject(entity);
	}

#ifdef IMPOSTOR_FILE_SAVE
	//Delete RTT texture
	assert(renderTexture);

	if (TextureManager::getSingletonPtr())
		TextureManager::getSingleton().remove(renderTexture);
#endif
}

void ImpostorTexture::removeTexture(ImpostorTexture* Texture)
{
	//Search for an existing impostor texture, in case it was already deleted
	for(std::map<String, ImpostorTexture *>::iterator iter=selfList.begin();
		iter!=selfList.end(); ++iter)
	{
		if(iter->second==Texture)
		{
			delete Texture;
			return;
		}
	}
	// no need to anything if it was not found, chances are that it was already deleted
}

ImpostorTexture *ImpostorTexture::getTexture(ImpostorPage *group, Entity *entity)
{
	//Search for an existing impostor texture for the given entity
	String entityKey = ImpostorBatch::generateEntityKey(entity);
	std::map<String, ImpostorTexture *>::iterator iter;
	iter = selfList.find(entityKey);
	
	//If found..
	if (iter != selfList.end()){
		//Return it
		return iter->second;		
	} else {
		if (group){
			//Otherwise, return a new texture
			return (new ImpostorTexture(group, entity));
		} else {
			//But if group is null, return null
			return NULL;
		}
	}
}
