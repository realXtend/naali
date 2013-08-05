/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneStructureWindow.h
    @brief  Window with tree view showing every entity in a scene. */

#pragma once

#include "SceneFwd.h"
#include "CoreTypes.h"

#include <QWidget>
#include <QTreeWidgetItem>

class SceneTreeWidget;
class Framework;
class EntityItem;

class QLineEdit;
class QPushButton;
class QTreeWidgetItem;
class QToolButton;

/// Window with tree view showing every entity in a scene.
/** This class will only handle adding and removing of entities and components and updating
    their names. The SceneTreeWidget implements most of the functionality. */
class SceneStructureWindow : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the window.
    /** @param fw Framework.
        @parent parent Parent widget. */
    explicit SceneStructureWindow(Framework *fw, QWidget *parent = 0);

    /// Destructor.
    ~SceneStructureWindow();

    /// Sets new scene to be shown in the tree view.
    /** Populates tree view with entities.
        If scene is set to null, the tree view is cleared and previous signal connections are disconnected.
        @param newScene Scene. */
    void SetScene(const ScenePtr &newScene);

    /// Event filter to catch and react to child widget events
    virtual bool eventFilter(QObject *obj, QEvent *e);

public slots:
    /// Sets do we want to show components in the tree view.
    /** @param show Visibility of components in the tree view. */
    void ShowComponents(bool show);

    /// Sets do we want to show asset references in the tree view.
    /** @param show Visibility of asset references in the tree view. */
    void ShowAssetReferences(bool show);

    /// Decorates (bolds) or undecorates item representing @c entity.
    /** @param entity Entity in question.
        @param selected Whether to decorate (true) or undecorate (false) the item. */
    void SetEntitySelected(const EntityPtr &entity, bool selected);

    /// Undecorates all selected entities.
    void ClearSelectedEntites();

protected:
    /// QWidget override.
    void changeEvent(QEvent* e);

private:
    /// Populates tree widget with all entities.
    void Populate();

    /// Clears tree widget.
    void Clear();

    /// Creates asset references to a single entity item
    void CreateAssetReference(QTreeWidgetItem *targetItem);

    /// Creates asset reference items to all entities
    void CreateAssetReferences();

    /// Clears i.e. deletes all asset reference items.
    void ClearAssetReferences();

    /// Create asset reference item to the tree widget.
    /** @param parentItem Parent item, can be entity or component item.
        @param attr AssetReference attribute. */
    void CreateAssetItem(QTreeWidgetItem *parentItem, IAttribute *attr);

    EntityItem* EntityItemOfEntity(Entity* ent);
    EntityItem* EntityItemById(entity_id_t id);
    void RemoveEntityItem(EntityItem* item);

    Framework *framework; ///< Framework.
    SceneWeakPtr scene; ///< Scene which we are showing the in tree widget currently.
    SceneTreeWidget *treeWidget; ///< Scene tree widget.
    QHash<QString, QTreeWidgetItem*> entityGroupItems_; /// < Entity groups
    bool showComponents; ///< Do we show components also in the tree view.
    bool showAssets; ///< Do we show asset references also in the tree view.
    QLineEdit *searchField; ///< Search field line edit.
    QPushButton *expandAndCollapseButton; ///< Expand/collapse all button.
    QToolButton * undoButton_; ///< Undo button with drop-down menu
    QToolButton * redoButton_; ///< Redo button with drop-down menu

private slots:
    /// Adds the entity to the tree widget.
    /** @param entity Entity to be added. */
    void AddEntity(Entity *entity);

    /// Removes entity from the tree widget.
    /** @param entity Entity to be removed. */
    void RemoveEntity(Entity *entity);

    /// Readds entity on server ack.
    /** @param entity Entity to be readded.
        @param oldId Old entity id */
    void AckEntity(Entity *entity, entity_id_t oldId);

    /// Updates temporary state of an entity and its components in the tree widget
    /** @param entity The entity which temporary state was toggled */
    void UpdateEntityTemporaryState(Entity *entity);

    /// Adds the entity to the tree widget.
    /** @param entity Altered entity.
        @param comp Component which was added. */
    void AddComponent(Entity *entity, IComponent *comp);

    /// Removes entity from the tree widget.
    /** @param entity Altered entity.
        @param comp Component which was removed. */
    void RemoveComponent(Entity *entity, IComponent *comp);

    /// @overload
    /** This is called only by EC_DynamicComponent when asset ref attribute is added to it.
        @param attr AssetReference attribute. */
    void AddAssetReference(IAttribute *attr);

    /// Removes asset reference from the tree widget.
    /** This is called only by EC_DynamicComponent when asset ref attribute is removed from it.
        @param attr AssetReference attribute. */
    void RemoveAssetReference(IAttribute *attr);

    /// When asset reference attribute changes, update UI accordingly.
    /** @param attr AssetReference attribute. */
    void UpdateAssetReference(IAttribute *attr);

    /// Updates entity's name in the tree widget if entity's EC_Name component's "name" attribute has changed.
    /** EC_Name component's AttributeChanged() signal is connected to this slot.
        @param attr EC_Name's attribute which was changed. */
    void UpdateEntityName(IAttribute *attr);

    /// Updates component's name in the tree widget if components name has changed.
    /** @param oldName Old component name.
        @param newName New component name. */
    void UpdateComponentName(const QString &oldName, const QString &newName);

    /// Sort items in the tree widget. The outstanding sort order is used.
    /** @param criteria Sorting criteria. Currently tr("ID") and tr("Name") are supported. */
    void Sort(const QString &criteria);

    /// Searches for items containing @c text (case-insensitive) and toggles their visibility.
    /** If match is found the item is set visible and expanded, otherwise it's hidden.
        @param filter Text used as a filter. */
    void Search(const QString &filter);

    /// Expands or collapses the whole tree view, depending on the previous action.
    void ExpandOrCollapseAll();

    /// Checks the expand status to mark it to the expand/collapse button
    void CheckTreeExpandStatus(QTreeWidgetItem *item);

    /// Removes entity from the tree widget by ID
    void RemoveEntityById(entity_id_t id);

    void OnUndoChanged(bool canUndo);
    void OnRedoChanged(bool canRedo);
};
