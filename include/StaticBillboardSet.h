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
//functions by taking advantage of the static nature of tree billboards (note:
//StaticBillboardSet does not allow billboards to be moved or deleted individually in
//real-time)
//-------------------------------------------------------------------------------------

#ifndef __StaticBillboardSet_H__
#define __StaticBillboardSet_H__

#include <OgrePrerequisites.h>
#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreVector.h>
#include <OgreMesh.h>
#include <OgreMaterial.h>
#include <OgreBillboard.h>
#include <OgreBillboardSet.h>
#include <OgreMaterialManager.h>
#include <OgreSceneNode.h>
#include <OgreStringConverter.h>

namespace Forests
{
   /** Different methods used to render billboards.
   \deprecated Billboards are always kept facing the camera by a vertex shader now. The value is ignored. */
   enum BillboardMethod
   {
      /// Use vertex shaders to keep billboards facing the camera
      BB_METHOD_ACCELERATED = 1,
      /// \deprecated same as BB_METHOD_ACCELERATED
      BB_METHOD_COMPATIBLE = 0
   };

   //--------------------------------------------------------------------------
   /// A faster alternative to Ogre's built-in BillboardSet class.
   ///
   /// This class provides a method of displaying billboards faster than Ogre's built-in
   /// BillboardSet functions by taking advantage of the static nature of tree billboards.
   /// All billboards are stored in a single static vertex buffer and a vertex shader
   /// keeps them facing the camera, so there is no per frame CPU work.
   ///
   /// @note StaticBillboardSet does not allow billboards to be moved or deleted individually in real-time
   class StaticBillboardSet
   {
      //-----------------------------------------------------------------------------
      //Internal class - do not use
      class StaticBillboard
      {
      public:
         // Constructor
         StaticBillboard(const Ogre::Vector3 &pos, float xScale, float yScale,
            const Ogre::ColourValue &clr, Ogre::uint16 texcrdIndexU, Ogre::uint16 texcrdIndexV) :
         xPos((float)pos.x), yPos((float)pos.y), zPos((float)pos.z), xScaleHalf(0.5f * xScale), yScaleHalf(0.5f * yScale),
            texcoordIndexU(texcrdIndexU), texcoordIndexV(texcrdIndexV)
         {
            color = clr.getAsBYTE();
         }

         // SVA mem friendly aligment
         float xPos, yPos, zPos;
         float xScaleHalf, yScaleHalf;
         Ogre::uint32 color;
         Ogre::uint16 texcoordIndexU, texcoordIndexV;
      };

   public:
      /**
      \brief Initializes a StaticBillboardSet object.
      \param mgr The SceneManager to be used to display the billboards.
      \param rootSceneNode The scene node the billboards will be attached to.
      \param method Ignored. See BillboardMethod.
      */
      StaticBillboardSet(Ogre::SceneManager *mgr, Ogre::SceneNode *rootSceneNode, BillboardMethod method = BB_METHOD_ACCELERATED);
      ~StaticBillboardSet();

      /**
      \brief Adds a billboard to the StaticBillboardSet at the specified position.
      \param position The desired position of the billboard.
      \param xScale The width scale of the billboard.
      \param yScale The height scale of the billboard.
      \param color The color of the billboard.
      \param texcoordIndexU The horizontal texture tile this billboard will use.
      \param texcoordIndexV The vertical texture tile this billboard will use.

      The texcoordIndex options are only applicable if you have used setTextureStacksAndSlices()
      to divide the applied material into a grid of images. They select which image of the grid
      is applied to the billboard.

      \note Any billboards created will not appear in the scene until you call build()
      */
      void createBillboard(const Ogre::Vector3 &position, float xScale = 1.0f, float yScale = 1.0f,
         const Ogre::ColourValue &color = Ogre::ColourValue::White, Ogre::uint16 texcoordIndexU = 0, Ogre::uint16 texcoordIndexV = 0)
      {
         mBillboardBuffer.push_back(StaticBillboard(position, xScale, yScale, color, texcoordIndexU, texcoordIndexV));
      }

      /**
      \brief Sets the billboard's origin (pivotal point)

      This function can be used to set what part of the billboard image is considered the
      origin, or "center". By default, the center of the image is used, so billboards will
      pivot around the center and positioning a billboard will place it's center at the desired
      location. Other origins, like BBO_BOTTOM_CENTER are good for trees, etc. BBO_CENTER is
      used by default.

      \note The origin only applies to billboards built afterwards, see build()
      */
      void setBillboardOrigin(Ogre::BillboardOrigin origin);

      /// Returns the current billboard origin.
      /// This returns the current billboard origin as set by setBillboardOrigin().
      Ogre::BillboardOrigin getBillboardOrigin() const   { return mBBOrigin; }

      /// \deprecated always returns BB_METHOD_ACCELERATED
      BillboardMethod getRenderMethod() const            { return BB_METHOD_ACCELERATED; }

      ///Sets whether or not this StaticBillboardSet will be rendered.
      /// \param visible The desired visibility state of the StaticBillboardSet (true/false)
      void setVisible(bool visible)
      {
         if (mVisible != visible)
         {
            mVisible = visible;
            mpSceneNode->setVisible(visible);
         }
      }

      /**
      \brief Enables/disables distance fade-out for this billboard set
      \param enabled Whether or not to enable fading
      \param visibleDist The distance where billboards will be fully opaque (alpha 1)
      \param invisibleDist The distance where billboards will be invisible (alpha 0)

      You can use this function to enable distance based alpha fading, so billboards will
      smoothly fade out into the distance. Note that the fading is performed 2-dimensionally,
      which means height is not taken into account when fading - only xz distances. This works
      well with flat terrain maps, but may not be well suited for spherical worlds.

      The distance ranges given specify how the final alpha values should be calculated -
      billboards at visibleDist will have alpha values of 1, and geometry at invisibleDist
      will have alpha values of 0.

      \note invisibleDist may be greater than or less than visibleDist, depending on
      whether the geometry is fading out or in to the distance.
      */
      void setFade(bool enabled, Ogre::Real visibleDist, Ogre::Real invisibleDist);

      /// Performs final steps required for the created billboards to appear in the scene.
      /// Until this is called, any billboards created with createBillboard() will not appear.
      void build();

      /// Deletes all billboards from the scene.
      /// The only way to delete a billboard in a StaticBillboardSet is to delete them all,
      /// which this function does.
      void clear();

      /**
      \brief Applies a material to the billboards in this set.
      \param materialName The name of the material to apply to this StaticBillboardSet.
      \param resourceGroup The resource group of the material.

      Passes without a vertex program get the billboard vertex shader assigned (which
      is used to keep the billboards facing the camera), so the material may be modified.
      */
      void setMaterial(const Ogre::String &materialName, const Ogre::String &resourceGroup =
         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

      /**
      \brief Sets how many horizontal slices and vertical stacks the currently applied material is using.
      \param stacks The number of vertical stacks.
      \param slices The number of horizontal slices.

      If the applied material contains multiple images all merged together into a grid
      you can use this function to gain access to the individual images. Simply set how
      many tiles are contained within the material horizontally and vertically, and use
      the texcoordIndexU and texcoordIndexV parameters of createBillboard() to specify
      which image is to be used for the billboard.
      */
      void setTextureStacksAndSlices(Ogre::uint16 stacks, Ogre::uint16 slices);

      /// \deprecated billboards are kept facing the camera by the vertex shader alone, so this does nothing
      static void updateAll(const Ogre::Vector3 &cameraDirection, const char* fragmentProgramName) {}

   private:
      /// Assigns the billboard vertex shader to all passes without a vertex program
      void prepareMaterial(const Ogre::MaterialPtr &material);
      ///
      Ogre::MaterialPtr getFadeMaterial(const Ogre::MaterialPtr &protoMaterial, Ogre::Real visibleDist, Ogre::Real invisibleDist);
      ///
      void destroyEntity();

   private:
      typedef std::vector<StaticBillboard> TBillBuf;

      bool                    mVisible;                  ///<
      bool                    mFadeEnabled;              ///<
      Ogre::SceneManager*     mpSceneMgr;                ///<
      Ogre::SceneNode*        mpSceneNode;               ///<
      Ogre::Entity*           mpEntity;                  ///<
      Ogre::MeshPtr           mPtrMesh;                  ///<
      Ogre::String            mEntityName;               ///<
      Ogre::MaterialPtr       mPtrMaterial;              ///<
      Ogre::MaterialPtr       mPtrFadeMaterial;          ///<
      float                   mfUFactor, mfVFactor;      ///<
      Ogre::BillboardOrigin   mBBOrigin;                 ///<
      Ogre::Real              mFadeVisibleDist;          ///<
      Ogre::Real              mFadeInvisibleDist;        ///<
      TBillBuf                mBillboardBuffer;          ///<

      // static data
   private:
      typedef std::map<Ogre::String, Ogre::MaterialPtr> FadedMaterialMap;

      static unsigned int     s_nSelfInstances;          ///< Instances counter
      static FadedMaterialMap s_mapFadedMaterial;        ///<
      static unsigned long GUID;
      static Ogre::String getUniqueID(const Ogre::String &prefix) { return prefix + Ogre::StringConverter::toString(++GUID); }
   };
}

#endif
