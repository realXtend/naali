/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   EnvironmentModule.cpp
    @brief  Environment plugin is be responsible of visual environment features like terrain & water. */

#include "DebugOperatorNew.h"

#include "EC_WaterPlane.h"
#include "EC_Terrain.h"

#include "Framework.h"
#include "SceneAPI.h"
#include "AssetAPI.h"
#include "GenericAssetFactory.h"
#include "IComponentFactory.h"
#include "StaticPluginRegistry.h"

#include "MemoryLeakCheck.h"

extern "C"
{

#ifndef ANDROID
DLLEXPORT void TundraPluginMain(Framework *fw)
#else
DEFINE_STATIC_PLUGIN_MAIN(EnvironmentModule)
#endif
{
    Framework::SetInstance(fw); // Inside this DLL, remember the pointer to the global framework object.
    fw->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_Terrain>));
    fw->Scene()->RegisterComponentFactory(MAKE_SHARED(GenericComponentFactory<EC_WaterPlane>));
    // Create an asset type factory for Terrain assets. The terrain assets are handled as binary blobs - the EC_Terrain parses it when showing the asset.
    fw->Asset()->RegisterAssetTypeFactory(MAKE_SHARED(BinaryAssetFactory, "Terrain", ".ntf"));
}

}
