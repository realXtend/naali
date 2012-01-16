// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <iomanip>

// Disable warnings C4702 coming from boost
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <boost/lexical_cast.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "CoreDefines.h"
#include "CoreTypes.h"

#include <QString>

/// @cond PRIVATE
class QStringLessThanNoCase
{
public:
    bool operator()(const QString &a, const QString &b) const
    {
        return QString::compare(a, b, Qt::CaseInsensitive) < 0;
    }
};
/// @endcond

QString QStringfromWCharArray(const wchar_t *string, int size);

/// Warning: Does not null-terminate the output. Prefer to use QStringToWString instead!
int QStringtoWCharArray(QString qstr, wchar_t *array);

std::wstring QStringToWString(const QString &qstr);
QString WStringToQString(const std::wstring &str);

std::wstring ToWString(const std::string &str);
std::wstring QStringToWString(const QString &qstr);
QString WStringToQString(const std::wstring &str);

/// Converts value to a string. May throw boost::bad_lexical_cast.
template <class T>
std::string ToString(const T &val) { return boost::lexical_cast<std::string>(val); }

/// Converts string to a primitive type, such as int or float. May throw boost::bad_lexical_cast.
template <typename T>
T ParseString(const std::string &val) { return boost::lexical_cast<T>(val); }

/// Converts string to a primitive type, such as int or float. Returns default value on boost::bad_lexical_cast
template <typename T>
T ParseString(const std::string &val, T default_value) 
{
    try
    {
        return boost::lexical_cast<T>(val); 
    }
    catch(boost::bad_lexical_cast e)
    {
        return default_value;
    }
}

/// Split a string by separator char
StringVector SplitString(const std::string& str, char separator);

/// Replaces all occurrences of a substring with another
/** @param str String to modify
    @param replace_this What substring to replace
    @param replace_with Substring to replace with */
std::string ReplaceSubstring(const std::string &str, const std::string &replace_this, const std::string &replace_with);

/// Replaces certain char in string with another char
/** @param str String to modify
    @param replace_this What char to replace
    @param replace_with Char to replace with */
std::string ReplaceChar(const std::string& str, char replace_this, char replace_with);

/// Replaces all occurrences of a substring with another
/** @param str String to modify
    @param replace_this What substring to replace
    @param replace_with Substring to replace with */
void ReplaceSubstringInplace(std::string &str, const std::string &replace_this, const std::string &replace_with);

/// Replaces certain char in string with another char
/** @param str String to modify
    @param replace_this What char to replace
    @param replace_with Char to replace with */
void ReplaceCharInplace(std::string& str, char replace_this, char replace_with);

/// Convert vector to string
std::string BufferToString(const std::vector<s8>& buffer);

/// Convert string to vector
std::vector<s8> StringToBuffer(const std::string& str);

/// Calculate SDBM hash for a string
uint GetHash(const std::string& str);

/// Calculate SDBM hash for a string
uint GetHash(const QString& str);

/// Parses boolean value from string, case-insensitive.
/** Accepted variations are on/off, true/false & 0/1* /
    @param value String to be inspected. */
bool ParseBool(QString value);
/// This is an overloaded function.
bool ParseBool(const std::string &value);

/// Converts boolean to "true" or "false".
inline QString BoolToString(bool value) { return value ? "true" : "false"; }
