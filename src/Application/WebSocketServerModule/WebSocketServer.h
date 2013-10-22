
#pragma once

#include "WebSocketServerModuleApi.h"
#include "Win.h"

#include "FrameworkFwd.h"
#include "WebSocketFwd.h"
#include "kNetFwd.h"
#include "AssetFwd.h"
#include "AssetReference.h"

#include "SyncState.h"
#include "MsgEntityAction.h"
#include "EntityAction.h"

#include <QObject>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QMutex>

#include "websocketpp.hpp"
#include "kNet/DataSerializer.h"
#include "boost/weak_ptr.hpp"

namespace WebSocket
{   
    typedef websocketpp::server::ptr ServerPtr;
    typedef websocketpp::server::handler::ptr HandlerPtr;
    typedef websocketpp::server::connection_ptr ConnectionPtr;
    typedef boost::weak_ptr<websocketpp::server::connection_type> ConnectionWeakPtr;
    typedef websocketpp::message::data_ptr DataPtr;
    typedef shared_ptr<kNet::DataSerializer> DataSerializerPtr;
    
    // WebSocket events
    struct SocketEvent
    {
        enum EventType
        {
            None = 0,
            Connected,
            Disconnected,
            Data
        };

        WebSocket::ConnectionPtr connection;
        DataSerializerPtr data;
        EventType type;

        SocketEvent() : type(None) {}
        SocketEvent(WebSocket::ConnectionPtr connection_, EventType type_) : connection(connection_), type(type_) {}
    };

    /// WebSocket server. 
    /** Manages user requestedConnections and receiving/sending out data with them.
        All signals emitted by this object will be in the main thread. */
    class WEBSOCKET_SERVER_MODULE_API Server : public QObject, public enable_shared_from_this<Server>
    {
    Q_OBJECT

    public:
        Server(Framework *framework);
        ~Server();
        
        bool Start();
        void Stop();        
        void Update(float frametime);
        
        /// Returns all users.
        WebSocket::UserConnectionList &UserConnections();
        
        friend class Handler;
        
    public slots:
        /// Returns all authenticated users.
        WebSocket::UserConnectionList AuthenticatedUsers() const;
        
        /// Returns client with id, null if not found.
        WebSocket::UserConnection *UserConnection(uint connectionId);
        
        /// Returns client with websocket connection ptr, null if not found.
        WebSocket::UserConnection *UserConnection(WebSocket::ConnectionPtr connection);
        
        /// Mirror the Server object API.
        WebSocket::UserConnection *GetUserConnection(uint connectionId) { return UserConnection(connectionId); }
        
        /// Entity action helpers.
        void SetActionSender(WebSocket::UserConnection *user);
        WebSocket::UserConnection *ActionSender() const;
        
    signals:
        /// The server has been started
        void ServerStarted();

        /// The server has been stopped
        void ServerStopped();
        
        /// A user is connecting. This is your chance to deny access.
        /** Call connection->DenyConnection() to deny access. */
        void UserAboutToConnect(WebSocket::UserConnection *connection);

        /// A user has connected (and authenticated)
        /** @param responseData The handler of this signal can add his own application-specific data to this structure.
            This data is sent to the client and the applications on the client computer can read them as needed. */
        void UserConnected(WebSocket::UserConnection *connection, QVariantMap *responseData);

        /// A user has disconnected
        void UserDisconnected(WebSocket::UserConnection *connection);
        
        /// Web client entity action
        void ClientEntityAction(WebSocket::UserConnection *source, MsgEntityAction action);
          
    protected:
        void Reset();

        void OnUserDisconnected(WebSocket::UserConnection *userConnection);
        void OnConnected(WebSocket::ConnectionPtr connection);
        void OnDisconnected(WebSocket::ConnectionPtr connection);
        void OnMessage(WebSocket::ConnectionPtr connection, const char *data, size_t size);
        void OnHttpRequest(WebSocket::ConnectionPtr connection);
        
    private:
        /// @note Does not lock the requestedConnections mutex
        uint NextFreeConnectionId() const;

        QString LC;
        ushort port_;
        
        Framework *framework_;
        
        WebSocket::ServerPtr server_;
        WebSocket::HandlerPtr handler_;
        WebSocket::UserConnectionList connections_;
        WebSocket::UserConnection *actionSender_;

        QMutex mutexEvents_;
        QList<SocketEvent*> events_;

        /// Time period for update, default 20 time a sec
        float updatePeriod_;

        /// Time accumulator for update
        float updateAcc_;
    };

    /// WebSocket server handler class. 
    /** @note Callbacks are executed in network
        thread(s) not the main thread. */
    class WEBSOCKET_SERVER_MODULE_API Handler : public websocketpp::server::handler 
    {
    public:
        Handler(Server *server);
        ~Handler();
        
        void Close();
        
        void validate(ConnectionPtr con);
        
        void on_handshake_init(ConnectionPtr con);
        void on_open(ConnectionPtr con);
        void on_close(ConnectionPtr con);
        void on_fail(ConnectionPtr con);

        void on_message(ConnectionPtr con, DataPtr data);
        bool on_ping(ConnectionPtr con, std::string msg);
        void on_pong(ConnectionPtr con, std::string msg);
        void on_pong_timeout(ConnectionPtr con, std::string msg);
        void http(ConnectionPtr con);
        
    private:
        Server *server_;
    };
}
