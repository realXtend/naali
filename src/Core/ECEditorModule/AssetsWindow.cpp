/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   AssetsWindow.cpp
    @brief  The main UI for managing asset storages and assets. */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "AssetsWindow.h"
#include "AssetTreeWidget.h"
#include "TreeWidgetUtils.h"
#include "SceneTreeWidgetItems.h"

#include "Framework.h"
#include "AssetAPI.h"
#include "IAsset.h"
#include "IAssetBundle.h"
#include "IAssetStorage.h"
#include "UiAPI.h"

#include "MemoryLeakCheck.h"

namespace
{
    bool HasSameRefAsPredecessors(QTreeWidgetItem *item)
    {
        QTreeWidgetItem *parent = 0, *child = item;
        while((parent = child->parent()) != 0)
        {
            if (parent->text(0).compare(child->text(0), Qt::CaseInsensitive) == 0)
                return true;
            child = parent;
        }

        return false;
    }
}

AssetsWindow::AssetsWindow(Framework *fw, QWidget *parent) :
    QWidget(parent),
    framework(fw),
    noStorageItem(0)
{
    Initialize();
    PopulateTreeWidget();
}

AssetsWindow::AssetsWindow(const QString &assetType_, Framework *fw, QWidget *parent) :
    QWidget(parent),
    framework(fw),
    noStorageItem(0),
    assetType(assetType_)
{
    Initialize();
    PopulateTreeWidget();

    // Asset picking layout
    QSpacerItem *spacer = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Fixed);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
    QPushButton *pickButton = new QPushButton(tr("Pick"), this);

    QHBoxLayout *hlayout2= new QHBoxLayout;
    hlayout2->insertSpacerItem(-1, spacer);
    hlayout2->addWidget(pickButton);
    hlayout2->addWidget(cancelButton);
    static_cast<QVBoxLayout *>(layout())->addLayout(hlayout2);

    connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), SLOT(ChangeSelectedAsset(QTreeWidgetItem *)));
    connect(pickButton, SIGNAL(clicked()), SLOT(PickAssetAndClose()));
    connect(cancelButton, SIGNAL(clicked()), SLOT(Cancel()));
}

AssetsWindow::~AssetsWindow()
{
    // Disable ResizeToContents, Qt goes sometimes into eternal loop after
    // ~AssetsWindow() if we have lots (hundreds or thousands) of items.
    treeWidget->blockSignals(true);
    treeWidget->header()->setResizeMode(QHeaderView::Interactive);
    QTreeWidgetItemIterator it(treeWidget);
    while(*it)
    {
        QTreeWidgetItem *item = *it;
        SAFE_DELETE(item);
        ++it;
    }
}

void AssetsWindow::PopulateTreeWidget()
{
    treeWidget->clear();
    alreadyAdded.clear();

    // Create "No provider" for assets without storage.
    SAFE_DELETE(noStorageItem);
    noStorageItem = new QTreeWidgetItem();
    noStorageItem->setText(0, tr("No Storage"));

    // Iterate storages
    CreateStorageItem(framework->Asset()->DefaultAssetStorage());
    foreach(const AssetStoragePtr &storage, framework->Asset()->AssetStorages())
        CreateStorageItem(storage);

    // Iterate asset bundles
    std::pair<QString, AssetBundlePtr> bundlePair;
    foreach(bundlePair, framework->Asset()->AssetBundles())
        AddBundle(bundlePair.second);

    // Iterate assets
    std::pair<QString, AssetPtr> pair;
    foreach(pair, framework->Asset()->Assets())
         AddAsset(pair.second);

    // Add the no provider last and hide if no children.
    treeWidget->addTopLevelItem(noStorageItem);
    noStorageItem->setHidden(noStorageItem->childCount() == 0);
}

void AssetsWindow::AddAsset(const AssetPtr &asset)
{
    if (alreadyAdded.find(asset) != alreadyAdded.end())
        return;
    if (!assetType.isEmpty() && assetType != asset->Type())
        return;

    AssetItem *item = CreateAssetItem(asset);
    if (!item)
        return;
    AddChildren(asset, item);

    connect(asset.get(), SIGNAL(Loaded(AssetPtr)), SLOT(UpdateAssetItem(AssetPtr)), Qt::UniqueConnection);
    connect(asset.get(), SIGNAL(Unloaded(IAsset *)), SLOT(UpdateAssetItem(IAsset *)), Qt::UniqueConnection);
    connect(asset.get(), SIGNAL(PropertyStatusChanged(IAsset *)), SLOT(UpdateAssetItem(IAsset *)), Qt::UniqueConnection);

    noStorageItem->setHidden(noStorageItem->childCount() == 0);

    // If we have an ongoing search, make sure that the new item is compared too.
    QString searchFilter = searchField->text().trimmed();
    if (!searchFilter.isEmpty())
        TreeWidgetSearch(treeWidget, 0, searchFilter);
}

void AssetsWindow::AddBundle(const AssetBundlePtr &bundle)
{
    CreateBundleItem(bundle);
}

void AssetsWindow::RemoveAsset(AssetPtr asset)
{
    QTreeWidgetItemIterator it(treeWidget);
    while(*it)
    {
        AssetItem *item = dynamic_cast<AssetItem *>(*it);
        if (item && item->Asset() && item->Asset() == asset)
        {
            QTreeWidgetItem *parent = item->parent();
            parent->removeChild(item);
            SAFE_DELETE(item);
            alreadyAdded.erase(asset);
        }

        ++it;
    }
}

void AssetsWindow::Search(const QString &filter)
{
    TreeWidgetSearch(treeWidget, 0, filter);
}

void AssetsWindow::UpdateAssetItem(IAsset *asset)
{
    QTreeWidgetItemIterator it(treeWidget);
    while(*it)
    {
        AssetItem *item = dynamic_cast<AssetItem *>(*it);
        if (item && item->Asset().get() == asset)
        {
            item->SetText(asset);
            break;
        }
        ++it;
    }
}

void AssetsWindow::Initialize()
{
    setWindowTitle(tr("Assets"));
    // Append asset type if we're viewing only assets of specific type.
    if (!assetType.isEmpty())
        setWindowTitle(windowTitle() + ": " + assetType);
    resize(450, 450);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    setLayout(layout);

    // Create child widgets
    treeWidget = new AssetTreeWidget(framework, this);
    treeWidget->setHeaderHidden(true);

    searchField = new QLineEdit(this);
    searchField->setPlaceholderText(tr("Search..."));

    expandAndCollapseButton = new QPushButton(tr("Expand All"), this);

    QHBoxLayout *hlayout= new QHBoxLayout;
    hlayout->addWidget(searchField);
    hlayout->addWidget(expandAndCollapseButton);

    layout->addLayout(hlayout);
    layout->addWidget(treeWidget);

    connect(searchField, SIGNAL(textEdited(const QString &)), SLOT(Search(const QString &)));
    connect(expandAndCollapseButton, SIGNAL(clicked()), SLOT(ExpandOrCollapseAll()));

    connect(framework->Asset(), SIGNAL(AssetCreated(AssetPtr)), SLOT(AddAsset(AssetPtr)));
    connect(framework->Asset(), SIGNAL(AssetAboutToBeRemoved(AssetPtr)), SLOT(RemoveAsset(AssetPtr)));

    connect(treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), SLOT(CheckTreeExpandStatus(QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), SLOT(CheckTreeExpandStatus(QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(AssetDoubleClicked(QTreeWidgetItem*, int)));
}

void AssetsWindow::AddChildren(const AssetPtr &asset, QTreeWidgetItem *parent)
{
    foreach(const AssetReference &ref, asset->FindReferences())
    {
        AssetPtr asset = framework->Asset()->GetAsset(ref.ref);
        if (asset && alreadyAdded.find(asset) == alreadyAdded.end())
        {
            AssetItem *item = new AssetItem(asset, parent);
            parent->addChild(item);
            alreadyAdded.insert(asset);

            // Check for recursive dependencies.
            if (HasSameRefAsPredecessors(item))
                item->setText(0, tr("Recursive dependency to ") + asset->Name());
            else
                AddChildren(asset, item);
        }
    }
}

void AssetsWindow::ExpandOrCollapseAll()
{
    treeWidget->blockSignals(true);
    bool treeExpanded = TreeWidgetExpandOrCollapseAll(treeWidget);
    treeWidget->blockSignals(false);
    expandAndCollapseButton->setText(treeExpanded ? tr("Collapse All") : tr("Expand All"));
}

void AssetsWindow::CheckTreeExpandStatus(QTreeWidgetItem * /*item*/)
{
    bool anyExpanded = false;
    QTreeWidgetItemIterator iter(treeWidget, QTreeWidgetItemIterator::HasChildren);
    while(*iter)
    {
        QTreeWidgetItem *iterItem = (*iter);
        if (iterItem->isExpanded())
        {
            if (iterItem->parent() && !iterItem->parent()->isExpanded())
                anyExpanded = false;
            else
            {
                anyExpanded = true;
                break;
            }
        }
        ++iter;
    }

    expandAndCollapseButton->setText(anyExpanded ? tr("Collapse All") : tr("Expand All"));
}

void AssetsWindow::AssetDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    AssetItem* assItem = dynamic_cast<AssetItem*>(item);
    if (!assItem || !assItem->Asset())
        return;
    
    QMenu* dummyMenu = new QMenu(this);
    dummyMenu->hide();
    QList<QObject*> targets;
    targets.push_back(assItem->Asset().get());
    
    framework->Ui()->EmitContextMenuAboutToOpen(dummyMenu, targets);
    const QList<QAction*>& actions = dummyMenu->actions();
    for (int i = 0; i < actions.size(); ++i)
    {
        if (actions[i]->text() == "Open")
        {
            actions[i]->activate(QAction::ActionEvent());
            break;
        }
    }
    dummyMenu->deleteLater();
}

void AssetsWindow::ChangeSelectedAsset(QTreeWidgetItem *current)
{
    // Note: clause if <=1 cause for some reason when activating item for the first time
    // treeWidget->selectedItems().size() returns 0, even though we should have 1.
    if (treeWidget->selectedItems().size() <= 1 && current)
    {
        AssetItem *item = dynamic_cast<AssetItem  *>(current);
        if (item && item->Asset())
            emit SelectedAssetChanged(item->Asset());
    }
}

void AssetsWindow::PickAssetAndClose()
{
    if (treeWidget->selectedItems().size() == 1)
    {
        AssetItem *item = dynamic_cast<AssetItem *>(treeWidget->currentItem());
        if (item && item->Asset())
            emit AssetPicked(item->Asset());
    }
    close();
}

void AssetsWindow::Cancel()
{
    emit PickCanceled();
    close();
}

AssetStorageItem *AssetsWindow::CreateStorageItem(const AssetStoragePtr &storage)
{
    for (int i=0; i<treeWidget->topLevelItemCount(); ++i)
    {
        AssetStorageItem *existing = dynamic_cast<AssetStorageItem*>(treeWidget->topLevelItem(i));
        if (existing && storage == existing->Storage())
            return 0;
    }

    AssetStorageItem *item = new AssetStorageItem(storage);
    treeWidget->addTopLevelItem(item);

    if (storage == framework->Asset()->DefaultAssetStorage())
    {
        QFont font = item->font(0);
        font.setBold(true);
        item->setFont(0, font);
    }
    return item;
}

AssetBundleItem *AssetsWindow::CreateBundleItem(const AssetBundlePtr &bundle)
{
    for (int i=0; i<treeWidget->topLevelItemCount(); ++i)
        if (FindBundleItemRecursive(treeWidget->topLevelItem(i), bundle))
            return 0;

    QTreeWidgetItem *p = FindParentItem(bundle);
    AssetBundleItem *item = new AssetBundleItem(bundle, p);
    p->addChild(item);
    return item;
}

AssetItem *AssetsWindow::CreateAssetItem(const AssetPtr &asset)
{
    for (int i=0; i<treeWidget->topLevelItemCount(); ++i)
        if (FindAssetItemRecursive(treeWidget->topLevelItem(i), asset))
            return 0;

    QTreeWidgetItem *p = FindParentItem(asset);
    AssetItem *item = new AssetItem(asset, p);
    p->addChild(item);
    return item;
}

AssetBundleItem *AssetsWindow::FindBundleItemRecursive(QTreeWidgetItem *parent, const AssetBundlePtr &bundle)
{
    if (!parent || parent->childCount() == 0)
        return false;

    AssetBundleItem *result = 0;
    for (int i=0; i<parent->childCount(); ++i)
    {
        AssetBundleItem *existing = dynamic_cast<AssetBundleItem*>(parent->child(i));
        if (existing && existing->AssetBundle() == bundle)
            result = existing;
        else
            result = FindBundleItemRecursive(parent->child(i), bundle);
        if (result)
            break;
    }
    return result;
}

AssetBundleItem *AssetsWindow::FindBundleItemRecursive(QTreeWidgetItem *parent, const QString &subAssetRef)
{
    if (!parent || parent->childCount() == 0)
        return false;

    AssetBundleItem *result = 0;
    for (int i=0; i<parent->childCount(); ++i)
    {
        AssetBundleItem *existing = dynamic_cast<AssetBundleItem*>(parent->child(i));
        if (existing && existing->Contains(subAssetRef))
            result = existing;
        else
            result = FindBundleItemRecursive(parent->child(i), subAssetRef);
        if (result)
            break;
    }
    return result;
}

AssetItem *AssetsWindow::FindAssetItemRecursive(QTreeWidgetItem *parent, const AssetPtr &asset)
{
    if (!parent || parent->childCount() == 0)
        return false;

    AssetItem *result = 0;
    for (int i=0; i<parent->childCount(); ++i)
    {
        AssetItem *existing = dynamic_cast<AssetItem*>(parent->child(i));
        if (existing && existing->Asset() == asset)
            result = existing;
        else
            result = FindAssetItemRecursive(parent->child(i), asset);
        if (result)
            break;
    }
    return result;
}

template <typename T>
QTreeWidgetItem *AssetsWindow::FindParentItem(const T &item)
{
    QString subAssetPart;
    AssetAPI::ParseAssetRef(item->Name(), 0, 0, 0, 0, 0, 0, 0, &subAssetPart);

    for (int i=0; i<treeWidget->topLevelItemCount(); ++i)
    {
        AssetStorageItem *existingStorage = dynamic_cast<AssetStorageItem*>(treeWidget->topLevelItem(i));
        if (existingStorage && existingStorage->Storage() == item->AssetStorage())
        {
            // If this is a sub asset to a bundle, find the bundle item from this storage.
            if (!subAssetPart.isEmpty())
            {
                AssetBundleItem *existingBundle = FindBundleItemRecursive(existingStorage, item->Name());
                if (existingBundle)
                    return existingBundle;
            }
            return existingStorage;
        }
    }
    // If this is a sub asset to a bundle, try to find the bundle also from no storage item.
    if (!subAssetPart.isEmpty())
    {
        AssetBundleItem *existingBundle = FindBundleItemRecursive(noStorageItem, item->Name());
        if (existingBundle)
            return existingBundle;
    }
    return noStorageItem;
}
