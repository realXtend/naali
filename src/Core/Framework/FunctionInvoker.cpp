/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   FuntionInvoker.cpp
 *  @brief  Utility class which wraps QMetaObject::invokeMethod() functionality with more user-friendly API.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "CoreException.h"
#include "FunctionInvoker.h"
#include "ArgumentType.h"
#include "LoggingFunctions.h"

#include "MemoryLeakCheck.h"

void FunctionInvoker::Invoke(QObject *obj, const QString &function, const QVariantList &params, 
                             QVariant *ret, QString *errorMsg)
{
    QList<IArgumentType *> args;

    foreach(QVariant p, params)
    {
        IArgumentType *arg = CreateArgumentType(p.typeName());
        if (!arg)
        {
            if (errorMsg)
                errorMsg->append("Could not generate argument for parameter type " + QString(p.typeName()));
            return;
        }

        arg->FromQVariant(p);
        args.push_back(arg);
    }

    Invoke(obj, function, args, ret, errorMsg);
}

void FunctionInvoker::Invoke(QObject *obj, const QString &function, QList<IArgumentType *> &arguments, 
                             QVariant *ret, QString *errorMsg)
{
    QList<QGenericArgument> args;
    foreach(IArgumentType *arg, arguments)
        args.push_back(arg->Value());

    while(args.size() < 10)
        args.push_back(QGenericArgument());

    try
    {
        IArgumentType *retArgType = CreateReturnValueArgument(obj, function);
        if (retArgType)
        {
            QGenericReturnArgument retArg = retArgType->ReturnValue();

            QMetaObject::invokeMethod(obj, function.toStdString().c_str(), Qt::DirectConnection, retArg,
                args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);

            if (ret)
                *ret = retArgType->ToQVariant();
        }
        else
        {
            QMetaObject::invokeMethod(obj, function.toStdString().c_str(), Qt::DirectConnection,
                args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
        }
    }
    catch(const Exception &e)
    {
        LogError("The function call threw an Exception \"" + std::string(e.what()) + "\"!");
        if (errorMsg)
            errorMsg->append("The function call threw an Exception \"" + QString(e.what()) + "\"!");
    }
    catch(const std::exception &e)
    {
        LogError("The function call threw a std::exception \"" + std::string(e.what()) + "\"!");
        if (errorMsg)
            errorMsg->append("The function call threw a std::exception \"" + QString(e.what()) + "\"!");
    }
    catch(...)
    {
        LogError("The function call threw an unknown exception!");
        if (errorMsg)
            errorMsg->append("The function call threw an unknown exception!");
    }
}

void FunctionInvoker::Invoke(QObject *obj, const QString &functionSignature, const QStringList &params, 
                             QVariant *ret, QString *errorMsg)
{
    QList<IArgumentType *> args = CreateArgumentList(obj, functionSignature);
    if (args.size() != params.size())
    {
        LogError("FunctionInvoker::Invoke: Parameter number mismatch: " + QString::number(params.size()) +
            " given, but " + QString::number(args.size()) + " expected.");
        return;
    }

    for(int i = 0; i < args.size(); ++i)
        args[i]->FromString(params[i]);

    int idx = functionSignature.indexOf("(");
    QString functionBasename = idx != -1 ? functionSignature.left(idx) : functionSignature;
    Invoke(obj, functionBasename, args, ret, errorMsg);
}

QList<IArgumentType *> FunctionInvoker::CreateArgumentList(const QObject *obj, const QString &signature)
{
    QList<IArgumentType *> args;
    QByteArray normalizedSignature = QMetaObject::normalizedSignature(signature.toStdString().c_str());
    const QMetaObject *mo = obj->metaObject();
    for(int i = mo->methodOffset(); i < mo->methodCount(); ++i)
    {
        const QMetaMethod &mm = mo->method(i);
        if (normalizedSignature == QByteArray(mm.signature()))
            foreach(const QByteArray &param, mm.parameterTypes())
            {
                IArgumentType *arg = CreateArgumentType(QString(param));
                if (arg)
                    args.append(arg);
                else
                    return QList<IArgumentType*>(); // We failed to create some argument - can't call this function!
            }
    }

    return args;
}

IArgumentType *FunctionInvoker::CreateArgumentType(const QString &type)
{
    IArgumentType *arg = 0;

    if (type == "void")
        arg = new VoidArgumentType;
    else if (type == "QString")
        arg = new ArgumentType<QString>(type.toStdString().c_str());
    else if (type == "QStringList")
        arg = new ArgumentType<QStringList>(type.toStdString().c_str());
    else if (type == "std::string")
        arg = new ArgumentType<std::string>(type.toStdString().c_str());
    else if (type == "bool")
        arg = new ArgumentType<bool>(type.toStdString().c_str());
    else if(type == "unsigned int" || type == "uint" || type == "size_t" || type == "entity_id_t")
        arg = new ArgumentType<unsigned int>(type.toStdString().c_str());
    else if (type == "int")
        arg = new ArgumentType<int>(type.toStdString().c_str());
    else if (type == "float")
        arg = new ArgumentType<float>(type.toStdString().c_str());
    else if (type == "double")
        arg = new ArgumentType<double>(type.toStdString().c_str());
    else
        LogError("FunctionInvoker: Unsupported argument type: " + type);

    return arg;
}

IArgumentType *FunctionInvoker::CreateReturnValueArgument(const QObject *obj, const QString &function)
{
    const QMetaObject *mo = obj->metaObject();
    for(int i = mo->methodOffset(); i < mo->methodCount(); ++i)
    {
        const QMetaMethod &mm = mo->method(i);
        QString mmSig = mm.signature();
        QString mmFunc = mmSig.mid(0, mmSig.indexOf('('));
        if (function == mmFunc)
        {
            QString returnType = mm.typeName();
            if (returnType.isEmpty())
                returnType = "void";

            return CreateArgumentType(returnType);
        }
    }

    return 0;
}
