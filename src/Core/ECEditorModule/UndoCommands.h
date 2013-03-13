/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   UndoCommands.h
    @brief  A collection of QUndoCommand-derived classes which apply to the operations in EC editor and Scene structure windows 
*/

#pragma once
#include "SceneFwd.h"

#include <QDomDocument>
#include <QDomElement>
#include <QUndoCommand>

#include "Color.h"

typedef QList<entity_id_t> EntityIdList;

class EntityIdChangeTracker;

/// EditAttributeCommand representing an "Edit" operation to the attributes
template <typename T>
class EditAttributeCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 100 };

    /// Constructor
    /* @param attr The attribute that is being edited
       @param value The old value of 'attr' attribute
       @param parent The parent command of this command (optional) */
    EditAttributeCommand(IAttribute * attr, const T& value, QUndoCommand * parent = 0);

    /// Returns this command's ID
    int id() const;

    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();
    /// QUndoCommand override
    virtual bool mergeWith(const QUndoCommand * other);

private:
    EntityWeakPtr entity_; ///< A weak pointer to this attribute's parent entity
    const QString componentName_; ///< Name of this attribute's parent component
    const QString componentType_; ///< Typename of this attribute's parent component
    const QString name_; ///< Name of the attribute
    const T oldValue_; ///< Old value of the attribute
    T newValue_; ///< New value of the attribute

    bool dontCallRedo_; ///< A workaround variable to prevent redo() from calling immediately when this command is pushed onto the stack
};

#include "UndoCommands.inl"

/// Merges two EditAttributeCommand<Color> objects, since editing 'Color' triggers two changes 
template<> bool EditAttributeCommand<Color>::mergeWith(const QUndoCommand *other);

/// AddAttributeCommand represents adding an attribute to a dynamic component
class AddAttributeCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 101 };

    /// Constructor
    /* @param comp A raw pointer to the component that an attribute is added to
       @param typeName The type of the attribute being added
       @param name The name of the attribute being added
       @param parent The parent command of this command (optional) */
    AddAttributeCommand(IComponent * comp, const QString typeName, const QString name, QUndoCommand * parent = 0);

    /// Returns this command's ID
    /* @returns This command's ID */
    int id() const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();

    EntityWeakPtr entity_; ///< A weak pointer to this attribute's parent entity
    const QString componentName_; ///< Name of this attribute's parent component
    const QString componentType_; ///< Typename of this attribute's parent component
    const QString attributeTypeName_; ///< Typename of this attribute
    const QString attributeName_; ///< Name of this attribute
};

/// RemoveAttributeCommand represents removing an existing attribute of a dynamic component
class RemoveAttributeCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 102 };

    /// Constructor
    /* @param attr A raw pointer to the attribute being removed
       @param parent The parent command of this command (optional) */
    RemoveAttributeCommand(IAttribute * attr, QUndoCommand * parent = 0);
    
    /// Returns this command's ID
    /* @returns This command's ID */
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();

    EntityWeakPtr entity_; ///< A weak pointer to this attribute's parent entity
    const QString componentName_; ///< Name of this attribute's parent component
    const QString componentType_; ///< Typename of this attribute's parent component
    const QString attributeTypeName_; ///< Typename of this attribute
    const QString attributeName_; ///< Name of this attribute
    QString value_; ///< Value of this attribute represented as string
};

/// AddComponentCommand represents adding a component to an entity or more entities
class AddComponentCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 103 };

    /// Constructor
    /* @param scene A weak pointer to the main camera scene
       @param tracker Pointer to the EntityIdChangeTracker object
       @param entities A list of IDs of entities that a component is being added to
       @param compType Typename of the component being added
       @param compName Name of the component being added
       @param sync Sync state of the component being added
       @param temp Temporary state of the component being added
       @param parent The parent command of this command (optional) */
    AddComponentCommand(SceneWeakPtr scene, EntityIdChangeTracker * tracker, EntityIdList entities, const QString compType, const QString compName, bool sync, bool temp, QUndoCommand * parent = 0);

    /// Returns this command's ID
    /* @returns This command's ID */
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();

    SceneWeakPtr scene_; ///< A weak pointer to the main camera scene
    EntityIdChangeTracker * tracker_; ///< Pointer to the tracker object, taken from an undo manager
    EntityIdList entityIds_; ///< List of IDs of entities that this component is being added to
    QString componentName_; ///< Name of the component being added
    QString componentType_; ///< Typename of the component being added
    bool sync_; ///< Sync state of the component
    bool temp_; ///< Temporary state of the component
};

/// EditXMLCommand represents editing entities and/or components as XML
class EditXMLCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 104 };

    EditXMLCommand(Scene * scene, const QDomDocument oldDoc, const QDomDocument newDoc, QUndoCommand * parent = 0);

    /// Returns this command's ID
    /* @returns This command's ID */
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();
    /// QUndoCommand override
    bool mergeWith(const QUndoCommand *other);

    /// Deserializes a document state
    /* @param docState The document state to be deserialized */
    void Deserialize(const QDomDocument docState);

    SceneWeakPtr scene_; ///< A weak pointer to the main camera scene
    QDomDocument oldState_; ///< Old state of the XML document
    QDomDocument newState_; ///< New state of the XML document
};

/// AddEntityCommand represents adding an entity to the scene
class AddEntityCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 105 };

    /// Constructor
    /* @param scene A raw pointer to the main camera scene
       @param tracker Pointer to the EntityIdChangeTracker object
       @param name The desired name of the entity being created
       @param sync The desired sync state of the entity being created
       @param temp The desired temporary state of the entity being created
       @param parent The parent command of this command (optional) */
    AddEntityCommand(Scene * scene, EntityIdChangeTracker * tracker, const QString name, bool sync, bool temp, QUndoCommand * parent = 0);

    /// Returns this command's ID
    /* @returns This command's ID */
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();

    SceneWeakPtr scene_; ///< A weak pointer to the main camera scene
    EntityIdChangeTracker * tracker_; ///< Pointer to the tracker object, taken from an undo manager
    QString entityName_; ///< Name of the entity being created
    entity_id_t entityId_; ///< ID of the entity
    bool sync_; ///< sync state of the entity
    bool temp_; ///< Temporary state of the entity
};

class RemoveCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 106 };

    /// Constructor
    /* @param scene A raw pointer to the main camera scene
       @param tracker Pointer to the EntityIdChangeTracker object
       @param entityList A weak pointer list of the entity/entities about to be removed
       @param componentList A weak pointer list of the component(s) about to be removed
       @param parent The parent command of this command (optional) */
    RemoveCommand(Scene * scene, EntityIdChangeTracker * tracker, QList<EntityWeakPtr> entityList, QList<ComponentWeakPtr> componentList, QUndoCommand * parent = 0);

    /// Returns this command's ID
    /* @returns This command's ID */
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();

    /// Recreates content from 'document'
    /* @param document The document containing XML data about the removed entities and/or components */
    void RecreateContent(QDomDocument document);

    EntityIdList entityList_; ///< Entity ID list of the entities being removed
    typedef QList<QPair<QString, QString> > ComponentList; ///< A typedef for QList containing QPair of component typenames and component names
    typedef QMap<entity_id_t, ComponentList> ParentEntityOfComponentMap; ///< A typedef for QMap with entity ID as key and a ComponentList as value
    ParentEntityOfComponentMap componentMap_; ///< Map that links QPairs of component typenames and names with their parent entity IDs

    SceneWeakPtr scene_; ///< A weak pointer to the main camera scene
    EntityIdChangeTracker * tracker_; ///< Pointer to the tracker object, taken from an undo manager
    QDomDocument entitiesDocument_; ///< XML document containing data about the entities to be removed
    QDomDocument componentsDocument_; ///< XML document containing data about the components to be removed
};

/// RenameCommand represents a rename operation over an entity or a component
class RenameCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 107 };

    /// Constructor 
    /* @param entity A weak pointer to the entity being renamed
       @param tracker Pointer to the EntityIdChangeTracker object
       @param oldName The old name of the entity
       @param newName The new name of the entity
       @param parent The parent command of this command (optional) */
    RenameCommand(EntityWeakPtr entity, EntityIdChangeTracker * tracker, const QString oldName, const QString newName, QUndoCommand * parent = 0);

    /// Returns this command's ID
    /* @returns This command's ID */
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();

    SceneWeakPtr scene_; ///< A weak pointer to the main camera scene
    EntityIdChangeTracker * tracker_; ///< Pointer to the tracker object, taken from an undo manager
    entity_id_t entityId_; ///< Entity ID of the entity being renamed
    QString oldName_; ///< Old entity name
    QString newName_; ///< New entity name
};

/// ToggleTemporaryCommand represents toggling the temporary state of entity / entities
class ToggleTemporaryCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 108 };

    /// Constructor
    /* @param entities A list of weak pointers to entities being toggled
       @param tracker Pointer to the EntityIdChangeTracker object
       @param temporary The desired temporary state to be set
       @param parent The parent command of this command (optional) */
    ToggleTemporaryCommand(QList<EntityWeakPtr> entities, EntityIdChangeTracker * tracker, bool temporary, QUndoCommand * parent = 0);

    /// Returns this command's ID
    /* @returns This command's ID */
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();
    void ToggleTemporary(bool temporary);

    SceneWeakPtr scene_; ///< A weak pointer to the main camera scene
    EntityIdChangeTracker * tracker_; ///< Pointer to the tracker object, taken from an undo manager
    EntityIdList entityIds_; ///< List of target entity IDs
    bool temporary_; ///< Temporary state
};

/*
class PasteCommand : public QUndoCommand
{
public:
    /// Internal QUndoCommand unique ID
    enum { Id = 109 };

    PasteCommand(QUndoCommand * parent = 0);

    /// Returns this command's ID
    int id () const;
    /// QUndoCommand override
    void undo();
    /// QUndoCommand override
    void redo();
};
*/