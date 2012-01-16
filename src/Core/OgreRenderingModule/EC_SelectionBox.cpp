// For conditions of distribution and use, see copyright notice in LICENSE
// Author: Nathan Letwory <nathan@letworyinteractive.com>

#include "StableHeaders.h"
#include "OgreRenderingModule.h"
#include "Renderer.h"
#include "Scene.h"
#include "EC_SelectionBox.h"
#include "LoggingFunctions.h"
#include "OgreWorld.h"
#include <Ogre.h>

using namespace OgreRenderer;

#include "MemoryLeakCheck.h"

EC_SelectionBox::EC_SelectionBox(Scene* scene) :
    IComponent(scene),
    selectionBox_(0)
{
    if (scene)
        world_ = scene->GetWorld<OgreWorld>();
    OgreWorldPtr world = world_.lock();
    Ogre::SceneManager* sceneMgr = world->OgreSceneManager();
    selectionBox_ = sceneMgr->createManualObject(world->GetUniqueObjectName("EC_SelectionBox"));
    selectionBox_->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
    selectionBox_->setUseIdentityProjection(true);
    selectionBox_->setUseIdentityView(true);
    selectionBox_->setQueryFlags(0);
    sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(selectionBox_);
}

EC_SelectionBox::~EC_SelectionBox()
{
    if (selectionBox_)
    {
        OgreWorldPtr world = world_.lock();
        if (world)
        {
            Ogre::SceneManager* sceneMgr = world->OgreSceneManager();
            sceneMgr->destroyManualObject(selectionBox_);
            selectionBox_ = 0;
        }
    }
}

void EC_SelectionBox::SetBoundingBox(QRect &view)
{
    if (world_.expired())
        return;
    OgreWorldPtr world = world_.lock();
    Renderer* renderer = world->Renderer();
    Ogre::RenderWindow *renderWindow = renderer->GetCurrentRenderWindow();
    float w= (float)renderWindow->getWidth();
    float h= (float)renderWindow->getHeight();
    float left = (float)(view.left()) / w, right = (float)(view.right()) / w;
    float top = (float)(view.top()) / h, bottom = (float)(view.bottom()) / h;
    
    if(left > right) { float tmp; tmp = left; left = right; right = tmp; }
    if(top > bottom) { float tmp; tmp = top; top = bottom; bottom = tmp; }
    // don't do selection box if too small
    if((right - left) * (bottom-top) < 0.0001) return;
    
    // correct coordinates for overlay
    left = left * 2 - 1;
    right = right * 2 - 1;
    top = 1 - top * 2;
    bottom = 1 - bottom * 2;
 
    selectionBox_->clear();
    selectionBox_->begin("BlueTransparent",Ogre::RenderOperation::OT_TRIANGLE_STRIP);
    selectionBox_->position(left, bottom, -1);
    selectionBox_->position(right, bottom, -1);
    selectionBox_->position(left, top, -1);
    selectionBox_->position(right, top, -1);
    selectionBox_->end();
    selectionBox_->begin("",Ogre::RenderOperation::OT_LINE_STRIP);
    selectionBox_->position(left, top, -1);
    selectionBox_->position(right, top, -1);
    selectionBox_->position(right, bottom, -1);
    selectionBox_->position(left, bottom, -1);
    selectionBox_->position(left, top, -1);
    selectionBox_->end();
    
    selectionBox_->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);
}

void EC_SelectionBox::Show()
{
    selectionBox_->clear();
    selectionBox_->setVisible(true);
}

void EC_SelectionBox::Hide()
{
    selectionBox_->clear();
    selectionBox_->setVisible(false);
}
