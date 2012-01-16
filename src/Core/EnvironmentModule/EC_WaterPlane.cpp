// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#define MATH_OGRE_INTEROP
#include "DebugOperatorNew.h"

#include "EC_WaterPlane.h"

#include "Entity.h"
#include "EC_Placeable.h"
#include "IAttribute.h"
#include "AttributeMetadata.h"
#include "Renderer.h"
#include "Scene.h"
#include "OgreMaterialUtils.h"
#include "OgreRenderingModule.h"
#include "OgreWorld.h"
#include "LoggingFunctions.h"

#include <Ogre.h>
#include <OgreQuaternion.h>
#include <OgreColourValue.h>
#include <OgreMath.h>

#include "MemoryLeakCheck.h"

EC_WaterPlane::EC_WaterPlane(Scene* scene) :
    IComponent(scene),
    xSize(this, "x-size", 5000),
    ySize(this, "y-size", 5000),
    depth(this, "Depth", 20),
    position(this, "Position", float3::zero),
    rotation(this, "Rotation", Quat::identity),
    scaleUfactor(this, "U factor", 0.0002f),
    scaleVfactor(this, "V factor", 0.0002f),
    xSegments(this, "Segments in x", 10),
    ySegments(this, "Segments in y", 10),
    materialName(this, "Material", QString("Ocean")),
    materialRef(this, "Material ref"),
   //textureNameAttr(this, "Texture", QString("DefaultOceanSkyCube.dds")),
    fogColor(this, "Fog color", Color(0.2f,0.4f,0.35f,1.0f)),
    fogStartDistance(this, "Fog start dist.", 100.f),
    fogEndDistance(this, "Fog end dist.", 2000.f),
    fogMode(this, "Fog mode", 3),
    entity_(0),
    node_(0),
    attached_(false),
    attachedToRoot_(false)
{
    static AttributeMetadata metadata;
    static bool metadataInitialized = false;
    if(!metadataInitialized)
    {
        metadata.enums[Ogre::FOG_NONE] = "NoFog";
        metadata.enums[Ogre::FOG_EXP] = "Exponential";
        metadata.enums[Ogre::FOG_EXP2] = "ExponentiallySquare";
        metadata.enums[Ogre::FOG_LINEAR] = "Linear";
        metadataInitialized = true;
    }

    fogMode.SetMetadata(&metadata);

    if (scene)
        world_ = scene->GetWorld<OgreWorld>();
    OgreWorldPtr world = world_.lock();
    if (world)
    {
        Ogre::SceneManager *sceneMgr = world->OgreSceneManager();
        node_ = sceneMgr->createSceneNode(world->GetUniqueObjectName("EC_WaterPlane_Root"));
    }

    QObject::connect(this, SIGNAL(AttributeChanged(IAttribute*, AttributeChange::Type)),
        SLOT(OnAttributeUpdated(IAttribute*, AttributeChange::Type)));

    lastXsize_ = xSize.Get();
    lastYsize_ = ySize.Get();

    connect(this, SIGNAL(ParentEntitySet()), this, SLOT(SetParent()));

    // If there exist placeable copy its position for default position and rotation.
   
    /*
    EC_Placeable* placeable = dynamic_cast<EC_Placeable*>(FindPlaceable().get());
    if (placeable != 0)
    {
        float3 vec = placeable->GetPosition();
        position.Set(vec,AttributeChange::Default);
   
        Quaternion rot =placeable->GetOrientation();
        rotation.Set(rot, AttributeChange::Default);
        ComponentChanged(AttributeChange::Default);
    }
    */
}

EC_WaterPlane::~EC_WaterPlane()
{
   if (world_.expired())
    return;
    
   OgreWorldPtr world = world_.lock();
   RemoveWaterPlane();

    if (node_ != 0)
    {
        Ogre::SceneManager* sceneMgr = world->OgreSceneManager();
        sceneMgr->destroySceneNode(node_);
        node_ = 0;
    }
}

void EC_WaterPlane::SetParent()
{
    if (!ViewEnabled())
        return;
    
    CreateWaterPlane();
    
    // Parent entity has set.
    // Has parent a placeable?
    EC_Placeable* placeable = dynamic_cast<EC_Placeable*>(FindPlaceable().get());
    if (placeable != 0)
    {
        // Are we currently attached?
        if (attached_ )
        {
            // Now there might be that we are attached to OgreRoot not to placeable node.
            DetachEntity();
            AttachEntity();
        }
    }

    connect(parentEntity,SIGNAL(ComponentAdded(IComponent*, AttributeChange::Type)),
        SLOT(ComponentAdded(IComponent*, AttributeChange::Type)));
    connect(parentEntity,SIGNAL(ComponentRemoved(IComponent*, AttributeChange::Type)),
        SLOT(ComponentRemoved(IComponent*, AttributeChange::Type)));
}

void EC_WaterPlane::ComponentAdded(IComponent* component, AttributeChange::Type type)
{
    if (component->TypeName() == EC_Placeable::TypeNameStatic())
    {
        DetachEntity();

        EC_Placeable* placeable = static_cast<EC_Placeable* >(component);
        if (placeable == 0)
            return;
        if (entity_ == 0)
            return;

        Ogre::SceneNode* node = placeable->GetSceneNode();
        
        node->addChild(node_);
        node_->attachObject(entity_);
        node_->setVisible(true);

        attached_ = true;
    }
}

void EC_WaterPlane::ComponentRemoved(IComponent* component, AttributeChange::Type type)
{
    if (world_.expired())
        return;
    
    if (component->TypeName() == EC_Placeable::TypeNameStatic() )
    {
        DetachEntity();

        // Attach entity directly to Ogre root.
        Ogre::SceneManager* sceneMgr = world_.lock()->OgreSceneManager();
        if (sceneMgr == 0)
            return;

        node_->attachObject(entity_);
        sceneMgr->getRootSceneNode()->addChild(node_);
        node_->setVisible(true);
        attachedToRoot_ = true;
        attached_ = true;
    }
}

float3 EC_WaterPlane::GetPointOnPlane(const float3 &point) const 
{
    if (node_ == 0)
        return float3::nan;

    Ogre::Quaternion rot = node_->_getDerivedOrientation();
    Ogre::Vector3 trans = node_->_getDerivedPosition();
    Ogre::Vector3 scale = node_->_getDerivedScale();

    Ogre::Matrix4 worldTM;
    worldTM.makeTransform(trans, scale, rot);

    // In Ogre 1.7.1 we could simply use the following line, but since we're also supporting Ogre 1.6.4 for now, the above
    // lines are used instead, which work in both.
    // Ogre::Matrix4 worldTM = node_->_getFullTransform(); // local->world. 

    Ogre::Matrix4 inv = worldTM.inverse(); // world->local
    Ogre::Vector4 local = inv * Ogre::Vector4(point.x, point.y, point.z, 1.f);
 
    local.y = 0;
    Ogre::Vector4 world = worldTM * local;
    return float3(world.x, world.y, world.z);
}

float EC_WaterPlane::GetDistanceToWaterPlane(const float3& point) const
{
     if (node_ == 0)
        return 0;

    float3 pointOnPlane = GetPointOnPlane(point);

    //Ogre::Vector3 local = node_->_getDerivedOrientation().Inverse() * ( OgreRenderer::ToOgreVector3(point) - node_->_getDerivedPosition() ) / node_->_getDerivedScale();
  
    return point.y - pointOnPlane.y;
}

bool EC_WaterPlane::IsTopOrBelowWaterPlane(const float3& point) const
{
     if (node_ == 0)
        return false;
     
    Ogre::Vector3 local = node_->_getDerivedOrientation().Inverse() * ( point - node_->_getDerivedPosition() ) / node_->_getDerivedScale();
 
    int x = xSize.Get(), y = ySize.Get();

    float xMax = x*0.5;
    float yMax = y*0.5;
    float xMin = -x*0.5;
    float yMin = -y*0.5;
    
    if (local.x > xMin && local.x < xMax && local.y > yMin && local.y < yMax)
        return true;

    return false;
}

bool EC_WaterPlane::IsPointInsideWaterCube(const float3& point) const
{
    if (entity_ == 0)
        return false;

  if (IsTopOrBelowWaterPlane(point))
  {
        float d = GetDistanceToWaterPlane(point);
        if (d < 0 && depth.Get() >= fabs(d) )
            return true;
  }

  return false; 
}

bool EC_WaterPlane::IsCameraInsideWaterCube()
{
    // Check that is camera inside of defined water plane.
    if (entity_ == 0)
        return false;

    Ogre::Camera *camera = world_.lock()->Renderer()->MainOgreCamera();
    if (!camera)
        return false;
    Ogre::Vector3 posCamera = camera->getDerivedPosition();

    if (IsTopOrBelowWaterPlane(float3(posCamera.x, posCamera.y, posCamera.z)))
    {
        float d = GetDistanceToWaterPlane(float3(posCamera.x, posCamera.y, posCamera.z));
        if (d < 0 && depth.Get() >= fabs(d))
            return true;
    }

    return false;
}

void EC_WaterPlane::CreateWaterPlane()
{
    if (!ViewEnabled())
        return;
    
    if (entity_)
        RemoveWaterPlane();
    
    OgreWorldPtr world = world_.lock();
    // Create water plane
    if (world)
    {
        Ogre::SceneManager *sceneMgr = world->OgreSceneManager();
        assert(sceneMgr);

        if (node_ != 0)
        {
            int x = xSize.Get();
            int y = ySize.Get();
            float uTile =  scaleUfactor.Get() * x; /// Default x-size 5000 --> uTile 1.0
            float vTile =  scaleVfactor.Get() * y;
            
            Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createPlane(Name().toStdString().c_str(),
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::Plane(Ogre::Vector3::UNIT_Y, 0),
                x, y, xSegments.Get(), ySegments.Get(), true, 1, uTile, vTile, Ogre::Vector3::UNIT_X);
            
            entity_ = sceneMgr->createEntity(world->GetUniqueObjectName("EC_WaterPlane_entity"), Name().toStdString().c_str());
            entity_->setMaterialName(materialName.Get().toStdString().c_str());
            entity_->setCastShadows(false);
            // Tries to attach entity, if there is not EC_Placeable availible, it will not attach object
            AttachEntity();
        }
    }
}

void EC_WaterPlane::RemoveWaterPlane()
{
    // Remove waterplane
    if (world_.expired() || !entity_)
        return;

    DetachEntity();

    Ogre::SceneManager* sceneMgr = world_.lock()->OgreSceneManager();
    sceneMgr->destroyEntity(entity_);
    entity_ = 0;
    
    Ogre::MeshManager::getSingleton().remove(Name().toStdString().c_str());
}

Ogre::ColourValue EC_WaterPlane::GetFogColorAsOgreValue() const
{
    Color col = fogColor.Get();
    return Ogre::ColourValue(col.r, col.g, col.b, col.a);
}

void EC_WaterPlane::OnAttributeUpdated(IAttribute* attribute, AttributeChange::Type change)
{
    ChangeWaterPlane(attribute);
}

void EC_WaterPlane::SetPosition()
{
    if ((!node_) || (!ViewEnabled()))
        return;
    
    float3 vec = position.Get();
    //node_->setPosition(vec.x, vec.y, vec.z);

#if OGRE_VERSION_MINOR <= 6 && OGRE_VERSION_MAJOR <= 1
    //Ogre::Vector3 current_pos = node_->_getDerivedPosition();
    Ogre::Vector3 tmp(vec.x,vec.y,vec.z);
  
    if (!vec.IsFinite())
        return;
   
    node_->setPosition(tmp);
#else
    Ogre::Vector3 pos(vec.x, vec.y, vec.z);
    if (!vec.IsFinite())
        return;
    //node_->_setDerivedPosition(pos);
    node_->setPosition(pos);
#endif
}

void EC_WaterPlane::SetOrientation()
{
    if ((!node_) || (!ViewEnabled()))
        return;
    
    // Set orientation
    Quat rot = rotation.Get();

#if OGRE_VERSION_MINOR <= 6 && OGRE_VERSION_MAJOR <= 1
    Ogre::Quaternion current_rot = node_->_getDerivedOrientation();
    Ogre::Quaternion tmp(rot.w, rot.x, rot.y, rot.z);
    Ogre::Quaternion rota = current_rot * tmp;
    node_->setOrientation(rota);
#else
    node_->_setDerivedOrientation(rot);
#endif
}

void EC_WaterPlane::ChangeWaterPlane(IAttribute* attribute)
{
    const QString &name = attribute->Name();
    if ((name == xSize.Name() || name == ySize.Name() || name == scaleUfactor.Name() ||
        name == scaleVfactor.Name()) && (lastXsize_ != xSize.Get() || lastYsize_ != ySize.Get()))
    {
        CreateWaterPlane();
        lastXsize_ = xSize.Get();
        lastYsize_ = ySize.Get();
    }
    else if (name == xSegments.Name() || name == ySegments.Name())
    {
        CreateWaterPlane();
    }
    else if (name == position.Name() )
    {
        // Change position
        SetPosition();
    }
    else if (name == rotation.Name() )
    {
        // Change rotation

        // Is there placeable component? If not use given rotation 
        //if (dynamic_cast<EC_Placeable*>(FindPlaceable().get()) == 0 )
        //{
           SetOrientation();
        //}
    }
    else if (name == depth.Name() )
    {
        // Change depth
        // Currently do nothing..
    }
    else if (name ==  materialName.Name())
    {
        //Change material
        if (entity_)
            entity_->setMaterialName(materialName.Get().toStdString().c_str());
    }
    /*
    // Currently commented out, working feature but not enabled yet.
    else if (name == textureNameAttr.GetNameString() )
    {

        QString currentMaterial = materialName.Get();
        
        // Check that has texture really changed. 
        
        StringVector names;
        Ogre::MaterialPtr materialPtr = Ogre::MaterialManager::getSingleton().getByName(currentMaterial.toStdString().c_str());
        
        if (materialPtr.get() == 0)
            return;

        OgreRenderer::GetTextureNamesFromMaterial(materialPtr, names);
        
        QString textureName = textureNameAttr.Get();
        
        for(StringVector::iterator iter = names.begin(); iter != names.end(); ++iter)
        {
            QString currentTextureName(iter->c_str());
            if (currentTextureName == textureName)
                return;
        }

        // So texture has really changed, let's change it. 
        OgreRenderer::SetTextureUnitOnMaterial(materialPtr, textureName.toStdString(), 0);
    }
    */
}

ComponentPtr EC_WaterPlane::FindPlaceable() const
{
    assert(framework);
    ComponentPtr comp;
    if(!ParentEntity())
        return comp;
    comp = ParentEntity()->GetComponent<EC_Placeable>();
    return comp;
}

void EC_WaterPlane::AttachEntity()
{
    if (attached_ || entity_ == 0)
        return;

    EC_Placeable* placeable = dynamic_cast<EC_Placeable* >(FindPlaceable().get());
    
    // If there exist placeable attach node and entity to it
    if (placeable != 0 )
    {
        Ogre::SceneNode* node = placeable->GetSceneNode();
        node->addChild(node_);
        node_->attachObject(entity_);
        node_->setVisible(true);
    }
    else
    {
        // There is no placeable attacht entity to OgreSceneRoot 
        Ogre::SceneManager* sceneMgr = world_.lock()->OgreSceneManager();
        node_->attachObject(entity_);
        sceneMgr->getRootSceneNode()->addChild(node_);
        node_->setVisible(true);
        attachedToRoot_ = true;
    }

    attached_ = true;
}

void EC_WaterPlane::DetachEntity()
{
    if (!attached_ || entity_ == 0)
        return;

    EC_Placeable* placeable = dynamic_cast<EC_Placeable*>(FindPlaceable().get());
    if (placeable != 0 && !attachedToRoot_)
    {
        Ogre::SceneNode* node = placeable->GetSceneNode();
        node_->detachObject(entity_);
        node->removeChild(node_); 
    }
    else
    {
        // Attached to root.
        // Sanity check..
        if (entity_->isAttached() )
        {
            Ogre::SceneManager* sceneMgr = world_.lock()->OgreSceneManager();
            node_->detachObject(entity_);
            sceneMgr->getRootSceneNode()->removeChild(node_);
            attachedToRoot_ = false;
        }
    }

    attached_ = false;
}
