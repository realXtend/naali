/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   TreeWidgetItemExpandMemory.h
    @brief  Utility class for keeping track of expanded items at a tree widget. */

#pragma once

#include "ECEditorModuleApi.h"

#include <QObject>
#include <QSet>

class Framework;
class QTreeWidget;
class QTreeWidgetItem;

/// Utility class for keeping track of expanded items at a tree widget.
/** Usage example:
    @code
        QTreeWidget *treeWidget = new QTreeWidget;
        TreeWidgetItemExpandMemory *mem = new TreeWidgetItemExpandMemory("GroupIdName", GetFramework());

        connect(treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)),
            mem, SLOT(HandleItemExpanded(QTreeWidgetItem *)), Qt::UniqueConnection);
        connect(treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
            mem, SLOT(HandleItemCollapsed(QTreeWidgetItem *)), Qt::UniqueConnection);
    @endcode */
class ECEDITOR_MODULE_API TreeWidgetItemExpandMemory : public QObject
{
    Q_OBJECT

public:
    /// Constructs the object and loads information about expanded items from config file.
    /** @param group Group name identifier.
        @param fw Framework. */
    TreeWidgetItemExpandMemory(const char *group, Framework *fw);

    /// Destroys the object and saves information about currently expanded items to config file.
    ~TreeWidgetItemExpandMemory();

    /// Expands or collapses @c item in @c treeWidget depending if its identifier is found (expand) or not (collpse) from the expand memory.
    /** @param treeWidet Tree widget containting the @c item.
        @param item Tree widget item. */
    void ExpandItem(QTreeWidget *treeWidget, const QTreeWidgetItem *item) const;

    /// Return identifier for the @c item.
    /** The text is full "path" of item and all its predecessors, separated with dots,
        e.g. "TopLevelItemName.SecondLevelItemName.ItemName".
        @note Currently uses text from item column 0 only.
        @param item Tree widget item. */
    QString IndentifierForItem(const QTreeWidgetItem *item) const;

    /// Loads information about expanded items from config file.
    /** Clears possible already existing state information. */
    void Load();

    /// Saves information about currently expanded items to config file.
    void Save();

public slots:
    /// Creates identifier text for @c item and adds it to set of indentifier texts.
    /** @param item Expanded tree widget item. */
    void HandleItemExpanded(QTreeWidgetItem *item);

    /// Creates identifier text for @c item and removes it from set of indentifier texts.
    /** @param item Collapsed tree widget item. */
    void HandleItemCollapsed(QTreeWidgetItem *item);

private:
    /// Returns information about expanded items as one string suitable for saving to config file.
    QString ToString() const;

    QSet<QString> items; ///< Set of item identifier texts.
    Framework *framework; ///< Framework.
    QString groupName; ///< Setting group name.
};
