// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "OgreModuleApi.h"
#include "OgreModuleFwd.h"
#include "CoreStringUtils.h"

#include <OgreAnimationState.h>

/// Ogre-specific mesh entity animation controller
/**
<table class="header">
<tr>
<td>
<h2>AnimationController</h2>
Ogre-specific mesh entity animation controller

Needs to be told of an OgreMesh component to be usable

Registered by OgreRenderer::OgreRenderingModule.

<b>Attributes</b>:
<ul>
<li>QString: animationState
<div>A freedata string that logic script(s) can use to sync animation state over network. Not interpreted by the AnimationController itself.</div>
<li>int: contextPriority.
<div>This input mapper's input context priority.</div> 
<li>bool: takeKeyboardEventsOverQt.
<div>Does the mapper receive keyboard input events even when a Qt widget has focus.</div> 
<li>bool: takeMouseEventsOverQt.
<div>Does the mapper receive mouse input events even when a Qt widget has focus.</div>
<li>int: executionType.
<div>Action execution type that is used for the Entity Actions.</div>
<li>bool: modifiersEnabled.
<div>Whether modifiers are checked for in the key events. Default true.</div>
</ul>

<b>Exposes the following scriptable functions:</b>
<ul>
<li>"EnableAnimation": Enables animation with optional fade-in time
        @param name Animation name
        @param looped Is animation looped
        @param fadein Animation fadein time, 0 = instant
        @param high_priority Whether animation uses high-priority blending (overrides and reduces weight of low-priority animations)
        @return true if animation exists and was enabled.
<li>"EnableExclusiveAnimation": Enables an exclusive animation. 
        This means that other animations will start fading out with the fade-out time specified
        @param name Animation name
        @param looped Is animation looped
        @param fadein Animation fadein time, 0 = instant
        @param fadeout Other animations' fadeout time, 0 = instant
        @param high_priority Whether animation uses high-priority blending (overrides and reduces weight of low-priority animations)
        @return true if animation exists and could be enabled
<li>"HasAnimationFinished": Checks whether non-looping animation has finished. If looping, returns always false
        @return true if animation finished
<li>"IsAnimationActive": Checks whether animation is active
        @param name Animation name
        @param check_fade_out if true, also fade-out (until totally faded) phase is interpreted as "active"
        @return true if animation active
<li>"DisableAnimation": Disables animation with optional fade-out time
        @param name Animation name
        @param fadeout Animation fadeout time, 0 = instant
        @return true if animation exists and could be disabled
<li>"DisableAllAnimations": Disables all animations with the same fadeout time
        @param fadeout Animation fadeout time
<li>"SetAnimationToEnd": Forwards animation to end, useful if animation is played in reverse
        @param name Animation name
<li>"SetAnimationSpeed": Sets relative speed of an active animation.
    Note that this speed will be forgotten if the animation is disabled
        @param name Animation name
        @param speedfactor Relative speed. 1 is default. Use negative speed to play in reverse
        @return true if successful (animation was currently playing)
<li>"SetAnimationWeight": Changes weight of an active animation (default 1.0). Return false if the animation doesn't exist or isn't active
        @param name Animation name
        @param weight Animation weight (from 0 to 1)
        @return true if successful (animation was currently playing)
<li>"SetAnimationPriority": Changes animation priority.
        Can lead to fun visual effects, but provided for completeness
        @param name Animation name
        @param high_priority High priority bit
        @return true if successful (animation was currently playing)
<li>"SetAnimationTimePosition": Sets time position of an active animation.
        @param name Animation name
        @param new_position New time position
        @return true if successful (animation was currently playing)
<li>"SetAnimationAutoStop": Sets autostop on animation
        @param name Animation name
        @param enable Autostop flag
        @return true if successful
<li>"SetAnimationNumLoops": Sets number of times the animation is repeated
        @param name Animation name
        @param repeats Number of repeats (0 = repeat indefinitely)
        @return true if successful
<li>"GetAvailableAnimations": Get stringlist of available animations
<li>"GetActiveAnimations": Get active animations as a simple stringlist
<li>"GetMeshEntity": Gets mesh entity component
<li>"SetMeshEntity": Gets mesh entity component
</ul>

<b>Reacts on the following actions:</b>
<ul>
<li>"PlayAnim": Plays an animation. Usage: PlayAnim <name> [fadein] [exclusive]
<li>"PlayLoopedAnim": Plays an animation in looped mode. Usage: PlayLoopedAnim <name> [fadein] [exclusive]
<li>"PlayReverseAnim": Plays an animation reversed. Usage: PlayReverseAnim <name> [fadein] [exclusive]
<li>"PlayAnimAutoStop": Plays an animation and autostops (fades out) when finished. Usage: PlayAnimAutoStop <name> [fadein] [exclusive]
<li>"StopAnim": Stops an animation. Usage: StopAnim <name> [fadeout]
<li>"StopAllAnims": Stops all animations. Usage: StopAllAnims [fadeout]
<li>"SetAnimSpeed": Sets speed of a running animation. Use negative speed to play in reverse. Usage: SetAnimSpeed <name> <speed>
<li>"SetAnimWeight": Sets blending weight of a running animation. May not be effective when animation is already fading in or out. Usage: SetAnimWeight <name> <weight>
</ul>
</td>
</tr>

Does not emit any actions.

<b>Depends on the component Mesh</b>.
</table>

*/
class OGRE_MODULE_API EC_AnimationController : public IComponent
{
    Q_OBJECT
    
    COMPONENT_NAME("EC_AnimationController", 14)
public slots:

    /// Gets mesh entity component
    EC_Mesh *GetMeshEntity() const { return mesh; }
    
    /// Gets mesh entity component
    void SetMeshEntity(EC_Mesh *new_mesh);

public:
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit EC_AnimationController(Scene* scene);
    ~EC_AnimationController();

    /// Animation state attribute. Is a "freedata" field to store the current animation state.
    /** It is up to a logic script to change & interpret this, AnimationController does not change or read it by itself.
     */
    Q_PROPERTY(QString animationState READ getanimationState WRITE setanimationState);
    DEFINE_QPROPERTY_ATTRIBUTE(QString, animationState);
        
    /// Enumeration of animation phase
    enum AnimationPhase
    {
        PHASE_FADEIN = 0,
        PHASE_PLAY,
        PHASE_FADEOUT,
        PHASE_STOP,
        PHASE_FREE //in external control. for dynamiccomponent testing now
    };

    /// Structure for an ongoing animation
    struct Animation
    {
        /// Autostop at end (default false)
        bool auto_stop_;

        /// Time in milliseconds it takes to fade in/out an animation completely
        float fade_period_;
        
        /// Weight of an animation in animation blending, maximum 1.0
        float weight_;

        /// Weight adjust
        float weight_factor_;

        /// How an animation is sped up or slowed down, default 1.0 (original speed)
        float speed_factor_;

        /// loop animation through num_repeats times, or loop if zero
        uint num_repeats_;

        /// priority. high priority will reduce the weight of low priority animations, if exists on the same bone tracks
        bool high_priority_;
        
        /// current phase
        AnimationPhase phase_;

        Animation() :
            auto_stop_(false),
            fade_period_(0.0),
            weight_(0.0),
            weight_factor_(1.0),
            speed_factor_(1.0),
            num_repeats_(0),
            high_priority_(false),
            phase_(PHASE_STOP)
        {
        }
    };

    typedef std::map<QString, Animation, QStringLessThanNoCase> AnimationMap;

public slots:
    /// Auto-associate mesh component if not yet set
    void AutoSetMesh();
    
    /// Updates animation(s) by elapsed time
    void Update(float frametime);
    
    /// Enables animation with optional fade-in time
    /* @param name Animation name
       @param looped Is animation looped
       @param fadein Animation fadein time, 0 = instant
       @param high_priority Whether animation uses high-priority blending (overrides and reduces weight of low-priority animations)
       @return true if animation exists and could be enabled
     */
    bool EnableAnimation(const QString& name, bool looped = true, float fadein = 0.0f, bool high_priority = false);

    /// Enables an exclusive animation. This means that other animations will start fading out with the fade-out time specified
    /* @param name Animation name
       @param looped Is animation looped
       @param fadein Animation fadein time, 0 = instant
       @param fadeout Other animations' fadeout time, 0 = instant
       @param high_priority Whether animation uses high-priority blending (overrides and reduces weight of low-priority animations)
       @return true if animation exists and could be enabled
     */
    bool EnableExclusiveAnimation(const QString& name, bool looped, float fadein = 0.0f, float fadeout = 0.0f, bool high_priority = false);

    /// Checks whether non-looping animation has finished. If looping, returns always false
    /* @return true if animation finished
     */
    bool HasAnimationFinished(const QString& name);

    /// Checks whether animation is active
    /** @param name Animation name
        @param check_fade_out if true, also fade-out (until totally faded) phase is interpreted as "active"
        @return true if animation active
     */
    bool IsAnimationActive(const QString& name, bool check_fadeout = true);

    /// Disables animation with optional fade-out time
    /** @param name Animation name
        @param fadeout Animation fadeout time, 0 = instant
        @return true if animation exists and could be disabled
     */
    bool DisableAnimation(const QString& name, float fadeout = 0.0f);

    /// Disables all animations with the same fadeout time
    /** @param fadeout Animation fadeout time
     */
    void DisableAllAnimations(float fadeout = 0.0f);

    /// Forwards animation to end, useful if animation is played in reverse
    /** @param name Animation name
     */
    void SetAnimationToEnd(const QString& name);

    /// Sets relative speed of an active animation. Note that this speed will be forgotten if the animation is disabled
    /** @param name Animation name
        @param speedfactor Relative speed. 1 is default. Use negative speed to play in reverse
        @return true if successful (animation was currently playing)
     */
    bool SetAnimationSpeed(const QString& name, float speedfactor);

    /// Changes weight of an active animation.
    /** @param name Animation name
        @param weight Animation weight (from 0 to 1)
        @return true if successful (animation was currently playing)
     */
    bool SetAnimationWeight(const QString& name, float weight);
    
    /// Changes animation priority. Can lead to fun visual effects, but provided for completeness
    /** @param name Animation name
        @param high_priority High priority bit
        @return true if successful (animation was currently playing)
     */
    bool SetAnimationPriority(const QString& name, bool high_priority);

    /// Sets time position of an active animation.
    /** @param name Animation name
        @param new_position New time position
        @return true if successful (animation was currently playing)
     */
    bool SetAnimationTimePosition(const QString& name, float new_position);

    /// Sets length-relative time position of an active animation (ie. 0 = start, 1 = end)
    /** @param name Animation name
        @param new_position New time position
        @return true if successful (animation was currently playing)
     */
    bool SetAnimationRelativeTimePosition(const QString& name, float new_position);
    
    /// Sets autostop on animation
    /** @param name Animation name
        @param enable Autostop flag
        @return true if successful
     */
    bool SetAnimationAutoStop(const QString& name, bool enable);

    /// Sets number of times the animation is repeated
    /** @param name Animation name
        @param repeats Number of repeats (0 = repeat indefinitely)
        @return true if successful
     */
    bool SetAnimationNumLoops(const QString& name, unsigned repeats);

    /// Get available animations
    QStringList GetAvailableAnimations();
    
    /// Get active animations as a simple stringlist
    QStringList GetActiveAnimations() const;
    
    /// Returns length of animation
    /** @param name Animation name
        @return length of animation in seconds, or 0 if no such animation
      */
    float GetAnimationLength(const QString& name);
    
    /// Returns time position of animation
    /** @param name Animation name
        @return time position of animation in seconds, or 0 if not active
      */
    float GetAnimationTimePosition(const QString& name);
    
    /// Returns relative time position of animation
    /** @param name Animation name
        @return time position of animation between 0 - 1, or 0 if not active
      */
    float GetAnimationRelativeTimePosition(const QString& name);
    
    /// Implements the PlayAnim action
    void PlayAnim(const QString &name, const QString &fadein, const QString &exclusive);
    /// Implements the PlayLoopedAnim action
    void PlayLoopedAnim(const QString &name, const QString &fadein, const QString &exclusive);
    /// Implements the PlayReverseAnim action
    void PlayReverseAnim(const QString &name, const QString &fadein, const QString &exclusive);
    /// Implements the PlayAnimAutoStop action
    void PlayAnimAutoStop(const QString &name, const QString &fadein, const QString &exclusive);
    /// Implements the StopAnim action
    void StopAnim(const QString &name, const QString &fadeout);
    /// Implements the StopAllAnims action
    void StopAllAnims(const QString &fadeout);
    /// Implements the SetAnimSpeed action
    void SetAnimSpeed(const QString &name, const QString &animspeed);
    /// Implements the SetAnimWeight action
    void SetAnimWeight(const QString &name, const QString &animweight);
    
public:

    /// Returns all running animations
    const AnimationMap& GetRunningAnimations() const { return animations_; }

private slots:
    /// Called when the parent entity has been set.
    void UpdateSignals();
    /// Called when component has been removed from the parent entity. Checks if the component removed was the mesh, and autodissociates it.
    void OnComponentRemoved(IComponent* component, AttributeChange::Type change);
    
signals:
    /// Emitted when a non-looping animation has finished
    void AnimationFinished(const QString& animationName);
    /// Emitted when a looping animation has completed a cycle
    void AnimationCycled(const QString& animationName);
    
private:
    
    /// Gets Ogre entity from the mesh entity component and checks if it has changed; in that case resets internal state
    Ogre::Entity* GetEntity();

    /// Gets animationstate from Ogre entity safely
    /** @param entity Ogre entity
        @param name Animation name
        @return animationstate, or null if not found
     */
    Ogre::AnimationState* GetAnimationState(Ogre::Entity* entity, const QString& name);
    
    /// Resets internal state
    void ResetState();
    
    /// Mesh entity component 
    EC_Mesh *mesh;
    
    /// Current mesh name
    std::string mesh_name_;
    
    /// Current animations
    AnimationMap animations_;
    
    /// Bone blend mask of high-priority animations
    Ogre::AnimationState::BoneBlendMask highpriority_mask_;

    /// Bone blend mask of low-priority animations
    Ogre::AnimationState::BoneBlendMask lowpriority_mask_;
};

