// For conditions of distribution and use, see copyright notice in license.txt

#pragma once

#include "IModule.h"
#include "ECEditorModuleApi.h"
#include "SceneFwd.h"
#include "InputFwd.h"

#include <QObject>
#include <QPointer>
#include <QVariantList>

class QScriptEngine;

class ECEditorWindow;
class EcXmlEditorWidget;
class TreeWidgetItemExpandMemory;
typedef boost::shared_ptr<TreeWidgetItemExpandMemory> ExpandMemoryPtr;

/// Implements and enables visual editing of ECs.
/** @defgroup ECEditorModuleClient ECEditorModule Client interface. */

/// Implements and enables visual editing of ECs.
/** @ingroup ECEditorModuleClient */
class ECEDITOR_MODULE_API ECEditorModule : public IModule
{
    Q_OBJECT

public:
    ECEditorModule();
    virtual ~ECEditorModule();

    void Initialize();
    void Uninitialize();

public slots:
    /// Shows the entity-component editor window.
    void ShowEditorWindow();

    /// Returns currently active editor.
    ECEditorWindow *ActiveEditor() const;

    /// Returns tree widget state memory object, which keeps track which items in EC editor are expanded.
    /** When constructing new EC editor windows use this if you want to keep all editor windows' state synchronized. */
    ExpandMemoryPtr ExpandMemory() const { return expandMemory; }

    /// Sets do we want to show visual editing aids (gizmo and highlights) when EC editor is open/active.
    /// This value is applicable for all open/active EC editors which are children of the main window.
    /** @note The effect of this depends whether or not we have EC_Highlight and EC_TranformGizmo available in the build.
        @param show Do we want to show or hide visual editings aids. */
    void ShowVisualEditingAids(bool show);

    /// Returns are we showing transform editing gizmo when EC editor is open/active.
    bool VisualEditingAidsEnabled() const { return showVisualAids; }

    /// Shows Doxygen documentation for symbol in external window.
    /** @param symbolName Name of the symbol (class, function, etc.) */
    void ShowDocumentation(const QString &symbolName);

    /// ECEditor has gained a focus event and need to set as active editor.
    /** @param editor editor that has focus. */
    void ECEditorFocusChanged(ECEditorWindow *editor);

    /// Creates EC attribute XML editor widget for entities and components.
    /** @param entities List of entity pointers. */
    void CreateXmlEditor(const QList<EntityPtr> &entities);

    /// This is an overloaded function.
    /** @param entity Entity pointer. */
    void CreateXmlEditor(EntityPtr entity);

    /// This is an overloaded function.
    /** @param component Component pointer. */
    void CreateXmlEditor(ComponentPtr component);

    /// This is an overloaded function.
    /** @param components List of component pointers. */
    void CreateXmlEditor(const QList<ComponentPtr> &components);

    /// Repositions the given editor relative to an active SceneStructureWindow or MainWindow.
    /** @param editor ECEditorWindow. */
    void RepositionEditor(ECEditorWindow *editor);

signals:
    /// Signal is emitted when active ECEditorWindow's selection has changed.
    /** @param compType Selected item's component type name.
        @param compName Selected item's component name.
        @param attrType Selected item's attribute type name (Empty if attribute isn't selected).
        @param attrName Selected item's attribute name (Empty if attribute isn't selected). */
    void SelectionChanged(const QString &compType, const QString &compName, const QString &attrType, const QString &attrName);

    /// Emitted when the active EC editor changes.
    /** @param editor The editor that just became active. */
    void ActiveEditorChanged(ECEditorWindow *editor);

private:
    InputContextPtr inputContext; ///< Input context.
    ExpandMemoryPtr expandMemory; ///< Keeps track which items in EC editor are expanded and collapsed.
    QPointer<EcXmlEditorWidget> xmlEditor; ///< EC XML editor window
    QPointer<ECEditorWindow> activeEditor; ///< Currently active ECEditorWindow.
    QPointer<ECEditorWindow> commonEditor; ///< ECEditorModule has one common editor for all parties to use.
    bool showVisualAids; ///< Do we want to show visual editing aids (gizmo and highlights) when EC editor is open/active.
    bool toggleSelectAllEntities;

private slots:
    /// Handles KeyPressed() signal from input context.
    /** @param e Key event. */
    void HandleKeyPressed(KeyEvent *e);

    /// Embeds the ECEditorModule types to the given script engine.
    void OnScriptEngineCreated(QScriptEngine* engine);
};
