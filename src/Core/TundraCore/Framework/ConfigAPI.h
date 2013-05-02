// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"

#include <QObject>
#include <QVariant>
#include <QString>

class Framework;

/// Convenience structure for dealing constantly with same config file/sections.
struct TUNDRACORE_API ConfigData
{
    ConfigData() {}

    ConfigData(const QString &cfgFile, const QString &cfgSection, const QString &cfgKey = QString(),
        const QVariant &cfgValue = QVariant(), const QVariant &cfgDefaultValue = QVariant()) :
        file(cfgFile),
        section(cfgSection),
        key(cfgKey),
        value(cfgValue),
        defaultValue(cfgDefaultValue)
    {
    }

    QString file;
    QString section;
    QString key;
    QVariant value;
    QVariant defaultValue;

    /// Returns string presentation of the contained data.
    QString ToString() const
    {
        return QString("ConfigData(file:%1 section:%2 key:%3 value:%4 defaultValue:%5)")
            .arg(file).arg(section).arg(key).arg(value.toString()).arg(defaultValue.toString());
    }

    /// @cond PRIVATE
    /// Same as ToString, exists for QtScript-compatibility.
    QString toString() const { return ToString(); }
    /// @endcond
};
Q_DECLARE_METATYPE(ConfigData)
Q_DECLARE_METATYPE(ConfigData*)

/// Configuration API for accessing config files.
/** The Configuration API utilizes QVariants extensively for script-compatibility.
    In C++ code use the QVariant::to*() functions to convert the values to the correct type.
    The Config API supports ini sections but you may also write to the root of the ini document without a section.

    The API is available as 'config' dynamic property.

    JavaScript example on usage:
    @code
    var file = "myconfig";

    config.Write(file, "world", new QUrl("http://server.com")); // QUrl
    config.Write(file, "port", 8013); // int
    config.Write(file, "login data", "username", "John Doe"); // QString
    config.Write(file, "login data", "password", "pass123"); // QString

    var username = config.Read(file, "login data", "username");
    if (username != null)
        print("Hello there " + username);
    etc.
    @endcode

    @note All file, key and section parameters are case-insensitive. This means all of them are transformed to 
    lower case before any accessing files. "MyKey" will get and set you same value as "mykey". */
class TUNDRACORE_API ConfigAPI : public QObject
{
    Q_OBJECT

public:
    ///\todo Make these properties so that can be obtained to scripts too.
    static QString FILE_FRAMEWORK;
    static QString SECTION_FRAMEWORK;
    static QString SECTION_SERVER;
    static QString SECTION_CLIENT;
    static QString SECTION_RENDERING;
    static QString SECTION_UI;
    static QString SECTION_SOUND;

public slots:
    /// Returns if a key exists in the config.
    /** @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key to look for in the file under section. */
    bool HasKey(QString file, QString section, QString key) const;
    bool HasKey(const ConfigData &data) const; /**< @overload @param data Filled ConfigData object */
    bool HasKey(const ConfigData &data, QString key) const; /**< @overload */

    /// @todo Add DeleteKey

    /// Returns value for a key in a config file
    /** @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key that value gets returned. For example: "username".
        @param defaultValue What you expect to get back if the file/section/key combination was not found.
        @return The value of key/section in file. */
    QVariant Read(QString file, QString section, QString key, const QVariant &defaultValue = QVariant()) const;
    QVariant Read(const ConfigData &data) const; /**< @overload @param data Filled ConfigData object. */
    /// @overload
    /** @param data ConfigData object that has file and section filled, also may have defaultValue and it will be used if input defaultValue is null. */
    QVariant Read(const ConfigData &data, QString key, const QVariant &defaultValue = QVariant()) const;

    /** Sets the value of key in a config file.
        @param file Name of the file. For example: "foundation" or "foundation.ini" you can omit the .ini extension.
        @param section The section in the config where key is. For example: "login".
        @param key Key that value gets set. For example: "username".
        @param value New value for the key.
        @note If setting value of type float, convert to double if you want the value to be human-readable in the file. */
    void Write(QString file, QString section, QString key, const QVariant &value); /**< @overload */
    void Write(const ConfigData &data); /**< @overload @param data Filled ConfigData object.*/
    void Write(const ConfigData &data, QString key, const QVariant &value); /**< @overload @param data ConfigData object that has file and section filled. */

    /// Returns the absolute path to the config folder where configs are stored. Guaranteed to have a trailing forward slash '/'.
    QString ConfigFolder() const { return configFolder_; }

    /// Declares a setting, meaning that if the setting doesn't exist in the config it will be created.
    /** @return The value of the setting the config, if the setting existed, or default value if the setting did not exist. */
    QVariant DeclareSetting(const QString &file, const QString &section, const QString &key, const QVariant &defaultValue);
     /// @overload
    /** @note ConfigData::value will take precedence over ConfigData::defaultValue, if both are set, as the value that will be used for the default value. */
    QVariant DeclareSetting(const ConfigData &data);
    QVariant DeclareSetting(const ConfigData &data, const QString &key, const QVariant &defaultValue); /**< @overload */

    // DEPRECATED
    /// @cond PRIVATE
    QVariant Get(QString file, QString section, QString key, const QVariant &defaultValue = QVariant()) const { return Read(file, section, key, defaultValue); } /**< @deprecated Use Read. @todo Add warning print */
    QVariant Get(const ConfigData &data) const { return Read(data); } /**< @deprecated Use Read. @todo Add warning print */
    QVariant Get(const ConfigData &data, QString key, const QVariant &defaultValue = QVariant()) const { return Read(data, key, defaultValue); } /**< @deprecated Use Read. @todo Add warning print */
    void Set(QString file, QString section, QString key, const QVariant &value) { Write(file, section, key, value); } /**< @deprecated Use Write. @todo Add warning print */
    void Set(const ConfigData &data)  { Write(data); } /**< @deprecated Use Write. @todo Add warning print */
    void Set(const ConfigData &data, QString key, const QVariant &value) { Write(data, key, value); } /**< @deprecated Use Write. @todo Add warning print */
    bool HasValue(QString file, QString section, QString key) const { return HasKey(file, section, key); } /**< @deprecated Use HasKey. @todo Add warning print @todo Remove */
    bool HasValue(const ConfigData &data) const { return HasKey(data); } /**< @deprecated Use HasKey. @todo Add warning print @todo Remove */
    bool HasValue(const ConfigData &data, QString key) const { return HasKey(data, key); } /**< @deprecated Use HasKey. @todo Add warning print @todo Remove */
    QString GetConfigFolder() const { return ConfigFolder(); } /**< @deprecated Use ConfigFolder. @todo Add warning print @todo Remove */
    /// @endcond
private:
    friend class Framework;

    /// @note Framework takes ownership of the object.
    explicit ConfigAPI(Framework *framework);

    /// Get absolute file path for file. Guarantees that it ends with .ini.
    QString GetFilePath(const QString &file) const;

    /// Returns if file provided by the ConfigAPI caller is secure and we should write/read from it.
    /** The purpose of this function is to verify the file provided by calling code
        does not go out of the confined ConfigAPI folder. For security reasons we cannot let
        eg. scripts open configs where they like. The whole operation will be canceled if this validation fails. */
    bool IsFilePathSecure(const QString &file) const;

    /// Prepare string for config usage. Removes spaces from end and start, replaces mid string spaces with '_' and forces to lower case.
    void PrepareString(QString &str) const;

    /// Opens up the Config API to the given data folder. This call will make sure that the required folders exist.
    /** @param configFolderName The name of the folder to store Tundra Config API data to. */
    void PrepareDataFolder(QString configFolderName);

    Framework *framework_;
    QString configFolder_; ///< Absolute path to the folder where to store the config files.
};
