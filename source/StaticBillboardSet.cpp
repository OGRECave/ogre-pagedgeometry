/*-------------------------------------------------------------------------------------
Copyright (c) 2006 John Judnich

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------------*/

//StaticBillboardSet.h
//Provides a method of displaying billboards faster than Ogre's built-in BillboardSet
//functions by taking advantage of the static nature of billboards (note: StaticBillboardSet
//does not allow billboards to be moved or deleted individually in real-time)
//-------------------------------------------------------------------------------------

#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreSceneNode.h>
#include <OgreString.h>
#include <OgreStringConverter.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareBuffer.h>
#include <OgreLogManager.h>
#include <OgreEntity.h>
#include <OgreTechnique.h>

#include "StaticBillboardSet.h"

using namespace Ogre;
using namespace Forests;


// Static data initialization
StaticBillboardSet::FadedMaterialMap StaticBillboardSet::s_mapFadedMaterial;
unsigned int StaticBillboardSet::s_nSelfInstances  = 0;
unsigned long StaticBillboardSet::GUID             = 0;


//-----------------------------------------------------------------------------
/// Constructor
StaticBillboardSet::StaticBillboardSet(SceneManager *mgr, SceneNode *rootSceneNode, BillboardMethod method) :
mVisible                (true),
mFadeEnabled            (false),
mpSceneMgr              (mgr),
mpSceneNode             (NULL),
mpEntity                (NULL),
mfUFactor               (1.f),
mfVFactor               (1.f),
mBBOrigin               (BBO_CENTER),
mFadeVisibleDist        (0.f),
mFadeInvisibleDist      (0.f)
{
   assert(rootSceneNode);

   mpSceneNode = rootSceneNode->createChildSceneNode();
   mEntityName = getUniqueID("SBSentity");

   ++s_nSelfInstances;
}


//-----------------------------------------------------------------------------
/// Destructor
StaticBillboardSet::~StaticBillboardSet()
{
   clear(); // Delete mesh data

   //Delete fade materials if no longer in use
   if (--s_nSelfInstances == 0)
   {
      for (FadedMaterialMap::iterator it = s_mapFadedMaterial.begin(); it != s_mapFadedMaterial.end(); ++it)
         MaterialManager::getSingleton().remove(it->second);
      s_mapFadedMaterial.clear();
   }

   //Delete scene node
   if (mpSceneNode->getParent())
      mpSceneNode->getParentSceneNode()->removeAndDestroyChild(mpSceneNode);
   else
      mpSceneNode->getCreator()->destroySceneNode(mpSceneNode);
}


//-----------------------------------------------------------------------------
///
void StaticBillboardSet::destroyEntity()
{
   if (mpEntity)
   {
      //Delete entity
      mpSceneNode->detachAllObjects();
      mpEntity->_getManager()->destroyEntity(mpEntity);
      mpEntity = NULL;

      //Delete mesh
      MeshManager::getSingleton().remove(mPtrMesh);
      mPtrMesh.reset();
   }
}


//-----------------------------------------------------------------------------
///
void StaticBillboardSet::clear()
{
   destroyEntity();

   //Remove any billboard data which might be left over if the user forgot to call build()
   mBillboardBuffer.clear();
}


//-----------------------------------------------------------------------------
/// Performs final steps required for the created billboards to appear in the scene.
void StaticBillboardSet::build()
{
   //Delete old entity and mesh data
   destroyEntity();

   //If there are no billboards to create, exit
   if (mBillboardBuffer.empty())
      return;

   //Create manual mesh to store billboard quads
   mPtrMesh = MeshManager::getSingleton().createManual(getUniqueID("SBSmesh"), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
   Ogre::SubMesh *pSubMesh = mPtrMesh->createSubMesh();
   pSubMesh->useSharedVertices = false;

   //Setup vertex format information
   pSubMesh->vertexData = new VertexData;
   pSubMesh->vertexData->vertexStart = 0;
   pSubMesh->vertexData->vertexCount = 4 * mBillboardBuffer.size();

   // position: xyz = pivot, w = height of the billboard center above the pivot
   // uv0: xy = offset of the corner along the camera aligned right/up axes, zw = texcoords
   VertexDeclaration* dcl = pSubMesh->vertexData->vertexDeclaration;
   size_t offset = 0;
   offset += dcl->addElement(0, offset, VET_FLOAT4, VES_POSITION).getSize();
   offset += dcl->addElement(0, offset, VET_FLOAT4, VES_TEXTURE_COORDINATES).getSize();
   offset += dcl->addElement(0, offset, VET_UBYTE4_NORM, VES_DIFFUSE).getSize();

   //Populate a new vertex buffer
   HardwareVertexBufferSharedPtr vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
      offset, pSubMesh->vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
   float* pReal = static_cast<float*>(vbuf->lock(HardwareBuffer::HBL_DISCARD));

   // the corners in the order top left, top right, bottom left, bottom right
   const float cornerX[4] = { -1.f, 1.f, -1.f, 1.f };
   const float cornerU[4] = { 0.f, 1.f, 0.f, 1.f };
   const float cornerV[4] = { 0.f, 0.f, 1.f, 1.f };
   // BBO_CENTER: the quad spans [-1, 1] along up, BBO_BOTTOM_CENTER: [0, 2]
   const float bottom = mBBOrigin == BBO_BOTTOM_CENTER ? 0.f : -1.f;
   const float cornerY[4] = { bottom + 2.f, bottom + 2.f, bottom, bottom };

   AxisAlignedBox bounds;

   for (TBillBuf::const_iterator bb = mBillboardBuffer.begin(), bbend = mBillboardBuffer.end(); bb != bbend; ++bb)
   {
      const float centerHeight = (bottom + 1.f) * bb->yScaleHalf;
      for (int c = 0; c < 4; ++c)
      {
         *pReal++ = bb->xPos;
         *pReal++ = bb->yPos;
         *pReal++ = bb->zPos;
         *pReal++ = centerHeight;

         *pReal++ = cornerX[c] * bb->xScaleHalf;
         *pReal++ = cornerY[c] * bb->yScaleHalf;
         *pReal++ = (bb->texcoordIndexU + cornerU[c]) * mfUFactor;
         *pReal++ = (bb->texcoordIndexV + cornerV[c]) * mfVFactor;

         *(reinterpret_cast<uint32*>(pReal++)) = bb->color;
      }

      //Update bounding box (the billboard can rotate in any direction around the pivot)
      const float extent = std::max(bb->xScaleHalf, (bottom + 2.f) * bb->yScaleHalf);
      const Vector3 pos(bb->xPos, bb->yPos, bb->zPos);
      bounds.merge(pos - extent);
      bounds.merge(pos + extent);
   }

   vbuf->unlock();
   pSubMesh->vertexData->vertexBufferBinding->setBinding(0, vbuf);

   // Populate index buffer
   const size_t billboardCount = mBillboardBuffer.size();
   pSubMesh->indexData->indexStart = 0;
   pSubMesh->indexData->indexCount = 6 * billboardCount;
   const bool use32BitIndices = pSubMesh->vertexData->vertexCount > 0xFFFF;
   HardwareIndexBufferSharedPtr &ptrIndBuf = pSubMesh->indexData->indexBuffer;
   ptrIndBuf = HardwareBufferManager::getSingleton().createIndexBuffer(
      use32BitIndices ? HardwareIndexBuffer::IT_32BIT : HardwareIndexBuffer::IT_16BIT,
      pSubMesh->indexData->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);

   void *pIBuf = ptrIndBuf->lock(HardwareBuffer::HBL_DISCARD);
   uint16 *pIBuf16 = static_cast<uint16*>(pIBuf);
   uint32 *pIBuf32 = static_cast<uint32*>(pIBuf);
   const uint32 quadIndices[6] = { 0, 2, 1, 1, 2, 3 };
   for (uint32 i = 0; i < billboardCount; ++i)
   {
      for (int j = 0; j < 6; ++j)
      {
         if (use32BitIndices)
            *pIBuf32++ = i * 4 + quadIndices[j];
         else
            *pIBuf16++ = uint16(i * 4 + quadIndices[j]);
      }
   }
   ptrIndBuf->unlock();

   mBillboardBuffer.clear(); //Empty the mBillboardBuffer now, because all billboards have been built

   // Finish up mesh
   mPtrMesh->_setBounds(bounds);
   mPtrMesh->_setBoundingSphereRadius(bounds.getHalfSize().length());

   // Loading mesh
   LogManager::getSingleton().setLogDetail(LL_LOW);
   mPtrMesh->load();
   LogManager::getSingleton().setLogDetail(LL_NORMAL);

   // Create an entity for the mesh
   mpEntity = mpSceneMgr->createEntity(mEntityName, mPtrMesh->getName(), mPtrMesh->getGroup());
   mpEntity->setCastShadows(false);
   mpEntity->setMaterial(mFadeEnabled ? mPtrFadeMaterial : mPtrMaterial);

   // Add to scene
   mpSceneNode->attachObject(mpEntity);
   mpEntity->setVisible(mVisible);
}


//-----------------------------------------------------------------------------
///
void StaticBillboardSet::setMaterial(const String &materialName, const Ogre::String &resourceGroup)
{
   bool needUpdateMat = !mPtrMaterial || mPtrMaterial->getName() != materialName || mPtrMaterial->getGroup() != resourceGroup;
   if (!needUpdateMat)
      return;

   mPtrMaterial = MaterialManager::getSingleton().getByName(materialName, resourceGroup);
   prepareMaterial(mPtrMaterial);

   if (mFadeEnabled)
      mPtrFadeMaterial = getFadeMaterial(mPtrMaterial, mFadeVisibleDist, mFadeInvisibleDist);

   //Apply material to entity
   if (mpEntity)
      mpEntity->setMaterial(mFadeEnabled ? mPtrFadeMaterial : mPtrMaterial);
}


//-----------------------------------------------------------------------------
///
void StaticBillboardSet::setFade(bool enabled, Real visibleDist, Real invisibleDist)
{
   if (enabled)
   {
      if (!mPtrMaterial)
         OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Billboard fading cannot be enabled without a material applied first", "StaticBillboardSet::setFade()");

      mPtrFadeMaterial = getFadeMaterial(mPtrMaterial, visibleDist, invisibleDist);

      //Apply material to entity
      if (mpEntity)
         mpEntity->setMaterial(mPtrFadeMaterial);
   }
   else if (mFadeEnabled)
   {
      //Apply material to entity
      if (mpEntity)
         mpEntity->setMaterial(mPtrMaterial);
   }

   mFadeEnabled = enabled;
   mFadeVisibleDist = visibleDist;
   mFadeInvisibleDist = invisibleDist;
}


//-----------------------------------------------------------------------------
///
void StaticBillboardSet::setTextureStacksAndSlices(Ogre::uint16 stacks, Ogre::uint16 slices)
{
   assert(stacks != 0 && slices != 0 && "division by zero");
   mfUFactor = 1.0f / slices;
   mfVFactor = 1.0f / stacks;
}


//-----------------------------------------------------------------------------
///
void StaticBillboardSet::prepareMaterial(const Ogre::MaterialPtr &material)
{
   const char* fragmentProgramName = mpSceneMgr->getFogMode() == FOG_NONE ? "Default_AlphaTest_NoFog" : "Default_AlphaTest";

   for (unsigned short t = 0; t < material->getNumTechniques(); ++t)
   {
      Technique *tech = material->getTechnique(t);
      for (unsigned short p = 0; p < tech->getNumPasses(); ++p)
      {
         Pass *pass = tech->getPass(p);
         if (pass->hasVertexProgram())
            continue;

         pass->setVertexProgram("Sprite_vp");
         pass->setFragmentProgram(fragmentProgramName);

         //The texture scroll is not applied to custom shaders by Ogre
         float uvScroll[2] = { 0.f, 0.f };
         if (pass->getNumTextureUnitStates() > 0)
         {
            uvScroll[0] = (float)pass->getTextureUnitState(0)->getTextureUScroll();
            uvScroll[1] = (float)pass->getTextureUnitState(0)->getTextureVScroll();
         }
         pass->getVertexProgramParameters()->setNamedConstant("uvScroll", uvScroll, 1, 2);
      }
   }
}


//-----------------------------------------------------------------------------
///
MaterialPtr StaticBillboardSet::getFadeMaterial(const Ogre::MaterialPtr &protoMaterial,
                                                Real visibleDist_, Real invisibleDist_)
{
   assert(protoMaterial);

   // Note: the signature must not contain anything specific to this set, so all sets can share the material
   Ogre::StringStream materialSignature;
   materialSignature << protoMaterial->getName() << "|";
   materialSignature << visibleDist_ << "|";
   materialSignature << invisibleDist_ << "|";

   FadedMaterialMap::iterator it = s_mapFadedMaterial.find(materialSignature.str());
   if (it != s_mapFadedMaterial.end())
      return it->second; //Use the existing fade material

   MaterialPtr fadeMaterial = protoMaterial->clone(getUniqueID("ImpostorFade"));

   //And apply the fade shader
   for (unsigned short t = 0; t < fadeMaterial->getNumTechniques(); ++t)
   {
      Technique *tech = fadeMaterial->getTechnique(t);
      for (unsigned short p = 0; p < tech->getNumPasses(); ++p)
      {
         Pass *pass = tech->getPass(p);

         //Use the fading variant of the billboard shader (e.g. Sprite_vp -> SpriteFade_vp)
         const String vertexProgram = pass->getVertexProgramName();
         if (!StringUtil::endsWith(vertexProgram, "_vp", false))
            continue;

         GpuProgramParametersSharedPtr protoParams = pass->getVertexProgramParameters();
         pass->setVertexProgram(vertexProgram.substr(0, vertexProgram.size() - 3) + "Fade_vp");
         pass->setFragmentProgram(mpSceneMgr->getFogMode() == FOG_NONE ? "Default_AlphaTest_NoFog" : "Default_AlphaTest");

         GpuProgramParametersSharedPtr params = pass->getVertexProgramParameters();
         params->copyMatchingNamedConstantsFrom(*protoParams);

         //Set fade ranges
         params->setNamedConstant("invisibleDist", invisibleDist_);
         params->setNamedConstant("fadeGap", invisibleDist_ - visibleDist_);

         //Fading is dithered, so no blending is needed. Complement the pattern of the detail level fading out
         pass->getFragmentProgramParameters()->setNamedConstant("ditherInvert", invisibleDist_ < visibleDist_ ? 1.f : 0.f);

      }  // for Pass

   }  // for Technique

   //Add it to the list so it can be reused later
   s_mapFadedMaterial.insert(std::pair<String, MaterialPtr>(materialSignature.str(), fadeMaterial));

   return fadeMaterial;
}


//-----------------------------------------------------------------------------
///
void StaticBillboardSet::setBillboardOrigin(BillboardOrigin origin)
{
   assert((origin == BBO_CENTER || origin == BBO_BOTTOM_CENTER) && "Invalid origin - only BBO_CENTER and BBO_BOTTOM_CENTER is supported");
   mBBOrigin = origin;
}
