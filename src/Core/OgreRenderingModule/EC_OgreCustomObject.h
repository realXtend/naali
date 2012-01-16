// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "OgreModuleApi.h"
#include "OgreModuleFwd.h"

#include "Math/float3.h"

/// Ogre custom object component
/// @todo Delete this class.
/**
<table class="header">
<tr>
<td>
<h2>OgreCustomObject</h2>

Needs to be attached to a placeable (aka scene node) to be visible.
Note that internally this converts the manual object to a mesh entity because of render queue bugs in Ogre
related to manual objects (still unfixed as of 1.6.2)

Registered by OgreRenderer::OgreRenderingModule.

\ingroup OgreRenderingModuleClient

<b>No Attributes</b>.

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

<b>Depends on the component Placeable</b>.
</table>
*/
class OGRE_MODULE_API EC_OgreCustomObject : public IComponent
{
    Q_OBJECT

public:
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit EC_OgreCustomObject(Scene* scene);

    virtual ~EC_OgreCustomObject();

    /// gets placeable component
    ComponentPtr GetPlaceable() const { return placeable_; }

    /// sets placeable component
    /** set a null placeable to detach the object, otherwise will attach
        @param placeable placeable component
     */
    void SetPlaceable(ComponentPtr placeable);

    /// sets draw distance
    /** @param draw_distance New draw distance, 0.0 = draw always (default)
     */
    void SetDrawDistance(float draw_distance);

    /// Sets if the object casts shadows or not.
    void SetCastShadows(bool enabled);

    /// Commit changes from a manual object
    /** converts ManualObject to mesh, makes an entity out of it & clears the manualobject.
        @return true if successful
     */
    bool CommitChanges(Ogre::ManualObject* object);

    /// Sets material on already committed geometry, similar to EC_Mesh
    /** @param index submesh index
        @param material_name material name
        @return true if successful
     */
    bool SetMaterial(uint index, const std::string& material_name);

    /// gets number of materials (submeshes) in committed geometry
    uint GetNumMaterials() const;

    /// gets material name from committed geometry
    /** @param index submesh index
        @return name if successful, empty if not committed / illegal index
     */
    const std::string& GetMaterialName(uint index) const;

    /// Returns true if geometry has been committed and mesh entity created
    bool IsCommitted() const { return entity_ != 0; }

    void GetBoundingBox(float3& min, float3& max) const;

    /// Returns the Ogre entity.
    Ogre::Entity *GetEntity() const { return entity_; }

    COMPONENT_NAME("EC_OgreCustomObject", 19)

private:
    
    /// attaches entity to placeable
    void AttachEntity();
    
    /// detaches entity from placeable
    void DetachEntity();
    
    /// removes old entity and mesh
    void DestroyEntity();
    
    /// placeable component 
    ComponentPtr placeable_;
    
    /// Ogre world ptr
    OgreWorldWeakPtr world_;
    
    /// Ogre mesh entity (converted from the manual object on commit)
    Ogre::Entity* entity_;
    
    /// object attached to placeable -flag
    bool attached_;
    
    /// whether should cast shadows
    bool cast_shadows_;
    
    /// draw distance
    float draw_distance_;
};

