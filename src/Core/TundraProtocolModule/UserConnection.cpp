// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "UserConnection.h"
#include "Entity.h"

#include "MemoryLeakCheck.h"

int UserConnection::GetConnectionID() const
{
    return userID;
}

void UserConnection::Exec(QObject* entity, const QString &action, const QString &p1, const QString &p2, const QString &p3)
{
    Entity* entityptr = dynamic_cast<Entity*>(entity);
    
    if (entityptr)
        emit ActionTriggered(this, entityptr, action, QStringList(QStringList() << p1 << p2 << p3));
}

void UserConnection::Exec(QObject* entity, const QString &action, const QStringList &params)
{
    Entity* entityptr = dynamic_cast<Entity*>(entity);
    
    if (entityptr)
        emit ActionTriggered(this, entityptr, action, params);
}

void UserConnection::SetProperty(const QString& key, const QString& value)
{
    properties[key] = value;
}

QString UserConnection::GetLoginData() const
{
    return loginData;
}

QString UserConnection::GetProperty(const QString& key) const
{
    static QString empty;
    
    std::map<QString, QString>::const_iterator i = properties.find(key);
    if (i != properties.end())
        return i->second;
    else
        return empty;
}

void UserConnection::DenyConnection(const QString &reason)
{
    properties["authenticated"] = "false";
    properties["reason"] = reason;
}

void UserConnection::Disconnect()
{
    if (connection)
        connection->Disconnect(0);
}

void UserConnection::Close()
{
    if (connection)
        connection->Close(0);
}
