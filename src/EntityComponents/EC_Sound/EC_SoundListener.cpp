/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   EC_SoundListener.cpp
    @brief  Entity-component which provides sound listener position for in-world 3D audio. */

#include "DebugOperatorNew.h"
#include "EC_SoundListener.h"
#include "Entity.h"
#include "EC_Placeable.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "AudioAPI.h"
#include "SceneAPI.h"
#include "FrameAPI.h"
#include "Framework.h"
#include "Profiler.h"
#include "MemoryLeakCheck.h"

EC_SoundListener::EC_SoundListener(Scene* scene):
    IComponent(scene),
    INIT_ATTRIBUTE_VALUE(active, "Active", false)
{
    // By default, this component is NOT network-replicated
    SetReplicated(false);

    connect(this, SIGNAL(ParentEntitySet()), SLOT(Initialize()));
}

EC_SoundListener::~EC_SoundListener()
{
}

void EC_SoundListener::Initialize()
{
    RetrievePlaceable();
    connect(framework->Frame(), SIGNAL(Updated(float)), SLOT(Update()));
    RegisterActions();
}

void EC_SoundListener::RetrievePlaceable()
{
    if (!ParentEntity())
        LogError("EC_SoundListener::RetrievePlaceable: Couldn't find an parent entity for EC_SoundListener. Cannot retrieve placeable component.");

    placeable_ = ParentEntity()->GetComponent<EC_Placeable>();
    if (placeable_.expired())
        LogError("EC_SoundListener::RetrievePlaceable: Couldn't find an EC_Placeable component from the parent entity.");
}

void EC_SoundListener::Update()
{
    if (active.Get() && !placeable_.expired())
    {
        PROFILE(EC_SoundListener_Update);
        GetFramework()->Audio()->SetListener(placeable_.lock()->WorldPosition(), placeable_.lock()->WorldOrientation());
    }
}

void EC_SoundListener::AttributesChanged()
{
    if (active.Get())
    {
        Scene* scene = ParentScene();
        if (!scene)
            return;
        // Disable all the other listeners, only one can be active at a time.
        foreach(const shared_ptr<EC_SoundListener> &listener, scene->Components<EC_SoundListener>())
            if (listener.get() != this)
                listener->active.Set(false, AttributeChange::Default);
    }
}

void EC_SoundListener::RegisterActions()
{
    Entity *entity = ParentEntity();
    assert(entity);
    if (entity)
        entity->ConnectAction("Active", this, SLOT(AttributesChanged()));
    else
        LogError("EC_SoundListener::RegisterActions: Failed to register actions because component's parent entity is null.");
}
