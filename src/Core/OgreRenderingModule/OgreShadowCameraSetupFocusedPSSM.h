// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

/// PSSM focused shadow camera setup.
/** Mostly copy-paste from Ogre's PSSMShadowCameraSetup, the difference is that we are not using LiSPSM.
    @cond PRIVATE */
class OgreShadowCameraSetupFocusedPSSM: public Ogre::FocusedShadowCameraSetup
{
public:
    typedef std::vector<float> SplitPointList;
    typedef std::vector<float> OptimalAdjustFactorList;

    /// Constructor, defaults to 3 splits
    OgreShadowCameraSetupFocusedPSSM();

    /// Destructor.
    ~OgreShadowCameraSetupFocusedPSSM();

    /// Calculate a new splitting scheme.
    /** @param splitCount The number of splits to use
        @param nearDist The near plane to use for the first split
        @param farDist The far plane to use for the last split
        @param lambda Factor to use to reduce the split size */
    void calculateSplitPoints(size_t splitCount, float nearDist, float farDist, float lambda = 0.95);

    /// Manually configure a new splitting scheme.
    /** @param newSplitPoints A list which is splitCount + 1 entries long, containing the
        split points. The first value is the near point, the last value is the
        far point, and each value in between is both a far point of the previous
        split, and a near point for the next one. */
    void setSplitPoints(const SplitPointList& newSplitPoints);

    /// Set the padding factor to apply to the near & far distances when matching up splits to one another, to avoid 'cracks'.
    void setSplitPadding(float pad) { mSplitPadding = pad; }

    /// Get the padding factor to apply to the near & far distances when matching up splits to one another, to avoid 'cracks'.
    float getSplitPadding() const { return mSplitPadding; }

    /// Get the number of splits. 
    size_t getSplitCount() const { return mSplitCount; }

    /// Returns a shadow camera with PSSM splits base on iteration.
    virtual void getShadowCamera(const Ogre::SceneManager *sm, const Ogre::Camera *cam,const Ogre::Viewport *vp, const Ogre::Light *light, Ogre::Camera *texCam, size_t iteration) const;

    /// Returns the calculated split points.
    inline const SplitPointList& getSplitPoints() const { return mSplitPoints; }

    /// Returns the optimal adjust factor for a given split.
    inline float getOptimalAdjustFactor(size_t splitIndex) const { return mOptimalAdjustFactors[splitIndex]; }

    /// Overridden, recommended internal use only since depends on current iteration
    float getOptimalAdjustFactor() const;

protected:
    size_t mSplitCount;
    SplitPointList mSplitPoints;
    OptimalAdjustFactorList mOptimalAdjustFactors;
    float mSplitPadding;
    mutable size_t mCurrentIteration;
};
/** @endcond */
