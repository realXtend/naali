// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IAssetTypeFactory.h"
#include "BinaryAsset.h"

/// A factory for instantiating assets of a templated type T.
/** GenericAssetFactory is a predefined concrete factory type anyone defining a new asset type can use
    to create new assets of any type. */
template<typename AssetType>
class GenericAssetFactory : public IAssetTypeFactory
{
public:
    explicit GenericAssetFactory(const char *assetType_)
    {
        assert(assetType_ && "Must specify an asset type for asset factory!");
        assetType = assetType_;
        assetType = assetType.trimmed();
        assert(!assetType.isEmpty() && "Must specify an asset type for asset factory!");
    }

    virtual QString Type() const { return assetType; }

    virtual AssetPtr CreateEmptyAsset(AssetAPI *owner, const QString &name) { return AssetPtr(new AssetType(owner, Type(), name)); }

private:
    QString assetType;
};

/// For simple asset types the client wants to parse, we define the BinaryAssetFactory type.
typedef GenericAssetFactory<BinaryAsset> BinaryAssetFactory;

