// For conditions of distribution and use, see copyright notice in LICENSE

#include "DebugOperatorNew.h"

#include "AssetRefListener.h"
#include "IAttribute.h"
#include "AssetReference.h"
#include "IComponent.h"
#include "Framework.h"
#include "AssetAPI.h"
#include "IAsset.h"
#include "IAssetTransfer.h"
#include "LoggingFunctions.h"

#include "MemoryLeakCheck.h"

AssetPtr AssetRefListener::Asset() const
{
    return asset.lock();
}

void AssetRefListener::HandleAssetRefChange(IAttribute *assetRef, const QString& assetType)
{
    Attribute<AssetReference> *attr = dynamic_cast<Attribute<AssetReference> *>(assetRef);
    if (!attr)
    {
        LogWarning("AssetRefListener::HandleAssetRefChange: Attribute's type not AssetReference (was " +
            (assetRef == 0 ? "null" : assetRef->TypeName()) + " instead).");
        return;
    }
    HandleAssetRefChange(attr->Owner()->GetFramework()->Asset(), attr->Get().ref, assetType);
}

void AssetRefListener::HandleAssetRefChange(AssetAPI *assetApi, QString assetRef, const QString& assetType)
{
    // Disconnect from any previous transfer we might be listening to
    if (!currentTransfer.expired())
    {
        IAssetTransfer* current = currentTransfer.lock().get();
        current->disconnect(this, SLOT(OnTransferSucceeded(AssetPtr)));
        current->disconnect(this, SLOT(OnTransferFailed(IAssetTransfer*, QString)));
        currentTransfer.reset();
    }
    
    assert(assetApi);

    // Connect to AssetAPI signal once. Must keep as Qt::QueuedConnection for a delayed signal.
    if (!myAssetAPI)
        myAssetAPI = assetApi;
    disconnect(myAssetAPI, SIGNAL(AssetCreated(AssetPtr)), this, SLOT(OnAssetCreated(AssetPtr)));

    assetRef = assetRef.trimmed();
    requestedRef = AssetReference(assetRef, assetType);
    inspectCreated = false;

    AssetTransferPtr transfer = assetApi->RequestAsset(assetRef, assetType);
    if (!transfer)
    {
        LogWarning("AssetRefListener::HandleAssetRefChange: Asset request for " + assetRef + " failed.");
        return;
    }

    connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), this, SLOT(OnTransferSucceeded(AssetPtr)), Qt::UniqueConnection);
    connect(transfer.get(), SIGNAL(Failed(IAssetTransfer*, QString)), this, SLOT(OnTransferFailed(IAssetTransfer*, QString)), Qt::UniqueConnection);
    currentTransfer = transfer;
    
    // Disconnect from the old asset's load signal
    AssetPtr assetData = asset.lock();
    if (assetData)
        disconnect(assetData.get(), SIGNAL(Loaded(AssetPtr)), this, SIGNAL(Loaded(AssetPtr)));
    asset = AssetPtr();
}

void AssetRefListener::OnTransferSucceeded(AssetPtr assetData)
{
    assert(assetData);
    if (!assetData)
        return;
    
    asset = assetData;
    
    // Connect to further reloads of the asset to be able to notify of them.
    connect(assetData.get(), SIGNAL(Loaded(AssetPtr)), this, SLOT(OnAssetLoaded(AssetPtr)), Qt::UniqueConnection);
    
    emit Loaded(assetData);
}

void AssetRefListener::OnAssetLoaded(AssetPtr assetData)
{
    if (assetData == asset.lock())
    {
        inspectCreated = false;
        emit Loaded(assetData);
    }
}

void AssetRefListener::OnTransferFailed(IAssetTransfer* transfer, QString reason)
{
    if (myAssetAPI)
        connect(myAssetAPI, SIGNAL(AssetCreated(AssetPtr)), this, SLOT(OnAssetCreated(AssetPtr)), Qt::QueuedConnection);

    inspectCreated = true;
    emit TransferFailed(transfer, reason);
}

void AssetRefListener::OnAssetCreated(AssetPtr asset)
{
    if (!asset.get() || !myAssetAPI || !inspectCreated)
        return;

    // If out latest failed ref is the same as the created asset.
    // Request it now so we can emit the Loaded signal.
    if (requestedRef.ref == asset->Name())
    {
        disconnect(myAssetAPI, SIGNAL(AssetCreated(AssetPtr)), this, SLOT(OnAssetCreated(AssetPtr)));

        LogInfo("AssetRefListener: Asset \"" + asset->Name() + "\" was created, re-requesting asset.");
        HandleAssetRefChange(myAssetAPI, requestedRef.ref, requestedRef.type);
    }
}
