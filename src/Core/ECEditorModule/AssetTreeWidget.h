/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   AssetTreeWidget.h
 *  @brief  Tree widget showing all available assets.
 */

#pragma once

#include <QTreeWidget>

#include "AssetFwd.h"

class QMenu;
class Framework;

class AssetItem;
class AssetStorageItem;
struct AssetTreeWidgetSelection;

/// Tree widget showing all available assets.
class AssetTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    /// Constructor.
    /** @param fw Framework pointer.
        @param parent Parent widget. */
    AssetTreeWidget(Framework *fw, QWidget *parent = 0);

protected:
    /// QWidget override. Show context menu with asset-spesific actions.
    void contextMenuEvent(QContextMenuEvent *e);

    /// QWidget override.
    void dragEnterEvent(QDragEnterEvent *e);

    /// QWidget override.
    void dragMoveEvent(QDragMoveEvent *e);

    /// QWidget override.
    void dropEvent(QDropEvent *e);

private:
    /// Creates and adds applicable actions to the right-click context menu.
    /** @param menu Context menu. */
    void AddAvailableActions(QMenu *menu);

    /// Returns list of selected asset and storage items.
    AssetTreeWidgetSelection SelectedItems() const;

    Framework *framework; ///< Framework.
    QMenu *contextMenu; ///< Right-click context menu.

private slots:
    /// Deletes selected asset(s) from source.
    void DeleteFromSource();

    /// Deletes selected assets(s) from cache.
    void DeleteFromCache();

    /// Forgets selected asset(s).
    void Forget();

    /// Unloads selected asset(s).
    void Unload();

    /// Reload selected asset(s) from cache.
    void ReloadFromCache();

    /// Reload selected asset(s) from source.
    void ReloadFromSource();

    /// Imports new asset(s).
    void Import();

    /// Called by open file dialog when it's closed.
    /** @param result Result of dialog closure. Open is 1, Cancel is 0. */
    void OpenFileDialogClosed(int result);

    /// Opens Request New Asset dialog.
    void RequestNewAsset();

    /// Creates new programmatic asset.
    void CreateAsset();

    /// Makes the currently selected asset storage the default system storage.
    void MakeDefaultStorage();

    /// Removes the currently selected asset storages from the system.
    void RemoveStorage();

    /// Called by Request New Asset dialog when it's closed.
    /** @param result Result of dialog closure. Save is 1, Cancel is 0. */
    void RequestNewAssetDialogClosed(int result);

    /// Copies asset reference to clipboard.
    void CopyAssetRef();

    /// Exports selected asset(s).
    void Export();

    /// Clones selected asset(s).
    void Clone();

    /// Called by CloneAssetDialog when it's closed.
    /** @param result Result of dialog closure. Save is 1, Cancel is 0. */
    void CloneAssetDialogClosed(int result);

    /// Called by SaveAssetAs save file dialog when it's closed.
    /** @param result Result of dialog closure. Save is 1, Cancel is 0. */
    void SaveAssetDialogClosed(int result);

    /// Opens Add Content window with the provided @files added to it.
    /** @param files List of filenames. */
    void Upload(const QStringList &files);

    /// Opens the location folder of an asset in operating system's file explorer.
    void OpenFileLocation();

    /// Opens the location folder of an asset in operating system's file explorer.
    void OpenInExternalEditor();

    /// Shows dialog for invoking functions for currently selected assets.
    void OpenFunctionDialog();

    /// Called by function dialog when it's finished.
    /** @param result Result of finished. Close is 0, Execute and Close is 1, Execute is 2. */
    void FunctionDialogFinished(int result);
};
