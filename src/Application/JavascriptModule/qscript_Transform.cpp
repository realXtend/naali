#include "QtScriptBindingsHelpers.h"

void ToExistingScriptValue_Transform(QScriptEngine *engine, const Transform &value, QScriptValue obj)
{
    obj.setProperty("pos", qScriptValueFromValue(engine, value.pos), QScriptValue::Undeletable);
    obj.setProperty("rot", qScriptValueFromValue(engine, value.rot), QScriptValue::Undeletable);
    obj.setProperty("scale", qScriptValueFromValue(engine, value.scale), QScriptValue::Undeletable);
}

static QScriptValue Transform_Transform(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 0) { printf("Error! Invalid number of arguments passed to function Transform_Transform in file %s, line %d!\nExpected 0, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform ret;
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_Transform_float3_float3_float3(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 3) { printf("Error! Invalid number of arguments passed to function Transform_Transform_float3_float3_float3 in file %s, line %d!\nExpected 3, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    float3 pos_ = qscriptvalue_cast<float3>(context->argument(0));
    float3 rot_ = qscriptvalue_cast<float3>(context->argument(1));
    float3 scale_ = qscriptvalue_cast<float3>(context->argument(2));
    Transform ret(pos_, rot_, scale_);
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_Transform_float3x3(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_Transform_float3x3 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    float3x3 m = qscriptvalue_cast<float3x3>(context->argument(0));
    Transform ret(m);
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_Transform_float3x4(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_Transform_float3x4 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    float3x4 m = qscriptvalue_cast<float3x4>(context->argument(0));
    Transform ret(m);
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_Transform_float4x4(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_Transform_float4x4 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    float4x4 m = qscriptvalue_cast<float4x4>(context->argument(0));
    Transform ret(m);
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_SetPos_float3(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_SetPos_float3 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float3 v = qscriptvalue_cast<float3>(context->argument(0));
    This.SetPos(v);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_SetPos_float_float_float(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 3) { printf("Error! Invalid number of arguments passed to function Transform_SetPos_float_float_float in file %s, line %d!\nExpected 3, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float x = qscriptvalue_cast<float>(context->argument(0));
    float y = qscriptvalue_cast<float>(context->argument(1));
    float z = qscriptvalue_cast<float>(context->argument(2));
    This.SetPos(x, y, z);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_SetRotation_float_float_float(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 3) { printf("Error! Invalid number of arguments passed to function Transform_SetRotation_float_float_float in file %s, line %d!\nExpected 3, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float x = qscriptvalue_cast<float>(context->argument(0));
    float y = qscriptvalue_cast<float>(context->argument(1));
    float z = qscriptvalue_cast<float>(context->argument(2));
    This.SetRotation(x, y, z);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_SetScale_float_float_float(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 3) { printf("Error! Invalid number of arguments passed to function Transform_SetScale_float_float_float in file %s, line %d!\nExpected 3, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float x = qscriptvalue_cast<float>(context->argument(0));
    float y = qscriptvalue_cast<float>(context->argument(1));
    float z = qscriptvalue_cast<float>(context->argument(2));
    This.SetScale(x, y, z);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_SetScale_float3(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_SetScale_float3 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float3 s = qscriptvalue_cast<float3>(context->argument(0));
    This.SetScale(s);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_ToFloat3x4_const(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 0) { printf("Error! Invalid number of arguments passed to function Transform_ToFloat3x4_const in file %s, line %d!\nExpected 0, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float3x4 ret = This.ToFloat3x4();
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_ToFloat4x4_const(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 0) { printf("Error! Invalid number of arguments passed to function Transform_ToFloat4x4_const in file %s, line %d!\nExpected 0, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float4x4 ret = This.ToFloat4x4();
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_FromFloat3x4_float3x4(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_FromFloat3x4_float3x4 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float3x4 m = qscriptvalue_cast<float3x4>(context->argument(0));
    This.FromFloat3x4(m);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_FromFloat4x4_float4x4(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_FromFloat4x4_float4x4 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float4x4 m = qscriptvalue_cast<float4x4>(context->argument(0));
    This.FromFloat4x4(m);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_SetRotationAndScale_float3x3(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_SetRotationAndScale_float3x3 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float3x3 mat = qscriptvalue_cast<float3x3>(context->argument(0));
    This.SetRotationAndScale(mat);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_SetOrientation_float3x3(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_SetOrientation_float3x3 in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float3x3 mat = qscriptvalue_cast<float3x3>(context->argument(0));
    This.SetOrientation(mat);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_SetOrientation_Quat(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_SetOrientation_Quat in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    Quat q = qscriptvalue_cast<Quat>(context->argument(0));
    This.SetOrientation(q);
    ToExistingScriptValue_Transform(engine, This, context->thisObject());
    return QScriptValue();
}

static QScriptValue Transform_Orientation3x3_const(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 0) { printf("Error! Invalid number of arguments passed to function Transform_Orientation3x3_const in file %s, line %d!\nExpected 0, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    float3x3 ret = This.Orientation3x3();
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_Orientation_const(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 0) { printf("Error! Invalid number of arguments passed to function Transform_Orientation_const in file %s, line %d!\nExpected 0, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    Quat ret = This.Orientation();
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_Mul_Transform_const(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_Mul_Transform_const in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    Transform rhs = qscriptvalue_cast<Transform>(context->argument(0));
    Transform ret = This.Mul(rhs);
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_ToString_const(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 0) { printf("Error! Invalid number of arguments passed to function Transform_ToString_const in file %s, line %d!\nExpected 0, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    QString ret = This.ToString();
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_toString_const(QScriptContext *context, QScriptEngine *engine)
{
    Transform This;
    if (context->argumentCount() > 0) This = qscriptvalue_cast<Transform>(context->argument(0)); // Qt oddity (bug?): Sometimes the built-in toString() function doesn't give us this from thisObject, but as the first argument.
    else This = qscriptvalue_cast<Transform>(context->thisObject());
    QString ret = This.toString();
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_SerializeToString_const(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 0) { printf("Error! Invalid number of arguments passed to function Transform_SerializeToString_const in file %s, line %d!\nExpected 0, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    Transform This = qscriptvalue_cast<Transform>(context->thisObject());
    QString ret = This.SerializeToString();
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_FromString_QString(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() != 1) { printf("Error! Invalid number of arguments passed to function Transform_FromString_QString in file %s, line %d!\nExpected 1, but got %d!\n", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }
    QString str = qscriptvalue_cast<QString>(context->argument(0));
    Transform ret = Transform::FromString(str);
    return qScriptValueFromValue(engine, ret);
}

static QScriptValue Transform_ctor(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 0)
        return Transform_Transform(context, engine);
    if (context->argumentCount() == 3 && QSVIsOfType<float3>(context->argument(0)) && QSVIsOfType<float3>(context->argument(1)) && QSVIsOfType<float3>(context->argument(2)))
        return Transform_Transform_float3_float3_float3(context, engine);
    if (context->argumentCount() == 1 && QSVIsOfType<float3x3>(context->argument(0)))
        return Transform_Transform_float3x3(context, engine);
    if (context->argumentCount() == 1 && QSVIsOfType<float3x4>(context->argument(0)))
        return Transform_Transform_float3x4(context, engine);
    if (context->argumentCount() == 1 && QSVIsOfType<float4x4>(context->argument(0)))
        return Transform_Transform_float4x4(context, engine);
    printf("Transform_ctor failed to choose the right function to call! Did you use 'var x = Transform();' instead of 'var x = new Transform();'?\n"); PrintCallStack(context->backtrace()); return QScriptValue();
}

static QScriptValue Transform_SetPos_selector(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 1 && QSVIsOfType<float3>(context->argument(0)))
        return Transform_SetPos_float3(context, engine);
    if (context->argumentCount() == 3 && QSVIsOfType<float>(context->argument(0)) && QSVIsOfType<float>(context->argument(1)) && QSVIsOfType<float>(context->argument(2)))
        return Transform_SetPos_float_float_float(context, engine);
    printf("Transform_SetPos_selector failed to choose the right function to call in file %s, line %d!\n", __FILE__, __LINE__); PrintCallStack(context->backtrace()); return QScriptValue();
}

static QScriptValue Transform_SetScale_selector(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 3 && QSVIsOfType<float>(context->argument(0)) && QSVIsOfType<float>(context->argument(1)) && QSVIsOfType<float>(context->argument(2)))
        return Transform_SetScale_float_float_float(context, engine);
    if (context->argumentCount() == 1 && QSVIsOfType<float3>(context->argument(0)))
        return Transform_SetScale_float3(context, engine);
    printf("Transform_SetScale_selector failed to choose the right function to call in file %s, line %d!\n", __FILE__, __LINE__); PrintCallStack(context->backtrace()); return QScriptValue();
}

static QScriptValue Transform_SetOrientation_selector(QScriptContext *context, QScriptEngine *engine)
{
    if (context->argumentCount() == 1 && QSVIsOfType<float3x3>(context->argument(0)))
        return Transform_SetOrientation_float3x3(context, engine);
    if (context->argumentCount() == 1 && QSVIsOfType<Quat>(context->argument(0)))
        return Transform_SetOrientation_Quat(context, engine);
    printf("Transform_SetOrientation_selector failed to choose the right function to call in file %s, line %d!\n", __FILE__, __LINE__); PrintCallStack(context->backtrace()); return QScriptValue();
}

void FromScriptValue_Transform(const QScriptValue &obj, Transform &value)
{
    value.pos = qScriptValueToValue<float3>(obj.property("pos"));
    value.rot = qScriptValueToValue<float3>(obj.property("rot"));
    value.scale = qScriptValueToValue<float3>(obj.property("scale"));
}

QScriptValue ToScriptValue_Transform(QScriptEngine *engine, const Transform &value)
{
    QScriptValue obj = engine->newVariant(QVariant::fromValue(value)); // The contents of this variant are NOT used. The real data lies in the data() pointer of this QScriptValue. This only exists to enable overload resolution to work for QObject slots.
    ToExistingScriptValue_Transform(engine, value, obj);
    return obj;
}

QScriptValue ToScriptValue_const_Transform(QScriptEngine *engine, const Transform &value)
{
    QScriptValue obj = engine->newVariant(QVariant::fromValue(value)); // The contents of this variant are NOT used. The real data lies in the data() pointer of this QScriptValue. This only exists to enable overload resolution to work for QObject slots.
    obj.setPrototype(engine->defaultPrototype(qMetaTypeId<Transform>()));
    obj.setProperty("pos", ToScriptValue_const_float3(engine, value.pos), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    obj.setProperty("rot", ToScriptValue_const_float3(engine, value.rot), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    obj.setProperty("scale", ToScriptValue_const_float3(engine, value.scale), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    return obj;
}

QScriptValue register_Transform_prototype(QScriptEngine *engine)
{
    QScriptValue proto = engine->newObject();
    proto.setProperty("SetPos", engine->newFunction(Transform_SetPos_selector, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("SetPos", engine->newFunction(Transform_SetPos_selector, 3), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("SetRotation", engine->newFunction(Transform_SetRotation_float_float_float, 3), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("SetScale", engine->newFunction(Transform_SetScale_selector, 3), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("SetScale", engine->newFunction(Transform_SetScale_selector, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("ToFloat3x4", engine->newFunction(Transform_ToFloat3x4_const, 0), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("ToFloat4x4", engine->newFunction(Transform_ToFloat4x4_const, 0), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("FromFloat3x4", engine->newFunction(Transform_FromFloat3x4_float3x4, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("FromFloat4x4", engine->newFunction(Transform_FromFloat4x4_float4x4, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("SetRotationAndScale", engine->newFunction(Transform_SetRotationAndScale_float3x3, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("SetOrientation", engine->newFunction(Transform_SetOrientation_selector, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("Orientation3x3", engine->newFunction(Transform_Orientation3x3_const, 0), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("Orientation", engine->newFunction(Transform_Orientation_const, 0), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("Mul", engine->newFunction(Transform_Mul_Transform_const, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("ToString", engine->newFunction(Transform_ToString_const, 0), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("toString", engine->newFunction(Transform_toString_const, 0), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("SerializeToString", engine->newFunction(Transform_SerializeToString_const, 0), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    proto.setProperty("metaTypeId", engine->toScriptValue<qint32>((qint32)qMetaTypeId<Transform>()));
    engine->setDefaultPrototype(qMetaTypeId<Transform>(), proto);
    engine->setDefaultPrototype(qMetaTypeId<Transform*>(), proto);
    qScriptRegisterMetaType(engine, ToScriptValue_Transform, FromScriptValue_Transform, proto);

    QScriptValue ctor = engine->newFunction(Transform_ctor, proto, 3);
    ctor.setProperty("FromString", engine->newFunction(Transform_FromString_QString, 1), QScriptValue::Undeletable | QScriptValue::ReadOnly);
    engine->globalObject().setProperty("Transform", ctor, QScriptValue::Undeletable | QScriptValue::ReadOnly);

    return ctor;
}

