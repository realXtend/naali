/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneInteract.h
    @brief  Transforms generic mouse and keyboard input events on scene entities to input-related entity actions and signals. */

#pragma once

#if defined (_WINDOWS)
#if defined(SCENEINTERACT_EXPORTS)
#define SCENEINTERACT_API __declspec(dllexport)
#else
#define SCENEINTERACT_API __declspec(dllimport)
#endif
#else
#define SCENEINTERACT_API
#endif

#include "IModule.h"
#include "SceneFwd.h"
#include "InputFwd.h"
#include "CoreDefines.h"

#include <QObject>

/// Transforms generic mouse and keyboard input events on scene entities to input-related entity actions and signals.
/** Performs a raycast to the mouse position each frame and executes entity actions depending current input.

    <b>Local</b> entity actions executed to the hit entity:
    <ul>
    <li>"MouseHoverIn" - Executed when mouse hover enters an entity.
    <div>No parameters.</div>
    <li>"MouseHover" - Executed when mouse hovers on an entity.
    <div>No parameters</div>
    <li>"MouseHoverOut" - Executed when mouse hover leaves an entity.
    <div></div>
    <li>"MousePress" - Executed when mouse button is clicked on an entity.
    <div>String parameters: (int)"Qt::MouseButton", (float,float,float)"x,y,z", (int)"submesh index"</div>
    <li>"MouseRelease" - Executed when mouse button is released on an entity.
    <div>String parameters: (int)"Qt::MouseButton", (float,float,float)"x,y,z", (int)"submesh index"</div>
    </ul> */
class SCENEINTERACT_API SceneInteract : public IModule
{
    Q_OBJECT

public:
    SceneInteract();
    ~SceneInteract() {}

    void Initialize();
    /// Executes "MouseHover" action each frame if raycast has hit an entity.
    void Update(f64 frameTime);

signals:
    /// Emitted when mouse cursor moves on top of an entity.
    /** @param entity Hit entity.
        @param Possible mouse button held down during the move.
        @param result Raycast result data object. */
    void EntityMouseMove(Entity *entity, Qt::MouseButton button, RaycastResult *result);

    /// Emitted when mouse was scrolled and raycast hit an entity.
    /** @param entity Hit entity.
        @param delta The difference in the mouse wheel position.
        @param result Raycast result data object. */
    void EntityMouseScroll(Entity *entity, int delta, RaycastResult *result);

    /// Emitted when scene was clicked and raycast hit an entity.
    /** @param entity Hit entity.
        @param Clicked mouse button
        @param result Raycast result data object. */
    void EntityClicked(Entity *entity, Qt::MouseButton button, RaycastResult *result);

    /// Emitted when scene was clicked and raycast hit an entity.
    /** @param entity Hit entity.
        @param Released mouse button.
        @param result Raycast result data object. */
    void EntityClickReleased(Entity *entity, Qt::MouseButton button, RaycastResult *result);

private:
    /// Performs raycast to last known mouse cursor position in the currently active scene.
    RaycastResult* Raycast();

    InputContextPtr input;
    int lastX; ///< Last known mouse cursor's x position.
    int lastY; ///< Last known mouse cursor's y position.
    bool itemUnderMouse; ///< Was there widget under mouse in last known position.
    EntityWeakPtr lastHitEntity; ///< Last entity raycast has hit.

private slots:
    void HandleKeyEvent(KeyEvent *e);
    void HandleMouseEvent(MouseEvent *e);
};
