/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   SceneStructureModule.cpp
 *  @brief  Provides UIs for scene and asset maintenance and content import.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "SceneStructureModule.h"
#include "SceneStructureWindow.h"
#include "AssetsWindow.h"
#include "SupportedFileTypes.h"
#include "AddContentWindow.h"

#include "SceneAPI.h"
#include "AssetAPI.h"
#include "IAsset.h"
#include "IAssetTransfer.h"
#include "Scene.h"
#include "Entity.h"
#include "ConsoleAPI.h"
#include "InputAPI.h"
#include "OgreRenderingModule.h"
#include "Renderer.h"
#include "SceneImporter.h"
#include "EC_Camera.h"
#include "EC_Placeable.h"
#include "EC_Mesh.h"
#include "UiAPI.h"
#include "UiGraphicsView.h"
#include "UiMainWindow.h"
#include "LoggingFunctions.h"
#include "SceneDesc.h"
#include "ECEditorModule.h"
#include "ECEditorWindow.h"
#include "OgreWorld.h"
#include "ConfigAPI.h"
#include "OgreMaterialUtils.h"

#include <QToolTip>
#include <QCursor>

#ifdef ASSIMP_ENABLED
#include <OpenAssetImport.h>
#endif

#include "MemoryLeakCheck.h"

// Shortcuts for config keys.
static const char *cSceneWindowPos = "scene window pos";
static const char *cAssetWindowPos = "asset window pos";

SceneStructureModule::SceneStructureModule() :
    IModule("SceneStructure"),
    sceneWindow(0),
    assetsWindow(0),
    toolTipWidget(0),
    toolTip(0)
{
}

SceneStructureModule::~SceneStructureModule()
{
    SAFE_DELETE(sceneWindow);
    SAFE_DELETE(assetsWindow);
    SAFE_DELETE(toolTipWidget);
}

void SceneStructureModule::Initialize()
{
    // No headless checks for these as they are useful in headless mode too.
    framework_->Console()->RegisterCommand("scenestruct", "Shows the Scene Structure window, hides it if it's visible.", 
        this, SLOT(ToggleSceneStructureWindow()));
    framework_->Console()->RegisterCommand("assets", "Shows the Assets window, hides it if it's visible.", 
        this, SLOT(ToggleAssetsWindow()));

    if (!framework_->IsHeadless())
    {
        inputContext = framework_->Input()->RegisterInputContext("SceneStructureInput", 102);
        connect(inputContext.get(), SIGNAL(KeyPressed(KeyEvent *)), this, SLOT(HandleKeyPressed(KeyEvent *)));

        UiGraphicsView *gv = framework_->Ui()->GraphicsView();
        connect(gv, SIGNAL(DragEnterEvent(QDragEnterEvent*, QGraphicsItem*)), SLOT(HandleDragEnterEvent(QDragEnterEvent*, QGraphicsItem*)));
        connect(gv, SIGNAL(DragLeaveEvent(QDragLeaveEvent*)), SLOT(HandleDragLeaveEvent(QDragLeaveEvent*)));
        connect(gv, SIGNAL(DragMoveEvent(QDragMoveEvent*, QGraphicsItem*)), SLOT(HandleDragMoveEvent(QDragMoveEvent*, QGraphicsItem*)));
        connect(gv, SIGNAL(DropEvent(QDropEvent*, QGraphicsItem*)), SLOT(HandleDropEvent(QDropEvent*, QGraphicsItem*)));

        // Stay in sync with EC editors' selection.
        connect(framework_->GetModule<ECEditorModule>(), SIGNAL(ActiveEditorChanged(ECEditorWindow *)),
            this, SLOT(SyncSelectionWithEcEditor(ECEditorWindow *)), Qt::UniqueConnection);

        toolTipWidget = new QWidget(0, Qt::ToolTip);
        toolTipWidget->setLayout(new QHBoxLayout());
        toolTipWidget->layout()->setMargin(0);
        toolTipWidget->layout()->setSpacing(0);
        toolTipWidget->setContentsMargins(0,0,0,0);
        toolTipWidget->setStyleSheet(
            "QWidget { background-color: transparent; } QLabel { padding: 2px; border: 0.5px solid grey; border-radius: 0px; \
            background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(246, 246, 246, 255), stop:1 rgba(237, 237, 237, 255)); }");

        toolTip = new QLabel(toolTipWidget);
        toolTip->setTextFormat(Qt::RichText);
        toolTipWidget->layout()->addWidget(toolTip);
    }
}

void SceneStructureModule::Uninitialize()
{
    SaveWindowPosition(sceneWindow.data(), cSceneWindowPos);
    SaveWindowPosition(assetsWindow.data(), cAssetWindowPos);
}

void SceneStructureModule::InstantiateContent(const QStringList &filenames, const float3 &worldPos, bool clearScene)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("SceneStructureModule::InstantiateContent: Could not retrieve main camera scene.");
        return;
    }

    QList<SceneDesc> sceneDescs;

    foreach(const QString &filename, filenames)
    {
        if (!IsSupportedFileType(filename))
        {
            LogError("SceneStructureModule::InstantiateContent: Unsupported file extension: " + filename + ".");
            continue;
        }

        if (filename.endsWith(cOgreSceneFileExtension, Qt::CaseInsensitive))
        {
            ///\todo Implement ogre.scene url drops at some point?
            TundraLogic::SceneImporter importer(scene->shared_from_this());
            sceneDescs.append(importer.CreateSceneDescFromScene(filename));
        }
        else if (filename.endsWith(cOgreMeshFileExtension, Qt::CaseInsensitive))
        {
            TundraLogic::SceneImporter importer(scene->shared_from_this());
            ///\todo Perhaps download the mesh before instantiating so we could inspect the mesh binary for materials and skeleton?
            /// The path is already there for tundra scene file web drops
            //if (IsUrl(filename)) ...
            sceneDescs.append(importer.CreateSceneDescFromMesh(filename));
        }
        else if (filename.toLower().indexOf(cTundraXmlFileExtension) != -1 && filename.toLower().indexOf(cOgreMeshFileExtension) == -1)
        {
            if (IsUrl(filename))
            {
                AssetTransferPtr transfer = framework_->Asset()->RequestAsset(filename);
                if (transfer.get())
                {
                    urlToDropPos[filename] = worldPos;
                    connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), SLOT(HandleSceneDescLoaded(AssetPtr)));
                    connect(transfer.get(), SIGNAL(Failed(IAssetTransfer*, QString)), SLOT(HandleSceneDescFailed(IAssetTransfer*, QString)));
                    break; // Only allow one .txml drop at a time
                }
            }
            else
                sceneDescs.append(scene->CreateSceneDescFromXml(filename));
        }
        else if (filename.toLower().indexOf(cTundraBinFileExtension) != -1)
        {
            if (IsUrl(filename))
            {
                AssetTransferPtr transfer = framework_->Asset()->RequestAsset(filename);
                if (transfer.get())
                {
                    urlToDropPos[filename] = worldPos;
                    connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), SLOT(HandleSceneDescLoaded(AssetPtr)));
                    connect(transfer.get(), SIGNAL(Failed(IAssetTransfer*, QString)), SLOT(HandleSceneDescFailed(IAssetTransfer*, QString)));
                    break; // Only allow one .tbin drop at a time
                }
            }
            else
            {
                sceneDescs.append(scene->CreateSceneDescFromBinary(filename));
            }
        }
        else
        {
#ifdef ASSIMP_ENABLED
            QFileInfo path(filename);
            AssImp::OpenAssetImport assimporter;
            QString extension = "." + path.extension();
            if (assimporter.IsSupportedExtension(extension))
            {
                std::string dirname = path.dir.path().toStdString();
                std::vector<AssImp::MeshData> meshNames;
                assimporter.GetMeshData(filename, meshNames);

                TundraLogic::SceneImporter sceneImporter(scene->shared_from_this());
                for(size_t i=0 ; i<meshNames.size() ; ++i)
                    sceneImporter.ImportMesh(meshNames[i].file_, dirname, meshNames[i].transform_,
                        "", "local://", AttributeChange::Default, false, meshNames[i].name_);

                return;
            }
#endif
        }
    }

    if (!sceneDescs.isEmpty())
    {
        AddContentWindow *addContent = new AddContentWindow(framework_, scene->shared_from_this());
        addContent->AddDescription(sceneDescs);
        if (worldPos != float3::zero)
            addContent->SetContentPosition(worldPos);
        addContent->show();
    }
}

void SceneStructureModule::InstantiateContent(const QString &filename, const float3 &worldPos, bool clearScene)
{
    return InstantiateContent(QStringList(filename), worldPos, clearScene);
}

void SceneStructureModule::CentralizeEntitiesTo(const float3 &pos, const QList<Entity *> &entities)
{
    float3 minPos(1e9f, 1e9f, 1e9f);
    float3 maxPos(-1e9f, -1e9f, -1e9f);

    QList<Entity*> filteredEntities;
    foreach(Entity *e, entities)
    {
        EC_Placeable *p = e->GetComponent<EC_Placeable>().get();
        // Ignore entities that doesn't have placable component or they are a child of another placable.
        if (p && p->parentRef.Get().IsEmpty())
            filteredEntities.push_back(e);
    }

    foreach(Entity *e, filteredEntities)
    {
        EC_Placeable *p = e->GetComponent<EC_Placeable>().get();
        if (p)
        {
            float3 pos = p->transform.Get().pos;
            minPos.x = std::min(minPos.x, pos.x);
            minPos.y = std::min(minPos.y, pos.y);
            minPos.z = std::min(minPos.z, pos.z);
            maxPos.x = std::max(maxPos.x, pos.x);
            maxPos.y = std::max(maxPos.y, pos.y);
            maxPos.z = std::max(maxPos.z, pos.z);
        }
    }

    // We assume that world's up axis is Y-coordinate axis.
    float3 importPivotPos = float3((minPos.x + maxPos.x) / 2, minPos.y, (minPos.z + maxPos.z) / 2);
    float3 offset = pos - importPivotPos;

    foreach(Entity *e, filteredEntities)
    {
        EC_Placeable *p = e->GetComponent<EC_Placeable>().get();
        if (p)
        {
            Transform t = p->transform.Get();
            t.pos += offset;
            p->transform.Set(t, AttributeChange::Default);
        }
    }
}

bool SceneStructureModule::IsSupportedFileType(const QString &fileRef)
{
    if (fileRef.endsWith(cTundraXmlFileExtension, Qt::CaseInsensitive) ||
        fileRef.endsWith(cTundraBinFileExtension, Qt::CaseInsensitive) ||
        fileRef.endsWith(cOgreMeshFileExtension, Qt::CaseInsensitive) ||
        fileRef.endsWith(cOgreSceneFileExtension, Qt::CaseInsensitive))
    {
        return true;
    }
    else
    {
#ifdef ASSIMP_ENABLED
        QFileInfo path(fileRef);
        AssImp::OpenAssetImport assimporter;
        QString extension = "." path.extension();
        if (assimporter.IsSupportedExtension(extension))
            return true;
#endif
        return false;
    }
}

bool SceneStructureModule::IsMaterialFile(const QString &fileRef)
{
    if (fileRef.toLower().endsWith(".material"))
        return true;
    return false;
}

bool SceneStructureModule::IsUrl(const QString &fileRef)
{
    if (fileRef.startsWith("http://") || fileRef.startsWith("https://"))
        return true;
    return false;
}

void SceneStructureModule::CleanReference(QString &fileRef)
{
    if (!IsUrl(fileRef))
    {
        QUrl fileUrl(fileRef);
        fileRef = fileUrl.path();
#ifdef _WINDOWS
        // We have '/' as the first char on windows and the filename
        // is not identified as a file properly. But on other platforms the '/' is valid/required.
        fileRef = fileRef.mid(1);
#endif
    }
}

void SceneStructureModule::ToggleSceneStructureWindow()
{
    if (sceneWindow)
    {
        sceneWindow->setVisible(!sceneWindow->isVisible());
        if (!sceneWindow->isVisible())
        {
            SaveWindowPosition(sceneWindow.data(), cSceneWindowPos);
            sceneWindow->close();
        }
        return;
    }

    sceneWindow = new SceneStructureWindow(framework_, framework_->Ui()->MainWindow());
    sceneWindow->setAttribute(Qt::WA_DeleteOnClose);
    sceneWindow->setWindowFlags(Qt::Tool);
    sceneWindow->SetScene(GetFramework()->Scene()->MainCameraScene()->shared_from_this());
    LoadWindowPosition(sceneWindow.data(), cSceneWindowPos);
    sceneWindow->show();

    // Reflect possible current selection of EC editor to Scene Structure window right away.
    SyncSelectionWithEcEditor(framework_->GetModule<ECEditorModule>()->ActiveEditor());
}

void SceneStructureModule::ToggleAssetsWindow()
{
    if (assetsWindow)
    {
        assetsWindow->setVisible(!assetsWindow->isVisible());
        if (!assetsWindow->isVisible())
        {
            SaveWindowPosition(assetsWindow.data(), cAssetWindowPos);
            assetsWindow->close();
        }
        return;
    }

    assetsWindow = new AssetsWindow(framework_, framework_->Ui()->MainWindow());
    assetsWindow->setAttribute(Qt::WA_DeleteOnClose);
    assetsWindow->setWindowFlags(Qt::Tool);
    LoadWindowPosition(assetsWindow.data(), cAssetWindowPos);
    assetsWindow->show();
}

void SceneStructureModule::SaveWindowPosition(QWidget *widget, const QString &settingName)
{
    if (widget)
    {
        ConfigData configData(ConfigAPI::FILE_FRAMEWORK, Name(), settingName, widget->pos());
        framework_->Config()->Set(configData);
    }
}

void SceneStructureModule::LoadWindowPosition(QWidget *widget, const QString &settingName)
{
    if (framework_->Ui()->MainWindow() && widget)
    {
        ConfigData configData(ConfigAPI::FILE_FRAMEWORK, Name(), settingName);
        QPoint pos = framework_->Config()->Get(configData).toPoint();
        UiMainWindow::EnsurePositionWithinDesktop(widget, pos);
    }
}

void SceneStructureModule::HandleKeyPressed(KeyEvent *e)
{
    if (e->eventType != KeyEvent::KeyPressed || e->keyPressCount > 1)
        return;

    InputAPI &input = *framework_->Input();
    const QKeySequence showSceneStruct = input.KeyBinding("ShowSceneStructureWindow", QKeySequence(Qt::ShiftModifier + Qt::Key_S));
    const QKeySequence showAssets = input.KeyBinding("ShowAssetsWindow", QKeySequence(Qt::ShiftModifier + Qt::Key_A));
    if (e->Sequence()== showSceneStruct)
    {
        ToggleSceneStructureWindow();
        e->handled = true;
    }
    if (e->Sequence() == showAssets)
    {
        ToggleAssetsWindow();
        e->handled = true;
    }
}

void SceneStructureModule::HandleDragEnterEvent(QDragEnterEvent *e, QGraphicsItem *widget)
{
    // Ignore drag event if widget not null
    if (widget)
        return;

    // If at least one file is supported, accept.
    bool accept = false;

    int acceptedCount = 0;
    QString dropResourceNames;
    currentToolTipSource.clear();
    if (e->mimeData()->hasUrls())
    {
        foreach(QUrl url, e->mimeData()->urls())
        {
            if (IsSupportedFileType(url.path()))
            {
                dropResourceNames.append(url.toString().split("/").last() + ", ");
                acceptedCount++;
                accept = true;
            }
            // Accept .material only if a single material is being dropped
            else if (IsMaterialFile(url.path()))
            {
                if (e->mimeData()->urls().count() == 1)
                {
                    dropResourceNames.append(url.toString().split("/").last());
                    acceptedCount++;
                    accept = true;
                }
            }
        }
    }
    
    if (accept)
    {
        if (dropResourceNames.endsWith(", "))
            dropResourceNames.chop(2);
        if (dropResourceNames.count(",") >= 2 && acceptedCount > 2)
        {
            int from = dropResourceNames.indexOf(",");
            dropResourceNames = dropResourceNames.left(dropResourceNames.indexOf(",", from+1));
            dropResourceNames.append(QString("... (%1 assets)").arg(acceptedCount));
        }
        if (!dropResourceNames.isEmpty())
        {
            if (dropResourceNames.count(",") > 0)
                currentToolTipSource = "<p style='white-space:pre'><span style='font-weight:bold;'>Sources:</span> " + dropResourceNames;
            else
                currentToolTipSource = "<p style='white-space:pre'><span style='font-weight:bold;'>Source:</span> " + dropResourceNames;
        }
    }

    e->setAccepted(accept);
}

void SceneStructureModule::HandleDragLeaveEvent(QDragLeaveEvent *e)
{
    if (!toolTipWidget)
        return;
    toolTipWidget->hide();
    currentToolTipSource.clear();
    currentToolTipDestination.clear();
}

void SceneStructureModule::HandleDragMoveEvent(QDragMoveEvent *e, QGraphicsItem *widget)
{
    // Ignore drag event if widget not null
    if (widget)
    {
        if (toolTipWidget)
            toolTipWidget->hide();
        e->ignore();
        return;
    }
    if (!e->mimeData()->hasUrls())
    {
        e->ignore();
        return;
    }

    currentToolTipDestination.clear();
    foreach(const QUrl &url, e->mimeData()->urls())
    {
        if (IsSupportedFileType(url.path()))
            e->accept();
        else if (IsMaterialFile(url.path()))
        {
            e->setAccepted(false);
            currentToolTipDestination = "<br><span style='font-weight:bold;'>Destination:</span> ";
            // Raycast to see if there is a submesh under the material drop
            OgreRenderer::RendererPtr renderer = framework_->GetModule<OgreRenderer::OgreRenderingModule>()->GetRenderer();
            if (renderer)
            {
                RaycastResult* res = renderer->Raycast(e->pos().x(), e->pos().y());
                if (res->entity)
                {
                    EC_Mesh *mesh = res->entity->GetComponent<EC_Mesh>().get();
                    if (mesh)
                    {
                        currentToolTipDestination.append("Submesh " + QString::number(res->submesh));
                        if (!mesh->Name().isEmpty())
                            currentToolTipDestination.append(" on " + mesh->Name());
                        else if (!mesh->ParentEntity()->Name().isEmpty())
                            currentToolTipDestination.append(" on " + mesh->ParentEntity()->Name());
                        currentToolTipDestination.append("</p>");
                        e->accept();
                    }
                }
            }
            if (!e->isAccepted())
            {
                currentToolTipDestination.append("None</p>");
                e->ignore();
            }
        }
    }

    if (e->isAccepted() && currentToolTipDestination.isEmpty())
    {
        OgreRenderer::RendererPtr renderer = framework_->GetModule<OgreRenderer::OgreRenderingModule>()->GetRenderer();
        if (renderer)
        {
            RaycastResult* res = renderer->Raycast(e->pos().x(), e->pos().y());
            if (res)
            {
                if (res->entity)
                {
                    QString entityName = res->entity->Name();
                    currentToolTipDestination = "<br><span style='font-weight:bold;'>Destination:</span> ";
                    if (!entityName.isEmpty())
                        currentToolTipDestination.append(entityName + " ");
                    QString xStr = QString::number(res->pos.x);
                    xStr = xStr.left(xStr.indexOf(".")+3);
                    QString yStr = QString::number(res->pos.y);
                    yStr = yStr.left(yStr.indexOf(".")+3);
                    QString zStr = QString::number(res->pos.z);
                    zStr = zStr.left(zStr.indexOf(".")+3);
                    currentToolTipDestination.append(QString("(%2 %3 %4)</p>").arg(xStr, yStr, zStr));
                }
                else
                    currentToolTipDestination = "<br><span style='font-weight:bold;'>Destination:</span> Dropping in front of camera</p>";
            }
        }
    }
    
    if (toolTipWidget && !currentToolTipSource.isEmpty())
    {
        if (currentToolTipDestination.isEmpty())
            currentToolTipDestination = "</p>";
        if (toolTip->text() != currentToolTipSource + currentToolTipDestination)
        {
            toolTip->setText(currentToolTipSource + currentToolTipDestination);
            toolTipWidget->resize(1,1);
        }
        toolTipWidget->move(QPoint(QCursor::pos().x()+25, QCursor::pos().y()+25));

        if (!toolTipWidget->isVisible())
            toolTipWidget->show();
    }
}

void SceneStructureModule::HandleDropEvent(QDropEvent *e, QGraphicsItem *widget)
{
    if (toolTipWidget)
        toolTipWidget->hide();

    // Drop happened on a grapchis view widget, ignore
    if (widget)
        return;

    if (e->mimeData()->hasUrls())
    {
        // Handle materials with own handler
        if (e->mimeData()->urls().count() == 1)
        {
            QString fileRef = e->mimeData()->urls().first().toString();
            if (IsMaterialFile(fileRef))
            {
                CleanReference(fileRef);
                HandleMaterialDropEvent(e, fileRef);
                return;
            }
        }

        // Handle other supported file types
        QList<Entity *> importedEntities;

        OgreRenderer::Renderer *renderer = framework_->Scene()->MainCameraScene()->GetWorld<OgreWorld>()->Renderer();
        if (!renderer)
            return;

        float3 worldPos;
        RaycastResult* res = renderer->Raycast(e->pos().x(), e->pos().y());
        if (!res->entity)
        {
            // No entity hit, use camera's position with hard-coded offset.
            Scene *scene = GetFramework()->Scene()->MainCameraScene();
            if (!scene)
                return;

            foreach(const EntityPtr &cam, scene->GetEntitiesWithComponent(EC_Camera::TypeNameStatic()))
                if (cam->GetComponent<EC_Camera>()->IsActive())
                {
                    EC_Placeable *placeable = cam->GetComponent<EC_Placeable>().get();
                    if (placeable)
                    {
                        Quat q = placeable->WorldOrientation();
                        float3 v = q * scene->ForwardVector();
                        worldPos = placeable->Position() + v * 20;
                        break;
                    }
                }
        }
        else
            worldPos = res->pos;

        QStringList files;
        foreach(const QUrl &url, e->mimeData()->urls())
        {
            QString fileRef = url.toString();
            CleanReference(fileRef);
            files.append(fileRef);
        }

        InstantiateContent(files, worldPos, false);

        // Calculate import pivot and offset for new content
        //if (importedEntities.size())
        //    CentralizeEntitiesTo(worldPos, importedEntities);

        e->acceptProposedAction();
    }
}

void SceneStructureModule::HandleMaterialDropEvent(QDropEvent *e, const QString &materialRef)
{
    // Raycast to see if there is a submesh under the material drop
    OgreRenderer::RendererPtr renderer = framework_->GetModule<OgreRenderer::OgreRenderingModule>()->GetRenderer();
    if (renderer)
    {
        RaycastResult* res = renderer->Raycast(e->pos().x(), e->pos().y());
        if (res->entity)
        {
            EC_Mesh *mesh = res->entity->GetComponent<EC_Mesh>().get();
            if (mesh)
            {
                uint subMeshCount = mesh->GetNumSubMeshes();
                uint subMeshIndex = res->submesh;
                if (subMeshIndex < subMeshCount)
                {
                    materialDropData.affectedIndexes.clear();

                    // Get the filename
                    QString matName = materialRef;
                    matName.replace("\\", "/");
                    matName = matName.split("/").last();

                    QString cleanMaterialRef = matName;

                    // Add our dropped material to the raycasted submesh,
                    // append empty string or the current material string to the rest of them
                    AssetReferenceList currentMaterials = mesh->getmeshMaterial();
                    AssetReferenceList afterMaterials;
                    for(uint i=0; i<subMeshCount; ++i)
                    {
                        if (i == subMeshIndex)
                        {
                            afterMaterials.Append(AssetReference(cleanMaterialRef));
                            materialDropData.affectedIndexes.append(i);
                        }
                        else if (i < (uint)currentMaterials.Size())
                            afterMaterials.Append(currentMaterials[i]);
                        else
                            afterMaterials.Append(AssetReference());
                    }
                    // Clear our any empty ones from the end
                    for(uint i=afterMaterials.Size(); i>0; --i)
                    {
                        AssetReference assetRef = afterMaterials[i-1];
                        if (assetRef.ref.isEmpty())
                            afterMaterials.RemoveLast();
                        else
                            break;
                    }

                    // Url: Finish now
                    // File: Finish when add content dialog gives Completed signal
                    materialDropData.mesh = mesh;
                    materialDropData.materials = afterMaterials;

                    if (IsUrl(materialRef))
                    {
                        QString baseUrl = materialRef.left(materialRef.length() - cleanMaterialRef.length());
                        FinishMaterialDrop(true, baseUrl);
                    }
                    else
                    {
                        Scene *scene = GetFramework()->Scene()->MainCameraScene();
                        if (!scene)
                        {
                            LogError("SceneStructureModule::HandleMaterialDropEvent: Could not retrieve main camera scene.");
                            return;
                        }

                        // Open source file for reading
                        QFile materialFile(materialRef);
                        if (!materialFile.open(QIODevice::ReadOnly))
                        {
                            LogError("SceneStructureModule::HandleMaterialDropEvent: Could not open dropped material file.");
                            return;
                        }

                        // Create scene description
                        SceneDesc sceneDesc;
                        sceneDesc.filename = materialRef;

                        // Add our material asset to scene description
                        AssetDesc ad;
                        ad.typeName = "material";
                        ad.source = materialRef;
                        ad.destinationName = matName;
                        ad.data = materialFile.readAll();
                        ad.dataInMemory = true;

                        sceneDesc.assets[qMakePair(ad.source, ad.subname)] = ad;
                        materialFile.close();

                        // Add texture assets to scene description
                        QSet<QString> textures = OgreRenderer::ProcessMaterialForTextures(ad.data);
                        if (!textures.empty())
                        {
                            QString dropFolder = materialRef;
                            dropFolder = dropFolder.replace("\\", "/");
                            dropFolder = dropFolder.left(dropFolder.lastIndexOf("/")+1);

                            foreach(const QString &textureName, textures)
                            {
                                AssetDesc ad;
                                ad.typeName = "texture";
                                ad.source = dropFolder + textureName;
                                ad.destinationName = textureName;
                                ad.dataInMemory = false;
                                sceneDesc.assets[qMakePair(ad.source, ad.subname)] = ad;
                            }
                        }

                        // Show add content window
                        AddContentWindow *addMaterials = new AddContentWindow(framework_, scene->shared_from_this());
                        connect(addMaterials, SIGNAL(Completed(bool, const QString&)), SLOT(FinishMaterialDrop(bool, const QString&)));
                        addMaterials->AddDescription(sceneDesc);
                        addMaterials->show();
                    }
                    e->acceptProposedAction();
                }
            }
        }
    }
}

void SceneStructureModule::FinishMaterialDrop(bool apply, const QString &materialBaseUrl)
{
    if (apply)
    {
        EC_Mesh *mesh = materialDropData.mesh;
        if (mesh)
        {
            // Inspect the base url where the assets were uploaded
            // rewrite the affeced materials to have the base url in front
            if (!materialDropData.affectedIndexes.empty())
            {
                AssetReferenceList rewrittenMats;
                AssetReferenceList mats = materialDropData.materials;
                for(uint i=0; i<(uint)mats.Size(); ++i)
                {
                    if (materialDropData.affectedIndexes.contains(i))
                    {
                        QString newRef = materialBaseUrl;
                        if (!newRef.endsWith("/")) // just to be sure
                            newRef.append("/");
                        newRef.append(mats[i].ref);
                        rewrittenMats.Append(AssetReference(newRef));
                    }
                    else
                        rewrittenMats.Append(mats[i]);
                }
                mesh->meshMaterial.Set(rewrittenMats, AttributeChange::Default);
            }
        }
    }
    materialDropData.mesh = 0;
    materialDropData.materials = AssetReferenceList();
    materialDropData.affectedIndexes.clear();
}

void SceneStructureModule::HandleSceneDescLoaded(AssetPtr asset)
{
    QApplication::restoreOverrideCursor();

    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("SceneStructureModule::HandleSceneDescLoaded: Could not retrieve main camera scene.");
        return;
    }

    // Resolve the adjust raycast pos of this drop
    float3 adjustPos = float3::zero;
    if (urlToDropPos.contains(asset->Name()))
    {
        adjustPos = urlToDropPos[asset->Name()];
        urlToDropPos.remove(asset->Name());
    }

    // Get xml data
    std::vector<u8> data;
    asset->SerializeTo(data);
    if (data.empty())
    {
        LogError("SceneStructureModule::HandleSceneDescLoaded:Failed to serialize txml.");
        return;
    }

    QByteArray data_qt((const char *)&data[0], data.size());
    if (data_qt.isEmpty())
    {
        LogError("SceneStructureModule::HandleSceneDescLoaded:Failed to convert txml data to QByteArray.");
        return;
    }

    // Init description
    SceneDesc sceneDesc;
    sceneDesc.filename = asset->Name();

    // Get data
    if (sceneDesc.filename.toLower().endsWith(cTundraXmlFileExtension))
        sceneDesc = scene->CreateSceneDescFromXml(data_qt, sceneDesc); 
    else if(sceneDesc.filename.toLower().endsWith(cTundraBinFileExtension))
        sceneDesc = scene->CreateSceneDescFromBinary(data_qt, sceneDesc);
    else
    {
        LogError("SceneStructureModule::HandleSceneDescLoaded:Somehow other than " + cTundraXmlFileExtension + 
            " or " + cTundraBinFileExtension + " file got drag and dropped to the scene? Cannot proceed with add content dialog.");
        return;
    }

    // Show add content window
    AddContentWindow *addContent = new AddContentWindow(framework_, scene->shared_from_this());
    addContent->AddDescription(sceneDesc);
    addContent->SetContentPosition(adjustPos);
    addContent->show();
}

void SceneStructureModule::HandleSceneDescFailed(IAssetTransfer *transfer, QString reason)
{
    QApplication::restoreOverrideCursor();
    LogError(QString("SceneStructureModule::HandleSceneDescFailed: Failed to download %1 with reason %2").arg(transfer->source.ref).arg(reason));
    if (urlToDropPos.contains(transfer->SourceUrl()))
        urlToDropPos.remove(transfer->SourceUrl());
}

void SceneStructureModule::SyncSelectionWithEcEditor(ECEditorWindow *editor)
{
    if (sceneWindow && editor)
    {
        sceneWindow->ClearSelectedEntites();
        foreach(const EntityPtr &entity, editor->SelectedEntities())
            sceneWindow->SetEntitySelected(entity, true);

        connect(editor, SIGNAL(EntitySelected(const EntityPtr &, bool)),
            sceneWindow, SLOT(SetEntitySelected(const EntityPtr &, bool)), Qt::UniqueConnection);
    }
}
