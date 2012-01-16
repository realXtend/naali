// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "Application.h"
#include "HttpAssetProvider.h"
#include "HttpAssetTransfer.h"
#include "IAssetUploadTransfer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "IAsset.h"
#include "LoggingFunctions.h"

#include <QAbstractNetworkCache>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "MemoryLeakCheck.h"

HttpAssetProvider::HttpAssetProvider(Framework *framework_) :
    framework(framework_),
    networkAccessManager(0)
{
    CreateAccessManager();
    connect(framework->App(), SIGNAL(ExitRequested()), SLOT(AboutToExit()));

    enableRequestsOutsideStorages = framework_->HasCommandLineParameter("--accept_unknown_http_sources");
}

HttpAssetProvider::~HttpAssetProvider()
{
}

void HttpAssetProvider::CreateAccessManager()
{
    if (!networkAccessManager)
    {
        networkAccessManager = new QNetworkAccessManager();
#ifndef DISABLE_QNETWORKDISKCACHE
        networkAccessManager->setCache(framework->Asset()->GetAssetCache());
#endif
        connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)), SLOT(OnHttpTransferFinished(QNetworkReply*)));
    }
}

void HttpAssetProvider::AboutToExit()
{
    // Check if someone has canceled the exit command.
    if (!framework->IsExiting())
        return;

    if (networkAccessManager)
    {
        // We must reset our AssetCaches parent before destroying QNAM
        // otherwise we will crash in AssetAPI without keepinga QPointer<AssetCache>
        // so we would know when the QObject is destroyed by qt. This would then again
        // including AssetCache.h to AssetAPI.h that we certainly dont want.
        QAbstractNetworkCache *cache = networkAccessManager->cache();
        if (cache)
            cache->setParent(0);
        SAFE_DELETE(networkAccessManager);
    }
}

QString HttpAssetProvider::Name()
{
    return "HttpAssetProvider";
}

bool HttpAssetProvider::IsValidRef(QString assetRef, QString)
{
    QString protocol;
    AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(assetRef.trimmed(), &protocol);
    if (refType == AssetAPI::AssetRefExternalUrl && 
        (protocol == "http" || protocol == "https"))
        return true;
    else
        return false;
}
        
AssetTransferPtr HttpAssetProvider::RequestAsset(QString assetRef, QString assetType)
{
    if (!networkAccessManager)
        CreateAccessManager();

    if (!enableRequestsOutsideStorages)
    {
        AssetStoragePtr storage = GetStorageForAssetRef(assetRef);
        if (!storage)
        {
            LogError("HttpAssetProvider::RequestAsset: Discarding asset request to URL \"" + assetRef + "\" because requests to sources outside HttpAssetStorages have been forbidden. (See --accept_unknown_http_sources).");
            return AssetTransferPtr();
        }
    }

    QString originalAssetRef = assetRef;
    assetRef = assetRef.trimmed();
    QString assetRefWithoutSubAssetName;
    AssetAPI::ParseAssetRef(assetRef, 0, 0, 0, 0, 0, 0, 0, 0, 0, &assetRefWithoutSubAssetName);
    assetRef = assetRefWithoutSubAssetName;
    if (!IsValidRef(assetRef))
    {
        LogError("HttpAssetProvider::RequestAsset: Cannot get asset from invalid URL \"" + assetRef + "\"!");
        return AssetTransferPtr();
    }

    QNetworkRequest request;
    request.setUrl(QUrl(assetRef));
    request.setRawHeader("User-Agent", "realXtend Tundra");

    QNetworkReply *reply = networkAccessManager->get(request);

    HttpAssetTransferPtr transfer = HttpAssetTransferPtr(new HttpAssetTransfer);
    transfer->source.ref = originalAssetRef;
    transfer->assetType = assetType;
    transfer->provider = shared_from_this();
    transfer->storage = GetStorageForAssetRef(assetRef);
    transfer->diskSourceType = IAsset::Cached; // The asset's disksource will represent a cached version of the original on the http server
    transfers[reply] = transfer;
    return transfer;
}

AssetUploadTransferPtr HttpAssetProvider::UploadAssetFromFileInMemory(const u8 *data, size_t numBytes, AssetStoragePtr destination, const QString &assetName)
{
    if (!networkAccessManager)
        CreateAccessManager();

    QString dstUrl = destination->GetFullAssetURL(assetName);
    QNetworkRequest request;
    request.setUrl(QUrl(dstUrl));
    request.setRawHeader("User-Agent", "realXtend Tundra");

    QByteArray dataArray((const char*)data, numBytes);
    QNetworkReply *reply = networkAccessManager->put(request, dataArray);

    AssetUploadTransferPtr transfer = AssetUploadTransferPtr(new IAssetUploadTransfer());
    transfer->destinationStorage = destination;
    transfer->destinationProvider = shared_from_this();
    transfer->destinationName = assetName;

    uploadTransfers[reply] = transfer;

    return transfer;
}

void HttpAssetProvider::DeleteAssetFromStorage(QString assetRef)
{
    if (!networkAccessManager)
        CreateAccessManager();

    assetRef = assetRef.trimmed();
    if (!IsValidRef(assetRef))
    {
        LogError("HttpAssetProvider::DeleteAssetFromStorage: Cannot delete asset from invalid URL \"" + assetRef + "\"!");
        return;
    }
    QUrl assetUrl(assetRef);
    QNetworkRequest request;
    request.setUrl(QUrl(assetRef));
    request.setRawHeader("User-Agent", "realXtend Tundra");

    networkAccessManager->deleteResource(request);
}

bool HttpAssetProvider::RemoveAssetStorage(QString storageName)
{
    for(size_t i = 0; i < storages.size(); ++i)
        if (storages[i]->storageName.compare(storageName, Qt::CaseInsensitive) == 0)
        {
            storages.erase(storages.begin() + i);
            return true;
        }

    return false;
}

AssetStoragePtr HttpAssetProvider::TryDeserializeStorageFromString(const QString &storage, bool fromNetwork)
{
    QMap<QString, QString> s = AssetAPI::ParseAssetStorageString(storage);
    if (s.contains("type") && s["type"].compare("HttpAssetStorage", Qt::CaseInsensitive) != 0)
        return AssetStoragePtr();
    if (!s.contains("src"))
        return AssetStoragePtr();
    
    QString path;
    QString protocolPath;
    AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(s["src"], 0, 0, &protocolPath, 0, 0, &path);

    if (refType != AssetAPI::AssetRefExternalUrl)
        return AssetStoragePtr();

    QString name = (s.contains("name") ? s["name"] : GenerateUniqueStorageName());

    bool liveUpdate = true;
    bool autoDiscoverable = false;
    if (s.contains("liveupdate"))
        liveUpdate = ParseBool(s["liveupdate"]);
    if (s.contains("autodiscoverable"))
        autoDiscoverable = ParseBool(s["autodiscoverable"]);
    
    HttpAssetStoragePtr newStorage = AddStorageAddress(protocolPath, name, liveUpdate, autoDiscoverable);

    // Set local dir if specified
    ///\bug Refactor these sets to occur inside AddStorageAddress so that when the NewStorageAdded signal is emitted, these values are up to date.
    if (newStorage)
    {
        if (!fromNetwork && s.contains("localdir")) // If we get a storage from a remote computer, discard the localDir parameter if it had been set.
            newStorage->localDir = GuaranteeTrailingSlash(s["localdir"]);
        if (s.contains("readonly"))
            newStorage->writable = !ParseBool(s["readonly"]);
        if (s.contains("replicated"))
            newStorage->SetReplicated(ParseBool(s["replicated"]));
        if (s.contains("trusted"))
            newStorage->trustState = IAssetStorage::TrustStateFromString(s["trusted"]);
    }
    
    return newStorage;
}

QString HttpAssetProvider::GenerateUniqueStorageName() const
{
    QString name = "Web";
    int counter = 2;
    while(GetStorageByName(name) != 0)
        name = "Web" + QString::number(counter++);
    return name;
}

void HttpAssetProvider::OnHttpTransferFinished(QNetworkReply *reply)
{
    // QNetworkAccessManager requires us to delete the QNetworkReply, or it will leak.
    reply->deleteLater();

    switch(reply->operation())
    {
    case QNetworkAccessManager::GetOperation:
    {
        QByteArray data = reply->readAll();
        TransferMap::iterator iter = transfers.find(reply);
        if (iter == transfers.end())
        {
            LogError("Received a finish signal of an unknown Http transfer!");
            return;
        }
        HttpAssetTransferPtr transfer = iter->second;
        assert(transfer);
        transfer->rawAssetData.clear();

        if (reply->error() == QNetworkReply::NoError)
        {
#ifndef DISABLE_QNETWORKDISKCACHE
            // If asset request creator has not allowed caching, remove it now
            AssetCache *cache = framework->Asset()->GetAssetCache();
            if (!transfer->CachingAllowed())
                cache->remove(reply->url());

            // Setting cache allowed as false is very important! The items are already in our cache via the 
            // QAccessManagers QAbstractNetworkCache (same as our AssetAPI::AssetCache). Network replies will already call them
            // so the AssetAPI::AssetTransferCompletes doesn't have to.
            // @note GetDiskSource() will return empty string if above cache remove was performed, this is wanted behaviour.
            transfer->SetCachingBehavior(false, cache->FindInCache(reply->url().toString()));
#endif

            // Copy raw data to transfer
            transfer->rawAssetData.insert(transfer->rawAssetData.end(), data.data(), data.data() + data.size());
            framework->Asset()->AssetTransferCompleted(transfer.get());
        }
        else
        {
            QString error = "Http GET for address \"" + reply->url().toString() + "\" returned an error: \"" + reply->errorString() + "\"";
            framework->Asset()->AssetTransferFailed(transfer.get(), error);
        }
        transfers.erase(iter);
        break;
    }
    case QNetworkAccessManager::PutOperation:
    case QNetworkAccessManager::PostOperation:
    {
        UploadTransferMap::iterator iter = uploadTransfers.find(reply);
        if (iter == uploadTransfers.end())
        {
            LogError("Received a finish signal of an unknown Http upload transfer!");
            return;
        }
        AssetUploadTransferPtr transfer = iter->second;

        if (reply->error() == QNetworkReply::NoError)
        {
            QString ref = reply->url().toString();
            LogDebug("Http upload to address \"" + ref + "\" returned successfully.");
            framework->Asset()->AssetUploadTransferCompleted(transfer.get());
            // Add the assetref to matching storage(s)
            AddAssetRefToStorages(ref);
        }
        else
        {
            LogError("Http upload to address \"" + reply->url().toString() + "\" failed with an error: \"" + reply->errorString() + "\"");
            ///\todo Call the following when implemented:
//            framework->Asset()->AssetUploadTransferFailed(transfer);
        }
        uploadTransfers.erase(iter);
        break;
    }
    case QNetworkAccessManager::DeleteOperation:
        if (reply->error() == QNetworkReply::NoError)
        {
            QString ref = reply->url().toString();
            LogInfo("Http DELETE to address \"" + ref + "\" returned successfully.");
            DeleteAssetRefFromStorages(ref);
            framework->Asset()->EmitAssetDeletedFromStorage(ref);
        }
        else
            LogError("Http DELETE to address \"" + reply->url().toString() + "\" failed with an error: \"" + reply->errorString() + "\"");
        break;
        /*
    default:
        LogInfo("Unknown operation for address \"" + reply->url().toString() + "\" finished with result: \"" + reply->errorString() + "\"");
        break;
        */
    }
}

HttpAssetStoragePtr HttpAssetProvider::AddStorageAddress(const QString &address, const QString &storageName, bool liveUpdate, bool autoDiscoverable)
{    QString locationCleaned = GuaranteeTrailingSlash(address.trimmed());

    // Check if a storage with this name already exists.
    for(size_t i = 0; i < storages.size(); ++i)
        if (storages[i]->storageName.compare(storageName, Qt::CaseInsensitive) == 0)
        {
            if (storages[i]->baseAddress != address)
                LogError("HttpAssetProvider::AddStorageAddress failed: A storage by name \"" + storageName + "\" already exists, but points to address \"" + storages[i]->baseAddress + "\" instead of \"" + address + "\"!");
            return HttpAssetStoragePtr();
        }

    // Add new if not found
    HttpAssetStoragePtr storage = HttpAssetStoragePtr(new HttpAssetStorage());
    storage->baseAddress = locationCleaned;
    storage->storageName = storageName;
    storage->liveUpdate = liveUpdate;
    storage->autoDiscoverable = autoDiscoverable;
    storage->provider = this->shared_from_this();
    storages.push_back(storage);
    
    // Tell the Asset API that we have created a new storage.
    framework->Asset()->EmitAssetStorageAdded(storage);

    if (storage->AutoDiscoverable())
        storage->RefreshAssetRefs(); // Initiate PROPFIND
    
    return storage;
}

std::vector<AssetStoragePtr> HttpAssetProvider::GetStorages() const
{
    std::vector<AssetStoragePtr> s;
    for(size_t i = 0; i < storages.size(); ++i)
        s.push_back(storages[i]);

    return s;
}

AssetStoragePtr HttpAssetProvider::GetStorageByName(const QString &name) const
{
    for(size_t i = 0; i < storages.size(); ++i)
        if (storages[i]->storageName.compare(name, Qt::CaseInsensitive) == 0)
            return storages[i];

    return AssetStoragePtr();
}

AssetStoragePtr HttpAssetProvider::GetStorageForAssetRef(const QString &assetRef) const
{
    QString namedStorage;
    QString protocolPath;
    AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(assetRef, 0, &namedStorage, &protocolPath);
    for (size_t i = 0; i < storages.size(); ++i)
        if (refType == AssetAPI::AssetRefNamedStorage && storages[i]->Name() == namedStorage)
    return storages[i];
        else if (refType == AssetAPI::AssetRefExternalUrl && assetRef.startsWith(storages[i]->baseAddress, Qt::CaseInsensitive))
    return storages[i];

    return HttpAssetStoragePtr();
}

void HttpAssetProvider::AddAssetRefToStorages(const QString& ref)
{
    for (size_t i = 0; i < storages.size(); ++i)
        if (ref.indexOf(storages[i]->baseAddress) == 0)
            storages[i]->AddAssetRef(ref);
}

void HttpAssetProvider::DeleteAssetRefFromStorages(const QString& ref)
{
    for (size_t i = 0; i < storages.size(); ++i)
        storages[i]->DeleteAssetRef(ref);
}
