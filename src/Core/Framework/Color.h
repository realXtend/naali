/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   Color.h
    @brief  A 4-component color value, component values are floating-points [0.0, 1.0]. */

#pragma once

#include "CoreTypes.h"

#include <QMetaType>
#include <QColor>

#ifdef MATH_OGRE_INTEROP
#include <OgreColourValue.h>
#endif

/// A 4-component color value, component values are floating-points [0.0, 1.0].
/** @todo Expose to QtScript by using QtScriptGenerator instead of manual exposing. */
class Color
{
public:
    float r;
    float g;
    float b;
    float a;

    /// The default ctor initializes values to 0.f, 0.f, 0.f, 1.f (opaque black).
    Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f)
    {
    }

    Color(const Color &color) : r(color.r), g(color.g), b(color.b), a(color.a)
    {
    }

    Color(float nr, float ng, float nb) : r(nr), g(ng), b(nb), a(1.0f)
    {
    }

    Color(float nr, float ng, float nb, float na) : r(nr), g(ng), b(nb), a(na)
    {
    }

    bool operator == (const Color& rhs) const
    {
        return (r == rhs.r) && (g == rhs.g) && (b == rhs.b) && (a == rhs.a);
    }

    bool operator != (const Color& rhs) const
    {
        return !(*this == rhs);
    }

    Color(const QColor &other) { r = other.redF(); g = other.greenF(); b = other.blueF(); a = other.alphaF(); }

    /// Implicit conversion to Color.
    operator QColor() const;

    /// Returns Color as QColor.
    QColor ToQColor() const { return *this; }

    /// Implicit conversion to string.
    /** @see ToString*/
    operator QString() const;

    /// Returns "Color(r,g,b,a)".
    QString ToString() const { return (QString)*this; }

    /// For QtScript-compatibility.
    QString toString() const { return (QString)*this; }

    /// Returns "r g b a".
    /** This is the preferred format for the Color if it has to be serialized to a string for machine transfer.
        @sa FromString */
    QString SerializeToString() const;

    /// Parses a string to a new Color.
    /** Accepted formats are: "r,g,b,a" or "(r,g,b,a)" or "(r;g;b;a)" or "r g b" or "r,g,b" or "(r,g,b)" or "(r;g;b)" or "r g b" .
        @sa SerializeToString */
    static Color FromString(const char *str);

    /// This is an overloaded function.
    static Color FromString(const QString &str) { return FromString(str.simplified().toStdString().c_str()); }

#ifdef MATH_OGRE_INTEROP
    /// Returns Color as Ogre::ColourValue.
    Color(const Ogre::ColourValue &other) { r = other.r; g = other.g; b = other.b; a = other.a; }

    /// Implicit conversion to Ogre::ColourValue.
    operator Ogre::ColourValue() const { return Ogre::ColourValue(r, g, b, a); }
#endif
};

Q_DECLARE_METATYPE(Color)
Q_DECLARE_METATYPE(Color*)
