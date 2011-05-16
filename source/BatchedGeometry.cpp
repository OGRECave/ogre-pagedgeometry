/*-------------------------------------------------------------------------------------
Copyright (c) 2006 John Judnich

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------------*/

//BatchedGeometry.h
//A "lightweight" version of Ogre::StaticGeometry, which gives you a little more control
//over the batch materials, etc.
//-------------------------------------------------------------------------------------

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreCamera.h>
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreSceneNode.h>
#include <OgreString.h>
#include <OgreStringConverter.h>
#include <OgreEntity.h>
#include <OgreSubMesh.h>
#include <OgreSubEntity.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwareBuffer.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <string>

#include "BatchedGeometry.h"
#include "PagedGeometry.h"

using namespace Ogre;
using namespace Forests;



//-------------------------------------------------------------------------------------
///
Forests::BatchedGeometry::BatchedGeometry(Ogre::SceneManager *mgr, Ogre::SceneNode *rootSceneNode) :
mfRadius             (0.f),
mfMinDistanceSquared (0.f),
mpSceneMgr           (mgr),
mpSceneNode          (NULL),
mpParentSceneNode    (rootSceneNode),
mbWithinFarDistance  (false),
mBuilt               (false),
mBoundsUndefined     (true)
{
   assert(rootSceneNode);
   //clear();  // <-- SVA stupid call
}

//-----------------------------------------------------------------------------
///
BatchedGeometry::~BatchedGeometry()
{
   clear();
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::addEntity(Entity *ent, const Vector3 &position,
                                const Quaternion &orientation,
                                const Vector3 &scale,
                                const Ogre::ColourValue &color)
{
   const MeshPtr &mesh = ent->getMesh();

   //If shared vertex data is used, extract into non-shared data
   extractVertexDataFromShared(mesh);	

   //For each subentity
   for (uint32 i = 0; i < ent->getNumSubEntities(); ++i)
   {
      //Get the subentity
      SubEntity *subEntity = ent->getSubEntity(i);
      SubMesh *subMesh = subEntity->getSubMesh();

      //Generate a format string that uniquely identifies this material & vertex/index format
      if (subMesh->vertexData == NULL)
         OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "SubMesh vertex data not found!", "BatchedGeometry::addEntity()");
      String formatStr = getFormatString(subEntity);

      //If a batch using an identical format exists...
      SubBatch *batch;
      SubBatchMap::iterator batchIter = subBatchMap.find(formatStr);
      if (batchIter != subBatchMap.end()){
         //Use the batch
         batch = batchIter->second;
      } else {
         //Otherwise create a new batch
         batch = new SubBatch(this, subEntity);
         subBatchMap.insert(std::pair<String, SubBatch*>(formatStr, batch));
      }

      //Now add the submesh to the compatible batch
      batch->addSubEntity(subEntity, position, orientation, scale, color);
   }

   //Update bounding box
   Matrix4 mat(orientation);
   mat.setScale(scale);
   AxisAlignedBox entBounds = ent->getBoundingBox();
   entBounds.transform(mat);

   if (mBoundsUndefined)
   {
      bounds.setMinimum(entBounds.getMinimum() + position);
      bounds.setMaximum(entBounds.getMaximum() + position);
      mBoundsUndefined = false;
   }
   else
   {
      Vector3 min = bounds.getMinimum();
      Vector3 max = bounds.getMaximum();
      min.makeFloor(entBounds.getMinimum() + position);
      max.makeCeil(entBounds.getMaximum() + position);
      bounds.setMinimum(min);
      bounds.setMaximum(max);
   }
}


//-----------------------------------------------------------------------------
///
uint32 CountUsedVertices(IndexData *id, std::map<uint32, uint32> &ibmap)
{
   uint32 i, count;
   switch (id->indexBuffer->getType()) {
      case HardwareIndexBuffer::IT_16BIT:
         {
            uint16 *data = (uint16*)id->indexBuffer->lock(id->indexStart * sizeof(uint16), 
               id->indexCount * sizeof(uint16), HardwareBuffer::HBL_READ_ONLY);

            for (i = 0; i < id->indexCount; i++) {
               uint16 index = data[i];
               if (ibmap.find(index) == ibmap.end()) ibmap[index] = (uint32)(ibmap.size());
            }
            count = (uint32)ibmap.size();
            id->indexBuffer->unlock();
         }
         break;

      case HardwareIndexBuffer::IT_32BIT:
         {
            uint32 *data = (uint32*)id->indexBuffer->lock(id->indexStart * sizeof(uint32), 
               id->indexCount * sizeof(uint32), HardwareBuffer::HBL_READ_ONLY);

            for (i = 0; i < id->indexCount; i++) {
               uint32 index = data[i];
               if (ibmap.find(index) == ibmap.end()) ibmap[index] = (uint32)(ibmap.size());
            }
            count = (uint32)ibmap.size();
            id->indexBuffer->unlock();
         }
         break;

      default:
         throw new Ogre::Exception(0, "Unknown index buffer type", "Converter.cpp::CountVertices");
         break;
   }

   return count;
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::extractVertexDataFromShared(const Ogre::MeshPtr &mesh)
{
   if (mesh.isNull() || !mesh->sharedVertexData)
      return;

   Mesh::SubMeshIterator subMeshIterator = mesh->getSubMeshIterator();

   // Get shared vertex data
   VertexData *oldVertexData = mesh->sharedVertexData;

   while (subMeshIterator.hasMoreElements()) {
      SubMesh *subMesh = subMeshIterator.getNext();

      // Get index data
      IndexData *indexData = subMesh->indexData;
      HardwareIndexBufferSharedPtr ib = indexData->indexBuffer;

      // Create new nonshared vertex data
      std::map<uint32, uint32> indicesMap;
      VertexData *newVertexData = new VertexData();
      newVertexData->vertexCount = CountUsedVertices(indexData, indicesMap);
      //delete newVertexData->vertexDeclaration;
      newVertexData->vertexDeclaration = oldVertexData->vertexDeclaration->clone();

      // Create new vertex buffers
      uint32 buffersCount = (uint32)oldVertexData->vertexBufferBinding->getBufferCount();
      for (uint32 bufferIndex = 0; bufferIndex < buffersCount; bufferIndex++) {

         // Lock shared vertex buffer
         HardwareVertexBufferSharedPtr oldVertexBuffer = oldVertexData->vertexBufferBinding->getBuffer(bufferIndex);
         size_t vertexSize = oldVertexBuffer->getVertexSize();
         uint8 *oldLock = (uint8*)oldVertexBuffer->lock(0, oldVertexData->vertexCount * vertexSize, HardwareBuffer::HBL_READ_ONLY);

         // Create and lock nonshared vertex buffer
         HardwareVertexBufferSharedPtr newVertexBuffer = HardwareBufferManager::getSingleton().createVertexBuffer(
            vertexSize, newVertexData->vertexCount, oldVertexBuffer->getUsage(), oldVertexBuffer->hasShadowBuffer());
         uint8 *newLock = (uint8*)newVertexBuffer->lock(0, newVertexData->vertexCount * vertexSize, HardwareBuffer::HBL_NORMAL);

         // Copy vertices from shared vertex buffer into nonshared vertex buffer
         std::map<uint32, uint32>::iterator i, iend = indicesMap.end();
         for (i = indicesMap.begin(); i != iend; i++) {
            memcpy(newLock + vertexSize * i->second, oldLock + vertexSize * i->first, vertexSize);
         }

         // Unlock vertex buffers
         oldVertexBuffer->unlock();
         newVertexBuffer->unlock();

         // Bind new vertex buffer
         newVertexData->vertexBufferBinding->setBinding(bufferIndex, newVertexBuffer);
      }

      // Re-create index buffer
      switch (indexData->indexBuffer->getType()) {
         case HardwareIndexBuffer::IT_16BIT:
            {
               uint16 *data = (uint16*)indexData->indexBuffer->lock(indexData->indexStart * sizeof(uint16), 
                  indexData->indexCount * sizeof(uint16), HardwareBuffer::HBL_NORMAL);

               for (uint32 i = 0; i < indexData->indexCount; i++) {
                  data[i] = (uint16)indicesMap[data[i]];
               }

               indexData->indexBuffer->unlock();
            }
            break;

         case HardwareIndexBuffer::IT_32BIT:
            {
               uint32 *data = (uint32*)indexData->indexBuffer->lock(indexData->indexStart * sizeof(uint32), 
                  indexData->indexCount * sizeof(uint32), HardwareBuffer::HBL_NORMAL);

               for (uint32 i = 0; i < indexData->indexCount; i++) {
                  data[i] = (uint32)indicesMap[data[i]];
               }

               indexData->indexBuffer->unlock();
            }
            break;

         default:
            throw new Ogre::Exception(0, "Unknown index buffer type", "Converter.cpp::CountVertices");
            break;
      }

      // Store new attributes
      subMesh->useSharedVertices = false;
      subMesh->vertexData = newVertexData;
   }

   // Release shared vertex data
   delete mesh->sharedVertexData;
   mesh->sharedVertexData = NULL;
}


//-----------------------------------------------------------------------------
///
BatchedGeometry::SubBatchIterator BatchedGeometry::getSubBatchIterator() const
{
   return BatchedGeometry::SubBatchIterator((SubBatchMap&)subBatchMap);
}


//-----------------------------------------------------------------------------
///
String BatchedGeometry::getFormatString(SubEntity *ent)
{
   static char buf[1024];
   // add materialname and buffer type
   int countWritten =  sprintf(buf, "%s|%d", ent->getMaterialName().c_str(), ent->getSubMesh()->indexData->indexBuffer->getType());

   // now add vertex decl
   const VertexDeclaration::VertexElementList &elemList = ent->getSubMesh()->vertexData->vertexDeclaration->getElements();
   for (VertexDeclaration::VertexElementList::const_iterator i = elemList.begin(), iend = elemList.end(); i != iend; ++i)
   {
      const VertexElement &el = *i;
      countWritten += sprintf(buf + countWritten, "|%d|%d|%d", el.getSource(), el.getSemantic(), el.getType());
   }

   return buf;
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::build()
{
   ///Make sure the batch hasn't already been built
   if (mBuilt)
      OGRE_EXCEPT(Exception::ERR_DUPLICATE_ITEM, "Invalid call to build() - geometry is already batched (call clear() first)", "BatchedGeometry::GeomBatch::build()");

   if (!subBatchMap.empty())
   {
      //Finish bounds information
      center = bounds.getCenter();                       //Calculate bounds center
      bounds.setMinimum(bounds.getMinimum() - center);	//Center the bounding box
      bounds.setMaximum(bounds.getMaximum() - center);	//Center the bounding box
      mfRadius = bounds.getMaximum().length();           //Calculate BB radius

      //Create scene node
      mpSceneNode = mpParentSceneNode->createChildSceneNode(center);

      //Build each batch
      SubBatchMap::iterator itEnd = subBatchMap.end();
      for (SubBatchMap::iterator i = subBatchMap.begin(); i != itEnd; ++i)
         i->second->build();

      //Attach the batch to the scene node
      mpSceneNode->attachObject(this);

      //Debug
      //sceneNode->showBoundingBox(true);

      mBuilt = true;
   }

}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::clear()
{
   //Remove the batch from the scene
   if (mpSceneNode)
   {
      mpSceneNode->removeAllChildren();
      if (mpSceneNode->getParent())
         mpSceneNode->getParentSceneNode()->removeAndDestroyChild(mpSceneNode->getName());
      else
         mpSceneMgr->destroySceneNode(mpSceneNode);

      mpSceneNode = 0;
   }

   //Reset bounds information
   mBoundsUndefined = true;
   center = Vector3::ZERO;
   mfRadius = 0;

   //Delete each batch
   for (SubBatchMap::iterator i = subBatchMap.begin(), iend = subBatchMap.end(); i != iend; ++i)
      delete i->second;

   subBatchMap.clear();
   mBuilt = false;
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::_updateRenderQueue(RenderQueue *queue)
{
   assert(isVisible() && "Ogre core code must detect that this MovableObject invisible");

   // SVA speed up adding
   Ogre::RenderQueueGroup *rqg = queue->getQueueGroup(getRenderQueueGroup());
   SubBatchMap::const_iterator i = subBatchMap.begin(), iend = subBatchMap.end();
   while (i != iend)
   {
      i->second->addSelfToRenderQueue(rqg);
      ++i;
   }

   ////If visible...
   //if (isVisible()){
   //   //Ask each batch to add itself to the render queue if appropriate
   //   for (SubBatchMap::iterator i = subBatchMap.begin(); i != subBatchMap.end(); ++i){
   //      i->second->addSelfToRenderQueue(queue, getRenderQueueGroup());
   //   }
   //}
}


//-----------------------------------------------------------------------------
///
bool BatchedGeometry::isVisible()
{
   return mVisible && mbWithinFarDistance;
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::_notifyCurrentCamera(Camera *cam)
{
   if (getRenderingDistance() == Ogre::Real(0.))
      mbWithinFarDistance = true;
   else
   {
      //Calculate camera distance
      Vector3 camVec = _convertToLocal(cam->getDerivedPosition()) - center;
      Real centerDistanceSquared = camVec.squaredLength();
      mfMinDistanceSquared = std::max(Ogre::Real(0.), centerDistanceSquared - (mfRadius * mfRadius));
      //Note: centerDistanceSquared measures the distance between the camera and the center of the GeomBatch,
      //while minDistanceSquared measures the closest distance between the camera and the closest edge of the
      //geometry's bounding sphere.

      //Determine whether the BatchedGeometry is within the far rendering distance
      mbWithinFarDistance = mfMinDistanceSquared <= Math::Sqr(getRenderingDistance());
   }
}


//-----------------------------------------------------------------------------
///
Ogre::Vector3 BatchedGeometry::_convertToLocal(const Vector3 &globalVec) const
{
   //Convert from the given global position to the local coordinate system of the parent scene node.
   return (mpParentSceneNode->getOrientation().Inverse() * globalVec);
}



//=============================================================================
// BatchedGeometry::SubBatch implementation
//=============================================================================


//-----------------------------------------------------------------------------
///
BatchedGeometry::SubBatch::SubBatch(BatchedGeometry *parent, SubEntity *ent) :
m_pBestTechnique     (NULL),
mpVertexData         (0),
mpIndexData          (0),
mBuilt               (false),
mRequireVertexColors (false),
mpSubMesh            (0),
mpParentGeom         (parent)
{
   assert(ent);
   mpSubMesh = ent->getSubMesh();

   const Ogre::MaterialPtr &parentMaterial = ent->getMaterial();
   if (parentMaterial.isNull())
      OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "BatchedGeometry. Empty parent material", "BatchedGeometry::SubBatch::SubBatch");

   // SVA clone material
   // This function is used to make a single clone of materials used, since the materials
   // will be modified by the batch system (and it wouldn't be good to modify the original materials
   // that the user may be using somewhere else).
   {
      Ogre::String newName = parentMaterial->getName() + "_Batched";
      mPtrMaterial = MaterialManager::getSingleton().getByName(newName, parentMaterial->getGroup());
      if (mPtrMaterial.isNull())
         mPtrMaterial = parentMaterial->clone(newName);
   }

   //Setup vertex/index data structure
   mpVertexData = mpSubMesh->vertexData->clone(false);
   mpIndexData = mpSubMesh->indexData->clone(false);

   //Remove blend weights from vertex format
   const VertexElement* blendIndices = mpVertexData->vertexDeclaration->findElementBySemantic(VES_BLEND_INDICES);
   const VertexElement* blendWeights = mpVertexData->vertexDeclaration->findElementBySemantic(VES_BLEND_WEIGHTS);
   if (blendIndices && blendWeights)
   {
      //Check for format errors
      assert(blendIndices->getSource() == blendWeights->getSource()
         && "Blend indices and weights should be in the same buffer");
      assert(blendIndices->getSize() + blendWeights->getSize() == mpVertexData->vertexBufferBinding->getBuffer(blendIndices->getSource())->getVertexSize()
         && "Blend indices and blend buffers should have buffer to themselves!");

      //Remove the blend weights
      mpVertexData->vertexBufferBinding->unsetBinding(blendIndices->getSource());
      mpVertexData->vertexDeclaration->removeElement(VES_BLEND_INDICES);
      mpVertexData->vertexDeclaration->removeElement(VES_BLEND_WEIGHTS);
      mpVertexData->closeGapsInBindings();
   }

   //Reset vertex/index count
   mpVertexData->vertexStart = 0;
   mpVertexData->vertexCount = 0;
   mpIndexData->indexStart = 0;
   mpIndexData->indexCount = 0;
}


//-----------------------------------------------------------------------------
///
BatchedGeometry::SubBatch::~SubBatch()
{
   clear();

   delete mpVertexData;
   delete mpIndexData;
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::SubBatch::addSubEntity(SubEntity *ent, const Vector3 &position,
                                             const Quaternion &orientation, const Vector3 &scale,
                                             const Ogre::ColourValue &color, void* userData)
{
   assert(!mBuilt);

   //Add this submesh to the queue
   QueuedMesh newMesh;
   newMesh.mesh = ent->getSubMesh();
   newMesh.position = position;
   newMesh.orientation = orientation;
   newMesh.scale = scale;
   newMesh.userData = userData;

   newMesh.color = color;
   if (newMesh.color != ColourValue::White)
   {
      mRequireVertexColors = true;
      VertexElementType format = Root::getSingleton().getRenderSystem()->getColourVertexElementType();
      switch (format){
            case VET_COLOUR_ARGB:
               std::swap(newMesh.color.r, newMesh.color.b);
               break;
            case VET_COLOUR_ABGR:
               break;
            default:
               OGRE_EXCEPT(0, "Unknown RenderSystem color format", "BatchedGeometry::SubBatch::addSubMesh()");
               break;
      }
   }

   meshQueue.push_back(newMesh);

   //Increment the vertex/index count so the buffers will have room for this mesh
   mpVertexData->vertexCount += ent->getSubMesh()->vertexData->vertexCount;
   mpIndexData->indexCount += ent->getSubMesh()->indexData->indexCount;
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::SubBatch::build()
{
   assert(!mBuilt);

   HardwareIndexBuffer::IndexType srcIndexType = mpSubMesh->indexData->indexBuffer->getType();
   HardwareIndexBuffer::IndexType destIndexType;
   if (mpVertexData->vertexCount > 0xFFFF || srcIndexType == HardwareIndexBuffer::IT_32BIT)
      destIndexType = HardwareIndexBuffer::IT_32BIT;
   else
      destIndexType = HardwareIndexBuffer::IT_16BIT;

   //Allocate the index buffer
   mpIndexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
      destIndexType, mpIndexData->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

   //Lock the index buffer
   uint32 *indexBuffer32;
   uint16 *indexBuffer16;
   if (destIndexType == HardwareIndexBuffer::IT_32BIT)
      indexBuffer32 = static_cast<uint32*>(mpIndexData->indexBuffer->lock(HardwareBuffer::HBL_DISCARD));
   else
      indexBuffer16 = static_cast<uint16*>(mpIndexData->indexBuffer->lock(HardwareBuffer::HBL_DISCARD));

   //Allocate & lock the vertex buffers
   std::vector<uchar*> vertexBuffers;
   std::vector<VertexDeclaration::VertexElementList> vertexBufferElements;

   VertexBufferBinding *vertBinding = mpVertexData->vertexBufferBinding;
   VertexDeclaration *vertDecl = mpVertexData->vertexDeclaration;

   for (Ogre::ushort i = 0; i < vertBinding->getBufferCount(); ++i)
   {
      HardwareVertexBufferSharedPtr buffer = HardwareBufferManager::getSingleton().createVertexBuffer(
         vertDecl->getVertexSize(i), mpVertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
      vertBinding->setBinding(i, buffer);

      vertexBuffers.push_back(static_cast<uchar*>(buffer->lock(HardwareBuffer::HBL_DISCARD)));
      vertexBufferElements.push_back(vertDecl->findElementsBySource(i));
   }

   //If no vertex colors are used, make sure the final batch includes them (so the shade values work)
   if (mRequireVertexColors)
   {
      if (!mpVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE))
      {
         Ogre::ushort i = (Ogre::ushort)vertBinding->getBufferCount();
         vertDecl->addElement(i, 0, VET_COLOUR, VES_DIFFUSE);

         HardwareVertexBufferSharedPtr buffer = HardwareBufferManager::getSingleton().createVertexBuffer(
            vertDecl->getVertexSize(i), mpVertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
         vertBinding->setBinding(i, buffer);

         vertexBuffers.push_back(static_cast<uchar*>(buffer->lock(HardwareBuffer::HBL_DISCARD)));
         vertexBufferElements.push_back(vertDecl->findElementsBySource(i));
      }

      Pass *p = mPtrMaterial->getTechnique(0)->getPass(0);
      p->setVertexColourTracking(TVC_AMBIENT);
   }

   //For each queued mesh...
   size_t indexOffset = 0;
   for (MeshQueueIterator it = meshQueue.begin(), itEnd = meshQueue.end(); it != itEnd; ++it)
   {
      const QueuedMesh &queuedMesh = (*it);  // SVA reference, no copy

      // If orientation identity we can skip many operation in vertex processing
      if (queuedMesh.orientation == Ogre::Quaternion::IDENTITY)
         _buildIdentiryOrientation(queuedMesh, mpParentGeom->center, vertexBufferElements, vertexBuffers, mpVertexData);
      else
         _buildFullTransform(queuedMesh, mpParentGeom->center, vertexBufferElements, vertexBuffers, mpVertexData);

      //Copy mesh index data into the index buffer
      Ogre::IndexData *sourceIndexData = queuedMesh.mesh->indexData;

      if (srcIndexType == HardwareIndexBuffer::IT_32BIT)
      {
         //Lock the input buffer
         uint32 *source = static_cast<uint32*>(sourceIndexData->indexBuffer->lock(
            sourceIndexData->indexStart, sourceIndexData->indexCount, HardwareBuffer::HBL_READ_ONLY));
         uint32 *sourceEnd = source + sourceIndexData->indexCount;

         //And copy it to the output buffer
         while (source != sourceEnd)
            *indexBuffer32++ = static_cast<uint32>(*source++ + indexOffset);

         //Unlock the input buffer
         sourceIndexData->indexBuffer->unlock();

         //Increment the index offset
         indexOffset += queuedMesh.mesh->vertexData->vertexCount;
      }
      else
      {
         if (destIndexType == HardwareIndexBuffer::IT_32BIT)
         {
            //-- Convert 16 bit to 32 bit indices --
            //Lock the input buffer
            uint16 *source = static_cast<uint16*>(sourceIndexData->indexBuffer->lock(
               sourceIndexData->indexStart, sourceIndexData->indexCount, HardwareBuffer::HBL_READ_ONLY));
            uint16 *sourceEnd = source + sourceIndexData->indexCount;

            //And copy it to the output buffer
            while (source != sourceEnd)
            {
               uint32 indx = *source++;
               *indexBuffer32++ = (indx + indexOffset);
            }

            sourceIndexData->indexBuffer->unlock();                  // Unlock the input buffer
            indexOffset += queuedMesh.mesh->vertexData->vertexCount; // Increment the index offset
         }
         else
         {
            //Lock the input buffer
            uint16 *source = static_cast<uint16*>(sourceIndexData->indexBuffer->lock(
               sourceIndexData->indexStart, sourceIndexData->indexCount, HardwareBuffer::HBL_READ_ONLY));
            uint16 *sourceEnd = source + sourceIndexData->indexCount;

            //And copy it to the output buffer
            while (source != sourceEnd)
               *indexBuffer16++ = static_cast<uint16>(*source++ + indexOffset);

            sourceIndexData->indexBuffer->unlock();                  // Unlock the input buffer
            indexOffset += queuedMesh.mesh->vertexData->vertexCount; // Increment the index offset
         }
      }

   }  // For each queued mesh

   //Unlock buffers
   mpIndexData->indexBuffer->unlock();
   for (Ogre::ushort i = 0; i < vertBinding->getBufferCount(); ++i)
      vertBinding->getBuffer(i)->unlock();

   meshQueue.clear();   // Clear mesh queue
   mBuilt = true;
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::SubBatch::_buildIdentiryOrientation(const QueuedMesh &queuedMesh,
                                                          const Ogre::Vector3 &parentGeomCenter,
                                                          const std::vector<VertexDeclaration::VertexElementList> &vertexBufferElements,
                                                          std::vector<Ogre::uchar*> &vertexBuffers,
                                                          Ogre::VertexData *dstVertexData)
{
   const VertexData *sourceVertexData = queuedMesh.mesh->vertexData;
   Ogre::Vector3 v3AddBatchPosition = queuedMesh.position - parentGeomCenter;

   //Copy mesh vertex data into the vertex buffer
   VertexBufferBinding *sourceBinds = sourceVertexData->vertexBufferBinding;
   VertexBufferBinding *destBinds = dstVertexData->vertexBufferBinding;

   // For each vertex buffer
   for (unsigned short ibuffer = 0, bufCnt = destBinds->getBufferCount(); ibuffer < bufCnt; ++ibuffer)
   {
      if (ibuffer < sourceBinds->getBufferCount()) // destanation buffer index smaller than source buffers count
      {
         //Lock the input buffer
         const HardwareVertexBufferSharedPtr &sourceBuffer = sourceBinds->getBuffer(ibuffer);
         uchar *sourceBase = static_cast<uchar*>(sourceBuffer->lock(HardwareBuffer::HBL_READ_ONLY));
         uchar *destBase = vertexBuffers[ibuffer]; //Get the locked output buffer

         const VertexDeclaration::VertexElementList &elems = vertexBufferElements[ibuffer];
         VertexDeclaration::VertexElementList::const_iterator iBegin = elems.begin(), iEnd = elems.end();
         float *sourcePtr, *destPtr;

         //Copy vertices
         for (size_t v = 0, vertexCounter = sourceVertexData->vertexCount; v < vertexCounter; ++v)
         {
            // Iterate over vertex elements
            VertexDeclaration::VertexElementList::const_iterator it = iBegin;
            while (it != iEnd)
            {
               const VertexElement &elem = *it;
               elem.baseVertexPointerToElement(sourceBase, &sourcePtr);
               elem.baseVertexPointerToElement(destBase, &destPtr);

               switch (elem.getSemantic())
               {
               case VES_POSITION:
                  {
                     Vector3 tmp(sourcePtr[0], sourcePtr[1], sourcePtr[2]);
                     tmp *= queuedMesh.scale;
                     tmp += v3AddBatchPosition;

                     destPtr[0] = tmp.x; destPtr[1] = tmp.y; destPtr[2] = tmp.z;
                  }
                  break;

               case VES_NORMAL:
               case VES_BINORMAL:
               case VES_TANGENT:
                  {
                     // Raw copy
                     destPtr[0] = sourcePtr[0];
                     destPtr[1] = sourcePtr[1];
                     destPtr[2] = sourcePtr[2];
                  }
                  break;

               case VES_DIFFUSE:
                  {
                     uint32 tmpColor = *((uint32*)sourcePtr++);
                     uint8 tmpR = ((tmpColor) & 0xFF)       * queuedMesh.color.r;
                     uint8 tmpG = ((tmpColor >> 8) & 0xFF)  * queuedMesh.color.g;
                     uint8 tmpB = ((tmpColor >> 16) & 0xFF) * queuedMesh.color.b;
                     uint8 tmpA = ((tmpColor >> 24) & 0xFF) * queuedMesh.color.a;

                     *((uint32*)destPtr++) = tmpR | (tmpG << 8) | (tmpB << 16) | (tmpA << 24);
                  }
                  break;

               default:
                  // Raw copy
                  memcpy(destPtr, sourcePtr, VertexElement::getTypeSize(elem.getType()));
                  break;
               };

               ++it;
            }  // while Iterate over vertex elements

            // Increment both pointers
            destBase += sourceBuffer->getVertexSize();
            sourceBase += sourceBuffer->getVertexSize();
         }

         vertexBuffers[ibuffer] = destBase;
         sourceBuffer->unlock(); // unlock the input buffer
      }
      else
      {
         size_t bufferNumber = destBinds->getBufferCount()-1;

         //Get the locked output buffer
         uint32 *startPtr = (uint32*)vertexBuffers[bufferNumber];
         uint32 *endPtr = startPtr + sourceVertexData->vertexCount;

         //Generate color
         uint8 tmpR = queuedMesh.color.r * 255;
         uint8 tmpG = queuedMesh.color.g * 255;
         uint8 tmpB = queuedMesh.color.b * 255;
         uint32 tmpColor = tmpR | (tmpG << 8) | (tmpB << 16) | (0xFF << 24);

         //Copy colors
         while (startPtr < endPtr)
            *startPtr++ = tmpColor;

         vertexBuffers[bufferNumber] += sizeof(uint32) * sourceVertexData->vertexCount;
      }

   }  // For each vertex buffer

}

//-----------------------------------------------------------------------------
///
void BatchedGeometry::SubBatch::_buildFullTransform(const QueuedMesh &queuedMesh,
                                                    const Ogre::Vector3 &parentGeomCenter,
                                                    const std::vector<VertexDeclaration::VertexElementList> &vertexBufferElements,
                                                    std::vector<Ogre::uchar*> &vertexBuffers,
                                                    Ogre::VertexData *dstVertexData)
{
   const VertexData *sourceVertexData = queuedMesh.mesh->vertexData;
   // Get rotation matrix for vertex rotation
   Ogre::Matrix3 m3MeshOrientation;
   queuedMesh.orientation.ToRotationMatrix(m3MeshOrientation);
   const Ogre::Real *mat = m3MeshOrientation[0];   // Ogre::Matrix3 is row major
   Ogre::Vector3 v3AddBatchPosition = queuedMesh.position - parentGeomCenter;

   //Copy mesh vertex data into the vertex buffer
   VertexBufferBinding *sourceBinds = sourceVertexData->vertexBufferBinding;
   VertexBufferBinding *destBinds = dstVertexData->vertexBufferBinding;

   // For each vertex buffer
   for (unsigned short ibuffer = 0, bufCnt = destBinds->getBufferCount(); ibuffer < bufCnt; ++ibuffer)
   {
      if (ibuffer < sourceBinds->getBufferCount()) // destanation buffer index smaller than source buffers count
      {
         //Lock the input buffer
         const HardwareVertexBufferSharedPtr &sourceBuffer = sourceBinds->getBuffer(ibuffer);
         uchar *sourceBase = static_cast<uchar*>(sourceBuffer->lock(HardwareBuffer::HBL_READ_ONLY));

         //Get the locked output buffer
         uchar *destBase = vertexBuffers[ibuffer];


         const VertexDeclaration::VertexElementList &elems = vertexBufferElements[ibuffer];
         VertexDeclaration::VertexElementList::const_iterator iBegin = elems.begin(), iEnd = elems.end();
         float *sourcePtr, *destPtr;

         //Copy vertices
         for (size_t v = 0, vertexCounter = sourceVertexData->vertexCount; v < vertexCounter; ++v)
         {
            VertexDeclaration::VertexElementList::const_iterator it = iBegin;
            // Iterate over vertex elements
            while (it != iEnd)
            {
               const VertexElement &elem = *it;
               elem.baseVertexPointerToElement(sourceBase, &sourcePtr);
               elem.baseVertexPointerToElement(destBase, &destPtr);

               switch (elem.getSemantic())
               {
               case VES_POSITION:
                  {
                     Vector3 tmp(sourcePtr[0], sourcePtr[1], sourcePtr[2]);
                     tmp *= queuedMesh.scale;
                     
                     // rotate vector by matrix. Ogre::Matrix3::operator* (const Vector3&) is not fast
                     tmp = Ogre::Vector3(
                        mat[0] * tmp.x + mat[1] * tmp.y + mat[2] * tmp.z,
                        mat[3] * tmp.x + mat[4] * tmp.y + mat[5] * tmp.z,
                        mat[6] * tmp.x + mat[7] * tmp.y + mat[8] * tmp.z);
                     tmp += v3AddBatchPosition;

                     destPtr[0] = tmp.x; destPtr[1] = tmp.y; destPtr[2] = tmp.z;
                  }
                  break;

               case VES_NORMAL:
               case VES_BINORMAL:
               case VES_TANGENT:
                  {
                     // rotate vector by matrix. Ogre::Matrix3::operator* (const Vector3&) is not fast
                     destPtr[0] = mat[0] * sourcePtr[0] + mat[1] * sourcePtr[1] + mat[2] * sourcePtr[2]; // x
                     destPtr[1] = mat[3] * sourcePtr[0] + mat[4] * sourcePtr[1] + mat[5] * sourcePtr[2]; // y
                     destPtr[2] = mat[6] * sourcePtr[0] + mat[6] * sourcePtr[1] + mat[6] * sourcePtr[2]; // z
                  }
                  break;

               case VES_DIFFUSE:
                  {
                     uint32 tmpColor = *((uint32*)sourcePtr);
                     uint8 tmpR = ((tmpColor) & 0xFF)       * queuedMesh.color.r;
                     uint8 tmpG = ((tmpColor >> 8) & 0xFF)  * queuedMesh.color.g;
                     uint8 tmpB = ((tmpColor >> 16) & 0xFF) * queuedMesh.color.b;
                     uint8 tmpA = ((tmpColor >> 24) & 0xFF) * queuedMesh.color.a;

                     *((uint32*)destPtr) = tmpR | (tmpG << 8) | (tmpB << 16) | (tmpA << 24);
                  }
                  break;

               default:
                  memcpy(destPtr, sourcePtr, VertexElement::getTypeSize(elem.getType()));
                  break;
               };

               ++it;
            }  // for VertexElementList

            // Increment both pointers
            destBase += sourceBuffer->getVertexSize();
            sourceBase += sourceBuffer->getVertexSize();
         }

         //Unlock the input buffer
         vertexBuffers[ibuffer] = destBase;
         sourceBuffer->unlock();
      }
      else
      {
         size_t bufferNumber = destBinds->getBufferCount()-1;

         //Get the locked output buffer
         uint32 *startPtr = (uint32*)vertexBuffers[bufferNumber];
         uint32 *endPtr = startPtr + sourceVertexData->vertexCount;

         //Generate color
         uint8 tmpR = queuedMesh.color.r * 255;
         uint8 tmpG = queuedMesh.color.g * 255;
         uint8 tmpB = queuedMesh.color.b * 255;
         uint32 tmpColor = tmpR | (tmpG << 8) | (tmpB << 16) | (0xFF << 24);

         //Copy colors
         while (startPtr < endPtr)
            *startPtr++ = tmpColor;

         vertexBuffers[bufferNumber] += sizeof(uint32) * sourceVertexData->vertexCount;
      }
   }
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::SubBatch::clear()
{
   //If built, delete the batch
   if (mBuilt)
   {
      mBuilt = false;

      //Delete buffers
      mpIndexData->indexBuffer.setNull();
      mpVertexData->vertexBufferBinding->unsetAllBindings();

      //Reset vertex/index count
      mpVertexData->vertexStart = 0;
      mpVertexData->vertexCount = 0;
      mpIndexData->indexStart = 0;
      mpIndexData->indexCount = 0;
   }

   //Clear mesh queue
   meshQueue.clear();
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::SubBatch::addSelfToRenderQueue(RenderQueueGroup *rqg)
{
   if (!mBuilt)
      return;

   //Update material technique based on camera distance
   m_pBestTechnique = mPtrMaterial->getBestTechnique(mPtrMaterial->getLodIndex(mpParentGeom->mfMinDistanceSquared * mpParentGeom->mfMinDistanceSquared));
   rqg->addRenderable(this, m_pBestTechnique, OGRE_RENDERABLE_DEFAULT_PRIORITY);
}


//-----------------------------------------------------------------------------
///
void BatchedGeometry::SubBatch::getRenderOperation(RenderOperation& op)
{
   op.operationType = RenderOperation::OT_TRIANGLE_LIST;
   op.srcRenderable = this;
   op.useIndexes = true;
   op.vertexData = mpVertexData;
   op.indexData = mpIndexData;
}


//-----------------------------------------------------------------------------
///
Real BatchedGeometry::SubBatch::getSquaredViewDepth(const Camera* cam) const
{
   Vector3 camVec = mpParentGeom->_convertToLocal(cam->getDerivedPosition()) - mpParentGeom->center;
   return camVec.squaredLength();
}

//-----------------------------------------------------------------------------
///
const Ogre::LightList& BatchedGeometry::SubBatch::getLights(void) const
{
   return mpParentGeom->queryLights();
}
