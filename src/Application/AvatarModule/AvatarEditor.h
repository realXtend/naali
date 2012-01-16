// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "AvatarModule.h"
#include "AvatarModuleApi.h"
#include "SceneFwd.h"

#include "ui_avatareditor.h"

#include <QWidget>

class QTabWidget;
class EC_Avatar;
class AvatarDescAsset;
typedef boost::shared_ptr<AvatarDescAsset> AvatarDescAssetPtr;

/// Avatar editing window.
class AV_MODULE_API AvatarEditor : public QWidget, public Ui::AvatarEditor
{
    Q_OBJECT

public:
    explicit AvatarEditor(AvatarModule *avatar_module);
    ~AvatarEditor();

public slots:
    /// Rebuild edit view
    void RebuildEditView();
    
    /// Save avatar
    void SaveAvatar();
    
    /// Load new avatar asset
    void LoadAvatar();

    /// Revert avatar edits
    void RevertAvatar();

    /// Change avatar's material
    void ChangeMaterial();

    /// New attachment
    void AddAttachment();

    /// Remove attachment
    void RemoveAttachment();

    /// Bone modifier scrollbar value handler
    void BoneModifierValueChanged(int value);

    /// Morph modifier scrollbar value handler
    void MorphModifierValueChanged(int value);

    /// Master appearance modifier scrollbar value handler
    void MasterModifierValueChanged(int value);

    /// Set avatar entity and asset to edit
    void SetEntityToEdit(EntityPtr entity);
    
protected:
    /// QWidget override.
    void changeEvent(QEvent* e);

signals:
    void EditorStatus(const QString &message, int timeout = 7000);
    void EditorError(const QString &message, int timeout = 7000);
    void EditorHideMessages();

private:
    /// Owner module.
    AvatarModule *avatar_module_;

    /// Create editor window
    void InitEditorWindow();
    
    /// Get the avatar entity, avatar component, and avatar description. If all are non-null, return true
    bool GetAvatarDesc(Entity*& entity, EC_Avatar*& avatar, AvatarDescAsset*& desc);

    /// Clear a panel
    void ClearPanel(QWidget* panel);

    /// Create or get a tabbed scrollarea panel
    QWidget* GetOrCreateTabScrollArea(QTabWidget* tabs, const std::string& name);

/*
    /// Ask a filename from the user. Store the directory used.
    std::string GetOpenFileName(const std::string& filter, const std::string& prompt);
    /// Ask a filename from the user for saving. Store the directory used.
    std::string GetSaveFileName(const std::string& filter, const std::string& prompt);
*/
    /// Last used directory for selecting avatars, attachments, textures
    std::string last_directory_;

    /// Avatar entity to edit
    EntityWeakPtr avatarEntity_;
    /// Avatar asset to edit
    boost::weak_ptr<AvatarDescAsset> avatarAsset_;

    bool reverting_;
};
