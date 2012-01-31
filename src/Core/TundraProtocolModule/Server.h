// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreDefines.h"
#include "CoreTypes.h"
#include "TundraProtocolModuleApi.h"
#include "TundraProtocolModuleFwd.h"

#include "kNet/Types.h"

#include <QObject>
#include <QVariant>

class QScriptEngine;

class Framework;

namespace TundraLogic
{
/// Implements Tundra server functionality.
class TUNDRAPROTOCOL_MODULE_API Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(TundraLogicModule* owner);
    ~Server();

    /// Perform any per-frame processing
    void Update(f64 frametime);

    /// Get matching userconnection from a messageconnection, or null if unknown
    UserConnection* GetUserConnection(kNet::MessageConnection* source) const;

    /// Get all connected users
    UserConnectionList& GetUserConnections() const;

    /// Get all authenticated users
    UserConnectionList GetAuthenticatedUsers() const;

    /// Set current action sender. Called by SyncManager
    void SetActionSender(UserConnection* user);

    /// Returns the backend server object. Use this object to Broadcast messages
    /// to all currently connected clients.
    kNet::NetworkServer *GetServer() const;

signals:
    /// A user is connecting. This is your chance to deny access.
    /** Call user->Disconnect() to deny access and kick the user out */ 
    void UserAboutToConnect(int connectionID, UserConnection* connection);
     
    /// A user has connected (and authenticated)
    /** @param responseData The handler of this signal can add his own application-specific data to this structure. This data is sent to the
        client and the applications on the client computer can read them as needed. */
    void UserConnected(int connectionID, UserConnection* connection, UserConnectedResponseData *responseData);
    
    void MessageReceived(UserConnection *connection, kNet::packet_id_t, kNet::message_id_t id, const char* data, size_t numBytes);

    /// A user has disconnected
    void UserDisconnected(int connectionID, UserConnection* connection);
    
    /// The server has been started
    void ServerStarted();
    
    /// The server has been stopped
    void ServerStopped();
    
public slots:
    /// Create server scene & start server
    /** @param protocol The server protocol to use, either "tcp" or "udp". If not specified, the default UDP will be used.
        @return True if successful, false otherwise. No scene will be created if starting the server fails. */
    bool Start(unsigned short port, QString protocol = "");

    /// Stop server & delete server scene
    void Stop();

    /// Get whether server is running
    bool IsRunning() const;

    /// Get whether server is about to start.
    bool IsAboutToStart() const;

    /// Get the running servers port.
    /// @return int Valid port if server is running. -1 if server is not running.
    int GetPort() const;

    /// Get the running servers protocol.
    /// @note This function returns QString due we dont want kNet::TransportLayer enum here. If the module creators feels its ok then change this.
    /// @return QString Will return 'udp' or 'tcp' if server is running. Otherwise an empty string.
    QString GetProtocol() const;
    
    /// Get connected users' connection ID's
    QVariantList GetConnectionIDs() const;
    
    /// Get userconnection structure corresponding to connection ID
    UserConnection* GetUserConnection(int connectionID) const;
    
    /// Get current sender of an action. Valid (non-null) only while an action packet is being handled. Null if it was invoked by server
    UserConnection* GetActionSender() const;
    
    /// Initialize server datatypes for a script engine
    void OnScriptEngineCreated(QScriptEngine* engine);

private slots:
    /// Handle a Kristalli protocol message
    void HandleKristalliMessage(kNet::MessageConnection* source, kNet::packet_id_t, kNet::message_id_t id, const char* data, size_t numBytes);

    /// Handle a user disconnecting
    void HandleUserDisconnected(UserConnection* user);

private:
    /// Handle a login message
    void HandleLogin(kNet::MessageConnection* source, const MsgLogin& msg);
        
    /// Current action sender
    UserConnection* actionsender_;
    
    /// Owning module
    TundraLogicModule* owner_;
    
    /// Framework pointer
    Framework* framework_;

    /// Current running servers port.
    int current_port_;

    /// Current running servers protocol.
    QString current_protocol_;
};

}
