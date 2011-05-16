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

#ifndef __BatchedGeometry_H__
#define __BatchedGeometry_H__

#include <OgrePrerequisites.h>
#include <OgreMovableObject.h>
#include <OgreSceneNode.h>
#include <OgreMaterialManager.h>

namespace Forests
{

   //--------------------------------------------------------------------------
   ///
   class BatchedGeometry: public Ogre::MovableObject
   {
   public:
      //--------------------------------------------------------------------------
      /// Visible chunk of geometry
      class SubBatch: public Ogre::Renderable
      {
      protected:
         // A structure defining the desired position/orientation/scale of a batched mesh. The
         // SubMesh is not specified since that can be determined by which MeshQueue this belongs to.
         struct QueuedMesh
         {
            Ogre::SubMesh *mesh;
            Ogre::Vector3 position;
            Ogre::Quaternion orientation;
            Ogre::Vector3 scale;
            Ogre::ColourValue color;
            void* userData;
         };
         typedef std::vector<QueuedMesh>::iterator MeshQueueIterator;
         typedef std::vector<QueuedMesh> MeshQueue;


      public:
         /// Constructor
         SubBatch(BatchedGeometry *parent, Ogre::SubEntity *ent);
         /// Destructor
         ~SubBatch();

         void addSubEntity(Ogre::SubEntity *ent, const Ogre::Vector3 &position, const Ogre::Quaternion &orientation, const Ogre::Vector3 &scale, const Ogre::ColourValue &color = Ogre::ColourValue::White, void* userData = NULL);
         virtual void build();
         void clear();

         void addSelfToRenderQueue(Ogre::RenderQueueGroup *rqg);
         void getRenderOperation(Ogre::RenderOperation& op);
         Ogre::Real getSquaredViewDepth(const Ogre::Camera* cam) const;
         const Ogre::LightList& getLights(void) const;

         void setMaterial(Ogre::MaterialPtr &mat)                 { mPtrMaterial = mat; }
         void setMaterialName(const Ogre::String &mat, const Ogre::String &rg =  Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME) {
            mPtrMaterial = Ogre::MaterialManager::getSingleton().getByName(mat, rg);
         }
         const Ogre::String& getMaterialName() const              { return mPtrMaterial->getName(); }

         Ogre::Technique *getTechnique() const                    { return m_pBestTechnique; }
         const Ogre::MaterialPtr& getMaterial(void) const         { return mPtrMaterial; }
         void getWorldTransforms(Ogre::Matrix4* xform) const      { *xform = mpParentGeom->_getParentNodeFullTransform(); }
         const Ogre::Quaternion& getWorldOrientation(void) const  { return mpParentGeom->mpSceneNode->_getDerivedOrientation(); }
         const Ogre::Vector3& getWorldPosition(void) const        { return mpParentGeom->mpSceneNode->_getDerivedPosition(); }
         bool castsShadows(void) const                            { return mpParentGeom->getCastShadows(); }

         Ogre::VertexData *mpVertexData;
         Ogre::IndexData *mpIndexData;

      private:
         /// Build vertex of QueuedMesh if it have identity orientation
         static void _buildIdentiryOrientation(const QueuedMesh &queuedMesh, const Ogre::Vector3 &parentGeomCenter,
            const std::vector<Ogre::VertexDeclaration::VertexElementList> &vertexBufferElements, std::vector<Ogre::uchar*> &vertexBuffers,
            Ogre::VertexData *dst);
         /// Build vertex of QueuedMesh if it have some orientation
         static void _buildFullTransform(const QueuedMesh &queuedMesh, const Ogre::Vector3 &parentGeomCenter,
            const std::vector<Ogre::VertexDeclaration::VertexElementList> &vertexBufferElements, std::vector<Ogre::uchar*> &vertexBuffers,
            Ogre::VertexData *dst);

         Ogre::Technique*  m_pBestTechnique;       ///< This is recalculated every frame

      protected:
         bool              mBuilt;                 ///<
         bool              mRequireVertexColors;   ///<
         Ogre::SubMesh*    mpSubMesh;              ///< Ogre::SubMesh for Index/Vertex buffers manipulation
         BatchedGeometry*  mpParentGeom;           ///<
         Ogre::MaterialPtr mPtrMaterial;
         MeshQueue         meshQueue;	//The list of meshes to be added to this batch
      }; // end class SubBatch


   public:
      BatchedGeometry(Ogre::SceneManager *mgr, Ogre::SceneNode *rootSceneNode);
      ~BatchedGeometry();

      virtual void addEntity(Ogre::Entity *ent, const Ogre::Vector3 &position, const Ogre::Quaternion &orientation = Ogre::Quaternion::IDENTITY, const Ogre::Vector3 &scale = Ogre::Vector3::UNIT_SCALE, const Ogre::ColourValue &color = Ogre::ColourValue::White);
      void build();
      void clear();

      Ogre::Vector3 _convertToLocal(const Ogre::Vector3 &globalVec) const;

      void _notifyCurrentCamera(Ogre::Camera *cam);
      void _updateRenderQueue(Ogre::RenderQueue *queue);
      bool isVisible();
      const Ogre::AxisAlignedBox &getBoundingBox(void) const   { return bounds; }
      Ogre::Real getBoundingRadius(void) const                 { return mfRadius; }
      const Ogre::String &getMovableType(void) const           { static const Ogre::String tp = "BatchedGeometry"; return tp; }

      void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables) { /* empty */ }



   protected:
      static void extractVertexDataFromShared(const Ogre::MeshPtr &mesh);


   private:
      Ogre::Real           mfRadius;
      Ogre::Real           mfMinDistanceSquared;
      Ogre::SceneManager*  mpSceneMgr;
      Ogre::SceneNode*     mpSceneNode;
      Ogre::SceneNode*     mpParentSceneNode;
      bool                 mbWithinFarDistance;


   protected:

      Ogre::String getFormatString(Ogre::SubEntity *ent);
      typedef std::map<Ogre::String, SubBatch*> SubBatchMap;	//Stores a list of GeomBatch'es, using a format string (generated with getGeometryFormatString()) as the key value
      SubBatchMap subBatchMap;
      Ogre::Vector3 center;	
      Ogre::AxisAlignedBox bounds;

      bool mBoundsUndefined;
      bool mBuilt;

   public:
      typedef Ogre::MapIterator<SubBatchMap> SubBatchIterator;
      SubBatchIterator getSubBatchIterator() const;
   };


}

#endif
