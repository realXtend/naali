// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <QObject>
#include <QByteArray>

#include "CoreTypes.h"
#include "AssetFwd.h"
#include "IAssetStorage.h"
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

/// Represents a currently ongoing asset upload operation.
class IAssetUploadTransfer : public QObject, public boost::enable_shared_from_this<IAssetUploadTransfer>
{
    Q_OBJECT

public:
    virtual ~IAssetUploadTransfer() {}

    /// Returns the current transfer progress in the range [0, 1].
    virtual float Progress() const { return 0.f; }

    /// Specifies the source file of the upload transfer, or none if this upload does not originate from a file in the system.
    QString sourceFilename;

    /// Contains the raw asset data to upload. If sourceFilename=="", the data is taken from this array instead.
    std::vector<u8> assetData;

    /// Specifies the destination name for the asset.
    QString destinationName;

    boost::weak_ptr<IAssetStorage> destinationStorage;

    AssetProviderWeakPtr destinationProvider;

    void EmitTransferCompleted();
    void EmitTransferFailed();

public slots:
    /// Returns the full assetRef address this asset will have when the upload is complete.
    QString AssetRef()
    { 
        boost::shared_ptr<IAssetStorage> storage = destinationStorage.lock();
        if (!storage)
            return "";
        return storage->GetFullAssetURL(destinationName);
    }

    QByteArray RawData() const { return QByteArray::fromRawData((const char*)&assetData[0], assetData.size()); }

    QString SourceFilename() const { return sourceFilename; }
    QString DestinationName() const { return destinationName; }

signals:
    void Completed(IAssetUploadTransfer *transfer);

    void Failed(IAssetUploadTransfer *transfer);
};

