// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "MumbleFwd.h"
#include "Math/float3.h"

#include <QObject>
#include <QString>

class MumbleUser : public QObject
{
Q_OBJECT

Q_PROPERTY(uint id READ Id)
Q_PROPERTY(uint channelId READ ChannelId)
Q_PROPERTY(QString name READ Name)
Q_PROPERTY(QString comment READ Comment)
Q_PROPERTY(QString hash READ Hash)
Q_PROPERTY(bool isSpeaking READ IsSpeaking)
Q_PROPERTY(bool isMuted READ IsMuted WRITE SetMuted)
Q_PROPERTY(bool isSelfMuted READ IsSelfMuted)
Q_PROPERTY(bool isSelfDeaf READ IsSelfDeaf)
Q_PROPERTY(bool isPositional READ IsPositional)
Q_PROPERTY(bool isMe READ IsMe)

public:
    MumbleUser(MumblePlugin *owner);
    ~MumbleUser();

    uint id;
    uint channelId;
    QString name;
    QString comment;
    QString hash;
    bool isSpeaking;
    bool isMuted;
    bool isSelfMuted;
    bool isSelfDeaf;
    bool isMe;
    bool isPositional;
    float3 pos;

    uint Id() { return id; }
    uint ChannelId() { return channelId; }
    
    QString Name() { return name; }
    QString Comment() { return comment; }
    QString Hash() { return hash; }

    bool IsSpeaking() { return isSpeaking; }
    bool IsMuted() { return isMuted; }
    bool IsSelfMuted() { return isSelfMuted; }
    bool IsSelfDeaf() { return isSelfDeaf; }
    bool IsPositional() { return isPositional; }
    bool IsMe() { return isMe; }

    // Only emits if the speaking boolean changed.
    // Also emits MumblePlugin::UserSpeaking signal.
    void SetAndEmitSpeaking(bool speaking);

    // Only emits if the positional boolean changed.
    // Also emits MumblePlugin::UserPositionalChanged signal.
    void SetAndEmitPositional(bool positional);

    void EmitSpeaking() { emit Speaking(id, isSpeaking); }
    void EmitMuted() { emit Muted(id, isMuted); }
    void EmitSelfMuted() { emit SelfMuted(id, isSelfMuted); }
    void EmitSelfDeaf() { emit SelfDeaf(id, isSelfDeaf); }
    void EmitPositionalChanged() { emit PositionalChanged(id, isPositional); }
    void EmitChannelChanged(MumbleChannel *channel) { emit ChannelChanged(channel); }

public slots:
    void SetMuted(bool muted);
    void Mute();
    void UnMute();

    MumbleChannel *Channel();
    
    QString toString() const;

signals:
    /// This users speaking state changed.
    /// If speaking is true it means that we are playing this users voice currently.
    /// If false the user is not sending voice to us currently and no playback is done.
    void Speaking(uint userId, bool speaking);

    /// This users local muted state was changed by us.
    /// If muted is false we can no longer hear this user,
    /// if true we can hear his oncoming voice if there is any.
    void Muted(uint userId, bool muted);
    
    /// This users self muted state changed. Meaning this user
    /// muted/unmuted outgoing voice himself.
    /// If selfMuted is true no one on the channel can hear him (guaranteed by the server).
    void SelfMuted(uint userId, bool selfMuted);

    /// This users self deaf state changed. Meaning this user
    /// deafen/undeafen both incoming and outgoing voice himself.
    /// If selfDeaf is true no one on the channel can hear him or send audio to him (guaranteed by the server).
    void SelfDeaf(uint userId, bool selfDeaf);

    /// This users positional audio boolean changed.
    /// @note The position itself it not emitted, you can access it from the pos property.
    void PositionalChanged(uint userId, bool positional);

    /// User changed channel.
    void ChannelChanged(MumbleChannel *channel);

private:
    MumblePlugin *owner_;
};

class MumbleChannel : public QObject
{
Q_OBJECT

Q_PROPERTY(uint id READ Id)
Q_PROPERTY(uint parentId READ ParentId)
Q_PROPERTY(QString name READ Name)
Q_PROPERTY(QString fullName READ FullName)
Q_PROPERTY(QString description READ Description)
Q_PROPERTY(QList<MumbleUser*> users READ Users)

public:
    MumbleChannel(MumblePlugin *owner);

    // Frees all channel user ptrs.
    ~MumbleChannel();

    uint id;
    uint parentId;
    QString name;
    QString fullName;
    QString description; 
    QList<MumbleUser*> users;

    uint Id() { return id; }
    uint ParentId() { return parentId; }
    QString Name() { return name; }
    QString FullName() { return fullName; }
    QString Description() { return description; }
    QList<MumbleUser*> Users() { return users; }

    bool AddUser(MumbleUser *user);
    bool RemoveUser(uint id);

    void EmitUsersChanged() { emit UsersChanged(users); }
    void EmitUserJoined(MumbleUser *user) { emit UserJoined(user); }
    void EmitUserLeft(uint userId) { emit UserLeft(userId); }

public slots:
    MumbleUser *User(uint id);
    QList<uint> MutedUserIds();

    QString toString() const;

signals:
    /// User list changed, meaning either new user joined or old user left.
    void UsersChanged(QList<MumbleUser*> users);

    /// User joined the channel.
    void UserJoined(MumbleUser *user);

    /// User left the channel. 
    /** @note User id is used as parameter because the user ptr will be 
        deleted after this signal. For queued signal handling this would 
        be a problem. You can get a null ptr when fetching the user 
        with this id from MumblePlugin or MumbleChannel functions, so its not advisable. */
    void UserLeft(uint userId);

private:
    MumblePlugin *owner_;
};
