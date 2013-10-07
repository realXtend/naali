// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "EC_OgreCompositor.h"
#include "Renderer.h"
#include "OgreRenderingModule.h"
#include "OgreCompositionHandler.h"

#include "Framework.h"
#include "FrameAPI.h"
#include "LoggingFunctions.h"

#include "MemoryLeakCheck.h"

EC_OgreCompositor::EC_OgreCompositor(Scene* scene) :
    IComponent(scene),
    INIT_ATTRIBUTE_VALUE(enabled, "Enabled", true),
    INIT_ATTRIBUTE_VALUE(compositorName, "Compositor ref", ""), ///<\todo Rename to "Compositor" or "Compositor name".
    INIT_ATTRIBUTE_VALUE(priority, "Priority", -1),
    INIT_ATTRIBUTE(parameters, "Parameters"),
    previousPriority(-1),
    compositionHandler(0)
{
    connect(this, SIGNAL(ParentEntitySet()), SLOT(OnParentEntitySet()));
}

EC_OgreCompositor::~EC_OgreCompositor()
{
    if (compositionHandler && !previousRef.isEmpty())
        compositionHandler->RemoveCompositorFromViewport(previousRef.toStdString());
}

void EC_OgreCompositor::OnParentEntitySet()
{
    if (!framework)
        return;

    OgreRenderingModule *owner = framework->Module<OgreRenderingModule>();
    compositionHandler = owner != 0 ?owner->Renderer()->CompositionHandler() : 0;
    if (!compositionHandler)
        return;

    // Ogre sucks. Enable a timed one-time refresh to overcome issue with black screen.
    framework->Frame()->DelayedExecute(0.01f, this, SLOT(OneTimeRefresh()));
}

QStringList EC_OgreCompositor::AvailableCompositors() const
{
    if (compositionHandler)
        return compositionHandler->AvailableCompositors();
    else
        return QStringList();
}

QStringList EC_OgreCompositor::ApplicableParameters() const
{
    if (compositionHandler)
        return compositionHandler->CompositorParameters(compositorName.Get().toStdString());
    else
        return QStringList();
}

void EC_OgreCompositor::AttributesChanged()
{
    if (enabled.ValueChanged())
    {
        OneTimeRefresh();
    }
    if (compositorName.ValueChanged() || priority.ValueChanged())
    {
        UpdateCompositor(compositorName.Get());
    }
    if (parameters.ValueChanged())
    {
        UpdateCompositorParams(compositorName.Get());
    }
}

void EC_OgreCompositor::UpdateCompositor(const QString &compositor)
{
    if (ViewEnabled() && enabled.Get())
    {
        if (previousRef != compositorName.Get() || previousPriority != priority.Get())
        {
            if (!previousRef.isEmpty())
                compositionHandler->RemoveCompositorFromViewport(previousRef.toStdString());

            if (!compositorName.Get().isEmpty())
            {
                if (priority.Get() == -1)
                    compositionHandler->AddCompositorForViewport(compositor.toStdString());
                else
                    compositionHandler->AddCompositorForViewportPriority(compositor.toStdString(), priority.Get());
            }
            
            previousRef = compositor;
            previousPriority = priority.Get();
        }
    }
}

void EC_OgreCompositor::UpdateCompositorParams(const QString &compositor)
{
    if (ViewEnabled() && enabled.Get())
    {
        QList<std::pair<std::string, Ogre::Vector4> > programParams;
        foreach(QVariant keyvalue, parameters.Get())
        {
            QStringList sepParams = keyvalue.toString().split('=');
            if (sepParams.size() > 1)
            {
                Ogre::Vector4 value(0, 0, 0, 0);
                QStringList valueList = sepParams[1].split(" ", QString::SkipEmptyParts);
                if (valueList.size() > 0)
                    value.x = valueList[0].toDouble();
                if (valueList.size() > 1)
                    value.y = valueList[1].toDouble();
                if (valueList.size() > 2)
                    value.z = valueList[2].toDouble();
                if (valueList.size() > 3)
                    value.w = valueList[3].toDouble();
                std::string name = sepParams[0].toStdString();

                programParams.push_back(std::make_pair(name, value));
            }
        }
        compositionHandler->SetCompositorParameter(compositorName.Get().toStdString(), programParams);
        compositionHandler->SetCompositorEnabled(compositorName.Get().toStdString(), false);
        compositionHandler->SetCompositorEnabled(compositorName.Get().toStdString(), true);
    }
}

void EC_OgreCompositor::OneTimeRefresh()
{
    if (!compositorName.Get().isEmpty())
    {
        UpdateCompositor(compositorName.Get());
        UpdateCompositorParams(compositorName.Get());
        compositionHandler->SetCompositorEnabled(compositorName.Get().toStdString(), enabled.Get());
    }
}
