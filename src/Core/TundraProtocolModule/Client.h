// For conditions of distribution and use, see copyright notice in license.txt

#pragma once

#include "CoreDefines.h"
#include "CoreTypes.h"
#include "TundraLogicModuleApi.h"
#include "UserConnectedResponseData.h"

#include <kNet.h>

#include <map>
#include <QObject>
#include <QUrl>

struct MsgLogin;
struct MsgLoginReply;
struct MsgClientJoined;
struct MsgClientLeft;

namespace kNet
{
    class MessageConnection;
    typedef unsigned long message_id_t;
}

namespace KristalliProtocol
{
    class KristalliProtocolModule;
}

class UserConnection;
typedef boost::shared_ptr<UserConnection> UserConnectionPtr;
typedef std::list<UserConnectionPtr> UserConnectionList;

class Framework;

namespace TundraLogic
{

class TundraLogicModule;

/// Provides Tundra client->server connection functions.
class TUNDRALOGIC_MODULE_API Client : public QObject
{
    Q_OBJECT

public:
    /// Constructor
    /** @param owner Owner module. */
    explicit Client(TundraLogicModule *owner);

    ~Client();

    void Update(f64 frametime);

    enum ClientLoginState
    {
        NotConnected = 0,
        ConnectionPending,
        ConnectionEstablished,
        LoggedIn
    };

    /// Returns connection/login state
    ClientLoginState GetLoginState() const { return loginstate_; }

    /// Returns the underlying kNet MessageConnection object that represents this connection.
    /// This function may return null in the case the connection is not active.
    kNet::MessageConnection* GetConnection();

    /// Logout immediately and delete the client scene content
    /// @param fail Pass in true if the logout was due to connection/login failure. False, if the connection was aborted deliberately by the client.
    void DoLogout(bool fail = false);

public slots:
    /// Connects and logs in. The QUrl's query parameters will be evaluated for the login data. 
    /// All query parameters that are not recognized will be added to the clients login properties as custom data.
    /** Minimum information needed to try a connection in the url are host and username. For query parameters only username, protocol and password 
        get special treatment, other params are inserted to the login properties as is.
        URL syntax: {tundra|http|https}://host[:port]/?username=x[&password=y][&protocol={udp|tcp}][&XXX=YYY]
        URL examples: tundra://server.com/?username=John%20Doe tundra://server.com:5432/?username=John%20Doe&password=pWd123&protocol=udp&myCustomValue=YYY&myOtherValue=ZZZ
        @param loginUrl The login URL.
        @note The input QUrl is expected to be in full percent encoding if it contains non ascii characters (so dont use QUrl::TolerantMode for parsing). 
        Username will be automatically decoded, other params are inserted to the login properties as is.
        @note The destination port is obtained from the URL's port, not from a query parameter. If no port is present, using Tundra's default port 2345. */
    void Login(const QUrl& loginUrl);

    /// Connect and login. Username and password will be encoded to XML key-value data
    /// @note This function will be deleted in the future.
    void Login(const QString& address, unsigned short port, const QString& username, const QString& password, const QString &protocol = QString());

    /// Connect and login using the login properties that were previously set with calls to SetLoginProperty.
    void Login(const QString& address, unsigned short port, kNet::SocketTransportLayer protocol = kNet::InvalidTransportLayer);

    /// Disconnects the client from the current server, and also deletes all contents from the client scene.
    /** Delays the logout by one frame, so it is safe to call from scripts. */
    void Logout();

    /// Returns client connection ID (from loginreply message). Is zero if not connected
    int GetConnectionID() const { return client_id_; }

    /// See if connected & authenticated
    bool IsConnected() const;

    /// Sets the given login property with the given value.
    /** Call this function prior connecting to a scene to specify data that should be carried to the server as initial login data.
        @param key The name of the login property to set. If a previous login property with this name existed, it is overwritten.
        @param value The value to specify for this login property. If "", the login property is deleted and will not be sent. */
    void SetLoginProperty(QString key, QString value);

    /// Returns the login property value of the given name.
    /// @return value of the key, or an empty string if the key was not found.
    QString GetLoginProperty(QString key);

    /// Returns all the currently set login properties as an XML text.
    QString LoginPropertiesAsXml() const;

    /// Returns all the login properties that will be used to login to the server.
    std::map<QString, QString> &GetAllLoginProperties() { return properties; }

    /// Deletes all set login properties.
    void ClearLoginProperties() { properties.clear(); }

signals:
    /// This signal is emitted right before this client is starting to connect to a Tundra server.
    /** Any script or other piece of code can listen to this signal, and as at this point, fill in any internal
        custom data (called "login properties") they need to add to the connection handshake. The server will get 
        all the login properties and a server-side script can do validation and storage of whether the client
        can be authorized to log in or not. */
    void AboutToConnect();

    /// This signal is emitted immediately after this client has successfully connected to a server.
    /// @param responseData This is the data that the server sent back to the client related to the connection.
    void Connected(UserConnectedResponseData *responseData);

    /// Triggered whenever a new message is received from the network.
    void NetworkMessageReceived(kNet::message_id_t id, const char *data, size_t numBytes);

    /// This signal is emitted when the client has disconnected from the server.
    void Disconnected();

    /// Emitted when a login attempt failed to a server.
    void LoginFailed(const QString &reason);

private slots:
    /// Handles a Kristalli protocol message
    void HandleKristalliMessage(kNet::MessageConnection* source, kNet::message_id_t id, const char* data, size_t numBytes);

    void OnConnectionAttemptFailed();

    /// Actually perform a delayed logout
    void DelayedLogout();

private:
    /// Handles pending login to server
    void CheckLogin();

    /// Handles a loginreply message
    void HandleLoginReply(kNet::MessageConnection* source, const MsgLoginReply& msg);

    /// Handles a client joined message
    void HandleClientJoined(kNet::MessageConnection* source, const MsgClientJoined& msg);

    /// Client: Handles a client left message
    void HandleClientLeft(kNet::MessageConnection* source, const MsgClientLeft& msg);

    ClientLoginState loginstate_; ///< Client's connection/login state
    std::map<QString, QString> properties; ///< Specifies all the login properties.
    bool reconnect_; ///< Whether the connect attempt is a reconnect because of dropped connection
    u8 client_id_; ///< User ID, once known
    TundraLogicModule* owner_; ///< Owning module
    Framework* framework_; ///< Framework pointer
};

}
