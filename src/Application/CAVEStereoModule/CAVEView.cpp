// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "CAVEView.h"
#include "ExternalRenderWindow.h"

#include "Renderer.h"
#include "OgreWorld.h"

#include <QDesktopWidget>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QApplication>

#include "Framework.h"
#include "UiAPI.h"
#include "UiMainWindow.h"

#include "MemoryLeakCheck.h"

namespace CAVEStereo
{
    CAVEView::CAVEView(const OgreRenderer::RendererPtr &renderer) :
        renderer_(renderer),
        camera_(0),
        render_window_(0)
    {
    }

    CAVEView::~CAVEView()
    {
        if (!renderer_.expired() && camera_)
        {
            Ogre::Root::getSingleton().detachRenderTarget(render_window_->getRenderWindow()->getName());
            renderer_.lock()->GetActiveOgreWorld()->GetSceneManager()->destroyCamera(camera_);
        }
        SAFE_DELETE(render_window_);
    }

    void CAVEView::Initialize(const QString& name,  Ogre::Vector3 &top_left, Ogre::Vector3 &bottom_left, Ogre::Vector3 &bottom_right, Ogre::Vector3 &eye_pos)
    {
        assert(!renderer_.expired());
        Initialize(name, renderer_.lock()->WindowWidth(), renderer_.lock()->WindowHeight(),top_left, bottom_left, bottom_right, eye_pos); 
    }

    void CAVEView::InitializePanorama(const QString& name, Ogre::Vector3 &top_left, Ogre::Vector3 &bottom_left, Ogre::Vector3 &bottom_right, Ogre::Vector3 &eye_pos,int n)
    {
        assert(!renderer_.expired());
        InitializePanorama(name, renderer_.lock()->WindowWidth(), renderer_.lock()->WindowHeight(),top_left, bottom_left, bottom_right, eye_pos, n); 
    }

    void CAVEView::GetProjectionParameters( Ogre::Vector3 &top_left, Ogre::Vector3 &bottom_left, Ogre::Vector3 &bottom_right, Ogre::Vector3 &eye_pos)
    {
        top_left = tl;
        bottom_left = lb;
        bottom_right = rb;
        eye_pos = ep;
    }

    void CAVEView::ReCalculateProjection(Ogre::Vector3 &top_left, Ogre::Vector3 &bottom_left, Ogre::Vector3 &bottom_right, Ogre::Vector3 &eye_pos)
    {
        lb = bottom_left;
        rb = bottom_right;
        tl = top_left;
        ep = eye_pos;

        assert(renderer_.lock());
        assert(camera_);
        assert(render_window_);
        bool openGL = false;
        if (renderer_.lock()->OgreRoot()->getRenderSystem()->getName() == "OpenGL Rendering Subsystem")
            openGL = true;

        //Projection magic be happening here.
        double n = camera_->getNearClipDistance();
        double f = camera_->getFarClipDistance();
        double l, r, b, t;

        //screenspace axes
        Ogre::Vector3 sr, su, sn;
        //from eye to screenpoints
        Ogre::Vector3 ebl,etl,ebr;

        Ogre::Matrix4 proj_mat, transl, change_base;

        sr=bottom_right-bottom_left;
        sr.normalise();
        su=top_left-bottom_left;
        su.normalise();
        sn=su.crossProduct(sr);
        sn.normalise();

        ebl = bottom_left-eye_pos;
        etl = top_left-eye_pos;
        ebr = bottom_right-eye_pos;

        qreal distance_to_plane;
        if(openGL)
        {
            sn = -sn;
            distance_to_plane = -sn.dotProduct(ebl);
        }
        else
        {
            distance_to_plane = sn.dotProduct(ebl);
        }

        l = sr.dotProduct(ebl)*n/distance_to_plane;
        r = sr.dotProduct(ebr)*n/distance_to_plane;
        b = su.dotProduct(ebl)*n/distance_to_plane;
        t = su.dotProduct(etl)*n/distance_to_plane;

        renderer_.lock()->OgreRoot()->getRenderSystem()->_makeProjectionMatrix(l,r,b,t,n,f,proj_mat);
        change_base=Ogre::Matrix4(sr.x,sr.y,sr.z,0,
                                  su.x,su.y,su.z,0,
                                  sn.x,sn.y,sn.z,0,
                                  0,0,0,1);

        transl.makeTrans(-eye_pos);
        proj_mat = proj_mat*change_base*transl;
        camera_->setCustomProjectionMatrix(true, proj_mat);
    }

    void CAVEView::Initialize(const QString& name, qreal window_width, qreal window_height, Ogre::Vector3 &top_left, Ogre::Vector3 &bottom_left, Ogre::Vector3 &bottom_right, Ogre::Vector3 &eye_pos)
    {
        assert(renderer_.lock().get());
        Ogre::Camera* original_cam = renderer_.lock()->MainOgreCamera();
        std::string std_name = name.toStdString();
        render_window_ = new ExternalRenderWindow();
        render_window_->CreateRenderWindow(std_name, window_width, window_height,0,0,false);
        render_window_->setGeometry(20,20,window_width,window_height);
        camera_ = renderer_.lock()->GetActiveOgreWorld()->GetSceneManager()->createCamera(std_name + "_camera");
        render_window_->getRenderWindow()->addViewport(camera_);
        camera_->getViewport()->setOverlaysEnabled(false);
        camera_->getViewport()->setShadowsEnabled(true);

        //setup the camera
        camera_->setCustomProjectionMatrix(false);
        camera_->setNearClipDistance(original_cam->getNearClipDistance());
        camera_->setFarClipDistance(original_cam->getFarClipDistance());
        camera_->setVisibilityFlags(original_cam->getVisibilityFlags());

        Ogre::SceneNode* node = dynamic_cast<Ogre::SceneNode*>(original_cam->getParentNode());
        if(node)
            node->attachObject(camera_);

        ReCalculateProjection(top_left, bottom_left, bottom_right, eye_pos);
    }

    void CAVEView::InitializePanorama(const QString& name, qreal window_width, qreal window_height, Ogre::Vector3 &top_left, Ogre::Vector3 &bottom_left, Ogre::Vector3 &bottom_right, Ogre::Vector3 &eye_pos, int window_number)
    {
        assert(renderer_.lock().get());
        Ogre::Camera* original_cam = renderer_.lock()->MainOgreCamera();
        std::string std_name = name.toStdString();
        render_window_ = new ExternalRenderWindow();
        QRect rect = QApplication::desktop()->screenGeometry();
        int new_render_width = rect.width();
        int new_render_height = rect.height();
        int new_render_x = ((new_render_width/2) - ((new_render_width*0.2)/2) );

        switch(window_number)
        {
            case 1:
                render_window_->CreateRenderWindow(std_name, new_render_width*0.2, new_render_height*0.2,0,0,false);
                render_window_->setGeometry(new_render_x + 2*(new_render_width*0.2),0.77*new_render_height,new_render_width*0.2, new_render_height*0.2);
                break;
            case 2:
                render_window_->CreateRenderWindow(std_name, new_render_width*0.2, new_render_height*0.2,0,0,false);
                render_window_->setGeometry(new_render_x + (new_render_width*0.2),0.77*new_render_height,new_render_width*0.2, new_render_height*0.2);
                break;
            case 3:
                render_window_->CreateRenderWindow(std_name, new_render_width*0.2, new_render_height*0.2,0,0,false);
                render_window_->setGeometry(new_render_x ,0.77*new_render_height,new_render_width*0.2, new_render_height*0.2);
                break;
            case 4:
                render_window_->CreateRenderWindow(std_name, new_render_width*0.2, new_render_height*0.2,0,0,false);
                render_window_->setGeometry(new_render_x - (new_render_width*0.2),0.77*new_render_height,new_render_width*0.2, new_render_height*0.2);
                break;
            case 5:
                render_window_->CreateRenderWindow(std_name, new_render_width*0.2, new_render_height*0.2,0,0,false);
                render_window_->setGeometry(new_render_x - 2*(new_render_width*0.2),0.77*new_render_height,new_render_width*0.2, new_render_height*0.2);
                break;
        }

        camera_ = renderer_.lock()->GetActiveOgreWorld()->GetSceneManager()->createCamera(std_name + "_camera");
        render_window_->getRenderWindow()->addViewport(camera_);
        camera_->getViewport()->setOverlaysEnabled(false);
        camera_->getViewport()->setShadowsEnabled(true);

        camera_->setCustomProjectionMatrix(false);
        camera_->setNearClipDistance(original_cam->getNearClipDistance());
        camera_->setFarClipDistance(original_cam->getFarClipDistance());
        camera_->setVisibilityFlags(original_cam->getVisibilityFlags());

        Ogre::SceneNode* node = dynamic_cast<Ogre::SceneNode*>(original_cam->getParentNode());
        if(node)
            node->attachObject(camera_);

        ReCalculateProjection(top_left, bottom_left, bottom_right, eye_pos);
    }
}
