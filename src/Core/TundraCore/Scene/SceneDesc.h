/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneDesc.h
    @brief  Light-weigth structures for describing scene and its contents. */

#pragma once

#include "TundraCoreApi.h"
#include "SceneFwd.h"

#include <QMap>
#include <QPair>

/// Description of a scene (Scene).
/** A source-agnostic scene graph description of a Tundra scene.
    A Tundra scene consist of entities, components, attributes and assets references.
    @sa EntityDesc, ComponentDesc, AttributeDesc and AssetDesc */
struct TUNDRACORE_API SceneDesc
{
    typedef QPair<QString, QString> AssetMapKey; ///< source-subname pair used to idenfity assets.
    typedef QMap<AssetMapKey, AssetDesc> AssetMap; ///< Map of assets.

    QString filename; ///< Name of the file from which the description was created.
    QString name; ///< Name.
    bool viewEnabled; ///< Is scene view enabled (ie. rendering-related components actually create stuff)
    QList<EntityDesc> entities; ///< List of entities the scene has.
    AssetMap assets; ///< Map of unique assets.

    /// Returns true if the scene description has no entities, false otherwise.
    bool IsEmpty() const { return entities.isEmpty(); }

    /// Equality operator. Returns true if all values match, false otherwise.
    bool operator ==(const SceneDesc &rhs) const
    {
        return name == rhs.name && viewEnabled == rhs.viewEnabled && entities == rhs.entities;
    }
};

/// Description of an entity (Entity).
struct TUNDRACORE_API EntityDesc
{
    QString id; ///< ID (if applicable).
    QString name; ///< Name (EC_Name::name).
    QString group; ///< Group (EC_Name::group).
    bool local; ///< Is entity local.
    bool temporary; ///< Is entity temporary.
    QList<ComponentDesc> components; ///< List of components the entity has.

    /// Default constructor.
    EntityDesc() : local(false), temporary(false) {}

    /// Constructor with full input param list.
    EntityDesc(const QString &entityId, const QString &entityName = "", bool isLocal = false, bool isTemporary = false) :
        id(entityId),
        name(entityName),
        local(isLocal),
        temporary(isTemporary)
    {
    }

    /// Equality operator. Returns true if ID and name match, false otherwise.
    bool operator ==(const EntityDesc &rhs) const
    {
        return id == rhs.id && name == rhs.name /*&& local == rhs.local && temporary == rhs.temporary && components == rhs.components*/;
    }
};

/// Description of an entity-component (EC_*, IComponent).
struct TUNDRACORE_API ComponentDesc
{
     /// Unique type name.
    /** @note Might or might not have the "EC_"-prefix so remember to take that into account.
        See IComponent::EnsureTypeNameWithoutPrefix, IComponent::EnsureTypeNameWithPrefix. */
    QString typeName;
    u32 typeId; /**< Unique type ID, if available, 0xffffffff if not. */
    QString name; ///< Name (if applicable).
    bool sync; ///< Synchronize component.
    QList<AttributeDesc> attributes; ///< List of attributes the component has.

    ComponentDesc() : sync(true), typeId(0xffffffff) {}

    /// Equality operator. Returns true if all values match, false otherwise.
    bool operator ==(const ComponentDesc &rhs) const
    {
        return typeName == rhs.typeName && name == rhs.name && attributes == rhs.attributes;
    }
};

/// Description of an attribute (IAttribute).
/** @note Attribute's type name, name and ID names are handled case-insensitively internally by the SceneAPI,
    so a case-insensitive comparison is always recommended these values. */
struct TUNDRACORE_API AttributeDesc
{
    QString typeName; ///< Attribute type name, f.ex. "Color".
    QString name; ///< Human-readable attribute name, f.ex. "Ambient light color".
    QString value; ///< Value of the attribute serialized to string, f.ex. "ambientLightColor".
    QString id; ///< Unique ID (within the parent component), i.e. the variable name, of the attribute.

#define LEX_CMP(a, b, sensitivity) if (a.compare(b, sensitivity) < 0) return true; else if (a.compare(b, sensitivity) > 0) return false;

    /// Less than operator.
    /** @note typeName, id, and name are compared case-insensitively, value as case-sensitively. */
    bool operator <(const AttributeDesc &rhs) const
    {
        LEX_CMP(typeName, rhs.typeName, Qt::CaseInsensitive);
        LEX_CMP(id, rhs.id, Qt::CaseInsensitive);
        LEX_CMP(name, rhs.name, Qt::CaseInsensitive);
        LEX_CMP(value, rhs.value, Qt::CaseSensitive);
        return false;
    }

#undef LEX_CMP

    /// Equality operator. Returns true if all values match, false otherwise.
    /** @note typeName, id, and name are compared case-insensitively, value as case-sensitively. */
    bool operator ==(const AttributeDesc &rhs) const
    {
        return typeName.compare(rhs.typeName,Qt::CaseInsensitive) == 0 &&
            name.compare(rhs.name, Qt::CaseInsensitive) == 0 &&
            id.compare(rhs.id, Qt::CaseInsensitive) == 0 &&
            value  == rhs.value;
    }
};

/// Description of an asset (*Asset, IAsset) or an asset reference (AssetReference).
struct TUNDRACORE_API AssetDesc
{
    QString source; ///< Specifies the source filename for the location of this asset.
    QByteArray data; ///< Specifies in-memory content for the asset data.

    /// If true, the data for this asset is loaded in memory, and specified by the member field 'data'. Otherwise,
    /// the data is loaded from disk, specified by the filename 'source'.
    bool dataInMemory;

    QString subname; ///< If the source filename is a container for multiple files, subname represents name within the file.
    QString typeName; ///< Type name of the asset.
    QString destinationName; ///< Name for the asset in the destination asset storage.

#define LEX_CMP(a, b) if ((a) < (b)) return true; else if ((a) > (b)) return false;

    /// Less than operator. Compares source and subname only.
    bool operator <(const AssetDesc &rhs) const
    {
        LEX_CMP(source, rhs.source)
        LEX_CMP(subname, rhs.subname)
        return false;
    }

#undef LEX_CMP

    /// Equality operator. Returns true if filenames match, false otherwise.
    bool operator ==(const AssetDesc &rhs) const { return source == rhs.source; }
};
