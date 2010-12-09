#include "StableHeaders.h"
#include "AvatarDescAsset.h"

#include <QDomDocument>

bool AvatarDescAsset::LoadFromFileInMemory(const u8 *data, size_t numBytes)
{
    // Just store the raw downloaded Avatar Appearance XML data for now..    
    QByteArray bytes((const char *)data, numBytes);
    avatarAppearanceXML = QString(bytes);
    // Parse the refs from the XML
    ParseReferences();

    return true;
}

void AvatarDescAsset::ParseReferences()
{
    assetReferences.clear();
    
    QDomDocument avatar_doc("Avatar");
    if (!avatar_doc.setContent(avatarAppearanceXML))
        return;
    
    // The avatar desc itself is incomplete in regard to asset references. We need to parse the "assetmap"
    // which is an extension originally developed for inventory/webdav avatars
    
    // Note: this is presently commented out, because AvatarAppearance does its own asset reference tracking, and
    // uses still the old Ogre resource system. The commenting out prevents assets from being doubly loaded
    
    //QDomElement avatar = avatar_doc.firstChildElement("avatar");
    //if (!avatar.isNull())
    //{
    //    QDomElement assetmap = avatar.firstChildElement("assetmap");
    //    if (!assetmap.isNull())
    //    {
    //        QDomElement asset = assetmap.firstChildElement("asset");
    //        while (!asset.isNull())
    //        {
    //            AssetReference newRef;
    //            newRef.ref = asset.attribute("id");
    //            if (!newRef.ref.isEmpty())
    //                assetReferences.push_back(newRef);
    //            
    //            asset = asset.nextSiblingElement("asset");
    //        }
    //    }
    //}
}
