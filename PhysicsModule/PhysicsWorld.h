// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_Physics_PhysicsWorld_h
#define incl_Physics_PhysicsWorld_h

#include "Core.h"
#include "ForwardDefines.h"
#include "PhysicsModuleApi.h"

#include <set>
#include <QObject>
#include <QVector>

class btCollisionConfiguration;
class btBroadphaseInterface;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
class btDispatcher;
class btDynamicsWorld;
class btCollisionObject;

namespace Physics
{

class PhysicsModule;

//! A physics world that encapsulates a Bullet physics world
class PHYSICS_MODULE_API PhysicsWorld : public QObject
{
    Q_OBJECT
    
public:
    PhysicsWorld(PhysicsModule* owner);
    virtual ~PhysicsWorld();
    
    //! Step the physics world. May trigger several internal simulation substeps, according to the deltatime given.
    void Simulate(f64 frametime);
    
    //! Process collision from an internal sub-step (Bullet post-tick callback)
    void ProcessPostTick(float substeptime);
    
public slots:
    //! Set physics update period (= length of each simulation step.) By default 1/60th of a second.
    /*! \param updatePeriod Update period
     */
    void SetPhysicsUpdatePeriod(float updatePeriod);
    
    //! Return internal physics timestep
    float GetPhysicsUpdatePeriod() const { return physicsUpdatePeriod_; }
    
    //! Set gravity that affects all moving objects of the physics world
    /*! \param gravity Gravity vector
     */
    void SetGravity(const Vector3df& gravity);
    
    //! Raycast to the world. Returns only a single (the closest) entity; other results like hit position/distance can be inspected by connecting to the RaycastResult signal
    /*! \param origin World origin position
        \param direction Direction to raycast to. Will be normalized automatically
        \param maxdistance Length of ray
        \param collisiongroup Collision filter group (0 = use default)
        \param collisionmask Collision filter mask (0 = use default)
        \return Entity (with rigidbody) that was hit, or null if no entity hit
     */
    Scene::Entity* Raycast(const Vector3df& origin, const Vector3df& direction, float maxdistance, int collisiongroup = 0, int collisionmask = 0);
    
    //! Return gravity
    Vector3df GetGravity() const;
    
    //! Return the Bullet world object
    btDynamicsWorld* GetWorld() const;
    
signals:
    //! A physics collision has happened between two entities. 
    /*! Note: both rigidbodies participating in the collision will also emit a signal separately. 
        Also, if there are several contact points, the signal will be sent multiple times for each contact.
        \param entityA The first entity
        \param entityB The second entity
        \param position World position of collision
        \param normal World normal of collision
        \param distance Contact distance
        \param impulse Impulse applied to the objects to separate them
        \param newCollision True if same collision did not happen on the previous frame. If collision has multiple contact points, newCollision can only be true for the first of them.
     */
    void PhysicsCollision(Scene::Entity* entityA, Scene::Entity* entityB, const Vector3df& position, const Vector3df& normal, float distance, float impulse, bool newCollision);
    
    //! Raycast result. Invoked when a raycast hits an entity
    /*! \param Scene::Entity* entity The hit entity
        \param position World position of ray hit
        \param normal World normal of ray hit
        \param distance Hit distance along ray
     */
     void RaycastResult(Scene::Entity* entity, const Vector3df& position, const Vector3df& normal, float distance);
     
     //! Emitted after each simulation step
     /*! \param frametime Length of simulation step
      */
     void Updated(float frametime);
     
private:
    //! Bullet collision config
    btCollisionConfiguration* collisionConfiguration_;
    //! Bullet collision dispatcher
    btDispatcher* collisionDispatcher_;
    //! Bullet collision broadphase
    btBroadphaseInterface* broadphase_;
    //! Bullet constraint equation solver
    btConstraintSolver* solver_;
    //! Bullet physics world
    btDiscreteDynamicsWorld* world_;
    
    //! Length of internal physics timestep
    float physicsUpdatePeriod_;
    
    //! Previous frame's collisions. We store these to know whether the collision was new or "ongoing"
    std::set<std::pair<btCollisionObject*, btCollisionObject*> > previousCollisions_;
};

}

#endif

