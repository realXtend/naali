// For conditions of distribution and use, see copyright notice in license.txt

#include "EC_WidgetBillboard.h"

#include "Framework.h"
#include "Entity.h"
#include "LoggingFunctions.h"

#include "EC_Billboard.h"
#include "EC_Camera.h"

#include "UiAPI.h"
#include "UiGraphicsView.h"

#include "AssetAPI.h"
#include "IAssetTransfer.h"
#include "InputAPI.h"
#include "InputContext.h"

#include "OgreRenderingModule.h"
#include "OgreTextureUnitState.h"
#include "OgreMaterialAsset.h"
#include "OgreBillboard.h"
#include "OgreBillboardSet.h"
#include "OgreCamera.h"
#include "OgreVector3.h"
#include "OgreMatrix4.h"
#include "TextureAsset.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QApplication>
#include <QPainter>
#include <QUuid>
#include <QGraphicsSceneEvent>
#include <QChildEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>

EC_WidgetBillboard::EC_WidgetBillboard(Scene* scene) :
    IComponent(scene),
    uiRef(this, "UI ref", AssetReference("", "QtUiFile")),
    visible(this, "Visible", true),
    acceptInput(this, "Accept Input", true),
    position(this, "Position", float3(0.0f, 0.0f, 0.0f)),
    ppm(this, "Pixels per meter", 300),
    cloneMaterialRef_("Ogre Media:LitTextured.material"),
    refListener_(0),
    widgetContainer_(0),
    rendering_(false),
    leftPressReleased_(true),
    trackingMouseMove_(false)
{
    using namespace OgreRenderer;

    if (!ViewEnabled() || framework->IsHeadless())
        return;

    OgreRenderingModule *renderModule = framework->GetModule<OgreRenderer::OgreRenderingModule>();
    if (renderModule)
    {
        renderer_ = renderModule->GetRenderer();
        if (!renderer_.get())
        {
            LogError("EC_WidgetBillboard: Failed to get Renderer from OgreRenderingModule!");
            return;
        }

        // Create manual material and texture to the asset system
        uniqueMaterialName_ = QString::fromStdString(renderer_->GetUniqueObjectName("EC_WidgetBillboard")) + ".material";
        uniqueTextureName_ = QString::fromStdString(renderer_->GetUniqueObjectName("EC_WidgetBillboard")) + ".png";

        materialAsset_ = framework->Asset()->CreateNewAsset("OgreMaterial", uniqueMaterialName_);
        textureAsset_ = framework->Asset()->CreateNewAsset("Texture", uniqueTextureName_);

        // Request the clone asset
        AssetTransferPtr transfer = framework->Asset()->RequestAsset(cloneMaterialRef_);
        connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), SLOT(PrepareComponent()));
        connect(transfer.get(), SIGNAL(Failed(IAssetTransfer*, QString)), SLOT(PrepareComponent()));

        // Connect component signals
        connect(this, SIGNAL(ParentEntitySet()), SLOT(PrepareComponent()), Qt::UniqueConnection);
        connect(this, SIGNAL(AttributeChanged(IAttribute*, AttributeChange::Type)), 
                SLOT(OnAttributeUpdated(IAttribute*, AttributeChange::Type)), Qt::UniqueConnection);

        // Asset reference listener
        refListener_ = new AssetRefListener();
        connect(refListener_, SIGNAL(Loaded(AssetPtr)), SLOT(OnUiAssetLoaded(AssetPtr)));
        connect(refListener_, SIGNAL(TransferFailed(IAssetTransfer*, QString)), SLOT(OnUiAssetLoadFailed(IAssetTransfer*, QString)));

        // RenderInternal timer
        renderTimer_.setSingleShot(true);
        connect(&renderTimer_, SIGNAL(timeout()), SLOT(RenderInternal()));

        // Init widget container and scene
        widgetContainer_ = new QGraphicsView();
        widgetContainer_->setAttribute(Qt::WA_DontShowOnScreen, true);
        widgetContainer_->setMouseTracking(true);
        widgetContainer_->installEventFilter(this);

        QGraphicsScene *scene = new QGraphicsScene(widgetContainer_);
        widgetContainer_->setScene(scene);

        connect(scene, SIGNAL(changed(const QList<QRectF>&)), SLOT(Render()));
        connect(framework->Ui()->GraphicsView(), SIGNAL(WindowResized(int, int)), SLOT(Render())); 
    }
    else
        LogError("EC_WidgetBillboard: Failed to get OgreRenderingModule!");
}

EC_WidgetBillboard::~EC_WidgetBillboard()
{
    // Stop rendering
    if (renderTimer_.isActive())
        renderTimer_.stop();

    // Release widget
    if (widget_)
    {
        if (widgetContainer_ && widgetContainer_->scene())
            widgetContainer_->scene()->removeItem(widget_->graphicsProxyWidget());
        SAFE_DELETE(widget_);
    }

    if (widgetContainer_ && widgetContainer_->scene())
    {
        widgetContainer_->scene()->clear();
        SAFE_DELETE(widgetContainer_);
    }

    // Release in code created assets
    if (materialAsset_.get() && framework->Asset()->GetAsset(materialAsset_->Name()).get())
        framework->Asset()->ForgetAsset(materialAsset_, false);
    materialAsset_.reset();
    if (textureAsset_.get() && framework->Asset()->GetAsset(textureAsset_->Name()).get())
        framework->Asset()->ForgetAsset(textureAsset_, false);
    textureAsset_.reset();
}

// Public slots

void EC_WidgetBillboard::Render()
{
    if (rendering_)
        return;
    if (!getvisible())
        return;

    if (!renderTimer_.isActive())
        renderTimer_.start(10);
}

// Private slots

void EC_WidgetBillboard::RenderInternal()
{
    if (!getvisible())
        return;
    if (!IsPrepared())
        return;
    if (!widget_)
        return;
    if (renderTimer_.isActive())
        return;

    OgreMaterialAsset *ogreMaterialAsset = dynamic_cast<OgreMaterialAsset*>(materialAsset_.get());
    TextureAsset *ogreTextureAsset = dynamic_cast<TextureAsset*>(textureAsset_.get());
    if (!ogreMaterialAsset || !ogreTextureAsset)
        return;

    // Protection against malformed widgets.
    if (widget_->width() <= 0 || widget_->height() <= 0)
        return;

    rendering_ = true;

    // Prepare render buffer.
    if (renderBuffer_.size() != widget_->size())
    {
        AttributeChanged(&ppm, AttributeChange::LocalOnly);
        renderBuffer_ = QImage(widget_->size(), QImage::Format_ARGB32_Premultiplied);
    }
    renderBuffer_.fill(Qt::transparent);
    
    // Pull widget state to render buffer
    widget_->render(&renderBuffer_, QPoint(), QRegion(), QWidget::DrawChildren);

    // Update ogre texture
    ogreTextureAsset->SetContents(renderBuffer_.width(), renderBuffer_.height(), renderBuffer_.bits(), renderBuffer_.byteCount(), 
                                  Ogre::PF_A8R8G8B8, false, true, false);

    // Set texture to material if needed
    Ogre::TextureUnitState *texUnit = ogreMaterialAsset->GetTextureUnit(0, 0, 0);
    if (texUnit && !ogreTextureAsset->ogreTexture.isNull() && texUnit->getTextureName() != ogreTextureAsset->ogreTexture->getName())
        ogreMaterialAsset->SetTexture(0, 0, 0, ogreTextureAsset->Name());

    // Show billboard if not yet in sync.
    // Even if billboard is created and visible is true
    // we want to show the billboard here as it not has some content.
    EC_Billboard *bb = GetBillboardComponent();
    if (bb->getshow() != getvisible())
        bb->setshow(getvisible());

    rendering_ = false;
}

bool EC_WidgetBillboard::IsPrepared()
{
    if (!GetBillboardComponent())
        return false;
    return true;
}

EC_Billboard *EC_WidgetBillboard::GetBillboardComponent()
{
    if (!ParentEntity())
        return 0;
    if (billboardCompName_.isEmpty())
        return 0;
    return dynamic_cast<EC_Billboard*>(ParentEntity()->GetComponent(EC_Billboard::TypeNameStatic(), billboardCompName_).get());
}

void EC_WidgetBillboard::PrepareComponent()
{
    // Wait until the needed asset have been loaded.
    AssetPtr cloneMaterialAsset = framework->Asset()->GetAsset(cloneMaterialRef_);
    if (!cloneMaterialAsset || !cloneMaterialAsset->IsLoaded())
        return;

    // Hook to parent entity signals
    connect(ParentEntity(), SIGNAL(ComponentRemoved(IComponent*, AttributeChange::Type)), SLOT(ComponentRemoved(IComponent*, AttributeChange::Type)), Qt::UniqueConnection);
    
    // Create billboard if needed
    EC_Billboard *bb = GetBillboardComponent();
    if (!bb)
    {
        billboardCompName_ = "SceneWidget-Billboard-" + QUuid::createUuid().toString().replace("{", "").replace("}", "");

        bb = dynamic_cast<EC_Billboard*>(ParentEntity()->GetOrCreateComponent(
            EC_Billboard::TypeNameStatic(), billboardCompName_, AttributeChange::LocalOnly, false).get());
        if (!bb)
        {
            LogError("EC_WidgetBillboard: Failed to create needed EC_Billboard to parent entity!");
            return;
        }
    }
    bb->SetTemporary(true);
    bb->show.Set(false, AttributeChange::LocalOnly);
    bb->position.Set(getposition(), AttributeChange::LocalOnly);

    // Get involved assets
    OgreMaterialAsset *ogreMaterialAsset = dynamic_cast<OgreMaterialAsset*>(materialAsset_.get());

    // Clone material, set our texture to it and set material for our EC_Billboard.
    if (ogreMaterialAsset)
    {
        if (cloneMaterialAsset)
        {
            ogreMaterialAsset->CopyContent(cloneMaterialAsset);
            ogreMaterialAsset->RemoveTechnique(0);
            ogreMaterialAsset->SetSceneBlend(0, 0, 0 /*SBT_TRANSPARENT_ALPHA*/);
            ogreMaterialAsset->SetDepthWrite(0, 0, false);
            ogreMaterialAsset->SetEmissiveColor(0,0, Color(1,1,1,1));
            bb->materialRef.Set(AssetReference(ogreMaterialAsset->Name()), AttributeChange::LocalOnly);
        }
        else
            LogError("EC_WidgetBillboard: Could not get '" + cloneMaterialRef_ + "' for cloning!");
    }
    else
        LogError("EC_WidgetBillboard: Created material assets are null!");

    RenderInternal();
}

void EC_WidgetBillboard::OnAttributeUpdated(IAttribute* attribute, AttributeChange::Type change)
{
    EC_Billboard *myBillboard = GetBillboardComponent();

    // Fetch ui asset and hide EC_Billboard if created.
    if (attribute == &uiRef)
    {
        if (!getuiRef().ref.isEmpty())
            refListener_->HandleAssetRefChange(attribute, "QtUiFile");
        else
        {
            // Ref was reseted: Destroy widget and hide target billboard.
            if (widget_)
            {
                if (widgetContainer_)
                {
                    widgetContainer_->scene()->removeItem(widget_->graphicsProxyWidget());
                    widgetContainer_->scene()->clear();
                }
                widget_->removeEventFilter(this);
                SAFE_DELETE(widget_);
            }
            if (myBillboard)
                myBillboard->show.Set(false, AttributeChange::LocalOnly);
        }
    }

    // Below things that are reflected to EC_Billboard
    if (myBillboard)
    {
        if (attribute == &visible && widget_)
            myBillboard->show.Set(getvisible(), AttributeChange::LocalOnly);
        else if (attribute == &position)
            myBillboard->position.Set(getposition(), AttributeChange::LocalOnly);
        else if (attribute == &ppm && widget_)
        {
            QSize wsize = widget_->size();
            float dx = (float)wsize.width() / (float)getppm();
            float dy = (float)wsize.height() / (float)getppm();
            myBillboard->width.Set(dx, AttributeChange::LocalOnly);
            myBillboard->height.Set(dy, AttributeChange::LocalOnly);
        }
    }
}

void EC_WidgetBillboard::ComponentRemoved(IComponent *component, AttributeChange::Type change)
{
    if (component != this)
        return;

    // If this component is being removed. Remove our unique local EC_Billboard
    EC_Billboard *myBillboard = GetBillboardComponent();
    if (ParentEntity() && myBillboard)
        ParentEntity()->RemoveComponent(EC_Billboard::TypeNameStatic(), billboardCompName_, AttributeChange::LocalOnly);

}

void EC_WidgetBillboard::OnUiAssetLoaded(AssetPtr asset)
{
    // Clean out the old widget
    if (widget_)
    {
        if (widgetContainer_)
        {
            widgetContainer_->scene()->removeItem(widget_->graphicsProxyWidget());
            widgetContainer_->scene()->clear();
        }
        widget_->removeEventFilter(this);
        SAFE_DELETE(widget_);
    }

    // Instantiate new widget and emit the ready signal for external hooks to its signals.
    widget_ = framework->Ui()->LoadFromFile(asset->Name(), false);
    if (!widget_)
    {
        LogError("EC_WidgetBillboard: Failed to instantiate source widget from " + asset->Name());
        return;
    }
    emit WidgetReady(widget_.data());

    // Do not render this widget on the viewable desktop
    widget_->setAttribute(Qt::WA_DontShowOnScreen, true);
    widget_->setMouseTracking(true);
    widget_->installEventFilter(this);

    // Insert into container
    widgetContainer_->resize(widget_->size());
    widgetContainer_->setSceneRect(0, 0, widget_->width(), widget_->height());
    QGraphicsProxyWidget *proxy = widgetContainer_->scene()->addWidget(widget_, Qt::Widget);
    proxy->setPos(0, 0);

    RenderInternal();
}

void EC_WidgetBillboard::OnUiAssetLoadFailed(IAssetTransfer *transfer, QString reason)
{
    EC_Billboard *myBillboard = GetBillboardComponent();
    if (myBillboard)
        myBillboard->show.Set(false, AttributeChange::LocalOnly);
}

bool EC_WidgetBillboard::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::UpdateRequest || e->type() == QEvent::Show || e->type() == QEvent::Hide || e->type() == QEvent::Move ||
        dynamic_cast<QPaintEvent*>(e) || dynamic_cast<QChildEvent*>(e) || dynamic_cast<QMouseEvent*>(e) || 
        dynamic_cast<QGraphicsSceneEvent*>(e) || dynamic_cast<QHoverEvent*>(e))
    {
        Render();
    }

    // Keep view/scene in sync with widget size
    if (obj == widget_ && widgetContainer_ && e->type() == QEvent::Resize)
    {
        QResizeEvent *rEvent = dynamic_cast<QResizeEvent*>(e);
        if (rEvent)
        {
            widgetContainer_->resize(rEvent->size());
            widgetContainer_->setSceneRect(0, 0, rEvent->size().width(), rEvent->size().height());
            Render();
        }
    }

    return QObject::eventFilter(obj, e);
}

void EC_WidgetBillboard::OnMouseEvent(MouseEvent *mEvent)
{
    if (!getacceptInput() || !getvisible())
        return;
    if (!IsPrepared() || !widget_)
        return;

    MouseEvent::EventType et = mEvent->eventType;

    // Filter out not wanted events here so we don't 
    // do the potentially costly raycast unnecessarily.
    if (mEvent->handled || mEvent->IsRightButtonDown())
        return;
    else if (et == MouseEvent::MouseScroll)
        return;

    bool hit;
    float2 uv;
    float distance;

    RaycastBillboard(mEvent->x, mEvent->y, hit, uv, distance);
    if (!hit)
    {
        CheckMouseState();
        return;
    }

    // Detect type and send the Qt event
    QPoint widgetPos((int)widget_->width() * uv.x, (int)widget_->height() * uv.y);
    QEvent::Type type = (et == MouseEvent::MousePressed ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease);
    if (et == MouseEvent::MouseMove)
    {
        if (!mEvent->IsLeftButtonDown())
        {
            SendWidgetMouseEvent(widgetPos, QEvent::MouseMove, Qt::NoButton);
            Render();
        }
        return;
    }
    else
        mEvent->handled = SendWidgetMouseEvent(widgetPos, type, (mEvent->button == MouseEvent::LeftButton ? Qt::LeftButton : Qt::RightButton));

    // For mouse release we need to send out a move event
    // so the ui gets updated correctly.
    if (type == QEvent::MouseButtonRelease)
    {
        SendWidgetMouseEvent(widgetPos, QEvent::MouseMove, Qt::NoButton);
        Render();
        leftPressReleased_ = true;
    }
    else
        leftPressReleased_ = false;
}

void EC_WidgetBillboard::RaycastBillboard(int mouseX, int mouseY, bool &hit, float2 &uv, float &distance)
{
    hit = false;
    
    if (!getacceptInput() || !getvisible())
        return;
    if (!IsPrepared() || !widget_ || !renderer_.get())
        return;

    // Gather needed ptrs
    Ogre::Camera *camera = renderer_->MainOgreCamera();
    if (!camera)
        return;
    RaycastResult *raycast = renderer_->Raycast(mouseX, mouseY);
    if (!raycast)
        return;
    EC_Billboard *myBillboard = GetBillboardComponent();
    if (!myBillboard)
        return;
    Ogre::Billboard *bb = myBillboard->GetBillBoard();
    Ogre::BillboardSet *bbSet = myBillboard->GetBillBoardSet();
    if (!bb || !bbSet)
        return;

    // Expand to range -1 - +1 and divert y because 
    // after view/projection transforms, y increases upwards
    QSize viewSize = framework->Ui()->GraphicsView()->size();
    float screenX = (float)mouseX / viewSize.width();
    float screenY = (float)mouseY / viewSize.height();
    screenX = (screenX * 2) - 1;
    screenY = (screenY * 2) - 1;
    screenY = -screenY;

    Ogre::Vector3 camPos = camera->getDerivedPosition();    

    Ogre::Matrix4 worldMatrix;
    bbSet->getWorldTransforms(&worldMatrix);

    QSizeF srcSize(myBillboard->getwidth(), myBillboard->getheight());
    
    Ogre::Vector3 worldPos;
    Ogre::Vector3 srcPos = bb->getPosition();
    srcPos = worldMatrix * srcPos;
    worldPos = srcPos;
    srcPos = camera->getViewMatrix() * srcPos;

    QRectF bbRect(srcPos.x - srcSize.width() * 0.5,
                  srcPos.y - srcSize.height() * 0.5,
                  srcSize.width(), srcSize.height());

    Ogre::Vector3 min(bbRect.topLeft().x(), bbRect.topLeft().y(), srcPos.z);
    Ogre::Vector3 max(bbRect.bottomRight().x(), bbRect.bottomRight().y(), srcPos.z);

    min = camera->getProjectionMatrix() * min;
    max = camera->getProjectionMatrix() * max;

    QRectF screenSpaceRect(min.x, min.y, max.x-min.x, max.y-min.y);
    if (screenSpaceRect.contains(screenX, screenY))
    {
        // Check if hit entity is closer than our billboard
        if (raycast->entity)
        {
            Ogre::Vector3 hitEntityPos(raycast->pos.x, raycast->pos.y, raycast->pos.z);
            if (Ogre::Vector3(hitEntityPos - camPos).squaredLength() < 
                Ogre::Vector3(worldPos - camPos).squaredLength())
                return;
        }

        screenX -= screenSpaceRect.left();
        screenX /= screenSpaceRect.width();
        screenY -= screenSpaceRect.top();
        screenY /= screenSpaceRect.height();

        // We have a hit, fill out the data
        hit = true;
        uv = float2(screenX, 1-screenY);
        distance = Ogre::Vector3(worldPos - camPos).squaredLength();

        // Don't register a hit for transparent widget parts.
        if (!renderBuffer_.isNull() && (renderBuffer_.pixel((int)widget_->width() * uv.x, (int)widget_->height() * uv.y) & 0xFF000000) == 0x00000000)
            hit = false;
    }
}

bool EC_WidgetBillboard::SendWidgetMouseEvent(QPoint pos, QEvent::Type type, Qt::MouseButton button, Qt::KeyboardModifier modifier)
{
    if (!widget_ || !widgetContainer_ || !widgetContainer_->viewport())
        return false;

    // External hooks for QWidgets that Qt does not provide clicked etc signals.
    QWidget *atPosWidget = widget_->childAt(pos);
    if (atPosWidget)
        emit WidgetMouseEvent(atPosWidget, type, button);
    if (type == QEvent::MouseMove && trackingMouseMove_ == false)
        trackingMouseMove_ = true;

    QMouseEvent qtEvent = QMouseEvent(type, pos, button, button, modifier);
    return QApplication::sendEvent(widgetContainer_->viewport(), &qtEvent);
}

void EC_WidgetBillboard::CheckMouseState()
{
    if (!widget_ || !widgetContainer_ || !widgetContainer_->scene())
        return;

    // If we have "unacked" press, release it now as 
    // we are no longer on top of the widget.
    if (!leftPressReleased_)
    {
        leftPressReleased_ = true;
        SendWidgetMouseEvent(QPoint(0,0), QEvent::MouseButtonRelease, Qt::LeftButton);
        SendWidgetMouseEvent(QPoint(0,0), QEvent::MouseMove, Qt::NoButton);
        Render();
    }

    // If we have tracking the mouse as in sent 
    // a QEvent::MouseMove out send a hover out now.
    if (trackingMouseMove_)
    {
        trackingMouseMove_ = false;
        emit WidgetMouseHoverOut();
    }
}
