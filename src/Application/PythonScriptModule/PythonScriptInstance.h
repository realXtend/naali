/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   PythonScriptInstance.h
 *  @brief  Python script instance used with EC_Script.
 */

#pragma once

#include "PythonFwd.h"
#include "IScriptInstance.h"

#include <PythonQtObjectPtr.h>
#include <QString>

/// Python script instance used with EC_Script.
class PythonScriptInstance : public IScriptInstance
{
public:
    /// Constructs new script instance. Creates new module/context for the script file.
    /** @param filename Filename of the script (include path and file extension).
        @param entity Parent entity.
    */
    PythonScriptInstance(const QString &filename, Entity *entity);

    /// Destructor.
    virtual ~PythonScriptInstance() {}

    /// IScriptInstance override.
    void Load();

    /// IScriptInstance override.
    void Unload();

    /// IScriptInstance override.
    void Run();

    /// IScriptInstance override.
    virtual QString GetLoadedScriptName() const { return filename_; }

    /// IScriptInstance override.
    bool IsEvaluated() const { return evaluated_; }
    
public slots:
    /// Dumps engine information into a string. Used for debugging/profiling.
    virtual QMap<QString, uint> DumpEngineInformation() { return QMap<QString, uint>(); };
    
private:
    PythonQtObjectPtr context_; ///< Python context for this script instance.
    QString filename_; ///< Script filename.
    QString moduleName_; ///< Python module name for the script file.
    bool evaluated_; ///< Whether the script has been evaluated
};
