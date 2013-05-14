// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "UndoCommands.h"
#include "EntityIdChangeTracker.h"
#include "TransformEditor.h"

#include "Scene.h"
#include "SceneAPI.h"
#include "IComponent.h"
#include "IAttribute.h"
#include "UiAPI.h"
#include "UiMainWindow.h"
#include "Entity.h"
#include "EC_DynamicComponent.h"
#include "Transform.h"
#include "EC_Placeable.h"

#include "MemoryLeakCheck.h"

/// Merges two EditAttributeCommand<Color> objects, since editing 'Color' triggers two changes
template<> bool EditAttributeCommand<Color>::mergeWith(const QUndoCommand *other)
{
    if (id() != other->id())
        return false;

    const EditAttributeCommand<Color> *otherCommand = dynamic_cast<const EditAttributeCommand<Color> *>(other);
    if (!otherCommand)
        return false;

    return (undoValue == otherCommand->undoValue);
}

AddAttributeCommand::AddAttributeCommand(IComponent * comp, const QString &typeName, const QString &name, QUndoCommand * parent) :
    entity_(comp->ParentEntity()->shared_from_this()),
    componentName_(comp->Name()),
    componentType_(comp->TypeName()),
    attributeTypeName_(typeName),
    attributeName_(name),
    QUndoCommand(parent)
{
    setText("+ Added " + typeName + " Attribute");
}

int AddAttributeCommand::id() const
{
    return Id;
}

void AddAttributeCommand::undo()
{
    EntityPtr ent = entity_.lock();
    if (!ent.get())
        return;

    ComponentPtr comp = ent->GetComponent(componentType_, componentName_);
    if (comp.get())
    {
        EC_DynamicComponent *dynComp = dynamic_cast<EC_DynamicComponent *>(comp.get());
        if (dynComp)
        {
            if (dynComp->ContainsAttribute(attributeName_))
            {
                dynComp->RemoveAttribute(attributeName_);
                dynComp->ComponentChanged(AttributeChange::Default);
            }
        }
    }
}

void AddAttributeCommand::redo()
{
    EntityPtr ent = entity_.lock();
    if (!ent.get())
        return;

    ComponentPtr comp = ent->GetComponent(componentType_, componentName_);
    if (comp.get())
    {
        EC_DynamicComponent *dynComp = dynamic_cast<EC_DynamicComponent *>(comp.get());
        if (dynComp)
            if (!dynComp->ContainsAttribute(attributeName_))
            {
                IAttribute * attr = dynComp->CreateAttribute(attributeTypeName_, attributeName_);
                if (attr)
                    dynComp->ComponentChanged(AttributeChange::Default);
                else
                    QMessageBox::information(dynComp->GetFramework()->Ui()->MainWindow(), QMessageBox::tr("Failed to create attribute"),
                    QMessageBox::tr("Failed to create %1 attribute \"%2\", please try again.").arg(attributeTypeName_).arg(attributeName_));
            }
    }
}


RemoveAttributeCommand::RemoveAttributeCommand(IAttribute *attr, QUndoCommand *parent) : 
    entity_(attr->Owner()->ParentEntity()->shared_from_this()),
    componentName_(attr->Owner()->Name()),
    componentType_(attr->Owner()->TypeName()),
    attributeTypeName_(attr->TypeName()),
    attributeName_(attr->Name()),
    value_(QString::fromStdString(attr->ToString())),
    QUndoCommand(parent)
{
    setText("- Removed " + attributeTypeName_ + " Attribute");
}

int RemoveAttributeCommand::id() const
{
    return Id;
}

void RemoveAttributeCommand::undo()
{
    EntityPtr ent = entity_.lock();
    if (!ent.get())
        return;

    ComponentPtr comp = ent->GetComponent(componentType_, componentName_);
    if (comp.get())
    {
        EC_DynamicComponent *dynComp = dynamic_cast<EC_DynamicComponent *>(comp.get());
        if (dynComp)
        {
            IAttribute * attr = dynComp->CreateAttribute(attributeTypeName_, attributeName_);
            attr->FromString(value_.toStdString(), AttributeChange::Default);
        }
    }
}

void RemoveAttributeCommand::redo()
{
    EntityPtr ent = entity_.lock();
    if (!ent.get())
        return;

    ComponentPtr comp = ent->GetComponent(componentType_, componentName_);
    if (comp.get())
    {
        EC_DynamicComponent *dynComp = dynamic_cast<EC_DynamicComponent *>(comp.get());
        if (dynComp)
        {
            dynComp->RemoveAttribute(attributeName_);
            dynComp->ComponentChanged(AttributeChange::Default);
        }
    }
}

AddComponentCommand::AddComponentCommand(const ScenePtr &scene, EntityIdChangeTracker * tracker, const EntityIdList &entities,
    const QString &compType, const QString &compName, bool sync, bool temp, QUndoCommand * parent) :
    scene_(scene),
    tracker_(tracker),
    entityIds_(entities),
    componentName_(compName),
    componentType_(compType),
    temp_(temp),
    sync_(sync),
    QUndoCommand(parent)
{
    setText("+ Added " + QString(componentType_).replace("EC_", "") + " Component" + (entities.size() == 1 ? "" : QString(" (to %1 entities)").arg(entities.size())));
}

int AddComponentCommand::id() const
{
    return Id;
}

void AddComponentCommand::undo()
{    
    // Call base impl that executes potential attribute edit commands
    QUndoCommand::undo();

    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    foreach (entity_id_t id, entityIds_)
    {
        EntityPtr ent = scene->EntityById(tracker_->RetrieveId(id));
        if (ent.get())
        {
            ComponentPtr comp = ent->GetComponent(componentType_, componentName_);
            if (comp.get())
            {
                sync_ = comp->IsReplicated();
                temp_ = comp->IsTemporary();

                ent->RemoveComponent(comp, AttributeChange::Default);
            }
        }
    }
}

void AddComponentCommand::redo()
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    foreach (entity_id_t id, entityIds_)
    {
        EntityPtr ent = scene->EntityById(tracker_->RetrieveId(id));
        if (ent.get())
        {
            Framework *fw = scene->GetFramework();
            ComponentPtr comp = fw->Scene()->CreateComponentByName(scene.get(), componentType_, componentName_);
            if (comp)
            {
                comp->SetReplicated(sync_);
                comp->SetTemporary(temp_);
                ent->AddComponent(comp, AttributeChange::Default);
                
                // Execute any child commands.
                AttributeVector attributes = comp->NonEmptyAttributes();
                for(int i=0; i<childCount(); ++i)
                {
                    QUndoCommand *command = const_cast<QUndoCommand*>(child(i));
                    IEditAttributeCommand *attrbCommand = dynamic_cast<IEditAttributeCommand*>(command);
                    if (!attrbCommand)
                    {
                        if (command)
                            command->redo();
                        continue;
                    }

                    // Check that this commands parent is the entity being processed.
                    EntityPtr attrCommandParent = scene->EntityById(tracker_->RetrieveId(attrbCommand->parentId));
                    if (attrCommandParent != ent)
                        continue;

                    // Find the correct attribute with name and type and update the weak ptr.
                    for (unsigned i = 0; i < attributes.size(); ++i)
                    {
                        if (attrbCommand->attributeName == attributes[i]->Name() && attrbCommand->attributeTypeName == attributes[i]->TypeName())
                        {
                            attrbCommand->attribute = AttributeWeakPtr(comp, attributes[i]);
                            attrbCommand->redo();
                            break;
                        }
                    }
                }
            }
        }
    }
}

EditXMLCommand::EditXMLCommand(const ScenePtr &scene, const QDomDocument &oldDoc, const QDomDocument &newDoc, QUndoCommand * parent) : 
    scene_(scene),
    oldState_(oldDoc),
    newState_(newDoc),
    QUndoCommand(parent)
{
    setText("* Edited XML");
}

int EditXMLCommand::id() const
{
    return Id;
}

void EditXMLCommand::undo()
{
    Deserialize(oldState_);
}

void EditXMLCommand::redo()
{
    Deserialize(newState_);
}

bool EditXMLCommand::mergeWith(const QUndoCommand *other)
{
    if (id() != other->id())
        return false;

    const EditXMLCommand *otherCommand = dynamic_cast<const EditXMLCommand *>(other);
    if (!otherCommand)
        return false;

    if ((oldState_.toString() != otherCommand->oldState_.toString()) && (newState_.toString() != otherCommand->newState_.toString()))
        return false;

    return true;
}

void EditXMLCommand::Deserialize(const QDomDocument docState)
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    QDomElement entityElement;
    QDomElement entitiesElement = docState.firstChildElement("entities");
    if (!entitiesElement.isNull())
        entityElement = entitiesElement.firstChildElement("entity");
    else
        entityElement = docState.firstChildElement("entity");

    while(!entityElement.isNull())
    {
        entity_id_t id = (entity_id_t)entityElement.attribute("id").toInt();

        EntityPtr entity = scene->GetEntity(id);
        if (entity)
        {
            QDomElement componentElement = entityElement.firstChildElement("component");
            while(!componentElement.isNull())
            {
                QString typeName = componentElement.attribute("type");
                QString name = componentElement.attribute("name");
                ComponentPtr comp = entity->GetComponent(typeName, name);
                if (comp)
                    comp->DeserializeFrom(componentElement, AttributeChange::Default);

                componentElement = componentElement.nextSiblingElement("component");
            }
        }
        else
        {
            LogWarning("EcXmlEditorWidget::Save: Could not find entity " + QString::number(id) + " in scene!");
        }

        entityElement = entityElement.nextSiblingElement("entity");
    }
}

AddEntityCommand::AddEntityCommand(const ScenePtr &scene, EntityIdChangeTracker * tracker, const QString &name, bool sync, bool temp, QUndoCommand *parent) :
    QUndoCommand(parent),
    scene_(scene),
    tracker_(tracker),
    entityName_(name),
    entityId_(0),
    sync_(sync),
    temp_(temp)
{
    setText("+ Added Entity " + entityName_.trimmed());
}

int AddEntityCommand::id() const
{
    return Id;
}

void AddEntityCommand::undo()
{
    QUndoCommand::undo();
    
    ScenePtr scene = scene_.lock();
    if (!scene)
        return;

    entity_id_t newId = tracker_->RetrieveId(entityId_);
    if (scene->EntityById(newId).get()) // intentionally avoid using shared_ptr so that we don't hold ref to the entity that will be deleted
        scene->RemoveEntity(newId, AttributeChange::Replicate);
}

void AddEntityCommand::redo()
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    entity_id_t newId = sync_ ? scene->NextFreeId() : scene->NextFreeIdLocal();
    tracker_->AppendUnackedId(newId);
    if (entityId_)
        tracker_->TrackId(entityId_, newId);

    entityId_ = newId;
    AttributeChange::Type changeType = sync_ ? AttributeChange::Replicate : AttributeChange::LocalOnly;
    EntityPtr entity = scene->CreateEntity(entityId_, QStringList(), changeType, sync_);

    if (!entityName_.isEmpty())
        entity->SetName(entityName_);

    entity->SetTemporary(temp_);

    // Execute any AddComponentCommand children to apply component back.
    QUndoCommand::redo();
}

RemoveCommand::RemoveCommand(const ScenePtr &scene, EntityIdChangeTracker * tracker, const QList<EntityWeakPtr> &entities, const QList<ComponentWeakPtr> &components, QUndoCommand * parent) :
    QUndoCommand(parent),
    scene_(scene),
    tracker_(tracker)
{
    Initialize(entities, components);
}

RemoveCommand::RemoveCommand(const ScenePtr &scene, EntityIdChangeTracker *tracker, const QList<EntityWeakPtr> &entities, QUndoCommand *parent) :
    QUndoCommand(parent),
    scene_(scene),
    tracker_(tracker)
{
    Initialize(entities, QList<ComponentWeakPtr>());
}

RemoveCommand::RemoveCommand(const ScenePtr &scene, EntityIdChangeTracker *tracker, const QList<ComponentWeakPtr> &components, QUndoCommand *parent) :
    QUndoCommand(parent),
    scene_(scene),
    tracker_(tracker)
{
    Initialize(QList<EntityWeakPtr>(), components);
}

RemoveCommand::RemoveCommand(const ScenePtr &scene, EntityIdChangeTracker *tracker, const EntityWeakPtr &entity, QUndoCommand *parent) :
    QUndoCommand(parent),
    scene_(scene),
    tracker_(tracker)
{
    Initialize(QList<EntityWeakPtr>(QList<EntityWeakPtr>() << entity), QList<ComponentWeakPtr>());
}

RemoveCommand::RemoveCommand(const ScenePtr &scene, EntityIdChangeTracker *tracker, const ComponentWeakPtr &component, QUndoCommand *parent) :
    QUndoCommand(parent),
    scene_(scene),
    tracker_(tracker)
{
    Initialize(QList<EntityWeakPtr>(), QList<ComponentWeakPtr>(QList<ComponentWeakPtr>() << component));
}

int RemoveCommand::id() const
{
    return Id;
}

void RemoveCommand::undo()
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    if (!entitiesDocument_.isNull())
    {
        QDomElement sceneElement = entitiesDocument_.firstChildElement("scene");
        QDomElement entityElement = sceneElement.firstChildElement("entity");
        while (!entityElement.isNull())
        {
            entity_id_t id = entityElement.attribute("id").toInt();
            bool sync = ParseBool(entityElement.attribute("sync"));

            entity_id_t newId = sync ? scene->NextFreeId() : scene->NextFreeIdLocal();
            tracker_->TrackId(id, newId);

            QString newIdStr;
            newIdStr.setNum((int)newId);
            entityElement.setAttribute("id", newIdStr);

            entityElement = entityElement.nextSiblingElement();
        }

        scene->CreateContentFromXml(entitiesDocument_, true, AttributeChange::Default);
    }

    if (!componentsDocument_.isNull())
    {
        QDomElement entityElement = componentsDocument_.firstChildElement("entity");
        while (!entityElement.isNull())
        {
            entity_id_t entityId = entityElement.attribute("id").toInt();
            EntityPtr ent = scene->EntityById(tracker_->RetrieveId(entityId));
            if (ent.get())
            {
                QDomElement compElement = entityElement.firstChildElement("component");
                while (!compElement.isNull())
                {
                    ComponentPtr component;
                    QString type = compElement.attribute("type");
                    QString name = compElement.attribute("name");
                    QString sync = compElement.attribute("sync");
                    QString temp = compElement.attribute("temporary");

                    Framework *fw = ent->ParentScene()->GetFramework();
                    component = fw->Scene()->CreateComponentByName(ent->ParentScene(), type, name);

                    if (!sync.isEmpty())
                        component->SetReplicated(ParseBool(sync));
                    if (!temp.isEmpty())
                        component->SetTemporary(ParseBool(temp));

                    ent->AddComponent(component);
                    component->DeserializeFrom(compElement, AttributeChange::Default);

                    compElement = compElement.nextSiblingElement("component");
                }
            }

            entityElement = entityElement.nextSiblingElement("entity");
        }
    }
}

void RemoveCommand::redo()
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    if (!componentMap_.isEmpty())
    {
        componentsDocument_ = QDomDocument();
        EntityIdList keys = componentMap_.keys();
        foreach (entity_id_t key, keys)
        {
            EntityPtr ent = scene->EntityById(tracker_->RetrieveId(key));
            if (ent.get())
            {
                QDomElement entityElem = componentsDocument_.createElement("entity");
                componentsDocument_.appendChild(entityElem);
                entityElem.setAttribute("id", ent->Id());
                entityElem.setAttribute("name", ent->Name());

                for (ComponentList::iterator i = componentMap_[key].begin(); i != componentMap_[key].end(); ++i)
                {
                    ComponentPtr comp = ent->GetComponent((*i).first, (*i).second);
                    if (comp.get())
                    {
                        comp->SerializeTo(componentsDocument_, entityElem, true);
                        ent->RemoveComponent(comp, AttributeChange::Replicate);
                    }
                }
            }
        }
    }

    if (!entityList_.isEmpty())
    {
        entitiesDocument_ = QDomDocument();
        QDomElement sceneElement = entitiesDocument_.createElement("scene");
        entitiesDocument_.appendChild(sceneElement);

        foreach(entity_id_t id, entityList_)
        {
            EntityPtr ent = scene->EntityById(tracker_->RetrieveId(id));
            if (ent.get())
            {
                ent->SerializeToXML(entitiesDocument_, sceneElement, true);
                scene->RemoveEntity(ent->Id(), AttributeChange::Replicate);
            }
        }
    }
}

void RemoveCommand::Initialize(const QList<EntityWeakPtr> &entityList, const QList<ComponentWeakPtr> &componentList)
{
    QStringList componentTypes;
    bool componentMultiParented = false;
    
    for (QList<EntityWeakPtr>::const_iterator i = entityList.begin(); i != entityList.end(); ++i)
        entityList_ << (*i).lock()->Id();

    for (QList<ComponentWeakPtr>::const_iterator i = componentList.begin(); i != componentList.end(); ++i)
    {
        ComponentPtr comp = (*i).lock();
        if (comp.get())
        {
            if (entityList_.contains(comp->ParentEntity()->Id()))
                continue;

            componentMap_[comp->ParentEntity()->Id()] << qMakePair(comp->TypeName(), comp->Name());

            QString cleanTypeName = QString(comp->TypeName()).replace("EC_", "");
            if (!componentTypes.contains(cleanTypeName))
                componentTypes << cleanTypeName;
                
            if (i != componentList.begin())
                componentMultiParented = true;
        }
    }
    
    if (!componentTypes.isEmpty() && entityList_.size() > 0)
        setText("* Removed Entities and Components");
    else if (!componentTypes.isEmpty())
        setText(QString("* Removed ") + (!componentTypes.isEmpty() ? componentTypes.join(", ") : "") + (componentTypes.size() > 1 ? " Components" : " Component") + (componentMultiParented ? " from multiple entities" : "")); 
    else if (entityList_.size() > 0)
        setText(QString("* Removed %1 Entities").arg(entityList_.size()));
}

RenameCommand::RenameCommand(EntityWeakPtr entity, EntityIdChangeTracker * tracker, const QString oldName, const QString newName, QUndoCommand * parent) : 
    tracker_(tracker),
    oldName_(oldName),
    newName_(newName),
    QUndoCommand(parent)
{
    if (newName_.trimmed().isEmpty())
        setText("* Removed name from Entity " + entity.lock()->Name().trimmed());
    else
        setText("* Renamed Entity to " + newName_.trimmed());
    scene_ = entity.lock()->ParentScene()->shared_from_this();
    entityId_ = entity.lock()->Id();
}

int RenameCommand::id() const
{
    return Id;
}

void RenameCommand::undo()
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    EntityPtr entity = scene->EntityById(tracker_->RetrieveId(entityId_));
    if (entity.get())
        entity->SetName(oldName_);
}

void RenameCommand::redo()
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    EntityPtr entity = scene->EntityById(tracker_->RetrieveId(entityId_));
    if (entity.get())
        entity->SetName(newName_);
}

ToggleTemporaryCommand::ToggleTemporaryCommand(const QList<EntityWeakPtr> &entities, EntityIdChangeTracker * tracker, bool temporary, QUndoCommand *parent) :
    tracker_(tracker),
    temporary_(temporary),
    QUndoCommand(parent)
{
    if (entities.size() > 1)
        setText(QString("* Made multiple entities ") + (temporary_ ? "temporary" : "non-temporary"));
    else
    {
        QString name = entities.at(0).lock()->Name();
        setText(QString("* Made Entity ") + name.trimmed() + (temporary_ ? " temporary" : " non-temporary"));
    }

    scene_ = entities.at(0).lock()->ParentScene()->shared_from_this();

    for(QList<EntityWeakPtr>::const_iterator i = entities.begin(); i != entities.end(); ++i)
        entityIds_ << (*i).lock()->Id();
}

int ToggleTemporaryCommand::id() const
{
    return Id;
}

void ToggleTemporaryCommand::undo()
{
    ToggleTemporary(!temporary_);
}

void ToggleTemporaryCommand::redo()
{
    ToggleTemporary(temporary_);
}

void ToggleTemporaryCommand::ToggleTemporary(bool temporary)
{
    ScenePtr scene = scene_.lock();
    if (!scene.get())
        return;

    foreach(entity_id_t id, entityIds_)
    {
        EntityPtr entity = scene->EntityById(tracker_->RetrieveId(id));
        if (entity.get())
            entity->SetTemporary(temporary);
    }
}

TransformCommand::TransformCommand(const TransformAttributeWeakPtrList &attributes, int numberOfItems, Action action, const float3 &offset, QUndoCommand *parent) : 
    targets_(attributes),
    nItems_(numberOfItems),
    action_(action),
    offset_(offset),
    rotation_(float3x4::identity),
    QUndoCommand(parent)
{
    SetCommandText();
}

TransformCommand::TransformCommand(const TransformAttributeWeakPtrList &attributes, int numberOfItems, Action action, const float3x4 &rotation, QUndoCommand *parent) :
    targets_(attributes),
    nItems_(numberOfItems),
    action_(action),
    rotation_(rotation),
    offset_(float3::zero),
    QUndoCommand(parent)
{
    SetCommandText();
}

int TransformCommand::id() const
{
    return Id;
}

void TransformCommand::SetCommandText()
{
    QString text;
    switch(action_)
    {
        case Translate:
            text += "* Translated";
            break;
        case TranslateX:
            text += "* Translated X-axis";
            break;
        case TranslateY:
            text += "* Translated Y-axis";
            break;
        case TranslateZ:
            text += "* Translated Z-axis";
            break;
        case Rotate:
            text += "* Rotated";
            break;
        case RotateX:
            text += "* Rotated X-axis";
            break;
        case RotateY:
            text += "* Rotated Y-axis";
            break;
        case RotateZ:
            text += "* Rotated Z-axis";
            break;
        case Scale:
            text += "* Scaled";
            break;
        case ScaleX:
            text += "* Scaled X-axis";
            break;
        case ScaleY:
            text += "* Scaled Y-axis";
            break;
        case ScaleZ:
            text += "* Scaled Z-axis";
            break;
    }
    if (nItems_ > 1)
        text += QString(" on %1 Entities").arg(nItems_);
    else
        text += " on Entity";
    setText(text);
}

void TransformCommand::undo()
{
    switch(action_)
    {
        case Translate:
        case TranslateX:
        case TranslateY:
        case TranslateZ:
            DoTranslate(true);
            break;
        case Rotate:
        case RotateX:
        case RotateY:
        case RotateZ:
            DoRotate(true);
            break;
        case Scale:
        case ScaleX:
        case ScaleY:
        case ScaleZ:
            DoScale(true);
            break;
    }
}

void TransformCommand::redo()
{
    switch(action_)
    {
        case Translate:
        case TranslateX:
        case TranslateY:
        case TranslateZ:
            DoTranslate(false);
            break;
        case Rotate:
        case RotateX:
        case RotateY:
        case RotateZ:
            DoRotate(false);
            break;
        case Scale:
        case ScaleX:
        case ScaleY:
        case ScaleZ:
            DoScale(false);
            break;
    }
}

void TransformCommand::DoTranslate(bool isUndo)
{
    const float3 offset = (isUndo ? offset_.Neg() : offset_);

    foreach(const TransformAttributeWeakPtr &attr, targets_)
        TransformEditor::Translate(attr, offset, AttributeChange::Default);
}

void TransformCommand::DoRotate(bool isUndo)
{
    const float3x4 rotation = (isUndo ? rotation_.Inverted() : rotation_);

    foreach(const TransformAttributeWeakPtr &attr, targets_)
        TransformEditor::Rotate(attr, rotation, AttributeChange::Default);
}

void TransformCommand::DoScale(bool isUndo)
{
    const float3 offset = (isUndo ? offset_.Neg() : offset_);

    foreach(const TransformAttributeWeakPtr &attr, targets_)
        TransformEditor::Scale(attr, offset, AttributeChange::Default);
}

bool TransformCommand::mergeWith(const QUndoCommand *other)
{
    if (id() != other->id())
        return false;

    const TransformCommand *otherCommand = dynamic_cast<const TransformCommand*>(other);
    if (!otherCommand)
        return false;
    if (action_ != otherCommand->action_)
        return false;
    if (targets_ != otherCommand->targets_)
        return false;

    if (action_ != Rotate && action_ != RotateX && action_ != RotateY && action_ != RotateZ)
        offset_ += otherCommand->offset_;
    else
        rotation_ = otherCommand->rotation_.Mul(rotation_);
    return true;
}


/*
PasteCommand::PasteCommand(QUndoCommand *parent) :
    QUndoCommand(parent)
{
}

int PasteCommand::id() const
{
    return Id;
}

void PasteCommand::undo()
{
}

void PasteCommand::redo()
{
}
*/
