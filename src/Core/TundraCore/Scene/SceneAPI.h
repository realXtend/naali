// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "SceneFwd.h"
#include "CoreTypes.h"
#include "CoreStringUtils.h"

#include <QObject>

class Framework;

/// Gives access to the scenes in the system.
/** With this API you can create, remove, query scenes and listen to scene additions and removals.
    Owned by Framework. */
class TUNDRACORE_API SceneAPI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList attributeTypes READ AttributeTypes) /**< @copydoc AttributeTypes */
    Q_PROPERTY(SceneMap scenes READ Scenes) /**< @copydoc Scenes */

public:
    ~SceneAPI();

    /// Creates new component of the type @c T.
    /** @param newComponentName Name for the component (optional).
        @note Important: if creating a non-parented component (parentScene == 0), do not get() the raw pointer immediately
        upon this function call as the reference count of the shared_ptr goes to zero and the object is deleted. */
    template<typename T>
    shared_ptr<T> CreateComponent(Scene* parentScene, const QString &newComponentName = "") const
    {
        /// @todo static_pointer_cast should be ok here
        return dynamic_pointer_cast<T>(CreateComponentById(parentScene, T::TypeIdStatic(), newComponentName));
    }

    /// Returns a list of all attribute type names that can be used in the CreateAttribute function to create an attribute.
    /** @note Unlike the available component types, the available attribute type set is static and cannot change at runtime. */
    static const QStringList &AttributeTypes();

    /// Returns the scene map for self reflection / introspection.
    SceneMap &Scenes();
    const SceneMap &Scenes() const;

public slots:
    /// Returns a pointer to a scene
    /** Manage the pointer carefully, as scenes may not get deleted properly if
        references to the pointer are left alive.

        @note Returns a shared pointer, but it is preferable to use a weak pointer, SceneWeakPtr,
              to avoid dangling references that prevent scenes from being properly destroyed.

        @param name Name of the scene to return
        @return The scene, or empty pointer if the scene with the specified name could not be found. */
    ScenePtr SceneByName(const QString &name) const;
    /// @todo ScenePtr SceneById(scene_id_t id) const;

    /// Returns the Scene the current active main camera is in.
    /** If there is no active main camera, this function returns the first found scene.
        If no scenes have been created, returns 0.
        @todo Refactor into ScenePtr MainCameraScene() const; */
    Scene *MainCameraScene();

    /// Creates new empty scene.
    /** @param name name of the new scene
        @param viewEnabled Whether the scene is view enabled
        @param authority True for server & standalone scenes, false for network client scenes
        @return The new scene, or empty pointer if scene with the specified name already exists. */
    ScenePtr CreateScene(const QString &name, bool viewEnabled, bool authority);

    /// Removes a scene with the specified name.
    /** The scene may not get deleted since there may be dangling references to it.
        If the scene does get deleted, removes all entities which are not shared with
        another existing scene.

        Does nothing if scene with the specified name doesn't exist.

        @param name name of the scene to delete
        @return True if the scene was found and removed, false otherwise. */
    bool RemoveScene(const QString &name);

    /// Return if a component factory has been registered for a type name.
    bool IsComponentFactoryRegistered(const QString &typeName) const;

    /// Registers a new factory to create new components of type name IComponentFactory::TypeName and ID IComponentFactory::TypeId.
    void RegisterComponentFactory(const ComponentFactoryPtr &factory);

    /// Creates a new component instance by specifying the typename of the new component to create, and the scene where to create.
    /** @note the @c componentTypeName doesn't need to have the "EC_" prefix. */
    ComponentPtr CreateComponentByName(Scene* scene, const QString &componentTypeName, const QString &newComponentName = "") const;

    /// Creates a new component instance by specifying the typeid of the new component to create, and the scene where to create.
    ComponentPtr CreateComponentById(Scene* scene, u32 componentTypeid, const QString &newComponentName = "") const;

    /// Looks up the given type id and returns the type name string for that id.
    QString ComponentTypeNameForTypeId(u32 componentTypeId) const;

    /// Looks up the given type name and returns the type id for that component type.
    u32 ComponentTypeIdForTypeName(const QString &componentTypeName) const;

    /// Looks up the attribute type name for an attribute type id
    static QString AttributeTypeNameForTypeId(u32 attributeTypeId);

    /// Looks up the type id for an attribute type name, or zero if not found
    /** @param attributeTypeName Attribute type name, handled as case-insensitive. */
    static u32 AttributeTypeIdForTypeName(const QString &attributeTypeName);

    /// Creates a clone of the specified component. The new component will be detached, i.e. it has no parent entity.
    ///\todo Implement this.
//    ComponentPtr CloneComponent(const ComponentPtr &component, const QString &newComponentName);

    /// Creates a new dynamic attribute without attaching it to a component and returns it. Returns null if illegal type ID.
    static IAttribute* CreateAttribute(u32 attributeTypeid, const QString& newAttributeId);

    /// Creates a new dynamic attribute without attaching it to a component and returns it. Returns null if illegal type.
    /** @param attributeTypeName Attribute type name, handled as case-insensitive.
        @param newAttributeId Arbitrary user-defined ID which identifies the attribute. 
        @note The name of the attribute will be assigned same as the ID. As dynamic attributes are serialized over the network per-object,
        it would unnecessarily increase the bandwidth needed to transfer both a name and an ID */
    static IAttribute* CreateAttribute(const QString &attributeTypeName, const QString& newAttributeId);

    /// Returns a list of all component type names that can be used in the CreateComponentByName function to create a component.
    QStringList ComponentTypes() const;

    // DEPRECATED
    /// @cond PRIVATE
    bool HasScene(const QString &name) const { return scenes.find(name) != scenes.end(); } /**< @deprecated Use GetScene instead @todo Remove */
    ScenePtr GetScene(const QString &name) const { return SceneByName(name); } /**< @deprecated Use SceneByName instead @todo Remove */
    QString GetComponentTypeName(u32 componentTypeId) const { return ComponentTypeNameForTypeId(componentTypeId); }
    u32 GetComponentTypeId(const QString &componentTypeName) const { return ComponentTypeIdForTypeName(componentTypeName); }
    static QString GetAttributeTypeName(u32 attributeTypeId) { return AttributeTypeNameForTypeId(attributeTypeId); }
    static u32 GetAttributeTypeId(const QString &attributeTypeName) { return AttributeTypeIdForTypeName(attributeTypeName); }
    /// @endcond

signals:
    /// Emitted after new scene has been added to framework.
    /** @param name new scene name. */
    void SceneAdded(const QString &name);
    /// Emitted after scene has been removed from the framework.
    /** @param name removed scene name. */
    void SceneRemoved(const QString &name);

    /// Emitted after new scene has been added to framework.
    /// @todo void SceneCreated(Scene *scene, AttributeChange::Type change);
    /// Emitted after scene has been removed from the framework.
    /// @todo void SceneRemoved(Scene *scene, AttributeChange::Type change);

private:
    friend class Framework;

    /// Constructor. Framework takes ownership of this object.
    /** @param owner Owner Framework. */
    explicit SceneAPI(Framework *owner);

    /// Frees all known scene and the scene interact object.
    /** Called by Framework during application shutdown. */
    void Reset();

    ComponentFactoryPtr GetFactory(const QString &typeName) const;
    ComponentFactoryPtr GetFactory(u32 typeId) const;

    typedef std::map<QString, ComponentFactoryPtr, QStringLessThanNoCase> ComponentFactoryMap;
    typedef std::map<u32, weak_ptr<IComponentFactory> > ComponentFactoryWeakMap;

    ComponentFactoryMap componentFactories;
    ComponentFactoryWeakMap componentFactoriesByTypeid;
    Framework *framework;
    SceneMap scenes; ///< All currently created scenes.
    static const QStringList attributeTypeNames;
};
