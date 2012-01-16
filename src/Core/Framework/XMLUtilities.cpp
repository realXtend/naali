// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "DebugOperatorNew.h"
#include <boost/algorithm/string.hpp>
#include <QList>
#include "MemoryLeakCheck.h"
#include "XMLUtilities.h"
#include "CoreStringUtils.h"

#include <QDomElement>

float ParseReal(const std::string& text, float default_value)
{
    float ret_value = default_value;
    try
    {
        ret_value = ParseString<float>(text);
    }
    catch(boost::bad_lexical_cast)
    {
    }        
    return ret_value;
}

int ParseInt(const std::string& text, int default_value)
{
    int ret_value = default_value;
    try
    {
        ret_value = ParseString<int>(text);
    }
    catch(boost::bad_lexical_cast)
    {
    }        
    return ret_value;
}    
/*
Vector3df ParseVector3(const std::string& text)
{
    Vector3df vec(0.0f, 0.0f, 0.0f);
    
    StringVector components = SplitString(text, ' ');
    if (components.size() == 3)
    {
        try
        {
            vec.x = ParseString<float>(components[0]);
            vec.y = ParseString<float>(components[1]);
            vec.z = ParseString<float>(components[2]);
        }
        catch(boost::bad_lexical_cast)
        {
        }
    }
    return vec;
}
*/
Color ParseColor(const std::string& text)
{
    Color color(0.0f, 0.0f, 0.0f);
    
    StringVector components = SplitString(text, ' ');
    if (components.size() == 3)
    {
        try
        {
            color.r = ParseString<float>(components[0]);
            color.g = ParseString<float>(components[1]);
            color.b = ParseString<float>(components[2]);
        }
        catch(boost::bad_lexical_cast)
        {
        }
    }
    if (components.size() == 4)
    {
        try
        {
            color.r = ParseString<float>(components[0]);
            color.g = ParseString<float>(components[1]);
            color.b = ParseString<float>(components[2]);
            color.a = ParseString<float>(components[3]);
        }
        catch(boost::bad_lexical_cast)
        {
        }
    }
    return color;
}
/*
Quaternion ParseEulerAngles(const std::string& text)
{
    Quaternion quat;
    
    StringVector components = SplitString(text, ' ');
    if (components.size() == 3)
    {
        try
        {
            float xrad = degToRad(ParseString<float>(components[0]));
            float yrad = degToRad(ParseString<float>(components[1]));
            float zrad = degToRad(ParseString<float>(components[2]));
            
            float angle = yrad * 0.5;
            double cx = cos(angle);
            double sx = sin(angle);

            angle = zrad * 0.5;
            double cy = cos(angle);
            double sy = sin(angle);

            angle = xrad * 0.5;
            double cz = cos(angle);
            double sz = sin(angle);

            quat.x = sx * sy * cz + cx * cy * sz;
            quat.y = sx * cy * cz + cx * sy * sz;
            quat.z = cx * sy * cz - sx * cy * sz;
            quat.w = cx * cy * cz - sx * sy * sz;
            
            quat.normalize();
        }
        catch(boost::bad_lexical_cast)
        {
        }
    }
    
    return quat;
}
*/
std::string WriteBool(bool value)
{
    if (value)
        return "true";
    else
        return "false";
}

std::string WriteReal(float value)
{
    return ToString<float>(value);
}

std::string WriteInt(int value)
{
    return ToString<int>(value);
}
/*
std::string WriteVector3(const Vector3df& vector)
{
    return ToString<float>(vector.x) + " " +
        ToString<float>(vector.y) + " " +
        ToString<float>(vector.z);
}
*/
std::string WriteColor(const Color& color)
{
    return ToString<float>(color.r) + " " +
        ToString<float>(color.g) + " " +
        ToString<float>(color.b) + " " +
        ToString<float>(color.a);
}
/*
std::string WriteEulerAngles(const Quaternion& quat)
{
    Vector3df radians;
    quat.toEuler(radians);
    
    return ToString<float>(radians.x * RADTODEG) + " " +
        ToString<float>(radians.y * RADTODEG) + " " + 
        ToString<float>(radians.z * RADTODEG);
    
    //Ogre::Matrix3 rotMatrix;
    //Ogre::Quaternion value = OgreRenderer::ToOgreQuaternion(quat);
    //value.ToRotationMatrix(rotMatrix);
    //Ogre::Radian anglex;
    //Ogre::Radian angley;
    //Ogre::Radian anglez;
    //rotMatrix.ToEulerAnglesXYZ(anglex, angley, anglez);

    //float angles[3];
    //angles[0] = anglex.valueDegrees();
    //angles[1] = angley.valueDegrees();
    //angles[2] = anglez.valueDegrees();
    //
    //return ToString<float>(angles[0]) + " " +
    //    ToString<float>(angles[1]) + " " + 
    //    ToString<float>(angles[2]);
}
*/
void SetAttribute(QDomElement& elem, const std::string& name, const char* value)
{
    elem.setAttribute(QString::fromStdString(name), value);
}
    
void SetAttribute(QDomElement& elem, const std::string& name, const std::string& value)
{
    elem.setAttribute(QString::fromStdString(name), QString::fromStdString(value));
}
 
void SetAttribute(QDomElement& elem, const std::string& name, float value)
{
    elem.setAttribute(QString::fromStdString(name), QString::fromStdString(ToString<float>(value)));
}

void SetAttribute(QDomElement& elem, const std::string& name, bool value)
{
    elem.setAttribute(QString::fromStdString(name), QString::fromStdString(WriteBool(value)));
}

void SetAttribute(QDomElement& elem, const std::string& name, int value)
{
    elem.setAttribute(QString::fromStdString(name), QString::fromStdString(ToString<int>(value)));
}

