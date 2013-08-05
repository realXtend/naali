// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "TundraLogicModule.h"
#include "Client.h"
#include "Server.h"
#include "SceneImporter.h"
#include "SyncManager.h"
#include "KristalliProtocolModule.h"

#include "Profiler.h"
#include "SceneAPI.h"
#include "AssetAPI.h"
#include "IAssetTransfer.h"
#include "IAsset.h"
#include "ConfigAPI.h"
#include "IComponentFactory.h"
#include "Scene/Scene.h"
#include "AssetAPI.h"
#include "ConsoleAPI.h"
#include "AssetAPI.h"
#include "CoreException.h"
#include "LoggingFunctions.h"
#include "InterestManager.h"

#include "EC_Name.h"
#include "EC_DynamicComponent.h"
#include "EC_InputMapper.h"

#ifdef EC_Highlight_ENABLED
#include "EC_Highlight.h"
#endif

#ifdef EC_HoveringText_ENABLED
#include "EC_HoveringText.h"
#endif

#ifdef EC_Sound_ENABLED
#include "EC_Sound.h"
#include "EC_SoundListener.h"
#endif

#ifdef EC_PlanarMirror_ENABLED
#include "EC_PlanarMirror.h"
#endif

#ifdef EC_ProximityTrigger_ENABLED
#include "EC_ProximityTrigger.h"
#endif

#ifdef EC_TransformGizmo_ENABLED
#include "EC_TransformGizmo.h"
#endif

#ifdef EC_LaserPointer_ENABLED
#include "EC_LaserPointer.h"
#endif

#include "StaticPluginRegistry.h"

#include "MemoryLeakCheck.h"

namespace TundraLogic
{

static const unsigned short cDefaultPort = 2345;

TundraLogicModule::TundraLogicModule() :
    IModule("TundraLogic"),
    kristalliModule_(0)
{
}

TundraLogicModule::~TundraLogicModule()
{
}

void TundraLogicModule::Load()
{
    // Name, DynamicComponent and InputMapper from TundraCore are present always.
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_Name>));
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_DynamicComponent>));
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_InputMapper>));

    // External EC's
#ifdef EC_Highlight_ENABLED
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_Highlight>));
#endif
#ifdef EC_HoveringText_ENABLED
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_HoveringText>));
#endif
#ifdef EC_Sound_ENABLED
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_Sound>));
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_SoundListener>));
#endif
#ifdef EC_PlanarMirror_ENABLED
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_PlanarMirror>));
#endif
#ifdef EC_ProximityTrigger_ENABLED
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_ProximityTrigger>));
#endif
#ifdef EC_TransformGizmo_ENABLED
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_TransformGizmo>));
#endif
#ifdef EC_LaserPointer_ENABLED
    framework_->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_LaserPointer>));
#endif
}

void TundraLogicModule::Initialize()
{
    syncManager_ = MAKE_SHARED(SyncManager, this);
    client_ = MAKE_SHARED(Client, this);
    server_ = MAKE_SHARED(Server, this);
    
    // Expose client and server to everyone
    framework_->RegisterDynamicObject("client", client_.get());
    framework_->RegisterDynamicObject("server", server_.get());

    // Expose SyncManager only on the server side for scripting
    if (server_->IsAboutToStart())
        framework_->RegisterDynamicObject("syncmanager", syncManager_.get());

    framework_->Console()->RegisterCommand("startServer", "Starts a server. Usage: startServer(port,protocol)",
        server_.get(), SLOT(Start(unsigned short,QString)));

    framework_->Console()->RegisterCommand("stopServer", "Stops the server", server_.get(), SLOT(Stop()));

    framework_->Console()->RegisterCommand("connect",
        "Connects to a server. Usage: connect(address,port,username,password,protocol)",
        client_.get(), SLOT(Login(const QString &, unsigned short, const QString &, const QString&, const QString &)));

    framework_->Console()->RegisterCommand("disconnect", "Disconnects from a server.", client_.get(), SLOT(Logout()));

    framework_->Console()->RegisterCommand("saveScene",
        "Saves scene into XML or binary. Usage: saveScene(filename,asBinary=false,saveTemporaryEntities=false,saveLocalEntities=true)",
        this, SLOT(SaveScene(QString, bool, bool, bool)), SLOT(SaveScene(QString)));

    framework_->Console()->RegisterCommand("loadScene",
        "Loads scene from XML or binary. Usage: loadScene(filename,clearScene=true,useEntityIDsFromFile=true)",
        this, SLOT(LoadScene(QString, bool, bool)));

    framework_->Console()->RegisterCommand("importScene",
        "Loads scene from a dotscene file. Optionally clears the existing scene."
        "Replace-mode can be optionally disabled. Usage: importScene(filename,clearScene=false,replace=true)",
        this, SLOT(ImportScene(QString, bool, bool)), SLOT(ImportScene(QString)));

    framework_->Console()->RegisterCommand("importMesh",
        "Imports a single mesh as a new entity. Position, rotation, and scale can be specified optionally."
        "Usage: importMesh(filename, pos = 0 0 0, rot = 0 0 0, scale = 1 1 1, inspectForMaterialsAndSkeleton=true)",
        this, SLOT(ImportMesh(QString, const float3 &, const float3 &, const float3 &, bool)), SLOT(ImportMesh(QString)));

    // Take a pointer to KristalliProtocolModule so that we don't have to take/check it every time
    kristalliModule_ = framework_->GetModule<KristalliProtocolModule>();
    if (!kristalliModule_)
        throw Exception("Fatal: could not get KristalliProtocolModule");

    // Read startup params when application event loop starts.
    QTimer::singleShot(0, this, SLOT(ReadStartupParameters()));
}

void TundraLogicModule::Uninitialize()
{
    kristalliModule_ = 0;
    syncManager_.reset();
    client_.reset();
    server_.reset();
}

void TundraLogicModule::Update(f64 frametime)
{
    PROFILE(TundraLogicModule_Update);
    // Update client & server
    if (client_)
        client_->Update(frametime);
    if (server_)
        server_->Update(frametime);
    // Run scene sync
    if (syncManager_)
        syncManager_->Update(frametime);
    // Run scene interpolation
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (scene)
        scene->UpdateAttributeInterpolations(frametime);
}

void TundraLogicModule::LoadStartupScene()
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        scene = framework_->Scene()->CreateScene("TundraServer", true, true).get();
//        framework_->Scene()->SetDefaultScene(scene);
    }

    bool hasFile = framework_->HasCommandLineParameter("--file");
    QStringList files = framework_->CommandLineParameters("--file");
    if (hasFile && files.isEmpty())
        LogError("TundraLogicModule: --file specified without a value.");

    foreach(QString file, files)
    {
        // If the file parameter uses the full storage specifier format, parse the "src" keyvalue
        if (file.indexOf(';') != -1 || file.indexOf('=') != -1)
        {
            QMap<QString, QString> keyValues = AssetAPI::ParseAssetStorageString(file);
            file = keyValues["src"];
        }

        // At this point, if we have a LocalAssetProvider, it has already also parsed the --file command line option
        // and added the appropriate path as a local asset storage. Here we assume that is the case, so that the
        // scene we now load will be able to refer to local:// assets in its subfolders.
        AssetAPI::AssetRefType sceneRefType = AssetAPI::ParseAssetRef(file);
        if (sceneRefType != AssetAPI::AssetRefLocalPath && sceneRefType != AssetAPI::AssetRefRelativePath)
        {
            LogInfo("TundraLogicModule: Starting transfer of startup scene " + file);
            AssetTransferPtr transfer = framework_->Asset()->RequestAsset(file);
            if (transfer)
            {
                connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), SLOT(StartupSceneTransfedSucceeded(AssetPtr)));
                connect(transfer.get(), SIGNAL(Failed(IAssetTransfer*, QString)), SLOT(StartupSceneTransferFailed(IAssetTransfer*, QString)));
            }
            else
                LogError("TundraLogicModule: Asset transfer initialization failed for startup scene " + file);
        }
        else
        {
            LoadScene(file, false, false);
        }
    }
}

void TundraLogicModule::ReadStartupParameters()
{
    // Check whether server should be auto started.
    const bool autoStartServer = framework_->HasCommandLineParameter("--server");
    const bool hasPortParam = framework_->HasCommandLineParameter("--port");
    ushort autoStartServerPort = cDefaultPort;
    if (hasPortParam && !autoStartServer)
        LogWarning("TundraLogicModule::ReadStartupParameters: --port parameter given, but --server parameter is not present. Server will not be started.");

    // Write default values to config if not present.
    ConfigData configData(ConfigAPI::FILE_FRAMEWORK, ConfigAPI::SECTION_SERVER, "port", cDefaultPort, cDefaultPort);
    if (!framework_->Config()->HasValue(configData))
        framework_->Config()->Set(configData);

    if (autoStartServer)
    {
        // Use parameter port or default to config value
        const QStringList portParam = framework_->CommandLineParameters("--port");
        if (hasPortParam && portParam.empty())
        {
            LogWarning("TundraLogicModule::ReadStartupParameters: --port parameter given without value. Using the default from config.");
            autoStartServerPort = GetFramework()->Config()->Get(configData).toInt();
        }
        else if (!portParam.empty())
        {
            bool ok;
            autoStartServerPort = portParam.first().toUShort(&ok);
            if (!ok)
            {
                LogError("TundraLogicModule::ReadStartupParameters: --port parameter is not a valid unsigned short.");
                GetFramework()->Exit();
            }
        }
    }

    /// @todo Move --netRate handling to SyncManager.
    const bool hasNetRate = framework_->HasCommandLineParameter("--netrate");
    const QStringList rateParam = framework_->CommandLineParameters("--netrate");
    if (hasNetRate && rateParam.empty())
        LogWarning("TundraLogicModule::ReadStartupParameters: --netrate parameter given without value.");
    if (!rateParam.empty())
    {
        bool ok;
        int rate = rateParam.first().toInt(&ok);
        if (ok && rate > 0)
            syncManager_->SetUpdatePeriod(1.f / (float)rate);
        else
            LogError("TundraLogicModule::ReadStartupParameters: --netrate parameter is not a valid integer.");
    }

    if (autoStartServer)
        server_->Start(autoStartServerPort); 
    if (framework_->HasCommandLineParameter("--file")) // Load startup scene here (if we have one)
        LoadStartupScene();

    // Web login handling, if we are on a server the request will be ignored down the chain.
    QStringList cmdLineParams = framework_->CommandLineParameters("--login");
    if (cmdLineParams.size() > 0)
    {
        // Do not use QUrl::TolerantMode as it will do things to percent encoding
        // we don't want a t this point, see http://doc.trolltech.com/4.7/qurl.html#ParsingMode-enum
        QUrl loginUrl(cmdLineParams.first(), QUrl::StrictMode);
        if (loginUrl.isValid())
            client_->Login(loginUrl);
        else
            LogError("TundraLogicModule::ReadStartupParameters: Login URL is not valid after strict parsing: " + cmdLineParams.first());
    }

    QStringList connectArgs = framework_->CommandLineParameters("--connect");
    if (connectArgs.size() > 1) /**< @todo If/when multi-connection support is on place, this should be changed! */
        LogWarning("TundraLogicModule::ReadStartupParameters: multiple --connect parameters given, ignoring all of them!");
    if (connectArgs.size() == 1)
    {
        QStringList params = connectArgs.first().split(';');
        if (params.size() >= 4)
            client_->Login(/*addr*/params[0], /*port*/params[1].toInt(), /*username*/params[3],
            /*optional passwd*/ params.size() >= 5 ? params[4] : "", /*protocol*/params[2]);
        else
            LogError("TundraLogicModule::ReadStartupParameters: Not enought parameters for --connect. Usage '--connect serverIp;port;protocol;name;password'. Password is optional.");
    }
}

void TundraLogicModule::StartupSceneTransfedSucceeded(AssetPtr asset)
{
    QString sceneDiskSource = asset->DiskSource();
    if (sceneDiskSource.isEmpty())
        LogError("Could not resolve disk source for loaded scene file " + asset->Name());
    else // Load the scene
        LoadScene(sceneDiskSource, false, false);
}

void TundraLogicModule::StartupSceneTransferFailed(IAssetTransfer *transfer, QString reason)
{
    LogError("Failed to load startup scene from " + transfer->SourceUrl() + " reason: " + reason);
}

bool TundraLogicModule::SaveScene(QString filename, bool asBinary, bool saveTemporaryEntities, bool saveLocalEntities)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::SaveScene: No active scene found!");
        return false;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::SaveScene: Empty filename given!");
        return false;
    }
    
    if (asBinary)
        return scene->SaveSceneBinary(filename, saveTemporaryEntities, saveLocalEntities);
    else
        return scene->SaveSceneXML(filename, saveTemporaryEntities, saveLocalEntities);
}

bool TundraLogicModule::LoadScene(QString filename, bool clearScene, bool useEntityIDsFromFile)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::LoadScene: No active scene found!");
        return false;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::LoadScene: Empty filename given!");
        return false;
    }

    LogInfo("Loading startup scene from " + filename + " ...");
    kNet::PolledTimer timer;
    bool useBinary = filename.indexOf(".tbin", 0, Qt::CaseInsensitive) != -1;
    QList<Entity *> entities;
    if (useBinary)
        entities = scene->LoadSceneBinary(filename, clearScene, useEntityIDsFromFile, AttributeChange::Default);
    else
        entities = scene->LoadSceneXML(filename, clearScene, useEntityIDsFromFile, AttributeChange::Default);
    LogInfo(QString("Loading of startup scene finished. %1 entities created in %2 msecs.").arg(entities.size()).arg(timer.MSecsElapsed()));
    return entities.size() > 0;
}

bool TundraLogicModule::ImportScene(QString filename, bool clearScene, bool replace)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::ImportScene: No active scene found!");
        return false;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::ImportScene: Empty filename given!");
        return false;
    }

    LogInfo("Importing Ogre .scene " + filename + " ...");

    kNet::PolledTimer timer;
    SceneImporter importer(scene->shared_from_this());
    QList<Entity *> entities = importer.Import(filename, QFileInfo(filename).dir().path(), Transform(),
        "local://", AttributeChange::Default, clearScene, replace);

    LogInfo(QString("Importing of Ogre .scene finished. %1 entities created in %2 msecs.").arg(entities.size()).arg(timer.MSecsElapsed()));
    return entities.size() > 0;
}

bool TundraLogicModule::ImportMesh(QString filename, const float3 &pos, const float3 &rot, const float3 &scale, bool inspect)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::ImportMesh: No active scene found!");
        return false;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::ImportMesh: Empty filename given!");
        return false;
    }

    LogInfo("Importing Ogre .mesh " + filename + " ...");

    SceneImporter importer(scene->shared_from_this());
    EntityPtr entity = importer.ImportMesh(filename, QFileInfo(filename).dir().path(), Transform(pos, rot, scale),
        "", "local://", AttributeChange::Default, inspect);
    if (!entity)
        LogError("TundraLogicModule::ImportMesh: import failed for " + filename + ".");
    return entity != 0;
}

bool TundraLogicModule::IsServer() const
{
    return kristalliModule_->IsServer();
}

}

using namespace TundraLogic;

extern "C"
{
#ifndef ANDROID
DLLEXPORT void TundraPluginMain(Framework *fw)
#else
DEFINE_STATIC_PLUGIN_MAIN(TundraLogicModule)
#endif
{
    Framework::SetInstance(fw); // Inside this DLL, remember the pointer to the global framework object.
    fw->RegisterModule(new KristalliProtocolModule());
    fw->RegisterModule(new TundraLogic::TundraLogicModule());
}
}
