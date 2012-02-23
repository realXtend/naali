// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#define MATH_OGRE_INTEROP

#include "OgreWorld.h"
#include "Renderer.h"
#include "EC_Camera.h"
#include "EC_Placeable.h"
#include "EC_Mesh.h"
#include "OgreCompositionHandler.h"
#include "OgreShadowCameraSetupFocusedPSSM.h"
#include "OgreBulletCollisionsDebugLines.h"

#include "Entity.h"
#include "Scene.h"
#include "Profiler.h"
#include "ConfigAPI.h"
#include "FrameAPI.h"
#include "Transform.h"
#include "Math/float2.h"
#include "Math/float3x4.h"
#include "Geometry/AABB.h"
#include "Geometry/OBB.h"
#include "Geometry/Plane.h"
#include "Geometry/LineSegment.h"
#include "Math/float3.h"
#include "Geometry/Circle.h"
#include "Geometry/Sphere.h"

#include <Ogre.h>

#include "MemoryLeakCheck.h"

OgreWorld::OgreWorld(OgreRenderer::Renderer* renderer, ScenePtr scene) :
    framework_(scene->GetFramework()),
    renderer_(renderer),
    scene_(scene),
    sceneManager_(0),
    rayQuery_(0),
    debugLines_(0),
    debugLinesNoDepth_(0)
{
    assert(renderer_->IsInitialized());
    sceneManager_ = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC, scene->Name().toStdString());
    if (!framework_->IsHeadless())
    {
        rayQuery_ = sceneManager_->createRayQuery(Ogre::Ray());
        rayQuery_->setQueryTypeMask(Ogre::SceneManager::FX_TYPE_MASK | Ogre::SceneManager::ENTITY_TYPE_MASK);
        rayQuery_->setSortByDistance(true);

        // If fog is FOG_NONE, force it to some default ineffective settings, because otherwise SuperShader shows just white
        if (sceneManager_->getFogMode() == Ogre::FOG_NONE)
            SetDefaultSceneFog();
        // Set a default ambient color that matches the default ambient color of EC_EnvironmentLight, in case there is no environmentlight component.
        sceneManager_->setAmbientLight(DefaultSceneAmbientLightColor());

        SetupShadows();

#include "DisableMemoryLeakCheck.h"
        debugLines_ = new DebugLines("PhysicsDebug");
        debugLinesNoDepth_ = new DebugLines("PhysicsDebugNoDepth");
#include "EnableMemoryLeakCheck.h"
        sceneManager_->getRootSceneNode()->attachObject(debugLines_);
        sceneManager_->getRootSceneNode()->attachObject(debugLinesNoDepth_);
        debugLinesNoDepth_->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
    }

    connect(framework_->Frame(), SIGNAL(Updated(float)), this, SLOT(OnUpdated(float)));
}

OgreWorld::~OgreWorld()
{
    if (rayQuery_)
        sceneManager_->destroyQuery(rayQuery_);
    
    if (debugLines_)
    {
        sceneManager_->getRootSceneNode()->detachObject(debugLines_);
        SAFE_DELETE(debugLines_);
    }
    if (debugLinesNoDepth_)
    {
        sceneManager_->getRootSceneNode()->detachObject(debugLinesNoDepth_);
        SAFE_DELETE(debugLinesNoDepth_);
    }
    
    // Remove all compositors.
    /// \todo This does not work with a proper multiscene approach
    OgreCompositionHandler* comp = renderer_->CompositionHandler();
    if (comp)
        comp->RemoveAllCompositors();
    
    Ogre::Root::getSingleton().destroySceneManager(sceneManager_);
}

std::string OgreWorld::GetUniqueObjectName(const std::string &prefix)
{
    return renderer_->GetUniqueObjectName(prefix);
}

void OgreWorld::FlushDebugGeometry()
{
    if (debugLines_)
        debugLines_->draw();
    if (debugLinesNoDepth_)
        debugLinesNoDepth_->draw();
}

Color OgreWorld::DefaultSceneAmbientLightColor()
{
    return Color(0.364f, 0.364f, 0.364f, 1.f);
}

void OgreWorld::SetDefaultSceneFog()
{
    if (sceneManager_)
    {
        sceneManager_->setFog(Ogre::FOG_LINEAR, Ogre::ColourValue::White, 0.001f, 2000.0f, 4000.0f);
        Renderer()->MainViewport()->setBackgroundColour(Color()); // Color default ctor == black
    }
}

RaycastResult* OgreWorld::Raycast(int x, int y)
{
    return Raycast(x, y, 0xffffffff);
}

RaycastResult* OgreWorld::Raycast(int x, int y, unsigned layerMask)
{
    PROFILE(OgreWorld_Raycast);
    
    result_.entity = 0;
    
    int width = renderer_->WindowWidth();
    int height = renderer_->WindowHeight();
    if (!width || !height)
        return &result_; // Headless
    Ogre::Camera* camera = VerifyCurrentSceneCamera();
    if (!camera)
        return &result_;
    
    float screenx = x / (float)width;
    float screeny = y / (float)height;

    Ogre::Ray ray = camera->getCameraToViewportRay(screenx, screeny);
    rayQuery_->setRay(ray);
    
    return RaycastInternal(layerMask);
}

RaycastResult* OgreWorld::Raycast(const Ray& ray, unsigned layerMask)
{
    if (!rayQuery_)
        return 0;
    rayQuery_->setRay(Ogre::Ray(ray.pos, ray.dir));
    return RaycastInternal(layerMask);
}

RaycastResult* OgreWorld::RaycastInternal(unsigned layerMask)
{
    result_.entity = 0;
    result_.component = 0;
    
    const Ogre::Ray& ogreRay = rayQuery_->getRay();
    Ray ray(ogreRay.getOrigin(), ogreRay.getDirection());
    
    Ogre::RaySceneQueryResult &results = rayQuery_->execute();
    float closestDistance = -1.0f;
    
    for(size_t i = 0; i < results.size(); ++i)
    {
        Ogre::RaySceneQueryResultEntry &entry = results[i];
    
        if (!entry.movable)
            continue;

        /// \todo Do we want results for invisible entities?
        if (!entry.movable->isVisible())
            continue;
        
        const Ogre::Any& any = entry.movable->getUserAny();
        if (any.isEmpty())
            continue;

        Entity *entity = 0;
        IComponent *component = 0;
        try
        {
            component = Ogre::any_cast<IComponent*>(any);
            entity = component ? component->ParentEntity() : 0;
        }
        catch(Ogre::InvalidParametersException &/*e*/)
        {
            continue;
        }
        
        EC_Placeable* placeable = entity->GetComponent<EC_Placeable>().get();
        if (placeable)
        {
            if (!(placeable->selectionLayer.Get() & layerMask))
                continue;
        }
        
        // If this MovableObject's bounding box is further away than our current best result, skip the detailed (e.g. triangle-level) test, as this object possibly can't be closer.
        if (closestDistance >= 0.0f && entry.distance > closestDistance)
            continue;

        Ogre::Entity* meshEntity = dynamic_cast<Ogre::Entity*>(entry.movable);
        if (meshEntity)
        {
            float meshClosestDistance;
            unsigned subMeshIndex;
            unsigned triangleIndex;
            float3 hitPoint;
            float3 normal;
            float2 uv;
            
            if (EC_Mesh::Raycast(meshEntity, ray, &meshClosestDistance, &subMeshIndex, &triangleIndex, &hitPoint, &normal, &uv))
            {
                if (closestDistance < 0.0f || meshClosestDistance < closestDistance)
                {
                    closestDistance = meshClosestDistance;
                    result_.entity = entity;
                    result_.component = component;
                    result_.pos = hitPoint;
                    result_.normal = normal;
                    result_.submesh = subMeshIndex;
                    result_.index = triangleIndex;
                    result_.u = uv.x;
                    result_.v = uv.y;
                }
            }
        }
        else
        {
            Ogre::BillboardSet *bbs = dynamic_cast<Ogre::BillboardSet*>(entry.movable);
            if (bbs)
            {
                float3x4 camWorldTransform = float4x4(renderer_->MainOgreCamera()->getParentSceneNode()->_getFullTransform()).Float3x4Part();

                float3 billboardFrontDir = camWorldTransform.Col(2).Normalized(); // The -direction this camera views in world space. (In Ogre, like common in OpenGL, cameras view towards -Z in their local space).
                float3 cameraUpDir = camWorldTransform.Col(1).Normalized(); // The direction of the up vector of the camera in world space.
                float3 cameraRightDir = camWorldTransform.Col(0).Normalized(); // The direction of the right vector of the camera in world space.

                Ogre::Matrix4 w_;
                bbs->getWorldTransforms(&w_);
                float3x4 world = float4x4(w_).Float3x4Part(); // The world transform of the whole billboard set.

                // Test a precise hit to each individual billboard in turn, and output the index of hit to the closest billboard in the set.
                for(int i = 0; i < bbs->getNumBillboards(); ++i)
                {
                    Ogre::Billboard *b = bbs->getBillboard(i);
                    float3 worldPos = world.MulPos(b->getPosition()); // A point on this billboard in world space. (@todo assuming this is the center point, but Ogre can use others!)
                    Plane billboardPlane(worldPos, billboardFrontDir); // The plane of this billboard in world space.
                    float d;
                    bool success = billboardPlane.Intersects(ray, &d);
                    if (!success || (closestDistance > 0.0f && d >= closestDistance))
                        continue;

                    float3 intersectionPoint = ray.GetPoint(d); // The point where the ray intersects the plane of the billboard.

                    // Compute the 3D world space -> local normalized 2D (x,y) coordinate frame mapping for this billboard.
                    float w = (b->hasOwnDimensions() ? b->getOwnWidth() : bbs->getDefaultWidth()) * world.Col(0).Length();
                    float h = (b->hasOwnDimensions() ? b->getOwnHeight() : bbs->getDefaultHeight()) * world.Col(1).Length();
                    float3x3 m(w*0.5f*cameraRightDir, h*0.5f*cameraUpDir, billboardFrontDir);
                    success = m.InverseColOrthogonal();
                    assume(success);

                    // Compute the 2D coordinates of the ray hit on the billboard plane.
                    const float3 hit = m * (intersectionPoint - worldPos);

                    /* Test code: To visualize the borders of the billboards, do this:
                    float3 tl = worldPos - w*0.5f*cameraRightDir + h*0.5f*cameraUpDir;
                    float3 tr = worldPos + w*0.5f*cameraRightDir + h*0.5f*cameraUpDir;
                    float3 bl = worldPos - w*0.5f*cameraRightDir - h*0.5f*cameraUpDir;
                    float3 br = worldPos + w*0.5f*cameraRightDir - h*0.5f*cameraUpDir;

                    DebugDrawLineSegment(LineSegment(tl, tr), 1,1,0);
                    DebugDrawLineSegment(LineSegment(tr, br), 1,0,0);
                    DebugDrawLineSegment(LineSegment(br, bl), 1,0,0);
                    DebugDrawLineSegment(LineSegment(bl, tl), 1,0,0); */

                    // The visible range of the billboard is normalized to [-1,1] in x & y. See if the hit is inside the billboard.
                    if (hit.x >= -1.f && hit.x <= 1.f && hit.y >= -1.f && hit.y <= 1.f)
                    {
                        closestDistance = d;
                        result_.entity = entity;
                        result_.component = component;
                        result_.pos = intersectionPoint;
                        result_.normal = billboardFrontDir;
                        result_.submesh = i; // Store in the 'submesh' index the index of the individual billboard we hit.
                        result_.index = (unsigned int)-1; // Not applicable for billboards.
                        result_.u = (hit.x + 1.f) * 0.5f;
                        result_.v = (hit.y + 1.f) * 0.5f;
                    }
                }
            }
            else
            {
                // Not a mesh entity, fall back to just using the bounding box - ray intersection
                if (closestDistance < 0.0f || entry.distance < closestDistance)
                {
                    closestDistance = entry.distance;
                    result_.entity = entity;
                    result_.component = component;
                    result_.pos = ogreRay.getPoint(closestDistance);
                    result_.normal = -ogreRay.getDirection();
                    result_.submesh = 0;
                    result_.index = 0;
                    result_.u = 0.0f;
                    result_.v = 0.0f;
                }
            }
        }
    }

    return &result_;
}

QList<Entity*> OgreWorld::FrustumQuery(QRect &viewrect) const
{
    PROFILE(OgreWorld_FrustumQuery);

    QList<Entity*>l;

    int width = renderer_->WindowWidth();
    int height = renderer_->WindowHeight();
    if (!width || !height)
        return l; // Headless
    Ogre::Camera* camera = VerifyCurrentSceneCamera();
    if (!camera)
        return l;

    float w= (float)width;
    float h= (float)height;
    float left = (float)(viewrect.left()) / w, right = (float)(viewrect.right()) / w;
    float top = (float)(viewrect.top()) / h, bottom = (float)(viewrect.bottom()) / h;
    
    if(left > right) std::swap(left, right);
    if(top > bottom) std::swap(top, bottom);
    // don't do selection box is too small
    if((right - left) * (bottom-top) < 0.0001) return l;
    
    Ogre::PlaneBoundedVolumeList volumes;
    Ogre::PlaneBoundedVolume p = camera->getCameraToViewportBoxVolume(left, top, right, bottom, true);
    volumes.push_back(p);

    Ogre::PlaneBoundedVolumeListSceneQuery *query = sceneManager_->createPlaneBoundedVolumeQuery(volumes);
    assert(query);

    Ogre::SceneQueryResult results = query->execute();
    for(Ogre::SceneQueryResultMovableList::iterator iter = results.movables.begin(); iter != results.movables.end(); ++iter)
    {
        Ogre::MovableObject *m = *iter;
        const Ogre::Any& any = m->getUserAny();
        if (any.isEmpty())
            continue;
        
        Entity *entity = 0;
        try
        {
            IComponent *component = Ogre::any_cast<IComponent*>(any);
            entity = component ? component->ParentEntity() : 0;
        }
        catch(Ogre::InvalidParametersException &/*e*/)
        {
            continue;
        }
        if(entity)
            l << entity;
    }

    sceneManager_->destroyQuery(query);

    return l;
}

bool OgreWorld::IsEntityVisible(Entity* entity) const
{
    EC_Camera* cameraComponent = VerifyCurrentSceneCameraComponent();
    if (cameraComponent)
        return cameraComponent->IsEntityVisible(entity);
    return false;
}

QList<Entity*> OgreWorld::VisibleEntities() const
{
    EC_Camera* cameraComponent = VerifyCurrentSceneCameraComponent();
    if (cameraComponent)
        return cameraComponent->VisibleEntities();
    return QList<Entity*>();
}

void OgreWorld::StartViewTracking(Entity* entity)
{
    if (!entity)
    {
        LogError("OgreWorld::StartViewTracking: null entity passed!");
        return;
    }

    EntityPtr entityPtr = entity->shared_from_this();
    for (unsigned i = 0; i < visibilityTrackedEntities_.size(); ++i)
    {
        if (visibilityTrackedEntities_[i].lock() == entityPtr)
            return; // Already added
    }
    
    visibilityTrackedEntities_.push_back(entity->shared_from_this());
}

void OgreWorld::StopViewTracking(Entity* entity)
{
    if (!entity)
    {
        LogError("OgreWorld::StopViewTracking: null entity passed!");
        return;
    }

    EntityPtr entityPtr = entity->shared_from_this();
    for (unsigned i = 0; i < visibilityTrackedEntities_.size(); ++i)
    {
        if (visibilityTrackedEntities_[i].lock() == entityPtr)
        {
            visibilityTrackedEntities_.erase(visibilityTrackedEntities_.begin() + i);
            return;
        }
    }
}

void OgreWorld::OnUpdated(float timeStep)
{
    PROFILE(OgreWorld_OnUpdated);
    // Do nothing if visibility not being tracked for any entities
    if (visibilityTrackedEntities_.empty())
    {
        if (!lastVisibleEntities_.empty())
            lastVisibleEntities_.clear();
        if (!visibleEntities_.empty())
            visibleEntities_.clear();
        return;
    }
    
    // Update visible objects from the active camera
    lastVisibleEntities_ = visibleEntities_;
    EC_Camera* activeCamera = VerifyCurrentSceneCameraComponent();
    if (activeCamera)
        visibleEntities_ = activeCamera->VisibleEntityIDs();
    else
        visibleEntities_.clear();
    
    for (unsigned i = visibilityTrackedEntities_.size() - 1; i < visibilityTrackedEntities_.size(); --i)
    {
        // Check if the entity has expired; erase from list in that case
        if (visibilityTrackedEntities_[i].expired())
            visibilityTrackedEntities_.erase(visibilityTrackedEntities_.begin() + i);
        else
        {
            Entity* entity = visibilityTrackedEntities_[i].lock().get();
            entity_id_t id = entity->Id();
            
            // Check for change in visibility status
            bool last = lastVisibleEntities_.find(id) != lastVisibleEntities_.end();
            bool now = visibleEntities_.find(id) != visibleEntities_.end();
            if (!last && now)
            {
                emit EntityEnterView(entity);
                entity->EmitEnterView(activeCamera);
            }
            else if (last && !now)
            {
                emit EntityLeaveView(entity);
                entity->EmitLeaveView(activeCamera);
            }
        }
    }
}

void OgreWorld::SetupShadows()
{
    Ogre::SceneManager* sceneManager = sceneManager_;
    // Debug mode Ogre might assert due to illegal shadow camera AABB, with empty scene. Disable shadows in debug mode.
#ifdef _DEBUG
        sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_NONE);
        return;
#else
    OgreRenderer::Renderer::ShadowQualitySetting quality = renderer_->ShadowQuality();
    bool using_pssm = (quality == OgreRenderer::Renderer::Shadows_High);
    bool soft_shadow = framework_->Config()->Get(ConfigAPI::FILE_FRAMEWORK, ConfigAPI::SECTION_RENDERING, "soft shadow").toBool();
    
    //unsigned short shadowTextureSize = settings.value("depthmap_size", "1024").toInt();  */
    float shadowFarDist = 50;
    unsigned short shadowTextureSize = 2048;
    unsigned short shadowTextureCount = 1;
    
    if(using_pssm)
    {
        shadowTextureSize = 1024;
        shadowTextureCount = 3;
    }
    
    Ogre::ColourValue shadowColor(0.6f, 0.6f, 0.6f);

    // This is the default material to use for shadow buffer rendering pass, overridable in script.
    // Note that we use the same single material (vertex program) for each object, so we're relying on
    // that we use Ogre software skinning. Hardware skinning would require us to do different vertex programs
    // for skinned/nonskinned geometry.
    std::string ogreShadowCasterMaterial = "rex/ShadowCaster";
    
    if (quality == OgreRenderer::Renderer::Shadows_Off)
    {
        sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_NONE);
        return;
    }
    
    sceneManager->setShadowColour(shadowColor);
    sceneManager->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, shadowTextureCount);
    sceneManager->setShadowTextureSettings(shadowTextureSize, shadowTextureCount, Ogre::PF_FLOAT32_R);
    sceneManager->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
    sceneManager->setShadowTextureCasterMaterial(ogreShadowCasterMaterial.c_str());
    sceneManager->setShadowTextureSelfShadow(true);
    
    Ogre::ShadowCameraSetupPtr shadowCameraSetup;
    
    if(using_pssm)
    {
#include "DisableMemoryLeakCheck.h"
        OgreShadowCameraSetupFocusedPSSM* pssmSetup = new OgreShadowCameraSetupFocusedPSSM();
#include "EnableMemoryLeakCheck.h"

        OgreShadowCameraSetupFocusedPSSM::SplitPointList splitpoints;
        splitpoints.push_back(0.1f); // Default nearclip
        //these splitpoints are hardcoded also to the shaders. If you modify these, also change them to shaders.
        splitpoints.push_back(3.5);
        splitpoints.push_back(11);
        splitpoints.push_back(shadowFarDist);
        pssmSetup->setSplitPoints(splitpoints);
        shadowCameraSetup = Ogre::ShadowCameraSetupPtr(pssmSetup);
    }
    else
    {
#include "DisableMemoryLeakCheck.h"
        Ogre::FocusedShadowCameraSetup* focusedSetup = new Ogre::FocusedShadowCameraSetup();
#include "EnableMemoryLeakCheck.h"
        shadowCameraSetup = Ogre::ShadowCameraSetupPtr(focusedSetup);
    }
    
    sceneManager->setShadowCameraSetup(shadowCameraSetup);
    sceneManager->setShadowFarDistance(shadowFarDist);
    
    // If set to true, problems with objects that clip into the ground
    sceneManager->setShadowCasterRenderBackFaces(false);
    
    //DEBUG
    /*if(renderer_.expired())
        return;
    Ogre::SceneManager *mngr = renderer_.lock()->OgreSceneManager();
    Ogre::TexturePtr shadowTex;
    Ogre::String str("shadowDebug");
    Ogre::Overlay* debugOverlay = Ogre::OverlayManager::getSingleton().getByName(str);
    if(!debugOverlay)
        debugOverlay= Ogre::OverlayManager::getSingleton().create(str);
    for(int i = 0; i<shadowTextureCount;i++)
    {
            shadowTex = mngr->getShadowTexture(i);

            // Set up a debug panel to display the shadow
            Ogre::MaterialPtr debugMat = Ogre::MaterialManager::getSingleton().create(
                "Ogre/DebugTexture" + Ogre::StringConverter::toString(i), 
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            debugMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
            Ogre::TextureUnitState *t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState(shadowTex->getName());
            t->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
            //t = debugMat->getTechnique(0)->getPass(0)->createTextureUnitState("spot_shadow_fade.png");
            //t->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
            //t->setColourOperation(LBO_ADD);

            Ogre::OverlayContainer* debugPanel = (Ogre::OverlayContainer*)
                (Ogre::OverlayManager::getSingleton().createOverlayElement("Panel", "Ogre/DebugTexPanel" + Ogre::StringConverter::toString(i)));
            debugPanel->_setPosition(0.8, i*0.25+ 0.05);
            debugPanel->_setDimensions(0.2, 0.24);
            debugPanel->setMaterialName(debugMat->getName());
            debugOverlay->add2D(debugPanel);
    }
    debugOverlay->show();*/

    if(soft_shadow)
    {
        for(size_t i=0;i<shadowTextureCount;i++)
        {
            ::GaussianListener* gaussianListener = new GaussianListener();
            Ogre::TexturePtr shadowTex = sceneManager->getShadowTexture(0);
            Ogre::RenderTarget* shadowRtt = shadowTex->getBuffer()->getRenderTarget();
            Ogre::Viewport* vp = shadowRtt->getViewport(0);
            Ogre::CompositorInstance *instance = Ogre::CompositorManager::getSingleton().addCompositor(vp, "Gaussian Blur");
            Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp, "Gaussian Blur", true);
            instance->addListener(gaussianListener);
            gaussianListener->notifyViewportSize(vp->getActualWidth(), vp->getActualHeight());
            gaussianListeners_.push_back(gaussianListener);
        }
    }
#endif
}

Ogre::Camera* OgreWorld::VerifyCurrentSceneCamera() const
{
    EC_Camera* cameraComponent = VerifyCurrentSceneCameraComponent();
    return cameraComponent ? cameraComponent->GetCamera() : 0;
}

EC_Camera* OgreWorld::VerifyCurrentSceneCameraComponent() const
{
    if (!renderer_)
        return 0;
    Entity *mainCamera = renderer_->MainCamera();
    if (!mainCamera)
        return 0;
    EC_Camera *cameraComponent = dynamic_cast<EC_Camera*>(mainCamera->GetComponent<EC_Camera>().get());
    if (!cameraComponent)
        return 0;
    Entity* entity = cameraComponent->ParentEntity();
    if (!entity || entity->ParentScene() != scene_.lock().get())
        return 0;
    
    return cameraComponent;
}

bool OgreWorld::IsActive() const
{
    return VerifyCurrentSceneCamera() != 0;
}

void OgreWorld::DebugDrawAABB(const AABB &aabb, float r, float g, float b, bool depthTest)
{
    for(int i = 0; i < 12; ++i)
        DebugDrawLineSegment(aabb.Edge(i), r, g, b, depthTest);
}

void OgreWorld::DebugDrawOBB(const OBB &obb, float r, float g, float b, bool depthTest)
{
    for(int i = 0; i < 12; ++i)
        DebugDrawLineSegment(obb.Edge(i), r, g, b, depthTest);
}

void OgreWorld::DebugDrawLineSegment(const LineSegment &l, float r, float g, float b, bool depthTest)
{
    if (depthTest)
    {
        if (debugLines_)
            debugLines_->addLine(l.a, l.b, float3(r,g,b));
    }
    else
    {
        if (debugLinesNoDepth_)
            debugLinesNoDepth_->addLine(l.a, l.b, float3(r,g,b));
    }
}

void OgreWorld::DebugDrawLine(const float3& start, const float3& end, float r, float g, float b, bool depthTest)
{
    if (depthTest)
    {
        if (debugLines_)
            debugLines_->addLine(start, end, float3(r,g,b));
    }
    else
    {
        if (debugLinesNoDepth_)
            debugLinesNoDepth_->addLine(start, end, float3(r,g,b));
    }
}

void OgreWorld::DebugDrawPlane(const Plane &plane, float r, float g, float b, const float3 &refPoint, float uSpacing, float vSpacing, 
                               int uSegments, int vSegments, bool depthTest)
{
    float U0 = -uSegments * uSpacing / 2.f;
    float V0 = -vSegments * vSpacing / 2.f;

    float U1 = uSegments * uSpacing / 2.f;
    float V1 = vSegments * vSpacing / 2.f;

    for(int y = 0; y < vSegments; ++y)
        for(int x = 0; x < uSegments; ++x)
        {
            float u = U0 + x * uSpacing;
            float v = V0 + y * vSpacing;
            DebugDrawLine(plane.Point(U0, v, refPoint), plane.Point(U1, v, refPoint), r, g, b, depthTest);
            DebugDrawLine(plane.Point(u, V0, refPoint), plane.Point(u, V1, refPoint), r, g, b, depthTest);
        }
}

void OgreWorld::DebugDrawTransform(const Transform &t, float axisLength, float boxSize, float r, float g, float b, bool depthTest)
{
    DebugDrawFloat3x4(t.ToFloat3x4(), axisLength, boxSize, r, g, b, depthTest);
}

void OgreWorld::DebugDrawFloat3x4(const float3x4 &t, float axisLength, float boxSize, float r, float g, float b, bool depthTest)
{
    AABB aabb(float3::FromScalar(-boxSize/2.f), float3::FromScalar(boxSize/2.f));
    OBB obb = aabb.Transform(t);
    DebugDrawOBB(obb, r, g, b);
    DebugDrawLineSegment(LineSegment(t.TranslatePart(), t.TranslatePart() + axisLength * t.Col(0)), 1, 0, 0, depthTest);
    DebugDrawLineSegment(LineSegment(t.TranslatePart(), t.TranslatePart() + axisLength * t.Col(1)), 0, 1, 0, depthTest);
    DebugDrawLineSegment(LineSegment(t.TranslatePart(), t.TranslatePart() + axisLength * t.Col(2)), 0, 0, 1, depthTest);
}

void OgreWorld::DebugDrawCircle(const Circle &c, int numSubdivisions, float r, float g, float b, bool depthTest)
{
    float3 p = c.GetPoint(0);
    for(int i = 1; i <= numSubdivisions; ++i)
    {
        float3 p2 = c.GetPoint(i * 2.f * 3.14f / numSubdivisions);
        DebugDrawLineSegment(LineSegment(p, p2), r, g, b, depthTest);
        p = p2;
    }
}

void OgreWorld::DebugDrawAxes(const float3x4 &t, bool depthTest)
{
    float3 translate, scale;
    Quat rotate;
    t.Decompose(translate, rotate, scale);
    
    DebugDrawLine(translate, translate + rotate * float3(scale.x, 0.f, 0.f), 1, 0, 0, depthTest);
    DebugDrawLine(translate, translate + rotate * float3(0., scale.y, 0.f), 0, 1, 0, depthTest);
    DebugDrawLine(translate, translate + rotate * float3(0.f, 0.f, scale.z), 0, 0, 1, depthTest);
}

void OgreWorld::DebugDrawSphere(const float3& center, float radius, int vertices, float r, float g, float b, bool depthTest)
{
    if (vertices <= 0)
        return;
    
    std::vector<float3> positions(vertices);

    Sphere sphere(center, radius);
    int actualVertices = sphere.Triangulate(&positions[0], 0, 0, vertices);
    for (int i = 0; i < actualVertices; i += 3)
    {
        DebugDrawLine(positions[i], positions[i + 1], r, g, b, depthTest);
        DebugDrawLine(positions[i + 1], positions[i + 2], r, g, b, depthTest);
        DebugDrawLine(positions[i + 2], positions[i], r, g, b, depthTest);
    }
}

void OgreWorld::DebugDrawLight(const float3x4 &t, int lightType, float range, float spotAngle, float r, float g, float b, bool depthTest)
{
    float3 translate, scale;
    Quat rotate;
    t.Decompose(translate, rotate, scale);
    float3 lightDirection = rotate * float3(0.0f, 0.0f, 1.0f);
    switch (lightType)
    {
        // Point
    case 0:
        DebugDrawCircle(Circle(translate, float3(1.f, 0.f, 0.f), range), 8, r, g, b, depthTest);
        DebugDrawCircle(Circle(translate, float3(0.f, 1.f, 0.f), range), 8, r, g, b, depthTest);
        DebugDrawCircle(Circle(translate, float3(0.f, 0.f, 1.f), range), 8, r, g, b, depthTest);
        break;
        
        // Spot
    case 1:
        {
            float3 endPoint = translate + range * lightDirection;
            float coneRadius = range * sinf(DegToRad(spotAngle));
            Circle spotCircle(endPoint, -lightDirection, coneRadius);
            
            DebugDrawCircle(Circle(endPoint, -lightDirection, coneRadius), 8, r, g, b, depthTest);
            for (int i = 1; i <= 8; ++i)
                DebugDrawLine(translate, spotCircle.GetPoint(i * 2.f * 3.14f / 8), r, g, b, depthTest);
        }
        break;
        
        // Directional
    case 2:
        {
            const float cDirLightRange = 10.f;
            float3 endPoint = translate + cDirLightRange * lightDirection;
            float3 offset = rotate * float3(1.f, 0.f, 0.f);
            DebugDrawLine(translate, endPoint, r, g, b, depthTest);
            DebugDrawLine(translate + offset, endPoint + offset, r, g, b, depthTest);
            DebugDrawLine(translate - offset, endPoint - offset, r, g, b, depthTest);
        }
        break;
    }
}

void OgreWorld::DebugDrawCamera(const float3x4 &t, float size, float r, float g, float b, bool depthTest)
{
    AABB aabb(float3(-size/2.f, -size/2.f, -size), float3(size/2.f, size/2.f, size));
    OBB obb = aabb.Transform(t);
    DebugDrawOBB(obb, r, g, b, depthTest);
    
    float3 translate(0, 0, -size * 1.25f);
    AABB aabb2(translate + float3::FromScalar(-size/4.f), translate + float3::FromScalar(size/4.f));
    OBB obb2 = aabb2.Transform(t);
    DebugDrawOBB(obb2, r, g, b, depthTest);
}

void OgreWorld::DebugDrawSoundSource(const float3 &soundPos, float soundInnerRadius, float soundOuterRadius, float r, float g, float b, bool depthTest)
{
    // Draw three concentric diamonds as a visual cue
    for(int i = 2; i < 5; ++i)
        DebugDrawSphere(soundPos, i/3.f, 24, i==2?1:0, i==3?1:0, i==4?1:0, depthTest);

    DebugDrawSphere(soundPos, soundInnerRadius, 24*3*3*3, 1, 0, 0, depthTest);
    DebugDrawSphere(soundPos, soundOuterRadius, 24*3*3*3, 0, 1, 0, depthTest);
}
