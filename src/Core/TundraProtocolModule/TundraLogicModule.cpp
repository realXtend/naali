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
#include "Scene.h"
#include "AssetAPI.h"
#include "ConsoleAPI.h"
#include "AssetAPI.h"
#include "CoreException.h"
#include "LoggingFunctions.h"

#include "EC_Name.h"
#include "EC_DynamicComponent.h"
#include "EC_InputMapper.h"

#ifdef EC_Highlight_ENABLED
#include "EC_Highlight.h"
#endif

#ifdef EC_HoveringText_ENABLED
#include "EC_HoveringText.h"
#endif

#ifdef EC_Ruler_ENABLED
#include "EC_Ruler.h"
#endif

#ifdef EC_Sound_ENABLED
#include "EC_Sound.h"
#include "EC_SoundListener.h"
#endif

#ifdef EC_Gizmo_ENABLED
#include "EC_Gizmo.h"
#endif

#ifdef EC_PlanarMirror_ENABLED
#include "EC_PlanarMirror.h"
#endif

#ifdef EC_ProximityTrigger_ENABLED
#include "EC_ProximityTrigger.h"
#endif

#ifdef EC_Billboard_ENABLED
#include "EC_Billboard.h"
#endif

#ifdef EC_ParticleSystem_ENABLED
#include "EC_ParticleSystem.h"
#endif

#ifdef EC_TransformGizmo_ENABLED
#include "EC_TransformGizmo.h"
#endif

#ifdef EC_LaserPointer_ENABLED
#include "EC_LaserPointer.h"
#endif

#include "MemoryLeakCheck.h"

namespace TundraLogic
{

static const unsigned short cDefaultPort = 2345;

TundraLogicModule::TundraLogicModule() :
    IModule("TundraLogic"),
    autoStartServer_(false),
    autoStartServerPort_(cDefaultPort),
    kristalliModule_(0)
{
}

TundraLogicModule::~TundraLogicModule()
{
}

void TundraLogicModule::Load()
{
    // Name, DynamicComponent (from Scene) and InputMapper (from Input) are present always.
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_Name>));
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_DynamicComponent>));
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_InputMapper>));

    // External EC's
#ifdef EC_Highlight_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_Highlight>));
#endif
#ifdef EC_HoveringText_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_HoveringText>));
#endif
#ifdef EC_Ruler_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_Ruler>));
#endif
#ifdef EC_SoundRuler_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_SoundRuler>));
#endif
#ifdef EC_ParticleSystem_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_ParticleSystem>));
#endif
#ifdef EC_Sound_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_Sound>));
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_SoundListener>));
#endif
#ifdef EC_Gizmo_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_Gizmo>));
#endif
#ifdef EC_PlanarMirror_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_PlanarMirror>));
#endif
#ifdef EC_Billboard_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_Billboard>));
#endif
#ifdef EC_ProximityTrigger_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_ProximityTrigger>));
#endif
#ifdef EC_TransformGizmo_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_TransformGizmo>));
#endif
#ifdef EC_LaserPointer_ENABLED
    framework_->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_LaserPointer>));
#endif
}

void TundraLogicModule::Initialize()
{
    syncManager_ = boost::shared_ptr<SyncManager>(new SyncManager(this));
    client_ = boost::shared_ptr<Client>(new Client(this));
    server_ = boost::shared_ptr<Server>(new Server(this));
    
    // Expose client and server to everyone
    framework_->RegisterDynamicObject("client", client_.get());
    framework_->RegisterDynamicObject("server", server_.get());

    // Expose SyncManager only on the server side for scripting
    if (server_->IsAboutToStart())
        framework_->RegisterDynamicObject("syncmanager", syncManager_.get());

    framework_->Console()->RegisterCommand("startserver", "Starts a server. Usage: startserver(port,protocol)",
        server_.get(), SLOT(Start(unsigned short,QString)));

    framework_->Console()->RegisterCommand("stopserver", "Stops the server", server_.get(), SLOT(Stop()));

    framework_->Console()->RegisterCommand("connect",
        "Connects to a server. Usage: connect(address,port,username,password,protocol)",
        client_.get(), SLOT(Login(const QString &, unsigned short, const QString &, const QString&, const QString &)));

    framework_->Console()->RegisterCommand("disconnect", "Disconnects from a server.", client_.get(), SLOT(Logout()));

    framework_->Console()->RegisterCommand("savescene",
        "Saves scene into XML or binary. Usage: savescene(filename,asBinary=false,saveTemporaryEntities=false,saveLocalEntities=true)",
        this, SLOT(SaveScene(QString, bool, bool, bool)), SLOT(SaveScene(QString)));

    framework_->Console()->RegisterCommand("loadscene",
        "Loads scene from XML or binary. Usage: loadscene(filename,clearScene=true,useEntityIDsFromFile=true)",
        this, SLOT(LoadScene(QString, bool, bool)));

    framework_->Console()->RegisterCommand("importscene",
        "Loads scene from a dotscene file. Optionally clears the existing scene."
        "Replace-mode can be optionally disabled. Usage: importscene(filename,clearScene=false,replace=true)",
        this, SLOT(ImportScene(QString, bool, bool)), SLOT(ImportScene(QString)));

    framework_->Console()->RegisterCommand("importmesh",
        "Imports a single mesh as a new entity. Position, rotation, and scale can be specified optionally."
        "Usage: importmesh(filename, pos = 0 0 0, rot = 0 0 0, scale = 1 1 1, inspectForMaterialsAndSkeleton=true)",
        this, SLOT(ImportMesh(QString, const float3 &, const float3 &, const float3 &, bool)), SLOT(ImportMesh(QString)));

    // Take a pointer to KristalliProtocolModule so that we don't have to take/check it every time
    kristalliModule_ = framework_->GetModule<KristalliProtocolModule>();
    if (!kristalliModule_)
        throw Exception("Fatal: could not get KristalliProtocolModule");

    ConfigData configData(ConfigAPI::FILE_FRAMEWORK, ConfigAPI::SECTION_SERVER, "port", cDefaultPort, cDefaultPort);
    // Write default values to config if not present.
    if (!framework_->Config()->HasValue(configData))
        framework_->Config()->Set(configData);

    // Check whether server should be auto started.
    if (framework_->HasCommandLineParameter("--server"))
    {
        autoStartServer_ = true;
        // Use parameter port or default to config value
        QStringList portParam = framework_->CommandLineParameters("--port");
        if (portParam.size() > 0)
        {
            bool ok;
            unsigned short port = portParam.first().toUShort(&ok);
            if (ok)
            {
                autoStartServerPort_ = port;
            }
            else
            {
                LogError("--port parameter is not a valid unsigned short.");
                GetFramework()->Exit();
            }
        }
        else
            autoStartServerPort_ = GetFramework()->Config()->Get(configData).toInt();
    }
    
    if (framework_->HasCommandLineParameter("--netrate"))
    {
        QStringList rateParam = framework_->CommandLineParameters("--netrate");
        if (rateParam.size() > 0)
        {
            bool ok;
            int rate = rateParam.first().toInt(&ok);
            if (ok && rate > 0)
                syncManager_->SetUpdatePeriod(1.f / (float)rate);
            else
                LogError("--netrate parameter is not a valid integer.");
        }
    }
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
    ///\todo Remove this hack and find a better solution
    static bool checkDefaultServerStart = true;
    if (checkDefaultServerStart)
    {
        if (autoStartServer_)
            server_->Start(autoStartServerPort_); 

        // Load startup scene here (if we have one)
        LoadStartupScene();

        checkDefaultServerStart = false;
    }
    ///\todo Remove this hack and find a better solution
    static bool checkLoginStart = true;
    if (checkLoginStart)
    {
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
                LogError("Login URL is not valid after strict parsing: " + cmdLineParams.first());
        }

        checkLoginStart = false;
    }
    ///\todo Remove this hack and find a better solution
    static bool checkConnectStart = true;
    if (checkConnectStart)
    {
        if (framework_->CommandLineParameters("--connect").size() == 1)
        {
            QStringList params = framework_->CommandLineParameters("--connect").first().split(';');
            if (params.size() >= 4)
                client_->Login(/*addr*/params[0], /*port*/params[1].toInt(), /*username*/params[3],
                    /*optional passwd*/ params.size() >= 5 ? params[4] : "", /*protocol*/params[2]);
            else
                LogError("Not enought parameters for --connect. Usage '--connect serverIp;port;protocol;name;password'. Password is optional.");
        }
        checkConnectStart = false;
    }

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

    foreach(const QString &file, files)
    {
        QString startupScene;
        // If the file parameter uses the full storage specifier format, parse the "src" keyvalue
        if (file.indexOf(';') != -1 || file.indexOf('=') != -1)
        {
            QMap<QString, QString> keyValues = AssetAPI::ParseAssetStorageString(file);
            startupScene = keyValues["src"];
        }
        else
            startupScene = file;
        
        // At this point, if we have a LocalAssetProvider, it has already also parsed the --file command line option
        // and added the appropriate path as a local asset storage. Here we assume that is the case, so that the
        // scene we now load will be able to refer to local:// assets in its subfolders.
        AssetAPI::AssetRefType sceneRefType = AssetAPI::ParseAssetRef(startupScene);
        if (sceneRefType != AssetAPI::AssetRefLocalPath && sceneRefType != AssetAPI::AssetRefRelativePath)
        {
            AssetTransferPtr sceneTransfer = framework_->Asset()->RequestAsset(startupScene);
            if (!sceneTransfer.get())
            {
                LogError("Asset transfer initialization failed for scene file " + startupScene + " failed");
                return;
            }
            connect(sceneTransfer.get(), SIGNAL(Succeeded(AssetPtr)), SLOT(StartupSceneLoaded(AssetPtr)));
            connect(sceneTransfer.get(), SIGNAL(Failed(IAssetTransfer*, QString)), SLOT(StartupSceneTransferFailed(IAssetTransfer*, QString)));
            LogInfo("[TundraLogic] Loading startup scene from " + startupScene);
        }
        else
        {
            LogInfo("[TundraLogic] Loading startup scene from " + startupScene);
            bool useBinary = startupScene.indexOf(".tbin") != -1;
            if (!useBinary)
                scene->LoadSceneXML(startupScene, false/*clearScene*/, false/*useEntityIDsFromFile*/, AttributeChange::Default);
            else
                scene->LoadSceneBinary(startupScene, false/*clearScene*/, false/*useEntityIDsFromFile*/, AttributeChange::Default);
        }
    }
}

void TundraLogicModule::StartupSceneLoaded(AssetPtr asset)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
        return;

    QString sceneDiskSource = asset->DiskSource();
    if (!sceneDiskSource.isEmpty())
    {
        bool useBinary = sceneDiskSource.endsWith(".tbin");
        if (!useBinary)
            scene->LoadSceneXML(sceneDiskSource, true/*clearScene*/, false/*useEntityIDsFromFile*/, AttributeChange::Default);
        else
            scene->LoadSceneBinary(sceneDiskSource, true/*clearScene*/, false/*useEntityIDsFromFile*/, AttributeChange::Default);
    }
    else
        LogError("Could not resolve disk source for loaded scene file " + asset->Name());
}

void TundraLogicModule::StartupSceneTransferFailed(IAssetTransfer *transfer, QString reason)
{
    LogError("Failed to load startup scene from " + transfer->SourceUrl() + " reason: " + reason);
}

void TundraLogicModule::SaveScene(QString filename, bool asBinary, bool saveTemporaryEntities, bool saveLocalEntities)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::SaveScene: No active scene found!");
        return;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::SaveScene: Empty filename given!");
        return;
    }
    
    bool success;
    if (!asBinary)
        success = scene->SaveSceneXML(filename, saveTemporaryEntities, saveLocalEntities);
    else
        success = scene->SaveSceneBinary(filename, saveTemporaryEntities, saveLocalEntities);

    if (!success)
        LogError("SaveScene failed!");
}

void TundraLogicModule::LoadScene(QString filename, bool clearScene, bool useEntityIDsFromFile)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::LoadScene: No active scene found!");
        return;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::LoadScene: Empty filename given!");
        return;
    }
    
    bool useBinary = false;
    if (filename.contains(".tbin", Qt::CaseInsensitive))
        useBinary = true;

    QList<Entity *> entities;
    if (!useBinary)
        entities = scene->LoadSceneXML(filename, clearScene, useEntityIDsFromFile, AttributeChange::Default);
    else
        entities = scene->LoadSceneBinary(filename, clearScene, useEntityIDsFromFile, AttributeChange::Default);

    LogInfo("TundraLogicModule::LoadScene: Loaded " + QString::number(entities.size()) + " entities.");
}

void TundraLogicModule::ImportScene(QString filename, bool clearScene, bool replace)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::ImportScene: No active scene found!");
        return;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::ImportScene: Empty filename given!");
        return;
    }

    QString path = QFileInfo(filename).dir().path();

    SceneImporter importer(scene->shared_from_this());
    QList<Entity *> entities = importer.Import(filename, path, Transform(), "local://", AttributeChange::Default, clearScene, replace);

    LogInfo("TundraLogicModule::ImportScene: Imported " + QString::number(entities.size()) + " entities.");
}

void TundraLogicModule::ImportMesh(QString filename, const float3 &pos, const float3 &rot, const float3 &scale, bool inspect)
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::ImportMesh: No active scene found!");
        return;
    }
    filename = filename.trimmed();
    if (filename.isEmpty())
    {
        LogError("TundraLogicModule::ImportMesh: Empty filename given!");
        return;
    }

    QString path = QFileInfo(filename).dir().path();

    SceneImporter importer(scene->shared_from_this());
    EntityPtr entity = importer.ImportMesh(filename, path, Transform(pos, rot, scale), "", "local://", AttributeChange::Default, inspect);
    if (!entity)
        LogError("TundraLogicModule::ImportMesh: import failed for " + filename + ".");
}

bool TundraLogicModule::IsServer() const
{
    return kristalliModule_->IsServer();
}

}

using namespace TundraLogic;

extern "C"
{
DLLEXPORT void TundraPluginMain(Framework *fw)
{
    Framework::SetInstance(fw); // Inside this DLL, remember the pointer to the global framework object.
    fw->RegisterModule(new KristalliProtocolModule());
    fw->RegisterModule(new TundraLogic::TundraLogicModule());
}
}
