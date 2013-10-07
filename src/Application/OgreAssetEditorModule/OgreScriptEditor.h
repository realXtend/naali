/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   OgreScriptEditor.h
    @brief  Text editing tool for OGRE material and particle scripts. */

#pragma once

#include "ui_OgreScriptEditor.h"
#include "OgreAssetEditorModuleApi.h"
#include "AssetFwd.h"

#include <QWidget>

class QPushButton;
class QLineEdit;
class QTextEdit;

//class PropertyTableWidget;

/// Text editing tool for OGRE material and particle scripts.
class ASSET_EDITOR_MODULE_API OgreScriptEditor : public QWidget, public Ui::OgreScriptEditor
{
    Q_OBJECT

public:
    OgreScriptEditor(const AssetPtr &scriptAsset, Framework *fw, QWidget *parent = 0);
    ~OgreScriptEditor();

    void SetScriptAsset(const AssetPtr &scriptAsset);

public slots:
    void Open();

private:
    /// Creates the text edit field for raw editing.
    void CreateTextEdit();

    /// Creates the property table for material property editing.
    /// @todo Regression. Re-implement or remove.
//    void CreatePropertyEditor();

    Framework *framework;
    AssetWeakPtr asset;
    QTextEdit *textEdit; ///< Text edit field used in raw edit mode.
//    PropertyTableWidget *propertyTable; ///< Table widget for editing material properties.

private slots:
    /// Saves changes made to the asset.
    void Save();

    /// Saves as new.
    void SaveAs();

    /// Validates the script name.
    /** @param name Name. */
    void ValidateScriptName(const QString &name);

    /// Validates the property's new value.
    /** @param row Row of the cell.
        @param column Column of the cell. */
    /// @todo Regression. Re-implement or remove.
//    void PropertyChanged(int row, int column);

    void OnAssetTransferSucceeded(AssetPtr asset);
    void OnAssetTransferFailed(IAssetTransfer *transfer, QString reason);
};
