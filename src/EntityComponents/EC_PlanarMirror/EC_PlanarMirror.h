/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   EC_PlanarMirror.h
 *  @brief  EC_PlanarMirror enables one to create planar mirrors.
 *  @note   The entity should have EC_Placeable available in advance.
 */

#pragma once

#include "IComponent.h"
#include "OgreModuleFwd.h"

#include <OgreTexture.h>

/// EntityComponent that will create a planar mirror texture (and optionally a plane showing it).
/**
<table class="header">
<tr>
<td>
<h2>PlanarMirror</h2>
EntityComponent that will create a planar mirror texture (and optionally a plane showing it).
NOTE: Assumes the the entity already has: EC_Placeable, EC_RttTarget and EC_Camera. Otherwise EC_PlanarMirror cannot function.
<b>Attributes</b>:
<ul>
<li>bool reflectionPlaneVisible
<div>Do we want to show the mirror plane</div>
</ul>

<b>Exposes the following scriptable functions:</b>
<ul>
<li>...
</ul>

<b>Reacts on the following actions:</b>
<ul>
<li>...
</ul>
</td>
</tr>

Does not emit any actions.

<b>Depends on EC_Camera, EC_Placeable and EC_RttTarget.</b>
</table>
*/
class EC_PlanarMirror : public IComponent
{
    Q_OBJECT
    COMPONENT_NAME("EC_PlanarMirror", 34)

public:
    EC_PlanarMirror(Scene *scene);
    ~EC_PlanarMirror();

    /// Do we want to show the mirror plane
    Q_PROPERTY(bool reflectionPlaneVisible READ getreflectionPlaneVisible WRITE setreflectionPlaneVisible);
    DEFINE_QPROPERTY_ATTRIBUTE(bool, reflectionPlaneVisible);

    //Returns the texture that you can set to be used on a material. Do not modify this texture yourself
    Ogre::Texture* GetMirrorTexture() const;

public slots:
    void Initialize();
    void Update(float val);
    void OnAttributeUpdated(IAttribute* attr);
    void WindowResized(int w,int h);

private:
    void CreatePlane();

    OgreRenderer::RendererPtr renderer_;
    
    static int mirror_cam_num_;
    
    Ogre::Camera* mirror_cam_;
    Ogre::Texture* mirror_texture_;
    Ogre::TextureUnitState* tex_unit_state_;
    Ogre::Material* mat_;
    Ogre::Entity* mirror_plane_entity_;
    Ogre::MovablePlane* mirror_plane_;
};
