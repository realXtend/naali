/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   EntityActionDialog.h
 *  @brief  Dialog for invoking entity actions.
 */

#pragma once

#include "ECEditorModuleApi.h"
#include "SceneFwd.h"
#include "EntityAction.h"

#include <QDialog>

class QLineEdit;
class QComboBox;
class QCheckBox;

struct InvokeItem;

/// Dialog for invoking entity actions.
/** Emits finished(0) when "Close" is clicked, finished(1) when "Close and Execute" is cliked,
    and finished(2), when "Execute" is cliked. The dialog is destroyed when hide() or close() is called for it. */
class ECEDITOR_MODULE_API EntityActionDialog : public QDialog
{
    Q_OBJECT

public:
    /// Constructs the dialog and populates action combo box with union of all the actions of all the @c entities.
    /** @param entities Entities for which action is performed.
        @param parent Parent widget. */
    EntityActionDialog(const QList<EntityWeakPtr> &entities, QWidget *parent = 0);

    /// Constructs the dialog and uses information of @c invokeItem to fill the currently active function and parameter editors.
    /** @param entities Entities for which action is performed.
        @param invokeItem Invoke history item 
        @param parent Parent widget. */
    EntityActionDialog(const QList<EntityWeakPtr> &entities, const InvokeItem &invokeItem, QWidget *parent = 0);

    /// Returns list of entities for which the action is triggered.
    QList<EntityWeakPtr> Entities() const;

    /// Returns execution type(s) of the action.
    EntityAction::ExecTypeField ExecutionType() const;

    /// Returns name of the action.
    QString Action() const;

    /// Returns parameter list for the action.
    QStringList Parameters() const;

protected:
    /// QWidget override.
    void hideEvent(QHideEvent *);

private:
    void Initialize();

    QComboBox *actionComboBox;
    QLineEdit *parametersLineEdit;
    QCheckBox *localCheckBox;
    QCheckBox *serverComboBox;
    QCheckBox *peersComboBox;
    QPushButton *execButton;
    QPushButton *execAndCloseButton;
    QList<EntityWeakPtr> entities;

private slots:
    /// Checks that we have some execution type checked. If not, "Execute" and "Execute and Close" buttons are disabled.
    void CheckExecuteAccepted();

    /// Emits finished(2).
    void Execute();
};
