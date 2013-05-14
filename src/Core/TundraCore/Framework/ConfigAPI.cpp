// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Framework.h"
#include "ConfigAPI.h"
#include "Application.h"
#include "AssetAPI.h"
#include "LoggingFunctions.h"

#include <QSettings>
#include <QDir>

QString ConfigAPI::FILE_FRAMEWORK = "tundra";
QString ConfigAPI::SECTION_FRAMEWORK = "framework";
QString ConfigAPI::SECTION_SERVER = "server";
QString ConfigAPI::SECTION_CLIENT = "client";
QString ConfigAPI::SECTION_RENDERING = "rendering";
QString ConfigAPI::SECTION_UI = "ui";
QString ConfigAPI::SECTION_SOUND = "sound";

ConfigAPI::ConfigAPI(Framework *framework) :
    QObject(framework),
    framework_(framework)
{
}

void ConfigAPI::PrepareDataFolder(QString configFolder)
{
    QDir config = QDir(Application::ParseWildCardFilename(configFolder.trimmed()));
    if (!config.exists())
    {
        bool success = config.mkpath(".");
        if (!success)
        {
            LogError("Failed to create configuration folder \"" + config.absolutePath() + "\"! Check that this path is valid, and it is write-accessible!");
            return;
        }
    }
    configFolder_ = GuaranteeTrailingSlash(config.absolutePath());
    LogInfo("* Config directory       : " + QDir::toNativeSeparators(configFolder_));
}

QString ConfigAPI::GetFilePath(const QString &file) const
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::GetFilePath: Config folder has not been prepared, returning empty string.");
        return "";
    }

    QString filePath = configFolder_ + file;
    if (!filePath.endsWith(".ini"))
        filePath.append(".ini");
    return filePath;
}

bool ConfigAPI::IsFilePathSecure(const QString &file) const
{
    if (file.trimmed().isEmpty())
    {
        LogError("ConfigAPI: File path to perform read/write operations is not permitted as it's an empty string.");
        return false;
    }

    bool secure = true;
    if (QDir::isAbsolutePath(file))
        secure = false;
    else if (file.contains(".."))
        secure = false;
    if (!secure)
        LogError("ConfigAPI: File path to perform read/write operations is not permitted: " + file);
    return secure;
}

void ConfigAPI::PrepareString(QString &str) const
{
    if (!str.isEmpty())
    {
        str = str.trimmed().toLower();  // Remove spaces from start/end, force to lower case
        str = str.replace(" ", "_");    // Replace ' ' with '_', so we don't get %20 in the config as spaces
        str = str.replace("=", "_");    // Replace '=' with '_', as = has special meaning in .ini files
        str = str.replace("/", "_");    // Replace '/' with '_', as / has a special meaning in .ini file keys/sections. Also file name cannot have a forward slash.
    }
}

bool ConfigAPI::HasKey(const ConfigData &data) const
{
    if (data.file.isEmpty() || data.section.isEmpty() || data.key.isEmpty())
    {
        LogWarning("ConfigAPI::HasKey: ConfigData does not have enough information.");
        return false;
    }
    return HasKey(data.file, data.section, data.key);
}

bool ConfigAPI::HasKey(const ConfigData &data, QString key) const
{
    if (data.file.isEmpty() || data.section.isEmpty())
    {
        LogWarning("ConfigAPI::HasKey: ConfigData does not have enough information.");
        return false;
    }
    return HasKey(data.file, data.section, key);
}

bool ConfigAPI::HasKey(QString file, QString section, QString key) const
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::HasKey: Config folder has not been prepared, returning false.");
        return false;
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    if (!IsFilePathSecure(file))
        return false;

    QSettings config(GetFilePath(file), QSettings::IniFormat);
    if (!section.isEmpty())
        key = section + "/" + key;
    return config.allKeys().contains(key);
}

QVariant ConfigAPI::Read(const ConfigData &data) const
{
    if (data.file.isEmpty() || data.section.isEmpty() || data.key.isEmpty())
    {
        LogWarning("ConfigAPI::Read: ConfigData does not have enough information.");
        return data.defaultValue;
    }
    return Read(data.file, data.section, data.key, data.defaultValue);
}

QVariant ConfigAPI::Read(const ConfigData &data, QString key, const QVariant &defaultValue) const
{
    if (data.file.isEmpty() || data.section.isEmpty())
    {
        LogWarning("ConfigAPI::Read: ConfigData does not have enough information.");
        return data.defaultValue;
    }
    if (defaultValue.isNull())
        return Read(data.file, data.section, key, data.defaultValue);
    else
        return Read(data.file, data.section, key, defaultValue);
}

QVariant ConfigAPI::Read(QString file, QString section, QString key, const QVariant &defaultValue) const
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::Read: Config folder has not been prepared, returning null QVariant.");
        return "";
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    // Don't return 'defaultValue' but null QVariant
    // as this is an error situation.
    if (!IsFilePathSecure(file))
        return QVariant();

    QSettings config(GetFilePath(file), QSettings::IniFormat);
    if (section.isEmpty())
        return config.value(key, defaultValue);
    else
        return config.value(section + "/" + key, defaultValue);
}

void ConfigAPI::Write(const ConfigData &data)
{
    if (data.file.isEmpty() || data.section.isEmpty() || data.key.isEmpty() || data.value.isNull())
    {
        LogWarning("ConfigAPI::Write: ConfigData does not have enough information.");
        return;
    }
    Write(data.file, data.section, data.key, data.value);
}

void ConfigAPI::Write(const ConfigData &data, QString key, const QVariant &value)
{
    if (data.file.isEmpty() || data.section.isEmpty())
    {
        LogWarning("ConfigAPI::Write: ConfigData does not have enough information.");
        return;
    }
    Write(data.file, data.section, key, value);
}

void ConfigAPI::Write(QString file, QString section, QString key, const QVariant &value)
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::Write: Config folder has not been prepared, can not write value to config.");
        return;
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    if (!IsFilePathSecure(file))
        return;

    QSettings config(GetFilePath(file), QSettings::IniFormat);
    if (!config.isWritable())
        return;
    if (section.isEmpty())
        config.setValue(key, value);
    else
        config.setValue(section + "/" + key, value);
    config.sync();
}

QVariant ConfigAPI::DeclareSetting(const QString &file, const QString &section, const QString &key, const QVariant &defaultValue)
{
    if (HasKey(file, section, key))
    {
        return Read(file, section, key);
    }
    else
    {
        Write(file, section, key, defaultValue);
        return defaultValue;
    }
}

QVariant ConfigAPI::DeclareSetting(const ConfigData &data)
{
    return DeclareSetting(data.file, data.section, data.key, data.value.isValid() ? data.value : data.defaultValue);
}

QVariant ConfigAPI::DeclareSetting(const ConfigData &data, const QString &key, const QVariant &defaultValue)
{
    return DeclareSetting(data.file, data.section, key, defaultValue);
}
