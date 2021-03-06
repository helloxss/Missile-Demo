/********************************************************************
 * File   : MainScene.cpp
 * Project: ToolsDemo
 *
 ********************************************************************
 * Created on 9/21/13 By Nonlinear Ideas Inc.
 * Copyright (c) 2013 Nonlinear Ideas Inc. All rights reserved.
 ********************************************************************
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software. If you use this
 *    software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and
 *    must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include "MainScene.h"
#include "Box2DDebugDrawLayer.h"
#include "GridLayer.h"
#include "DebugLinesLayer.h"
#include "DebugMenuLayer.h"
#include "TapDragPinchInput.h"
#include "Notifier.h"
#include "Viewport.h"
#include "Missile.h"
#include "MovingEntity.h"
#include "DebugMessageLayer.h"
#include "SunBackgroundLayer.h"


MainScene::MainScene() :
_entity(NULL),
_dragBehavior(DB_TRACK),
_meType(MT_MISSILE)
{
}

MainScene::~MainScene()
{
   delete _entity;
}

void MainScene::CreateEntity()
{
   Vec2 position(0,0);
   if(_entity != NULL)
   {
      delete _entity;
   }
   switch(_meType)
   {
      case MT_MISSILE:
         _entity = new Missile(*_world,position);
         break;
      case MT_MOVING_ENTITY:
         _entity = new MovingEntity(*_world,position);
         break;
      case MT_MAX:
         assert(false);
         break;
   }
}

void MainScene::CreatePhysics()
{
   // Set up the viewport
   static const float32 worldSizeMeters = 100.0;
   
   // Initialize the Viewport
   Viewport::Instance().Init(worldSizeMeters);
   
   _world = new b2World(Vec2(0.0,0.0));
   // Do we want to let bodies sleep?
   // No for now...makes the debug layer blink
   // which is annoying.
   _world->SetAllowSleeping(false);
   _world->SetContinuousPhysics(true);
}

bool MainScene::init()
{
   
   // Create physical world
   CreatePhysics();
   
   // Add a color background.  This will make it easier on the eyes.
   //addChild(CCLayerColor::create(ccc4(200, 200, 200, 255)));
   
   addChild(SunBackgroundLayer::create());
   
   // Adding the debug lines so that we can draw the path followed.
   addChild(DebugLinesLayer::create());
   
   // Touch Input Layer
   _tapDragPinchInput = TapDragPinchInput::create(this);
   addChild(_tapDragPinchInput);
   
   // Box2d Debug
   addChild(Box2DDebugDrawLayer::create(_world));
   
   // Grid
   addChild(GridLayer::create());
   
   // Debug Message
   addChild(DebugMessageLayer::create());
   
   // Add the menu.
   CreateMenu();
   
   // Populate physical world
   CreateEntity();
   
   return true;
}

MainScene* MainScene::create()
{
   MainScene *pRet = new MainScene();
   if (pRet && pRet->init())
   {
      pRet->autorelease();
      return pRet;
   }
   else
   {
      CC_SAFE_DELETE(pRet);
      return NULL;
   }
}

void MainScene::onEnter()
{
   CCScene::onEnter();
}

void MainScene::onExit()
{
   CCScene::onExit();
}

void MainScene::onEnterTransitionDidFinish()
{
   CCScene::onEnterTransitionDidFinish();
   // It is a good practice to attach/detach from the Notifier
   // on screen transition times.  This gets you out of the question
   // of when the scene deletion occurs.
   Notifier::Instance().Attach(this, Notifier::NE_DEBUG_BUTTON_PRESSED);
   
   // Schedule Updates
   scheduleUpdate();
}

void MainScene::onExitTransitionDidStart()
{
   CCScene::onExitTransitionDidStart();
   // It is a good practice to attach/detach from the Notifier
   // on screen transition times.  This gets you out of the question
   // of when the scene deletion occurs.
   Notifier::Instance().Detach(this);
   
   // Turn off updates
   unscheduleUpdate();
}

// Handler for Notifier Events
void MainScene::Notify(Notifier::NOTIFIED_EVENT_TYPE_T eventType, const void* eventData)
{
   switch(eventType)
   {
      default:
         assert(false);
         break;
      case Notifier::NE_DEBUG_BUTTON_PRESSED:
         HandleMenuChoice((uint32)eventData);
         break;
   }
}

void MainScene::UpdateMissile()
{
   _entity->Update();
}

void MainScene::UpdatePhysics()
{
   const int velocityIterations = 8;
   const int positionIterations = 1;
   float32 fixedDT = SECONDS_PER_TICK;
   // Instruct the world to perform a single step of simulation. It is
   // generally best to keep the time step and iterations fixed.
   _world->Step(fixedDT, velocityIterations, positionIterations);
}

void MainScene::update(float dt)
{
   UpdateMissile();
   UpdatePhysics();
}

void MainScene::PinchViewport(const CCPoint& p0Org,const CCPoint& p1Org,
                              const CCPoint& p0,const CCPoint& p1)
{
   Viewport& vp = Viewport::Instance();
   float32 distOrg = ccpDistance(p0Org, p1Org);
   float32 distNew = ccpDistance(p0, p1);
   
   if(distOrg < 1)
      distOrg = 1;
   if(distNew < 1)
      distNew = 1;
   
   float32 scaleAdjust = distNew/distOrg;
   Vec2 centerOld = vp.Convert(ccpMidpoint(p0Org, p1Org));
   Vec2 centerNew = vp.Convert(ccpMidpoint(p0, p1));
   
   vp.SetCenter(_viewportCenterOrg-centerNew+centerOld);
   vp.SetScale(scaleAdjust*_viewportScaleOrg);
}


// Handler for Tap/Drag/Pinch Events
void MainScene::TapDragPinchInputTap(const TOUCH_DATA_T& point)
{
   
}
void MainScene::TapDragPinchInputLongTap(const TOUCH_DATA_T& point)
{
}



void MainScene::TapDragPinchInputPinchBegin(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   _entity->CommandIdle();
   Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
   _tapDragPinchInput->DrawDebug();
   _viewportCenterOrg = Viewport::Instance().GetCenterMeters();
   _viewportScaleOrg = Viewport::Instance().GetScale();
   PinchViewport(GetPinchPoint0().pos, GetPinchPoint1().pos, point0.pos, point1.pos);
}
void MainScene::TapDragPinchInputPinchContinue(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
   _tapDragPinchInput->DrawDebug();
   PinchViewport(GetPinchPoint0().pos, GetPinchPoint1().pos, point0.pos, point1.pos);
}
void MainScene::TapDragPinchInputPinchEnd(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
}
void MainScene::TapDragPinchInputDragBegin(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
   _tapDragPinchInput->DrawDebug();
   switch(_dragBehavior)
   {
      case DB_TRACK:
         _entity->CommandTurnTowards(Viewport::Instance().Convert(point0.pos));
         break;
      case DB_SEEK:
         _entity->CommandSeek(Viewport::Instance().Convert(point0.pos));
         break;
      case DB_PATH:
      {
         LINE_PIXELS_DATA ld;
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _path.clear();
         _path.push_back(Viewport::Instance().Convert(point0.pos));
         _path.push_back(Viewport::Instance().Convert(point1.pos));
         _entity->CommandIdle();
         
         ld.start = point0.pos;
         ld.end = point1.pos;
         _lastPoint = point1.pos;
         Notifier::Instance().Notify(Notifier::NE_DEBUG_LINE_DRAW_ADD_LINE_PIXELS,&ld);
         
      }
         break;
   }
}
void MainScene::TapDragPinchInputDragContinue(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   switch(_dragBehavior)
   {
      case DB_TRACK:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _tapDragPinchInput->DrawDebug();
         _entity->SetTargetPosition(Viewport::Instance().Convert(point1.pos));
         break;
      case DB_SEEK:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _tapDragPinchInput->DrawDebug();
         _entity->SetTargetPosition(Viewport::Instance().Convert(point1.pos));
         break;
      case DB_PATH:
      {
         
         LINE_PIXELS_DATA ld;
         _path.push_back(Viewport::Instance().Convert(point1.pos));
         ld.start = _lastPoint;
         ld.end = point1.pos;
         _lastPoint = point1.pos;
         Notifier::Instance().Notify(Notifier::NE_DEBUG_LINE_DRAW_ADD_LINE_PIXELS,&ld);
      }
         break;
   }
}
void MainScene::TapDragPinchInputDragEnd(const TOUCH_DATA_T& point0, const TOUCH_DATA_T& point1)
{
   switch(_dragBehavior)
   {
      case DB_TRACK:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _entity->CommandIdle();
         break;
      case DB_SEEK:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _entity->CommandIdle();
         break;
      case DB_PATH:
         _entity->CommandFollowPath(_path);
         break;
   }
}

void MainScene::CreateMenu()
{
   vector<string> labels;
   labels.push_back("Debug");
   labels.push_back("Zoom In");
   labels.push_back("Normal View");
   labels.push_back("Zoom Out");
   labels.push_back("Cmd: Track");
   labels.push_back("Cmd: Seek");
   labels.push_back("Cmd: Path");
   labels.push_back("Next Type");
   
   
   CCSize scrSize = CCDirector::sharedDirector()->getWinSize();
   
   DebugMenuLayer* layer = DebugMenuLayer::create(labels,ccp(scrSize.width*0.1,scrSize.height*0.5));
   layer->GetMenu()->setColor(ccc3(90, 90, 90));
   assert(layer != NULL);
   addChild(layer);
}

void MainScene::SetZoom(float scale)
{
   Viewport::Instance().SetScale(scale);
}


void MainScene::ToggleDebug()
{
   Notifier::Instance().Notify(Notifier::NE_DEBUG_TOGGLE_VISIBILITY);
}

void MainScene::HandleMenuChoice(uint32 choice)
{
   switch(choice)
   {
      case 0:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         ToggleDebug();
         break;
      case 1:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         SetZoom(0.5);
         Viewport::Instance().SetCenter(Vec2(0,0));
         break;
      case 2:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         SetZoom(1.0);
         Viewport::Instance().SetCenter(Vec2(0,0));
         break;
      case 3:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         SetZoom(1.5);
         Viewport::Instance().SetCenter(Vec2(0,0));
         break;
      case 4:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _dragBehavior = DB_TRACK;
         break;
      case 5:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _dragBehavior = DB_SEEK;
         break;
      case 6:
         Notifier::Instance().Notify(Notifier::NE_RESET_DRAW_CYCLE);
         _dragBehavior = DB_PATH;
         break;
      case 7:
         _meType++;
         if(_meType == MT_MAX)
            _meType = MT_MIN;
         CreateEntity();
         // If the entity was already following a path,
         // put this one on the path as well.
         switch(_dragBehavior)
      {
         case DB_PATH:
            _entity->CommandFollowPath(_path);
            break;
         case DB_SEEK:
            break;
         case DB_TRACK:
            break;
      }
         break;
      default:
         assert(false);
         break;
   }
}

