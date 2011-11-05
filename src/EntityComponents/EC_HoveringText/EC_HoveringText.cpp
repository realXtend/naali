/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   EC_HoveringText.cpp
 *  @brief  Shows a hovering text attached to an entity.
 */

#define MATH_OGRE_INTEROP

#include "DebugOperatorNew.h"

#include "EC_HoveringText.h"
#include "Renderer.h"
#include "EC_Placeable.h"
#include "Entity.h"
#include "LoggingFunctions.h"
#include "Scene.h"
#include "Framework.h"
#include "OgreRenderingModule.h"
#include "OgreWorld.h"
#include "OgreMaterialUtils.h"
#include "AssetAPI.h"
#include "TextureAsset.h"

#include <Ogre.h>
#include <QFile>
#include <QPainter>
#include <QTimer>
#include <QTimeLine>

#include "MemoryLeakCheck.h"

EC_HoveringText::EC_HoveringText(Scene* scene) :
    IComponent(scene),
    font_(QFont("Arial", 100)),
    textColor_(Qt::black),
    billboardSet_(0),
    billboard_(0),
    usingGrad(this, "Use Gradient", false),
    text(this, "Text"),
    font(this, "Font", "Arial"),
    fontColor(this, "Font Color"),
    fontSize(this, "Font Size", 100),
    backgroundColor(this, "Background Color", Color(1.0f,1.0f,1.0f,0.0f)),
    position(this, "Position", float3(0.0f, 0.0f, 0.0f)),
    gradStart(this, "Gradient Start", Color(0.0f,0.0f,0.0f,1.0f)),
    gradEnd(this, "Gradient End", Color(1.0f,1.0f,1.0f,1.0f)),
    borderColor(this, "Border Color", Color(0.0f,0.0f,0.0f,0.0f)),
    borderThickness(this, "Border Thickness", 0.0),
    overlayAlpha(this, "Overlay Alpha", 1.0),
    width(this, "Width", 1.0),
    height(this, "Height", 1.0),
    texWidth(this, "Texture Width", 256),
    texHeight(this, "Texture Height", 256),
    cornerRadius(this, "Corner radius", float2(20.0, 20.0))
{
    if (scene)
        world_ = scene->GetWorld<OgreWorld>();
}

EC_HoveringText::~EC_HoveringText()
{
    if (texture_.get() != 0)
    {
        AssetAPI* asset = framework->Asset();
        asset->ForgetAsset(texture_,false);
    }
    Destroy();
}

void EC_HoveringText::Destroy()
{
    if (!ViewEnabled())
        return;

    if (!world_.expired())
    {
        Ogre::SceneManager* sceneMgr = world_.lock()->GetSceneManager();
        
        try{
        Ogre::MaterialManager::getSingleton().remove(materialName_);
        } catch(...)
        {
        }
        try{
        if (billboardSet_ && billboard_)
            billboardSet_->removeBillboard(billboard_);
        } catch(...)
        {
        }
        try{
        
        if (billboardSet_)
        {
            sceneMgr->destroyBillboardSet(billboardSet_);
        }

        } catch(...)
        {
        }
    }

    billboard_ = 0;
    billboardSet_ = 0;
    textureName_ = "";
    materialName_ = "";
}

void EC_HoveringText::SetPosition(const float3& position)
{
    if (!ViewEnabled())
        return;

    if (billboard_)
        billboard_->setPosition(position);
}

void EC_HoveringText::SetFont(const QFont &font)
{
    font_ = font;
    Redraw();
}

void EC_HoveringText::SetTextColor(const QColor &color)
{
    textColor_ = color;
    Redraw();
}

void EC_HoveringText::SetBackgroundGradient(const QColor &start_color, const QColor &end_color)
{
    bg_grad_.setColorAt(0.0, start_color);
    bg_grad_.setColorAt(1.0, end_color);
}

void EC_HoveringText::Show()
{
    if (!ViewEnabled())
        return;

    if (billboardSet_)
        billboardSet_->setVisible(true);
}

void EC_HoveringText::Hide()
{
    if (!ViewEnabled())
        return;

    if (billboardSet_)
        billboardSet_->setVisible(false);
}

void EC_HoveringText::SetOverlayAlpha(float alpha)
{
    Ogre::MaterialManager &mgr = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr material = mgr.getByName(materialName_);
    if (!material.get() || material->getNumTechniques() < 1 || material->getTechnique(0)->getNumPasses() < 1 || material->getTechnique(0)->getPass(0)->getNumTextureUnitStates() < 1)
        return;

    material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setAlphaOperation(
        Ogre::LBX_BLEND_MANUAL, Ogre::LBS_TEXTURE, Ogre::LBS_MANUAL, 1.0, 0.0, alpha);
}

void EC_HoveringText::SetBillboardSize(float width, float height)
{
    if (billboard_)
        billboard_->setDimensions(width, height);
}

bool EC_HoveringText::IsVisible() const
{
    if (!ViewEnabled())
        return false;

    if (billboardSet_)
        return billboardSet_->isVisible();
    else
        return false;
}

void EC_HoveringText::ShowMessage(const QString &text)
{
    if (!ViewEnabled())
        return;
    if (world_.expired())
        return;
    
    OgreWorldPtr world = world_.lock();
    Ogre::SceneManager *scene = world->GetSceneManager();
    assert(scene);
    if (!scene)
        return;

    //Scene::Entity *entity = ParentEntity();
    Entity* entity = ParentEntity();
    assert(entity);
    if (!entity)
        return;

    EC_Placeable *node = entity->GetComponent<EC_Placeable>().get();
    if (!node)
        return;

    Ogre::SceneNode *sceneNode = node->GetSceneNode();
    assert(sceneNode);
    if (!sceneNode)
        return;

    // Create billboard if it doesn't exist.
    if (!billboardSet_)
    {
        billboardSet_ = scene->createBillboardSet(world->GetUniqueObjectName("EC_HoveringText"), 1);
        assert(billboardSet_);

        materialName_ = world->GetUniqueObjectName("EC_HoveringText_material");
        OgreRenderer::CloneMaterial("HoveringText", materialName_);
        billboardSet_->setMaterialName(materialName_);
        billboardSet_->setCastShadows(false);

        sceneNode->attachObject(billboardSet_);
    }

    if (billboardSet_ && !billboard_)
    {
        billboard_ = billboardSet_->createBillboard(Ogre::Vector3(0, 0, 0.7f));

        SetBillboardSize(width.Get(), height.Get());
        SetPosition(position.Get());
    }

    Redraw();
}

void EC_HoveringText::Redraw()
{
    if (!ViewEnabled())
        return;

    if (world_.expired() || !billboardSet_ || !billboard_)
        return;

    bool textEmpty = text.Get().isEmpty();
    
    try
    {
        if (texture_.get() == 0)
        {       
            AssetAPI* asset = framework->Asset();

            textureName_ = asset->GenerateUniqueAssetName("tex", "EC_HoveringText_").toStdString();
            QString name(textureName_.c_str());
            texture_  = boost::dynamic_pointer_cast<TextureAsset>(asset->CreateNewAsset("Texture", name));  
            
            assert(texture_);
            
            if (texture_ == 0)
            {
                LogError("Failed to create texture " + textureName_);
                return;
            }
        }       
       
        QBrush brush(backgroundColor.Get());
       
        if (usingGrad.Get())
        {   
            QRect rect(0,0,texWidth.Get(), texHeight.Get());
            bg_grad_.setStart(QPointF(0,rect.top()));
            bg_grad_.setFinalStop(QPointF(0,rect.bottom()));
            brush = QBrush(bg_grad_);
        }

        QColor borderCol;
        Color col = borderColor.Get();
        borderCol.setRgbF(col.r, col.g, col.b, col.a);

        QPen borderPen;
        borderPen.setColor(borderCol);
        borderPen.setWidthF(borderThickness.Get());
        
        float2 corners =  cornerRadius.Get();

        if (!textEmpty)
        {
            // Disable mipmapping, as Ogre seems to bug with it
            texture_->SetContentsDrawText(texWidth.Get(), 
                                    texHeight.Get(), 
                                    text.Get(), 
                                    textColor_, 
                                    font_, 
                                    brush, 
                                    borderPen, Qt::AlignCenter | Qt::TextWordWrap, false, false, corners.x, corners.y);
        }
        else
            texture_->SetContentsDrawText(texWidth.Get(), texHeight.Get(), text.Get(), textColor_, font_, QBrush(), QPen(), Qt::AlignCenter | Qt::TextWordWrap, false, false, 0.0f, 0.0f);
    }
    catch(Ogre::Exception &e)
    {
        LogError("Failed to create texture " + textureName_  + ": " + std::string(e.what()));
        return;
    }

    // Set new texture for the material
    assert(!materialName_.empty());
    if (!materialName_.empty())
    {
        Ogre::MaterialManager &mgr = Ogre::MaterialManager::getSingleton();
        Ogre::MaterialPtr material = mgr.getByName(materialName_);
        assert(material.get());
        OgreRenderer::SetTextureUnitOnMaterial(material, textureName_);
    }
}

void EC_HoveringText::AttributesChanged()
{
    if (font.ValueChanged() || fontSize.ValueChanged())
    {
        SetFont(QFont(font.Get(), fontSize.Get()));
    }
    if (fontColor.ValueChanged())
    {
        Color col = fontColor.Get();
        textColor_.setRgbF(col.r, col.g, col.b, col.a);
    }
    if (position.ValueChanged())
    {
        SetPosition(position.Get());
    }
    if (gradStart.ValueChanged() || gradEnd.ValueChanged())
    {
        QColor colStart;
        QColor colEnd;
        Color col = gradStart.Get();
        colStart.setRgbF(col.r, col.g, col.b);
        col = gradEnd.Get();
        colEnd.setRgbF(col.r, col.g, col.b);
        SetBackgroundGradient(colStart, colEnd);
    }
    if (overlayAlpha.ValueChanged())
        SetOverlayAlpha(overlayAlpha.Get());
    if (width.ValueChanged() || height.ValueChanged())
        SetBillboardSize(width.Get(), height.Get());

    // Changes to the following attributes require a (expensive) repaint of the texture on the CPU side.
    bool repaint = text.ValueChanged() || font.ValueChanged() || fontSize.ValueChanged() || fontColor.ValueChanged()
        || backgroundColor.ValueChanged() || borderColor.ValueChanged() || borderThickness.ValueChanged() || usingGrad.ValueChanged()
        || gradStart.ValueChanged() || gradEnd.ValueChanged() || texWidth.ValueChanged() || texHeight.ValueChanged()
        || cornerRadius.ValueChanged();

    // Changes to the following attributes do not alter the texture contents, and don't require a repaint:
    // position, overlayAlpha, width, height.

    // Repaint the new text with new appearance.
    if (repaint)
        ShowMessage(text.Get());
}