// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "MemoryLeakCheck.h"
#include "OgreShadowCameraSetupFocusedPSSM.h"

OgreShadowCameraSetupFocusedPSSM::OgreShadowCameraSetupFocusedPSSM() : mSplitPadding(1.0f)
{
    calculateSplitPoints(3, 100, 100000);
}

OgreShadowCameraSetupFocusedPSSM::~OgreShadowCameraSetupFocusedPSSM()
{
}

void OgreShadowCameraSetupFocusedPSSM::calculateSplitPoints(size_t splitCount, float nearDist, float farDist, float lambda)
{
    if (splitCount < 2)
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Cannot specify less than 2 splits", 
        "PSSMShadowCameraSetup::calculateSplitPoints");

    mSplitPoints.resize(splitCount + 1);
    mOptimalAdjustFactors.resize(splitCount);
    mSplitCount = splitCount;

    mSplitPoints[0] = nearDist;
    for(size_t i = 1; i < mSplitCount; i++)
    {
        float fraction = (float)i / (float)mSplitCount;
        float splitPoint = lambda * nearDist * Ogre::Math::Pow(farDist / nearDist, fraction) +
            (1.0 - lambda) * (nearDist + fraction * (farDist - nearDist));

        mSplitPoints[i] = splitPoint;
    }
    mSplitPoints[splitCount] = farDist;
}

void OgreShadowCameraSetupFocusedPSSM::setSplitPoints(const SplitPointList& newSplitPoints)
{
    if (newSplitPoints.size() < 3) // 3, not 2 since splits + 1 points
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS, "Cannot specify less than 2 splits", 
        "PSSMShadowCameraSetup::setSplitPoints");
    mSplitCount = newSplitPoints.size() - 1;
    mSplitPoints = newSplitPoints;
    mOptimalAdjustFactors.resize(mSplitCount);
}

float OgreShadowCameraSetupFocusedPSSM::getOptimalAdjustFactor() const
{
    // simplifies the overriding of the LiSPSM opt adjust factor use
    return mOptimalAdjustFactors[mCurrentIteration];
}

void OgreShadowCameraSetupFocusedPSSM::getShadowCamera(const Ogre::SceneManager *sm, const Ogre::Camera *cam,const Ogre::Viewport *vp, const Ogre::Light *light, Ogre::Camera *texCam, size_t iteration) const
{
    // apply the right clip distance.
    float nearDist = mSplitPoints[iteration];
    float farDist = mSplitPoints[iteration + 1];

    // Add a padding factor to internal distances so that the connecting split point will not have bad artifacts.
    if (iteration > 0)
        nearDist -= mSplitPadding;
    if (iteration < mSplitCount - 1)
        farDist += mSplitPadding;

    mCurrentIteration = iteration;

    // Ouch, I know this is hacky, but it's the easiest way to re-use LiSPSM / Focussed
    // functionality right now without major changes
    Ogre::Camera* _cam = const_cast<Ogre::Camera*>(cam);
    float oldNear = _cam->getNearClipDistance();
    float oldFar = _cam->getFarClipDistance();
    _cam->setNearClipDistance(nearDist);
    _cam->setFarClipDistance(farDist);

    Ogre::FocusedShadowCameraSetup::getShadowCamera(sm, cam, vp, light, texCam, iteration);

    // restore near/far
    _cam->setNearClipDistance(oldNear);
    _cam->setFarClipDistance(oldFar);
}
