﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using DocGenerator;

namespace QScriptBindings
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 3)
            {
                Console.WriteLine("First cmdline parameter should be the absolute path to the root where doxygen generated the documentation XML files.");
                Console.WriteLine("Second cmdline parameter should be the output directory.");
                Console.WriteLine("Trhid and all subsequent parameters specify class names to generate bindings for.");
                return;
            }

            CodeStructure s = new CodeStructure();
            s.LoadSymbolsFromDirectory(args[0]);

            string outputDirectory = args[1];

            for(int i = 2; i < args.Length; ++i)
                GenerateBindings(s, args[i], outputDirectory + "\\qscript_" + args[i] + ".cpp");
        }

        static bool IsBadType(string type)
        {
            return type.Contains("bool *") || type.EndsWith("float *") || type.EndsWith("float3 *") || type.Contains("std::") || type.Contains("char*") || type.Contains("char *") || type.Contains("[");
        }

        static bool IsScriptable(Symbol s)
        {
            if (s.argList.Contains("["))
                return false;
            if (IsBadType(s.type))
                return false;
            foreach (Parameter p in s.parameters)
                if (IsBadType(p.type) || IsBadType(p.BasicType()))
                    return false;

            foreach (string str in s.Comments())
                if (str.Contains("[noscript]"))
                    return false;
            if (s.returnComment != null && s.returnComment.Contains("[noscript]"))
                return false;
            return true;
        }

        static void GenerateBindings(CodeStructure s, string className, string outFilename)
        {
            if (!s.symbolsByName.ContainsKey(className))
            {
                Console.WriteLine("Error: Symbol " + className + " not found");
                return;
            }

            Symbol classSymbol = s.symbolsByName[className];
            if (classSymbol.kind != "class" && classSymbol.kind != "struct")
            {
                Console.WriteLine("Error: Symbol " + className + " is not a class or a struct!");
                return;
            }

            TextWriter tw = new StreamWriter(outFilename);
            tw.WriteLine("#include \"QtScriptBindingsHelpers.h\"");
            tw.WriteLine("");

            GenerateToExistingScriptValue(classSymbol, tw);

            HashSet<string> functionNames = new HashSet<string>();
            foreach (Symbol child in classSymbol.children)
            {
                if (!IsScriptable(child))
                    continue;

                if (child.kind == "function" && !child.name.Contains("operator"))
                {
                    GenerateClassFunction(child, tw);
                    functionNames.Add(child.name);
                }
                else if (child.kind == "variable" && !child.isStatic && IsScriptable(child) && child.visibilityLevel == VisibilityLevel.Public)
                {
//                    GenerateClassMemberVariableGet(child, tw);
//                    GenerateClassMemberVariableSet(child, tw);
                }
            }

            foreach (string functionName in functionNames)
                GenerateClassFunctionSelector(classSymbol, functionName, tw);

            //GenerateClassCtor(classSymbol, tw);

//            GenerateScriptClass(classSymbol, tw);
            GenerateFromScriptValue(classSymbol, tw);
            GenerateToScriptValue(classSymbol, tw);
            GenerateToScriptValueConst(classSymbol, tw);
            GenerateClassPrototype(classSymbol, tw);

            tw.Close();
        }

        static public string Indent(int num)
        {
            string s = "";
            int indentSize = 4;
            for (int i = 0; i < num * indentSize; ++i)
                s += " ";
            return s;
        }

        static string GetScriptFunctionName(Symbol function)
        {
            if (function.name == function.parent.name)
            {
                Symbol classSymbol = function.parent;
                List<Symbol> functions = new List<Symbol>();
                foreach (Symbol s in classSymbol.children)
                    if (s.name == function.name && IsScriptable(s))
                        functions.Add(s);

                if (functions.Count < 2)
                    return function.parent.name + "_ctor"; // No need to generate a selector.
            }
            string str = function.parent.name + "_" + function.name;
            for (int i = 0; i < function.parameters.Count; ++i)
                str += "_" + function.parameters[i].BasicTypeId();

            if (function.isConst)
                str += "_const";
            return str;
        }

        static string GetMemberVariableGetCppFuncName(Symbol variable)
        {
            return variable.parent.name + "_" + variable.name + "_get";
        }

        static string GetMemberVariableSetCppFuncName(Symbol variable)
        {
            return variable.parent.name + "_" + variable.name + "_set";
        }

        static string GetMemberVariableGetScriptFuncName(Symbol variable)
        {
            return variable.name; // Scripts access the member variables using foo.member(); notation

            // Alternate notation: foo.getMember();
//            string firstChar = variable.name.Substring(0, 1);
//            return "get" + firstChar.ToUpper() + variable.name.Substring(1);

        }

        static string GetMemberVariableSetScriptFuncName(Symbol variable)
        {
            string firstChar = variable.name.Substring(0, 1);
            return "set" + firstChar.ToUpper() + variable.name.Substring(1);
        }

        static string GetScriptFunctionSelectorName(Symbol function)
        {
            if (function.name == function.parent.name)
                return function.parent.name  + "_ctor";
            string str = function.parent.name + "_" + function.name + "_selector";
            return str;
        }

        static bool NeedsClassFunctionSelector(Symbol classSymbol, string functionName)
        {
            if (classSymbol.name == functionName)
                return true; // Always generate a selector for the ctor of the class, because it gets a fixed name 'class_ctor'.

            int nameCount = 0;
            List<Symbol> functions = new List<Symbol>();
            foreach (Symbol s in classSymbol.children)
                if (s.name == functionName && IsScriptable(s))
                {
                    ++nameCount;
                    if (nameCount >= 2)
                        return true;
                }
            return false;
        }

        static void GenerateClassFunctionSelector(Symbol classSymbol, string functionName, TextWriter tw)
        {
            List<Symbol> functions = new List<Symbol>();
            foreach (Symbol s in classSymbol.children)
                if (s.name == functionName && IsScriptable(s))
                    functions.Add(s);

            if (functions.Count < 2)
                return; // No need to generate a selector.

            tw.WriteLine("static QScriptValue " + GetScriptFunctionSelectorName(functions[0]) + "(QScriptContext *context, QScriptEngine *engine)");
            tw.WriteLine("{");
            foreach (Symbol f in functions)
            {
                tw.Write(Indent(1) + "if (context->argumentCount() == " + f.parameters.Count);
                for(int i = 0; i < f.parameters.Count; ++i)
                {
                    tw.Write(" && ");
                    tw.Write("QSVIsOfType<" + f.parameters[i].BasicType() + ">(context->argument(" + i + "))");
                }
                tw.WriteLine(")");
                tw.WriteLine(Indent(2) + "return " + GetScriptFunctionName(f) + "(context, engine);");
            }
            bool isClassCtor = (functionName == classSymbol.name);
            if (isClassCtor)
                tw.WriteLine(Indent(1) + "printf(\"" + GetScriptFunctionSelectorName(functions[0]) + " failed to choose the right function to call! Did you use 'var x = " + classSymbol.name + "();' instead of 'var x = new " + classSymbol.name + "();'?\\n\"); PrintCallStack(context->backtrace()); return QScriptValue();");
            else
                tw.WriteLine(Indent(1) + "printf(\"" + GetScriptFunctionSelectorName(functions[0]) + " failed to choose the right function to call in file %s, line %d!\\n\", __FILE__, __LINE__); PrintCallStack(context->backtrace()); return QScriptValue();");
            tw.WriteLine("}");
            tw.WriteLine("");
        }
/*
        static void GenerateClassMemberVariableGet(Symbol variable, TextWriter tw)
        {
            Symbol Class = variable.parent;

            tw.WriteLine("static QScriptValue " + GetMemberVariableGetCppFuncName(variable) + "(QScriptContext *context, QScriptEngine *engine)");
            tw.WriteLine("{");

            tw.WriteLine(Indent(1) + "if (context->argumentCount() != 0) { printf(\"Error! Invalid number of arguments passed to function " + GetMemberVariableGetCppFuncName(variable) + " in file %s, line %d!\\nExpected 0, but got %d!\\n\", __FILE__, __LINE__, context->argumentCount()); return QScriptValue(); }");

            tw.WriteLine(Indent(1) + Class.name + " *This = " + "TypeFromQScriptValue<" + Class.name + "*>(context->thisObject());");
            tw.WriteLine(Indent(1) + "if (!This) { printf(\"Error! Invalid context->thisObject in file %s, line %d\\n!\", __FILE__, __LINE__); return QScriptValue(); }");

            tw.WriteLine(Indent(1) + "return qScriptValueFromValue(context->engine(), This->" + variable.name + ");");
            tw.WriteLine("}");
            tw.WriteLine("");
        }

        static void GenerateClassMemberVariableSet(Symbol variable, TextWriter tw)
        {
            Symbol Class = variable.parent;

            tw.WriteLine("static QScriptValue " + GetMemberVariableSetCppFuncName(variable) + "(QScriptContext *context, QScriptEngine *engine)");
            tw.WriteLine("{");

            tw.WriteLine(Indent(1) + "if (context->argumentCount() != 1) { printf(\"Error! Invalid number of arguments passed to function " + GetMemberVariableSetCppFuncName(variable) + " in file %s, line %d!\\nExpected 1, but got %d!\\n\", __FILE__, __LINE__, context->argumentCount()); return QScriptValue(); }");

            tw.WriteLine(Indent(1) + Class.name + " *This = " + "TypeFromQScriptValue<" + Class.name + "*>(context->thisObject());");
            tw.WriteLine(Indent(1) + "if (!This) { printf(\"Error! Invalid context->thisObject in file %s, line %d\\n!\", __FILE__, __LINE__); return QScriptValue(); }");
            tw.WriteLine(Indent(1) + variable.type + " " + variable.name + " = qscriptvalue_cast<" + variable.type + ">(context->argument(0));");
            tw.WriteLine(Indent(1) + "This->" + variable.name + " = " + variable.name + ";");
            tw.WriteLine(Indent(1) + "return QScriptValue();");
            tw.WriteLine("}");
            tw.WriteLine("");
        }
        */

        public static bool HasOpaqueQVariantBasedMarshalling(Symbol Class)
        {
            Symbol ctor = Class.FindChildByName(Class.name);
            foreach (string s in ctor.Comments())
            {
                if (s.Contains("[opaque-qtscript]"))
                    return true;
            }
            return false;
        }

        static void GenerateToExistingScriptValue(Symbol Class, TextWriter tw)
        {
            tw.WriteLine("void ToExistingScriptValue_" + Class.name + "(QScriptEngine *engine, const " + Class.name + " &value, QScriptValue obj)");
            tw.WriteLine("{");

            if (HasOpaqueQVariantBasedMarshalling(Class)) // If enabled, this type is marshalled opaquely.
                tw.WriteLine(Indent(1) + "obj.setData(engine->newVariant(QVariant::fromValue(value)));");

            foreach (Symbol v in Class.children)
                if (v.kind == "variable" && IsScriptable(v) && v.visibilityLevel == VisibilityLevel.Public && !v.isStatic)
                {
                    string flags = v.IsConst() ? "QScriptValue::Undeletable | QScriptValue::ReadOnly" : "QScriptValue::Undeletable";

                    if (v.IsConst() && !Symbol.IsPODType(v.type))
                        tw.WriteLine(Indent(1) + "obj.setProperty(\"" + v.name + "\", ToScriptValue_const_" + v.type + "(engine, value." + v.name
                            + "), " + flags + ");");
                    else
                        tw.WriteLine(Indent(1) + "obj.setProperty(\"" + v.name + "\", qScriptValueFromValue(engine, value." + v.name
                            + "), " + flags + ");");
                }

            tw.WriteLine("}");
            tw.WriteLine("");
        }

        static void GenerateToScriptValue(Symbol Class, TextWriter tw)
        {
            tw.WriteLine("QScriptValue ToScriptValue_" + Class.name + "(QScriptEngine *engine, const " + Class.name + " &value)");
            tw.WriteLine("{");
//            tw.WriteLine(Indent(1) + "QScriptValue obj = engine->newObject();");
            tw.WriteLine(Indent(1) + "QScriptValue obj = engine->newVariant(QVariant::fromValue(value)); // The contents of this variant are NOT used. The real data lies in the data() pointer of this QScriptValue. This only exists to enable overload resolution to work for QObject slots.");

            tw.WriteLine(Indent(1) + "ToExistingScriptValue_" + Class.name + "(engine, value, obj);");
            tw.WriteLine(Indent(1) + "return obj;");
            tw.WriteLine("}");
            tw.WriteLine("");
        }

        static void GenerateToScriptValueConst(Symbol Class, TextWriter tw)
        {
            tw.WriteLine("QScriptValue ToScriptValue_const_" + Class.name + "(QScriptEngine *engine, const " + Class.name + " &value)");
            tw.WriteLine("{");
//            tw.WriteLine(Indent(1) + "QScriptValue obj = engine->newObject();");
            tw.WriteLine(Indent(1) + "QScriptValue obj = engine->newVariant(QVariant::fromValue(value)); // The contents of this variant are NOT used. The real data lies in the data() pointer of this QScriptValue. This only exists to enable overload resolution to work for QObject slots."); 

            tw.WriteLine(Indent(1) + "obj.setPrototype(engine->defaultPrototype(qMetaTypeId<" + Class.name + ">()));");

            if (HasOpaqueQVariantBasedMarshalling(Class)) // If enabled, this type is marshalled opaquely.
                tw.WriteLine(Indent(1) + "obj.setData(engine->newVariant(QVariant::fromValue(value)));");

            foreach (Symbol v in Class.children)
                if (v.kind == "variable" && IsScriptable(v) && v.visibilityLevel == VisibilityLevel.Public && !v.isStatic)
                {
                    string conversionFunc = Symbol.IsPODType(v.type) ? "qScriptValueFromValue" : ("ToScriptValue_const_" + v.type);
                    tw.WriteLine(Indent(1) + "obj.setProperty(\"" + v.name + "\", " + conversionFunc + "(engine, value." + v.name
                        + "), QScriptValue::Undeletable | QScriptValue::ReadOnly);");
                }

            //            tw.WriteLine(Indent(1) + "obj.setPrototype(engine->defaultPrototype(qMetaTypeId<" + Class.name + ">()));");
            tw.WriteLine(Indent(1) + "return obj;");
            tw.WriteLine("}");
            tw.WriteLine("");
        }

        static void GenerateFromScriptValue(Symbol Class, TextWriter tw)
        {
            tw.WriteLine("void FromScriptValue_" + Class.name + "(const QScriptValue &obj, " + Class.name + " &value)");
            tw.WriteLine("{");

            if (HasOpaqueQVariantBasedMarshalling(Class)) // If enabled, this type is marshalled opaquely.
                tw.WriteLine(Indent(1) + "value = obj.data().toVariant().value<" + Class.name + ">();");

            foreach (Symbol v in Class.children)
                if (v.kind == "variable" && IsScriptable(v) && v.visibilityLevel == VisibilityLevel.Public && !v.isStatic)
                    tw.WriteLine(Indent(1) + "value." + v.name + " = qScriptValueToValue<" + v.type + ">(obj.property(\"" + v.name + "\"));");

            tw.WriteLine("}");
            tw.WriteLine("");
        }
/*
        static void GenerateScriptClass(Symbol Class, TextWriter tw)
        {
            tw.WriteLine("class " + Class.name + "_scriptclass : public QScriptClass");
            tw.WriteLine("{");
            tw.WriteLine("public:");
            tw.WriteLine(Indent(1) + "QScriptValue objectPrototype;");
            tw.WriteLine(Indent(1) + Class.name + "_scriptclass(QScriptEngine *engine):QScriptClass(engine){}");
            tw.WriteLine(Indent(1) + "QScriptValue property(const QScriptValue &object, const QScriptString &name, uint id)");
            tw.WriteLine(Indent(1) + "{");
            tw.WriteLine(Indent(2) + Class.name + " *This = TypeFromQScriptValue<" + Class.name + "*>(object);");
            tw.WriteLine(Indent(2) + "if (!This) { printf(\"Error! Cannot convert QScriptValue to type " + Class.name + " in file %s, line %d!\\nTry using " + Class.name + ".get%s() and " + Class.name + ".set%s() to query the member variable '%s'!\\n\", __FILE__, __LINE__, Capitalize((QString)name).c_str(), Capitalize((QString)name).c_str(), ((QString)name).toStdString().c_str()); return QScriptValue(); }");
            tw.WriteLine(Indent(2) + "QString name_ = (QString)name;");
            foreach (Symbol v in Class.children)
                if (v.kind == "variable" && IsScriptable(v) && v.visibilityLevel == VisibilityLevel.Public)
                {
//                    tw.Write(Indent(2) + "if ((QString)name == (QString)\"" + v.name + "\")");
// Experimental: Access members directly using 'foo.x_' and 'foo.x_ptr'.
                    if (v.isStatic)
                    {
                        //tw.Write(Indent(2) + "if (name_ == \"" + v.name + "\")");
                        //tw.WriteLine(" return TypeToQScriptValue(engine(), This->" + v.name + ");");
                    }
                    else
                    {
                        tw.Write(Indent(2) + "if (name_ == \"" + v.name + "_\")");
                        tw.WriteLine(" return TypeToQScriptValue(engine(), This->" + v.name + ");");
                        if (!Symbol.IsPODType(v.type))
                        {
                            tw.Write(Indent(2) + "if (name_ == \"" + v.name + "_ptr\")");
                            tw.WriteLine(" return TypeToQScriptValue(engine(), &This->" + v.name + ");");
                        }
                    }
                }

            tw.WriteLine(Indent(2) + "return QScriptValue();");
            tw.WriteLine(Indent(1) + "}");

            tw.WriteLine(Indent(1) + "void setProperty(QScriptValue &object, const QScriptString &name, uint id, const QScriptValue &value)");
            tw.WriteLine(Indent(1) + "{");
            tw.WriteLine(Indent(2) + Class.name + " *This = TypeFromQScriptValue<" + Class.name + "*>(object);");
            tw.WriteLine(Indent(2) + "if (!This) { printf(\"Error! Cannot convert QScriptValue to type " + Class.name + " in file %s, line %d!\\nTry using " + Class.name + ".get%s() and " + Class.name + ".set%s() to query the member variable '%s'!\\n\", __FILE__, __LINE__, Capitalize((QString)name).c_str(), Capitalize((QString)name).c_str(), ((QString)name).toStdString().c_str()); return; }");
            tw.WriteLine(Indent(2) + "QString name_ = (QString)name;");

            foreach (Symbol v in Class.children)
                if (v.kind == "variable" && IsScriptable(v) && !v.IsConst() && v.visibilityLevel == VisibilityLevel.Public)
                {
//                    tw.Write(Indent(2) + "if (name_ == (QString)\"" + v.name + "\")");
//                    tw.WriteLine(" This->" + v.name + " = TypeFromQScriptValue<" + v.type + ">(value);");
                    // Experimental: Access members directly using 'foo.x_' and 'foo.x_ptr'.
                    if (v.isStatic)
                    {
                        //tw.Write(Indent(2) + "if (name_ == \"" + v.name + "\")");
                       // tw.WriteLine(" This->" + v.name + " = TypeFromQScriptValue<" + v.type + ">(value);");
                    }
                    else
                    {
                        tw.Write(Indent(2) + "if (name_ == \"" + v.name + "_\")");
                        tw.WriteLine(" This->" + v.name + " = TypeFromQScriptValue<" + v.type + ">(value);");
                        if (!Symbol.IsPODType(v.type))
                        {
                            tw.Write(Indent(2) + "if (name_ == \"" + v.name + "_ptr\")");
                            tw.WriteLine(" This->" + v.name + " = *TypeFromQScriptValue<" + v.type + "*>(value);");
                        }
                    }
                }
            tw.WriteLine(Indent(1) + "}");

            tw.WriteLine(Indent(1) + "QueryFlags queryProperty(const QScriptValue &object, const QScriptString &name, QueryFlags flags, uint *id)");
            tw.WriteLine(Indent(1) + "{");
            tw.WriteLine(Indent(2) + "QString name_ = (QString)name;");
            foreach (Symbol v in Class.children)
                if (v.kind == "variable" && IsScriptable(v) && v.visibilityLevel == VisibilityLevel.Public)
                {
                    if (v.isStatic)
                    {
                        //tw.Write(Indent(2) + "if (name_ == \"" + v.name + "\")");
                    }
                    else
                    {
                        //                    tw.Write(Indent(2) + "if (name_ == \"" + v.name + "\")");
                        if (Symbol.IsPODType(v.type))
                            tw.Write(Indent(2) + "if (name_ == \"" + v.name + "_\")");
                        else
                            tw.Write(Indent(2) + "if (name_ == \"" + v.name + "_\" || name_ == \"" + v.name + "_ptr\")");
                        tw.WriteLine(" return flags;");
                    }
//                    tw.WriteLine(" return flags;");
                }
            tw.WriteLine(Indent(2) + "return 0;");
            tw.WriteLine(Indent(1) + "}");

            tw.WriteLine(Indent(1) + "QScriptValue prototype() const { return objectPrototype; }");
            tw.WriteLine("};");
        }
        */
        static void GenerateClassFunction(Symbol function, TextWriter tw)
        {
            Symbol Class = function.parent;

            tw.WriteLine("static QScriptValue " + GetScriptFunctionName(function) + "(QScriptContext *context, QScriptEngine *engine)");
            tw.WriteLine("{");
            int argIdx = 0;

            bool isClassCtor = (function.name == Class.name);

            if (function.name != "toString") // Qt oddities: It seems sometimes the hardcoded toString is called with this as the first argument and not as 'this'.
                tw.WriteLine(Indent(1) + "if (context->argumentCount() != " + function.parameters.Count + ") { printf(\"Error! Invalid number of arguments passed to function " + GetScriptFunctionName(function) + " in file %s, line %d!\\nExpected " + function.parameters.Count + ", but got %d!\\n\", __FILE__, __LINE__, context->argumentCount()); PrintCallStack(context->backtrace()); return QScriptValue(); }");

            // Test that we have a valid this.
            if (!function.isStatic && !isClassCtor)
            {
//                tw.WriteLine(Indent(1) + Class.name + " *This = " + "TypeFromQScriptValue<" + Class.name + "*>(context->thisObject());");
//                tw.WriteLine(Indent(1) + Class.name + " *This = " + "qscriptvalue_cast<" + Class.name + "*>(context->thisObject());");
                if (function.name == "toString") // Qt oddities: It seems sometimes the hardcoded toString is called with this as the first argument and not as 'this'.
                {
                    tw.WriteLine(Indent(1) + Class.name + " This;");
                    tw.WriteLine(Indent(1) + "if (context->argumentCount() > 0) This = qscriptvalue_cast<" + Class.name + ">(context->argument(0)); // Qt oddity (bug?): Sometimes the built-in toString() function doesn't give us this from thisObject, but as the first argument.");
                    tw.WriteLine(Indent(1) + "else This = qscriptvalue_cast<" + Class.name + ">(context->thisObject());");

                }
                else
                    tw.WriteLine(Indent(1) + Class.name + " This = " + "qscriptvalue_cast<" + Class.name + ">(context->thisObject());");
                //                tw.WriteLine(Indent(1) + "if (!This && context->argumentCount() > 0) This = TypeFromQScriptValue<" + Class.name + "*>(context->argument(0)); // Qt oddity (bug?): Sometimes the built-in toString() function doesn't give us this from thisObject, but as the first argument.");
//                tw.WriteLine(Indent(1) + "if (!This) { printf(\"Error! Invalid context->thisObject in function " + GetScriptFunctionName(function) + " in file %s, line %d\\n!\", __FILE__, __LINE__); return QScriptValue(); }");
            }

            // Unmarshall all parameters to the function.
            foreach (Parameter p in function.parameters)
                tw.WriteLine(Indent(1) + p.BasicType() + " " + p.name + " = qscriptvalue_cast<" + p.BasicType() + ">(context->argument(" + (argIdx++) + "));");
//            tw.WriteLine(Indent(1) + p.BasicType() + " " + p.name + " = TypeFromQScriptValue<" + p.BasicType() + ">(context->argument(" + (argIdx++) + "));");

            if (isClassCtor) // Is this function a ctor of this class.
            {
                if (function.parameters.Count == 0)
                    tw.WriteLine(Indent(1) + Class.name + " ret;"); // Create a new instance of this class, no parameters.
                else
                    tw.Write(Indent(1) + Class.name + " ret("); // Create a new instance of this class, one or more parameters.
            }
            else
            {
                string instanceName = (function.isStatic ? (Class.name + "::") : "This.");
//                string instanceName = (function.isStatic ? (Class.name + "::") : "This->");
                if (function.type != "void")
                    tw.Write(Indent(1) + function.type + " ret = " + instanceName + function.name + "("); // Make the function call.
                else
                    tw.Write(Indent(1) + instanceName + function.name + "("); // Make the function call.
            }

            for (int i = 0; i < function.parameters.Count; ++i)
                tw.Write(function.parameters[i].name + (i + 1 < function.parameters.Count ? ", " : ""));
            if (!isClassCtor || function.parameters.Count > 0)
                tw.WriteLine(");");

            // If the function is non-const, regenerate the proper QScriptValue as the result.
            if (!function.IsConst() && !function.isStatic && function.name != Class.name)
                tw.WriteLine(Indent(1) + "ToExistingScriptValue_" + Class.name + "(engine, This, context->thisObject());");

            // Return the return value as QScriptValue.
            if (!isClassCtor)
            {
                if (function.type != "void")
//                    tw.WriteLine(Indent(1) + "return TypeToQScriptValue(engine, ret);");
                    tw.WriteLine(Indent(1) + "return qScriptValueFromValue(engine, ret);");
                else
                    tw.WriteLine(Indent(1) + "return QScriptValue();");
            }
            else
            {
//                tw.WriteLine(Indent(1) + "return TypeToQScriptValue(engine, ret);");
                tw.WriteLine(Indent(1) + "return qScriptValueFromValue(engine, ret);");
            }

            tw.WriteLine("}");
            tw.WriteLine("");
        }

        static int CountMaxArgumentsForClassCtor(Symbol Class)
        {
            int args = 0;
            foreach (Symbol c in Class.children)
                if (c.name == Class.name)
                    args = Math.Max(args, c.parameters.Count);
            return args;
        }

        static void GenerateClassPrototype(Symbol Class, TextWriter tw)
        {
            HashSet<string> registeredFunctions = new HashSet<string>();
            tw.WriteLine("QScriptValue register_" + Class.name + "_prototype(QScriptEngine *engine)");
            tw.WriteLine("{");
//            tw.WriteLine(Indent(1) + "engine->setDefaultPrototype(qMetaTypeId<" + Class.name + "*>(), QScriptValue());");
//            tw.WriteLine(Indent(1) + "QScriptValue proto = engine->newVariant(qVariantFromValue((" + Class.name + "*)0));");
            tw.WriteLine(Indent(1) + "QScriptValue proto = engine->newObject();");

            // Add each member function to the prototype.
            foreach (Symbol child in Class.children)
                if (!registeredFunctions.Contains(child.name + "_____" + child.parameters.Count) && child.kind == "function" && !child.isStatic && child.name != Class.name && !child.name.Contains("operator") && IsScriptable(child))
                {
                    tw.WriteLine(Indent(1) + "proto.setProperty(\"" + child.name + "\", engine->newFunction(" + (NeedsClassFunctionSelector(Class, child.name) ? GetScriptFunctionSelectorName(child) : GetScriptFunctionName(child))
                        + ", " + child.parameters.Count + "), QScriptValue::Undeletable | QScriptValue::ReadOnly);");
                    registeredFunctions.Add(child.name + "_____" + child.parameters.Count);
                }
/*
            // Add setters and getters for each member variable to the prototype.
            foreach (Symbol child in Class.children)
                if (child.kind == "variable" && !child.isStatic && child.name != Class.name && !child.name.Contains("operator") && IsScriptable(child) && child.visibilityLevel == VisibilityLevel.Public)
                {
                    tw.WriteLine(Indent(1) + "proto.setProperty(\"" + GetMemberVariableGetScriptFuncName(child) + "\", engine->newFunction(" + GetMemberVariableGetCppFuncName(child) + ", 1));");
                    tw.WriteLine(Indent(1) + "proto.setProperty(\"" + GetMemberVariableSetScriptFuncName(child) + "\", engine->newFunction(" + GetMemberVariableSetCppFuncName(child) + ", 1));");
                }
*/
//            tw.WriteLine(Indent(1) + Class.name + "_scriptclass *sc = new " + Class.name + "_scriptclass(engine);");
//            tw.WriteLine(Indent(1) + "engine->setProperty(\"" + Class.name + "_scriptclass\", QVariant::fromValue<QScriptClass*>(sc));");
//            tw.WriteLine(Indent(1) + "proto.setScriptClass(sc);");
//            tw.WriteLine(Indent(1) + "sc->objectPrototype = proto;");

            tw.WriteLine(Indent(1) + "proto.setProperty(\"metaTypeId\", engine->toScriptValue<qint32>((qint32)qMetaTypeId<" + Class.name + ">()));");
            tw.WriteLine(Indent(1) + "engine->setDefaultPrototype(qMetaTypeId<" + Class.name + ">(), proto);");
            tw.WriteLine(Indent(1) + "engine->setDefaultPrototype(qMetaTypeId<" + Class.name + "*>(), proto);");
            tw.WriteLine(Indent(1) + "qScriptRegisterMetaType(engine, ToScriptValue_" + Class.name + ", FromScriptValue_" + Class.name + ", proto);");
            tw.WriteLine("");

            tw.WriteLine(Indent(1) + "QScriptValue ctor = engine->newFunction(" + Class.name + "_ctor, proto, " + CountMaxArgumentsForClassCtor(Class) + ");");

            registeredFunctions.Clear();
            foreach (Symbol child in Class.children)
                if (!registeredFunctions.Contains(child.name + "_____" + child.parameters.Count) && child.kind == "function" && child.isStatic && child.name != Class.name && !child.name.Contains("operator") && IsScriptable(child))
                {
                    tw.WriteLine(Indent(1) + "ctor.setProperty(\"" + child.name + "\", engine->newFunction(" + (NeedsClassFunctionSelector(Class, child.name) ? GetScriptFunctionSelectorName(child) : GetScriptFunctionName(child))
                        + ", " + child.parameters.Count + "), QScriptValue::Undeletable | QScriptValue::ReadOnly);");
                    registeredFunctions.Add(child.name + "_____" + child.parameters.Count);
                }

            foreach (Symbol child in Class.children)
                if (child.kind == "variable" && child.isStatic && child.name != Class.name && !child.name.Contains("operator") && IsScriptable(child))
                {
//                    tw.WriteLine(Indent(1) + "ctor.setProperty(\"" + child.name + "\", TypeToQScriptValue(engine, " + Class.name + "::" + child.name + "));");
                    tw.WriteLine(Indent(1) + "ctor.setProperty(\"" + child.name + "\", qScriptValueFromValue(engine, " + Class.name + "::" + child.name +
                        "), QScriptValue::Undeletable" + ( child.IsConst() ? " | QScriptValue::ReadOnly" : "" ) + ");");
                    registeredFunctions.Add(child.name + "_____" + child.parameters.Count);
                }

            tw.WriteLine(Indent(1) + "engine->globalObject().setProperty(\"" + Class.name + "\", ctor, QScriptValue::Undeletable | QScriptValue::ReadOnly);");
            tw.WriteLine("");

            tw.WriteLine(Indent(1) + "return ctor;");

            tw.WriteLine("}");
            tw.WriteLine("");
            
        }
    }
}
