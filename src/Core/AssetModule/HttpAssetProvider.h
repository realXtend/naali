// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <boost/enable_shared_from_this.hpp>
#include "AssetModuleApi.h"
#include "IAssetProvider.h"
#include "AssetFwd.h"
#include "HttpAssetTransfer.h"
#include "HttpAssetStorage.h"

#include <QDateTime>
#include <QByteArray>

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

class HttpAssetStorage;
typedef boost::shared_ptr<HttpAssetStorage> HttpAssetStoragePtr;

/// Adds support for downloading assets over the web using the 'http://' specifier.
class ASSET_MODULE_API HttpAssetProvider : public QObject, public IAssetProvider, public boost::enable_shared_from_this<HttpAssetProvider>
{
    Q_OBJECT

public:
    explicit HttpAssetProvider(Framework *framework);
    
    virtual ~HttpAssetProvider();
    
    /// Returns the name of this asset provider.
    virtual QString Name();
    
    /// Checks an asset id for validity
    /** @return true if this asset provider can handle the id */
    virtual bool IsValidRef(QString assetRef, QString assetType = "");
    
    virtual AssetTransferPtr RequestAsset(QString assetRef, QString assetType);

    /// Adds the given http URL to the list of current asset storages.
    /// Returns the newly created storage, or 0 if a storage with the given name already existed, or if some other error occurred.
    /// @param storageName An identifier for the storage. Remember that Asset Storage names are case-insensitive.
    /// @param liveUpdate Whether assets will be reloaded whenever they change. \todo For HTTP storages, this currently means only watching the disk cache changes
    /// @param autoDiscoverable Whether recursive PROPFIND queries will be immediately performed on the storage to discover assets
    HttpAssetStoragePtr AddStorageAddress(const QString &address, const QString &storageName, bool liveUpdate = true, bool autoDiscoverable = false);

    virtual std::vector<AssetStoragePtr> GetStorages() const;

    virtual AssetStoragePtr GetStorageByName(const QString &name) const;

    virtual AssetStoragePtr GetStorageForAssetRef(const QString &assetRef) const;

    /// Starts an asset upload from the given file in memory to the given storage.
    virtual AssetUploadTransferPtr UploadAssetFromFileInMemory(const u8 *data, size_t numBytes, AssetStoragePtr destination, const QString &assetName);

    /// Issues a http DELETE request for the given asset.
    virtual void DeleteAssetFromStorage(QString assetRef);
    
    /// @param storageName An identifier for the storage. Remember that Asset Storage names are case-insensitive.
    virtual bool RemoveAssetStorage(QString storageName);

    virtual AssetStoragePtr TryDeserializeStorageFromString(const QString &storage, bool fromNetwork);

    QString GenerateUniqueStorageName() const;

    /// Return the network access manager
    QNetworkAccessManager* GetNetworkAccessManager() { return networkAccessManager; }
    
    /// Constructs QDateTime from a QByteArray header. Can detect and parse following formats:
    /// ANSI C's asctime(), RFC 822, updated by RFC 1123 and RFC 850, obsoleted by RFC 1036.
    QDateTime FromHttpDate(const QByteArray &value);
    
    /// Constructs a QByteArray from QDateTime. Returns value as Sun, 06 Nov 1994 08:49:37 GMT - RFC 822.
    QByteArray ToHttpDate(const QDateTime &dateTime);

private slots:
    void AboutToExit();
    void OnHttpTransferFinished(QNetworkReply *reply);
    
private:
    Framework *framework;
    
    /// Creates our QNetworkAccessManager
    void CreateAccessManager();

    /// Add assetref to http storage(s) after successful upload or discovery
    void AddAssetRefToStorages(const QString& ref);

    /// Delete assetref from http storages after successful delete
    void DeleteAssetRefFromStorages(const QString& ref);
    
    /// Specifies the currently added list of HTTP asset storages.
    /// This array will never store null pointers.
    std::vector<HttpAssetStoragePtr> storages;

    /// The top-level Qt object that manages all network gets.
    QNetworkAccessManager *networkAccessManager;

    /// Maps each Qt Http download transfer we start to Asset API internal HttpAssetTransfer struct.
    typedef std::map<QNetworkReply*, HttpAssetTransferPtr> TransferMap;
    TransferMap transfers;

    /// Maps each Qt Http upload transfer we start to Asset API internal HttpAssetTransfer struct.
    typedef std::map<QNetworkReply*, AssetUploadTransferPtr> UploadTransferMap;
    UploadTransferMap uploadTransfers;

    /// If true, asset requests outside any registered storages are also accepted, and will appear as
    /// assets with no storage. If false, all requests to assets outside any registered storage will fail.
    bool enableRequestsOutsideStorages;
};

