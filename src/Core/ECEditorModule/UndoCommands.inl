// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IAttribute.h"
#include "IComponent.h"
#include "Entity.h"

template <typename T>
EditAttributeCommand<T>::EditAttributeCommand(IAttribute *attr, QUndoCommand *parent) :
    IEditAttributeCommand(parent),
    undoValue(static_cast<Attribute<T> *>(attr)->Get())
{
    Initialize(attr, true);
}

template <typename T>
EditAttributeCommand<T>::EditAttributeCommand(IAttribute *attr, const T& valueToApply, QUndoCommand *parent) :
    IEditAttributeCommand(parent),
    undoValue(static_cast<Attribute<T> *>(attr)->Get()),
    redoValue(valueToApply)
{
    Initialize(attr, false);
}

/// @note Had to move the implementations of virtual functions to UndoCommands.h due to bogus MSVC warnings:
/// http://connect.microsoft.com/VisualStudio/feedback/details/778490/implicit-instantiation-of-virtual-template-method-causes-confusing-warning
#if 0
template <typename T>
int EditAttributeCommand<T>::id() const
{
    return Id;
}

template <typename T>
void EditAttributeCommand<T>::undo()
{
    EntityPtr ent = entity_.lock();
    if (!ent.get())
        return;

    ComponentPtr comp = ent->GetComponent(componentType_, componentName_);
    if (comp.get())
    {
        IAttribute *attr = comp->GetAttribute(name_);
        Attribute<T> *attribute = dynamic_cast<Attribute<T> *>(attr);
        if (attribute)
        {
            newValue_ = attribute->Get();
            attribute->Set(oldValue_, AttributeChange::Default);
        }
    }
}

template <typename T>
void EditAttributeCommand<T>::redo()
{
    if (dontCallRedo_)
    {
        dontCallRedo_ = false;
        return;
    }

    EntityPtr ent = entity_.lock();
    if (!ent.get())
        return;

    ComponentPtr comp = ent->GetComponent(componentType_, componentName_);
    if (comp.get())
    {   
        IAttribute *attr = comp->GetAttribute(name_);
        Attribute<T> *attribute = dynamic_cast<Attribute<T> *>(attr);
        if (attribute)
            attribute->Set(newValue_, AttributeChange::Default);
    }
}

//\ todo: Should we make a specialization for certain types of attributes that their 'Edit attribute' commands are merged,
//\       or should we keep each atomic change to the attributes in the stack?
//\       (e.g. if 'Transform' attribute for each of the components of each pos, rot, and scale float3s are edited, it 
//\        will push 9 commands into the undo stack)

template <typename T>
bool EditAttributeCommand<T>::mergeWith(const QUndoCommand * other)
{
    // Don't merge commands yet. This is only for the 'Color' attribute specialization
    return false;

    /*
    if (id() != other->id())
        return false;

    const EditAttributeCommand<T> *otherCommand = dynamic_cast<const EditAttributeCommand<T> *>(other);
    if (!otherCommand)
        return false;

    return true;
    */
}
#endif
