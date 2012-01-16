// For conditions of distribution and use, see copyright notice in LICENSE

#include "Framework.h"
#include "SceneAPI.h"
#include "IComponentFactory.h"

#include "SceneWidgetComponents.h"
#include "EC_WidgetCanvas.h"
#include "EC_WebView.h"
#include "EC_SlideShow.h"
#include "EC_WidgetBillboard.h"

extern "C"
{
    DLLEXPORT void TundraPluginMain(Framework *fw)
    {
        Framework::SetInstance(fw);

        // Register module
        IModule *module = new SceneWidgetComponents();
        fw->RegisterModule(module);

        // Register component factories
        fw->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_WidgetCanvas>));
        fw->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_WebView>));
        fw->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_SlideShow>));
        fw->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EC_WidgetBillboard>));
    }
}
