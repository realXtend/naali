// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpAssetProvider.h"
#include "HttpAssetStorage.h"
#include "LoggingFunctions.h"

#include <QNetworkReply>
#include <QBuffer>
#include <QDomDocument>

HttpAssetStorage::HttpAssetStorage()
{
}

QString HttpAssetStorage::GetFullAssetURL(const QString &localName)
{
    if (localName.startsWith("http://") || localName.startsWith("https://"))
        return localName;
    else
        return GuaranteeTrailingSlash(baseAddress) + localName;
}

QString HttpAssetStorage::Type() const
{
    return "HttpAssetStorage";
}

bool HttpAssetStorage::Trusted() const
{
    // HttpAssetStorages from the local system area always trusted. Otherwise, use the specified trust setting.
    return !localDir.isEmpty() || trustState == StorageTrusted;
}

IAssetStorage::TrustState HttpAssetStorage::GetTrustState() const
{ 
    if (!localDir.isEmpty())
        return StorageTrusted;
    else
        return trustState;
}

void HttpAssetStorage::RefreshAssetRefs()
{
    // If searches already ongoing, let them finish and don't start a new refresh
    if (!searches.empty())
        return;
    
//    assetRefs.clear();
    
    QNetworkAccessManager* mgr = GetNetworkAccessManager();
    if (!mgr)
    {
        LogError("Could not get QNetworkAccessManager; unable to refresh asset refs.");
        return;
    }
    connect(mgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(OnHttpTransferFinished(QNetworkReply*)), Qt::UniqueConnection);
    
    QUrl baseUrl(baseAddress);
    PerformSearch(baseUrl.path());
}

QString HttpAssetStorage::SerializeToString(bool networkTransfer) const
{
    QString str = "type=" + Type() + ";name=" + storageName +  ";src=" + baseAddress + ";readonly=" + BoolToString(!writable) +
        ";liveupdate=" + BoolToString(liveUpdate) + ";autodiscoverable=" + BoolToString(autoDiscoverable) + ";replicated=" +
        BoolToString(isReplicated) + ";trusted=" + TrustStateToString(trustState);
    if (!networkTransfer)
        str = str + (localDir.isEmpty() ? QString() : ";localdir=" + localDir);
    return str;
}

void HttpAssetStorage::PerformSearch(QString path)
{
    QNetworkAccessManager* mgr = GetNetworkAccessManager();
    if (!mgr)
        return;
    
    QUrl searchUrl(baseAddress);
    searchUrl.setPath(path);
    
    LogDebug("Performing PROPFIND on " + searchUrl.toString());
    
    QNetworkRequest request;
    request.setUrl(searchUrl);
    request.setRawHeader("User-Agent", "realXtend Tundra");
    request.setRawHeader("Depth", "1");
    
    SearchRequest newSearch;
    
    newSearch.reply = mgr->sendCustomRequest(request, "PROPFIND");
    searches.push_back(newSearch);
}

QNetworkAccessManager* HttpAssetStorage::GetNetworkAccessManager()
{
    HttpAssetProvider* httpProvider = dynamic_cast<HttpAssetProvider*>(provider.lock().get());
    if (!httpProvider)
        return 0;
    else
        return httpProvider->GetNetworkAccessManager();
}

void HttpAssetStorage::OnHttpTransferFinished(QNetworkReply *reply)
{
    // Note: we reuse the HttpAssetProvider's QNetworkAccessManager, and HttpAssetProvider will deletelater
    // the QNetworkReply objects, so we don't have to do it
    bool known = false;
    for (unsigned i = 0; i < searches.size(); ++i)
    {
        if (reply == searches[i].reply)
        {
            searches.erase(searches.begin() + i);
            known = true;
            break;
        }
    }
    
    if (!known)
        return;
    
    switch(reply->operation())
    {
    case QNetworkAccessManager::CustomOperation:
        {
            QByteArray response = reply->readAll();
            if (reply->error() != QNetworkReply::NoError)
                LogError("PROPFIND failed for url " + reply->url().toString());
            else
            {
                QDomDocument doc;
                if (!doc.setContent(response))
                {
                    LogError("Failed to deserialize PROPFIND response");
                    return;
                }
                
                QDomElement response = doc.firstChildElement("D:multistatus").firstChildElement("D:response");
                if (response.isNull())
                    response = doc.firstChildElement("D:response");
                while (!response.isNull())
                {
                    QDomElement ref = response.firstChildElement("D:href");
                    if (!ref.isNull())
                    {
                        QString refUrl = ref.text();
                        // If url ends in a slash, it's a directory we should query further
                        if (refUrl.endsWith('/'))
                        {
                            // Except if it's the base
                            if (refUrl != reply->url().path())
                                PerformSearch(refUrl);
                        }
                        else
                        {
                            QUrl baseUrl(baseAddress);
                            QString newAssetRef = baseUrl.scheme() + "://" + baseUrl.authority() + refUrl;
                            if (!assetRefs.contains(newAssetRef))
                            {
                                assetRefs.push_back(newAssetRef);
                                emit AssetChanged(refUrl, "", IAssetStorage::AssetCreate);
                            }
                            LogDebug("PROPFIND found assetref " + newAssetRef);
                        }
                    }
                    
                    response = response.nextSiblingElement("D:response");
                }
            }
        }
        break;
    }

    // If no outstanding searches, asset discovery is done
    if (searches.empty())
        LogDebug("HttpAssetStorage::OnHttpTransferFinished: Asset discovery done.");
}

void HttpAssetStorage::AddAssetRef(const QString& ref)
{
    if (!assetRefs.contains(ref))
    {
        assetRefs.push_back(ref);
        emit AssetChanged(QUrl(ref).authority(), "", IAssetStorage::AssetCreate);
    }
}

void HttpAssetStorage::DeleteAssetRef(const QString& ref)
{
    if (assetRefs.contains(ref))
    {
        assetRefs.removeAll(ref);
        emit AssetChanged(QUrl(ref).authority(), "", IAssetStorage::AssetDelete);
    }
}
