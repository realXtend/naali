/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   EC_SkyX.h
 *  @brief  A sky component using SkyX, http://www.ogre3d.org/tikiwiki/SkyX
 */

#pragma once

#include "IComponent.h"
#include "Math/float3.h"
#include "Math/float2.h"

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

    /// Do we want to use volumetric clouds.
    DEFINE_QPROPERTY_ATTRIBUTE(bool, volumetricClouds);
    Q_PROPERTY(bool volumetricClouds READ getvolumetricClouds WRITE setvolumetricClouds);

    /// The time multiplier can be a also a negative number, 0 will disable auto-updating.
    DEFINE_QPROPERTY_ATTRIBUTE(float, timeMultiplier);
    Q_PROPERTY(float timeMultiplier READ gettimeMultiplier WRITE settimeMultiplier);

    /// Time of the day: x = time in [0, 24]h range, y = sunrise hour in [0, 24]h range, z = sunset hour in [0, 24] range.
    DEFINE_QPROPERTY_ATTRIBUTE(float3, time);
    Q_PROPERTY(float3 time READ gettime WRITE settime);

    /// Weather (Volumetric clouds only): x = humidity i.e. percentage of clouds coverage [0,1], y = average cloud size [0,1].
    DEFINE_QPROPERTY_ATTRIBUTE(float2, weather);
    Q_PROPERTY(float2 weather READ getweather WRITE setweather);

    /// Wind direction, in degrees.
    DEFINE_QPROPERTY_ATTRIBUTE(float, windDirection);
    Q_PROPERTY(float windDirection READ getwindDirection WRITE setwindDirection);

    /// Wind speed, volumetric clouds only.
//    DEFINE_QPROPERTY_ATTRIBUTE(float, windSpeed);
//    Q_PROPERTY(float windSpeed READ getwindSpeed WRITE setwindSpeed);

    /// The height at the clouds will reside.
    /** For regular clouds this is always relative fixed offset from the active camera along the world up-axis.
        For volumetric clouds x denotes absolute cloud field y-coord start height and y is cloud field volume height (both in world coordinates). */
//    DEFINE_QPROPERTY_ATTRIBUTE(float, cloudHeight);
//    Q_PROPERTY(float cloudHeight READ getcloudHeight WRITE setcloudHeight);

public slots:
    /// Returns position of the sun.
    float3 SunPosition() const;

private:
    EC_SkyXImpl *impl;

private slots:
    void Create();
    void CreateSunlight();

    /// Called when the main view active camera has changed.
    void OnActiveCameraChanged(Entity *newActiveCamera);
    void UpdateAttribute(IAttribute *attr);
    void Update(float frameTime);
};
