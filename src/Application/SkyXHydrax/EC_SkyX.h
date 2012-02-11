/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   EC_SkyX.h
    @brief  A sky component using SkyX, http://www.ogre3d.org/tikiwiki/SkyX */

#pragma once

#include "IComponent.h"
#include "Math/float3.h"

namespace Ogre { class Camera; }
struct EC_SkyXImpl;

/// A Sky component using SkyX, http://www.ogre3d.org/tikiwiki/SkyX
/** This is a singleton type component and only one component per scene is allowed.
    Provides means of creating photorealistic environments together with EC_Hydrax.
    @note Requires SkyX Ogre add-on. */
class EC_SkyX : public IComponent
{
    Q_OBJECT
    COMPONENT_NAME("EC_SkyX", 38)

public:
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit EC_SkyX(Scene* scene);
    ~EC_SkyX();

    /// Different cloud types supported by SkyX
    enum CloudType
    {
        None, ///< Disabled.
        Normal, ///< Cloud layer at fixed height above camera.
        Volumetric, ///< Volumetric clouds.
    };

    /// Used cloud type, see CloudType.
    DEFINE_QPROPERTY_ATTRIBUTE(int, cloudType);
    Q_PROPERTY(int cloudType READ getcloudType WRITE setcloudType);

    /// The time multiplier can be a also a negative number, 0 will disable auto-updating.
    DEFINE_QPROPERTY_ATTRIBUTE(float, timeMultiplier);
    Q_PROPERTY(float timeMultiplier READ gettimeMultiplier WRITE settimeMultiplier);

    /// Time of day in [0,24]h range, 
    DEFINE_QPROPERTY_ATTRIBUTE(float, time);
    Q_PROPERTY(float time READ gettime WRITE settime);

    /// Sunrise time in [0,24]h range
    DEFINE_QPROPERTY_ATTRIBUTE(float, sunriseTime);
    Q_PROPERTY(float sunriseTime READ getsunriseTime WRITE setsunriseTime);

    /// Sunset time in [0,24]h range.
    DEFINE_QPROPERTY_ATTRIBUTE(float, sunsetTime);
    Q_PROPERTY(float sunsetTime READ getsunsetTime WRITE setsunsetTime);

    /// Cloud coverage with range [0,100]. (Volumetric clouds only)
    DEFINE_QPROPERTY_ATTRIBUTE(float, cloudCoverage);
    Q_PROPERTY(float cloudCoverage READ getcloudCoverage WRITE setcloudCoverage);

    /// Average cloud size with range [0,100]. (Volumetric clouds only)
    DEFINE_QPROPERTY_ATTRIBUTE(float, cloudAverageSize);
    Q_PROPERTY(float cloudAverageSize READ getcloudAverageSize WRITE setcloudAverageSize);

    /// The height at the clouds will reside.
    DEFINE_QPROPERTY_ATTRIBUTE(float, cloudHeight);
    Q_PROPERTY(float cloudHeight READ getcloudHeight WRITE setcloudHeight);

    /// Moon phase with range [0,100] where 0 means fully covered moon, 50 clear moon and 100 fully covered moon.
    DEFINE_QPROPERTY_ATTRIBUTE(float, moonPhase);
    Q_PROPERTY(float moonPhase READ getmoonPhase WRITE setmoonPhase);

    /// Sun inner radius.
    DEFINE_QPROPERTY_ATTRIBUTE(float, sunInnerRadius);
    Q_PROPERTY(float sunInnerRadius READ getsunInnerRadius WRITE setsunInnerRadius);

    /// Sun outer radius.
    DEFINE_QPROPERTY_ATTRIBUTE(float, sunOuterRadius);
    Q_PROPERTY(float sunOuterRadius READ getsunOuterRadius WRITE setsunOuterRadius);

    /// Wind direction, in degrees.
    DEFINE_QPROPERTY_ATTRIBUTE(float, windDirection);
    Q_PROPERTY(float windDirection READ getwindDirection WRITE setwindDirection);

    /// Wind speed. Might need different value with normal versus volumetric clouds to actually get same speed.
    DEFINE_QPROPERTY_ATTRIBUTE(float, windSpeed);
    Q_PROPERTY(float windSpeed READ getwindSpeed WRITE setwindSpeed);

public slots:
    /// Returns whether or not the sun is visible (above horizon).
    bool IsSunVisible() const;

    /// Returns position of the sun.
    float3 SunPosition() const;

    /// Returns whether or not the moon is visible (above horizon).
    bool IsMoonVisible() const;

    /// Returns position of the moon.
    float3 MoonPosition() const;

private slots:
    void Create();
    void OnActiveCameraChanged(Entity *camEntity);
    void UpdateAttribute(IAttribute *attr, AttributeChange::Type change);
    void Update(float frameTime);

private:
    EC_SkyXImpl *impl;

    void Remove();
    void CreateLights();
    void RegisterListeners();
    void UnregisterListeners();

    // VCloudManager register/unregister functions.
    // If input camera is null, Tundras active camera is used.
    void RegisterCamera(Ogre::Camera *camera = 0);
    void UnregisterCamera(Ogre::Camera *camera = 0);
    void HandleVCloudsCamera(Ogre::Camera *camera, bool registerCamera);
    void ApplyAtmosphereOptions();
    void UnloadNormalClouds();
    void UnloadVolumetricClouds();
};
