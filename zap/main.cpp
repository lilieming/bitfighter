////////////////////////
//To do prior to 1.0 release
//
////////////////////////
// Nitnoid
// Make color defs below constant... need to change associated externs too!

// Some time
// Add mouse coords to diagnostics screen, raw key codes

// Editor
// list of levels you can edit

// Long term
// Admin select level w/ preview(?)
// Implement level selection based on MinPlayers & MaxPlayers

//Test:

// TODO:
// Turrets at end of wall can rotate through wall.  Why?
// Create color global for reticle color


/*
XXX need to document timers, main function XXX
<ul>
<h4>Big changes</h4>
<li>Added energy item</li>
<li>Started trasnsition of Join menu into Game Lobby</li>

<h4></h4>
<li>Added Restart Level to game options menu if you have level change permissions</li>
<li>Moved Add 2 Mins option to LevelChanger permission level (with older servers you'll still need admin rights)</li>
<li>Added asteroids to help screen, enhanced instruction colors</li>
<li>Fixed several in-game crashing issues</li>
<li>Minor efficiency improvements on rendering routines</li>

<h4>Bots</h4>
<li>Added enhanced timer class to Lua scripting</li>
<li>Moved some "unwrapped" functionality into main() function</li>

<h4>Editor</h4>
<li>Many small improvements to editor</li>
<li>Fixed editor crashing issue caused by double-delete of item</li>
<li>Fixed editor crash while caused by pasting objects in the middle of a drag operation</li>
<li>Undo/redo more efficient -- will perform better with large levels and deep undo states</li>
<li>Added autosave -- will keep a current copy of the l

<h4>Bugs</h4>
<li>Fixed rare Zap-era crash condition when player shoots a soccer ball, but quits game before goal is scored</li>
<li>Ctrl-left click now starts a wall</li>

*/

//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "IniFile.h"

#include "../tnl/tnl.h"
#include "../tnl/tnlRandom.h"
#include "../tnl/tnlGhostConnection.h"
#include "../tnl/tnlNetInterface.h"
#include "../tnl/tnlJournal.h"

#ifdef TNL_OS_MAC_OSX
#include "Directory.h"
#endif

#include "zapjournal.h"

#include "../glut/glutInclude.h"
#include <stdarg.h>
#include <sys/stat.h>

using namespace TNL;

#include "UI.h"
#include "UIGame.h"
#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIEditor.h"
#include "UIErrorMessage.h"
#include "UIDiagnostics.h"
#include "UICredits.h"
#include "game.h"
#include "gameNetInterface.h"
#include "masterConnection.h"
#include "sfx.h"
#include "sparkManager.h"
#include "input.h"
#include "keyCode.h"
#include "config.h"
#include "md5wrapper.h"

#include "screenShooter.h"


#ifdef TNL_OS_MAC_OSX
#include <unistd.h>
#endif

namespace Zap
{

string gHostName;                // Server name used when hosting a game (default set in config.h, set in INI or on cmd line)
string gHostDescr;               // Brief description of host
const char *gWindowTitle = "Bitfighter";

// The following things can be set via command line parameters
S32 gMaxPlayers;                 // Max players allowed -- can change on cmd line, or INI.  Default value in config.h
U32 gSimulatedLag;               // Simulate a slow network -- can change on cmd line
F32 gSimulatedPacketLoss;        // Simulate a bad network -- can change on cmd line


#ifdef ZAP_DEDICATED
bool gDedicatedServer = true;    // This will allow us to omit the -dedicated parameter when compiled in dedicated mode
#else
bool gDedicatedServer = false;   // Usually, we just want to play.  If true, we'll be in server-only, no-player mode
#endif

bool gQuit = false;
bool gIsServer = false;

// Handle any md5 requests
md5wrapper md5;


bool gShowAimVector = false;     // Do we render an aim vector?  This should probably not be a global, but until we find a better place for it...
bool gDisableShipKeyboardInput;  // Disable ship movement while user is in menus

U32 gUseStickNumber = 1;         // Which joystick do you want to use (1 = first, which is typical)
U32 gSticksFound = 0;            // Which joystick we're actually using...

CIniFile gINI("bitfighter.ini");    // This is our INI file

CmdLineSettings gCmdLineSettings;
IniSettings gIniSettings;

ControllerTypeType gAutoDetectedJoystickType;   // Remember what sort of joystick was found for diagnostic purposes


// Some colors -- other candidates include global and local chat colors, which are defined elsewhere.  Include here?
Color gNexusOpenColor(0, 0.7, 0);
Color gNexusClosedColor(0.85, 0.3, 0);
Color gErrorMessageTextColor(1, 0.5, 0.5);
Color gNeutralTeamColor(0.8, 0.8, 0.8);         // Objects that are neutral (on team -1)
Color gHostileTeamColor(0.5, 0.5, 0.5);         // Objects that are "hostile-to-all" (on team -2)
Color gMasterServerBlue(0.8, 0.8, 1);           // Messages about successful master server statii
Color gHelpTextColor(0, 1, 0);

S32 gMaxPolygonPoints = 32;                     // Max number of points we can have in Nexuses, LoadoutZones, etc.

bool gReadyToConnectToMaster = false;           // When we're sure we have a nickname, we'll set this to true and proceed to connect to the master

const char *gServerPassword = NULL;
const char *gAdminPassword = NULL;
const char *gLevelChangePassword = NULL;

Address gMasterAddress;
Address gConnectAddress;
Address gBindAddress(IPProtocol, Address::Any, 28000);      // Good for now, may be overwritten by INI or cmd line setting
      // Above is equivalent to ("IP:Any:28000")

string gLevelDir = "levels";              // Where our levels are stored, can be overwritten by ini or cmd line param
Vector<StringTableEntry> gLevelList;      // Holds a list of the levels we'll play when we're hosting

// Lower = more slippery!  Not used at the moment...
F32 gNormalFriction = 1000;   // Friction between vehicle and ground, ordinary
F32 gSlipFriction = 400;      // Friction, on a slip square

char gJoystickName[gJoystickNameLength] = "";

extern Point gMousePos;

// Since GLUT reports the current mouse pos via a series of events, and does not make
// its position available upon request, we'll store it when it changes so we'll have
// it when we need it.
void setMousePos(S32 x, S32 y)
{
   gMousePos.x = x;
   gMousePos.y = y;
}

enum HostingModePhases { NotHosting, LoadingLevels, DoneLoadingLevels, Hosting };
HostingModePhases gHostingModePhase;

Screenshooter gScreenshooter;    // For taking screen shots

ZapJournal gZapJournal;    // Our main journaling object

// Handler called by GLUT when window is reshaped
void GLUT_CB_reshape(int nw, int nh)
{
   gZapJournal.reshape(nw, nh);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, reshape, (S32 newWidth, S32 newHeight), (newWidth, newHeight))
{
   // If we are entering fullscreen mode, then we don't want to mess around with proportions and all that.  Just save window size and get out.
   if(gIniSettings.fullscreen)
   {
      // The following block will attempt to keep graphics from being stretched on a monitor with non-standard proportions
      // It works, but the effect is worse than the stretching, in my opinion.
      //F32 fact;
      //if((newWidth - UserInterface::canvasWidth) > (newHeight - UserInterface::canvasHeight))
      //   fact = max((F32) newHeight / (F32) UserInterface::canvasHeight, 0.15f);
      //else
      //   fact = max((F32) newWidth / (F32) UserInterface::canvasWidth, 0.15f);

      //newHeight = UserInterface::canvasHeight * fact;
      //newWidth = UserInterface::canvasWidth * fact;

      UserInterface::windowWidth = newWidth;
      UserInterface::windowHeight = newHeight;
      return;
   }

   // Constrain window to correct proportions...
   if((newWidth - UserInterface::canvasWidth) > (newHeight - UserInterface::canvasHeight))
      gIniSettings.winSizeFact = max((F32) newHeight / (F32) UserInterface::canvasHeight, 0.15f);
   else
      gIniSettings.winSizeFact = max((F32) newWidth / (F32) UserInterface::canvasWidth, 0.15f);

   newHeight = (S32)(UserInterface::canvasHeight * gIniSettings.winSizeFact);
   newWidth  = (S32)(UserInterface::canvasWidth  * gIniSettings.winSizeFact);

   glutReshapeWindow(newWidth, newHeight);

   UserInterface::windowWidth = newWidth;
   UserInterface::windowHeight = newHeight;

   gINI.SetValueF("Settings", "WindowScalingFactor", gIniSettings.winSizeFact, true);
}

// Handler called by GLUT when mouse motion is detected
void GLUT_CB_motion(int x, int y)
{
   gZapJournal.motion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, motion, (S32 x, S32 y), (x, y))
{
   setMousePos(x, y);

   if(UserInterface::current)
      UserInterface::current->onMouseDragged(x, y);
}

// Handler called by GLUT when "passive" mouse motion is detected
void GLUT_CB_passivemotion(int x, int y)
{
   gZapJournal.passivemotion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, passivemotion, (S32 x, S32 y), (x, y))
{

   // Glut sometimes fires spurious events.  Let's ignore those.
   if(x == gMousePos.x && y == gMousePos.y)
      return;

   setMousePos(x, y);

   if(UserInterface::current)
      UserInterface::current->onMouseMoved(x, y);
}

void keyDown(KeyCode keyCode, char ascii)    // Launch the onKeyDown event
{
   if(UserInterface::current)
      UserInterface::current->onKeyDown(keyCode, ascii);
}

// Sometimes we need to pretend a key was pressed, such as for those that don't
// generate events (shift/ctrl/alt, controller buttons, etc.)
void simulateKeyDown(KeyCode keyCode)
{
   setKeyState(keyCode, true);
   keyDown(keyCode, 0);
}

void keyUp(KeyCode keyCode)              // Launch the onKeyUp event
{
   if(UserInterface::current)
      UserInterface::current->onKeyUp(keyCode);
}

void simulateKeyUp(KeyCode keyCode)
{
   setKeyState(keyCode, false);
   keyUp(keyCode);
}

// GLUT handler for key-down events
void GLUT_CB_keydown(unsigned char key, S32 x, S32 y)
{
   gZapJournal.keydown(key);
}


TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, keydown, (U8 key), (key))
{
   // First check for some "universal" keys.  If keydown isn't one of those, we'll pass the key onto the keyDown handler
   // Check for ALT-ENTER --> toggles window mode/full screen
   if(key == '\r' && (glutGetModifiers() & GLUT_ACTIVE_ALT))
      gOptionsMenuUserInterface.toggleFullscreen();
   else if(key == 17)      // GLUT reports Ctrl-Q as 17
      gScreenshooter.phase = 1;
   else
   {
      KeyCode keyCode = standardGLUTKeyToKeyCode(key);
      setKeyState(keyCode, true);
      keyDown(keyCode, keyToAscii(key, keyCode));
   }
}

#ifndef ZAP_DEDICATED

// GLUT handler for key-up events
void GLUT_CB_keyup(unsigned char key, int x, int y)
{
   gZapJournal.keyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, keyup, (U8 key), (key))
{
   KeyCode keyCode = standardGLUTKeyToKeyCode(key);
   setKeyState(keyCode, false);
   keyUp(keyCode);
}

// GLUT handler for mouse clicks
void GLUT_CB_mouse(int button, int state, int x, int y)
{
   gZapJournal.mouse(button, state, x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, mouse, (S32 button, S32 state, S32 x, S32 y), (button, state, x, y))
{
   setMousePos(x, y);

   if(!UserInterface::current) return;    // Bail if no current UI

   if(button == GLUT_LEFT_BUTTON)
   {
      setKeyState(MOUSE_LEFT, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_LEFT, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_LEFT);
   }
   else if(button == GLUT_RIGHT_BUTTON)
   {
      setKeyState(MOUSE_RIGHT, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_RIGHT, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_RIGHT);
   }
   else if(button == GLUT_MIDDLE_BUTTON)
   {
      setKeyState(MOUSE_MIDDLE, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_MIDDLE, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_MIDDLE);
   }
}

// GLUT callback for special key down (special keys are things like F1-F12)
void GLUT_CB_specialkeydown(int key, int x, int y)
{
   gZapJournal.specialkeydown(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkeydown, (S32 key), (key))
{
   KeyCode keyCode = specialGLUTKeyToKeyCode(key);
   setKeyState(keyCode, true);

   if(keyCode == keyDIAG && !gDiagnosticInterface.isActive())   // Turn on diagnostic overlay if not already on
   {
      UserInterface::playBoop();
      gDiagnosticInterface.activate();
   }
   else
      keyDown(keyCode, keyToAscii(key, keyCode));           // Launch onKeyDown event
}


// GLUT callback for special key up
void GLUT_CB_specialkeyup(int key, int x, int y)
{
   gZapJournal.specialkeyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkeyup, (S32 key), (key))
{
   KeyCode keyCode = specialGLUTKeyToKeyCode(key);
   setKeyState(keyCode, false);
   keyUp(keyCode);
}
#endif

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkeydown, (U32 key), (key))
{
   if(key == 0)         // shift
   {
      setKeyState(KEY_SHIFT, true);
      keyDown(KEY_SHIFT, 0);
   }
   else if(key == 1)    // ctrl
   {
      setKeyState(KEY_CTRL, true);
      keyDown(KEY_CTRL, 0);
   }
   else if(key == 2)    // alt
   {
      setKeyState(KEY_ALT, true);
      keyDown(KEY_ALT, 0);
   }
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkeyup, (U32 key), (key))
{
   if(key == 0)         // shift
   {
      setKeyState(KEY_SHIFT, false);
      keyUp(KEY_SHIFT);
   }
   else if(key == 1)    // ctrl
   {
      setKeyState(KEY_CTRL, false);
      keyUp(KEY_CTRL);
   }
   else if(key == 2)    // alt
   {
      setKeyState(KEY_ALT, false);
      keyUp(KEY_ALT);
   }
}


void exitGame(S32 errcode)
{
   #ifdef TNL_OS_XBOX
      extern void xboxexit();
      xboxexit();
   #else
      exit(errcode);
   #endif
}


// Exit the game, back to the OS
void exitGame()
{
   exitGame(0);
}


// If we can't load any levels, here's the plan...
void abortHosting()
{
   if(gDedicatedServer)
   {
      logprintf("No levels were loaded from folder %s.  Cannot host a game.", gLevelDir.c_str());
      s_logprintf("No levels were loaded from folder %s.  Cannot host a game.", gLevelDir.c_str());
      //printf("No levels were loaded from folder %s.  Cannot host a game.", gLevelDir.c_str());      ==> Does nothing
      exitGame(1);
   }
   else
   {
      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("HOUSTON, WE HAVE A PROBLEM");
      gErrorMsgUserInterface.setMessage(1, "No levels were loaded.  Cannot host a game.");
      gErrorMsgUserInterface.setMessage(3, "Check the LevelDir parameter in your INI file,");
      gErrorMsgUserInterface.setMessage(4, "or your command-line parameters to make sure");
      gErrorMsgUserInterface.setMessage(5, "you have correctly specified a folder containing");
      gErrorMsgUserInterface.setMessage(6, "valid level files.");
      gErrorMsgUserInterface.setMessage(8, "Trying to load levels from folder:");
      gErrorMsgUserInterface.setMessage(9, gLevelDir.c_str());
      gErrorMsgUserInterface.activate();
   }
   delete gServerGame;
   gServerGame = NULL;

   gMainMenuUserInterface.levelLoadDisplayDisplay = false;
   gMainMenuUserInterface.levelLoadDisplayFadeTimer.clear();

   return;
}


// Host a game (and maybe even play a bit, too!)
void initHostGame(Address bindAddress, bool testMode)
{
   gServerGame = new ServerGame(bindAddress, gMaxPlayers, gHostName.c_str(), testMode);

   // Don't need to build our level list when in test mode because we're only running that one level stored in editor.tmp
   if(!testMode)
      LevelListLoader::buildLevelList();

   // Parse all levels, make sure they are in some sense valid, and record some critical parameters
   if(gLevelList.size())
   {
      gServerGame->setLevelList(gLevelList);
      gServerGame->resetLevelLoadIndex();
      gMainMenuUserInterface.levelLoadDisplayDisplay = true;
   }
   else
   {
      abortHosting();
      return;
   }

  gHostingModePhase = LoadingLevels;      // Do this even if there are no levels, so hostGame error handling will be triggered
}


void hostGame()
{
   gHostingModePhase = Hosting;
   s_logprintf("----------\nbitfighter server started %s", getTimeStamp().c_str());
   s_logprintf("hostname=[%s], hostdescr=[%s]", gServerGame->getHostName(), gServerGame->getHostDescr());
   s_logprintf("hosting %d levels:", gServerGame->getLevelNameCount());

   for(S32 i = 0; i < gServerGame->getLevelNameCount(); i++)
      s_logprintf("\t%s [%s]", gServerGame->getLevelNameFromIndex(i).getString(), gServerGame->getLevelFileNameFromIndex(i).c_str());

   if(gServerGame->getLevelNameCount())   // Levels loaded --> start game!
      gServerGame->cycleLevel(0);         // Start the first level

   else        // No levels loaded... we'll crash if we try to start a game
   {
      abortHosting();
      return;
   }

   gMainMenuUserInterface.levelLoadDisplayDisplay = false;
   gMainMenuUserInterface.levelLoadDisplayFadeTimer.reset();

   if(!gDedicatedServer)                  // If this isn't a dedicated server...
      joinGame(Address(), false, true);   // ...then we'll play, too!
      //      (let the system assign ip and port, false -> not from master, true -> local connection)
   //else
   //   printf("Bitfighter host launched.\n");     ==> Does nothing
}


// This is the master idle loop that gets registered with GLUT and is called on every game tick.
// This in turn calls the idle functions for all other objects in the game.
void idle()
{
   if(gHostingModePhase == LoadingLevels)
      gServerGame->loadNextLevel();
   else if(gHostingModePhase == DoneLoadingLevels)
      hostGame();

   checkModifierKeyState();      // Most keys are handled as events by GLUT...  but not Ctrl, Alt, Shift!
   static S64 lastTimer = Platform::getHighPrecisionTimerValue();
   static F64 unusedFraction = 0;

   S64 currentTimer = Platform::getHighPrecisionTimerValue();

   F64 timeElapsed = Platform::getHighPrecisionMilliseconds(currentTimer - lastTimer) + unusedFraction;
   U32 integerTime = U32(timeElapsed);

   if(integerTime >= 10)
   {
      lastTimer = currentTimer;
      unusedFraction = timeElapsed - integerTime;

      gZapJournal.idle(integerTime);
   }

   // So, what's with all the SDL code in here?  I looked at converting from GLUT to SDL, in order to get
   // a richer set of keyboard events.  Looks possible, but SDL appears to be missing some very handy
   // windowing code (e.g. the ability to resize or move a window) that GLUT has.  So until we find a
   // platform independent window library, we'll stick with GLUT, or maybe go to FreeGlut.
   // Note that moving to SDL will require our journaling system to be re-engineered.
   // Note too that SDL will require linking in SDL.lib and SDLMain.lib, and including the SDL.dll in the EXE folder.

   /* SDL requires an active polling loop.  We could use something like the following:
   while(SDL_PollEvent(&e))
   {
      switch(e.type)
      {
         case SDL_KEYDOWN:
            gZapJournal.keydown((S32) e.key.keysym.sym);      // Cast to S32 to ensure journaling system can cope
            break;
         case SDL_KEYUP:
            gZapJournal.keyup((S32) e.key.keysym.sym);
            break;
         case SDL_MOUSEMOTION:
            break;
         case SDL_VIDEORESIZE:
            window_resized(e.resize.w, e.resize.h);
            break;
         case SDL_QUIT:       // User closed game window
            exitGame();
            break;
      }
   }

   gZapJournal.display();    // Draw the screen --> GLUT handles this via callback, with SDL we need to do it in our main loop
   END SDL event polling */

   // Sleep a bit so we don't saturate the system. For a non-dedicated server,
   // sleep(0) helps reduce the impact of OpenGL on windows.
   U32 sleepTime = 1;

   if(gClientGame) sleepTime = 0;      // Live player at the console

   // If there are no players, set sleepTime to 40 to further reduce impact on the server.
   // We'll only go into this longer sleep on dedicated servers when there are no players.
   if(gDedicatedServer && !(gServerGame && gServerGame->getGameType() && gServerGame->getGameType()->mClientList.size()))
      sleepTime = 40;

   Platform::sleep(sleepTime);

   gZapJournal.processNextJournalEntry();    // Does nothing unless we're playing back a journal...

}  // end idle()



TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, idle, (U32 integerTime), (integerTime))
{
   if(UserInterface::current)
      UserInterface::current->idle(integerTime);

   if(gHostingModePhase != LoadingLevels)    // Don't idle games during level load
   {
      if(gClientGame)
         gClientGame->idle(integerTime);
      if(gServerGame)
         gServerGame->idle(integerTime);
   }

   if(gClientGame)
      glutPostRedisplay();
}

void dedicatedServerLoop()
{
   for(;;)        // Loop forever!
      idle();     // Idly!
}

#ifndef ZAP_DEDICATED
void GLUT_CB_display(void)
{
   gZapJournal.display();

   if(gScreenshooter.phase)      // We're in mid-shot, so be sure to visit the screenshooter!
      gScreenshooter.saveScreenshot();
}
#endif

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, display, (), ())
{
   glFlush();
   UserInterface::renderCurrent();

   // Render master connection state if we're not connected
   if(gClientGame && gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->getConnectionState() != NetConnection::Connected)
   {
      glColor3f(1, 1, 1);
      UserInterface::drawStringf(10, 550, 15, "Master Server - %s", gConnectStatesTable[gClientGame->getConnectionToMaster()->getConnectionState()]);
   }

   // Swap the buffers. This this tells the driver to render the next frame from the contents of the
   // back-buffer, and to set all rendering operations to occur on what was the front-buffer.
   // Double buffering prevents nasty visual tearing from the application drawing on areas of the
   // screen that are being updated at the same time.
   glutSwapBuffers();
   //SDL_GL_SwapBuffers();  // Use this if we convert to SDL
 }

#include <stdio.h>
// Each instnatiation of a LogConsumer subclass gets a copy of all log messages.  Here we'll log both
// to the screen as well as to a file called bitfighter.log
class StdoutLogConsumer : public LogConsumer   // Dumps logs to stdout
{
public:
   void logString(const char *string)
   {
      printf("%s", string);
   }
} gStdoutLogConsumer;


class FileLogConsumer : public LogConsumer     // Dumps logs to bitfighter.log
{
private:
   FILE *f;
public:
   FileLogConsumer(const char* logFile="bitfighter.log")
   {
      f = fopen(logFile, "w");
      logString("------ Bitfighter Log File ------\n");
   }

   ~FileLogConsumer()
   {
      if(f)
         fclose(f);
   }

   void logString(const char *string)
   {
      if(f)
      {
         fprintf(f, "%s", string);
         fflush(f);
      }
   }
} gFileLogConsumer;


//
class ServerFileLogConsumer : public LogConsumer    // Dumps logs to bitfighter.log
{
private:
   FILE *f;
public:
   ServerFileLogConsumer(const char* logFile="bitfighter_server.log")
   {
      f = fopen(logFile, "a");
      setFilterType(LogConsumer::ServerFilter);
   }

   ~ServerFileLogConsumer()
   {
      if(f)
         fclose(f);
   }

   void logString(const char *string)
   {
      if(f)
      {
         fprintf(f, "%s", string);
         fflush(f);
      }
   }
} gServerLogConsumer;


// Player has selected a game from the QueryServersUserInterface, and is ready to join
void joinGame(Address remoteAddress, bool isFromMaster, bool local)
{
   if(isFromMaster && gClientGame->getConnectionToMaster())
   {
      gClientGame->getConnectionToMaster()->requestArrangedConnection(remoteAddress);
      gGameUserInterface.activate();
   }
   else
   {
      GameConnection *theConnection = new GameConnection();
      gClientGame->setConnectionToServer(theConnection);

      // Use name specified in name entry screen, falling back to defaultName (from INI) if blank
      const char *name = gNameEntryUserInterface.getText()[0] ? gNameEntryUserInterface.getText() : gIniSettings.defaultName.c_str();

      theConnection->setClientName(name);

      theConnection->setSimulatedNetParams(gSimulatedPacketLoss, gSimulatedLag);

      if(local)   // Local client
      {
         theConnection->connectLocal(gClientGame->getNetInterface(), gServerGame->getNetInterface());
         theConnection->setIsAdmin(true);          // Local connection is always admin
         theConnection->setIsLevelChanger(true);   // Local connection can always change levels

         GameConnection *gc = dynamic_cast<GameConnection *>(theConnection->getRemoteConnectionObject());

         if(gc)                              // gc might evaluate false if a bad password was supplied to a password-protected server
         {
            gc->setIsAdmin(true);            // Set isAdmin on server
            gc->setIsLevelChanger(true);     // Set isLevelChanger on server

            gc->s2cSetIsAdmin(true);         // Set isAdmin on the client
            gc->s2cSetIsLevelChanger(true);  // Set isLevelChanger on the client
            gc->setServerName(gServerGame->getHostName());     // Server name is whatever we've set locally
         }
      }
      else        // Remote client
         theConnection->connect(gClientGame->getNetInterface(), remoteAddress);  // (method in tnlNetConnection)

      gGameUserInterface.activate();
   }
}

// Disconnect from servers and exit game in an orderly fashion
void endGame()
{
   // Disconnect from master server
   if(gClientGame && gClientGame->getConnectionToMaster())
      gClientGame->getConnectionToMaster()->cancelArrangedConnectionAttempt();

   // Disconnect from game server
   if(gClientGame && gClientGame->getConnectionToServer())
      gClientGame->getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   delete gServerGame;
   gServerGame = NULL;
}


// Run when we're quitting the game
void onExit()
{
   endGame();
   SFXObject::shutdown();
   ShutdownJoystick();

   // Save settings to capture window position
   gINI.SetValue("Settings", "WindowMode", (gIniSettings.fullscreen ? "Fullscreen" : "Window"), true);
   if(!gIniSettings.fullscreen)
   {
      gINI.SetValueI("Settings", "WindowXPos", glutGet(GLUT_WINDOW_X), true);
      gINI.SetValueI("Settings", "WindowYPos", glutGet(GLUT_WINDOW_Y), true);
   }

   gINI.WriteFile();

   NetClassRep::logBitUsage();
   TNL::logprintf("Bye!");

   exitGame();
}


// If we're running in dedicated mode, these things need to be set as such.
void setParamsForDedicatedMode()
{
   gCmdLineSettings.clientMode = false;
   gCmdLineSettings.serverMode = true;
   gDedicatedServer = true;
   gReadyToConnectToMaster = true;

   gCmdLineSettings.connectRemote = false;
}



// Read the command line params... if we're replaying a journal, we'll process those params as if they were actually there, while
// ignoring those params that were provided.
TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, readCmdLineParams, (Vector<StringPtr> argv), (argv))
{
   S32 argc = argv.size();

   // Process command line args  --> see http://bitfighter.org/wiki/index.php?title=Command_line_parameters
   for(S32 i = 0; i < argc; i+=2)
   {
      bool hasAdditionalArg = (i != argc - 1 && argv[i + 1].getString()[0] != '-');     // Assume "args" starting with "-" are actually follow-on params
      bool has2AdditionalArgs = hasAdditionalArg && (i != argc - 2);

      // Connect to a game server
      if(!stricmp(argv[i], "-connect"))       // additional arg required
      {
         if(hasAdditionalArg)
         {
            gCmdLineSettings.connectRemote = true;
            gCmdLineSettings.connect = argv[i+1];
         }
         else
         {
            logprintf("You must specify a server address to connect to with the -connect option");
            exitGame(1);
         }
      }
      // Specify a master server
      else if(!stricmp(argv[i], "-master"))        // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.masterAddress = argv[i+1];
         else
         {
            logprintf("You must specify a master server address with -master option");
            exitGame(1);
         }
      }
      // Address to use when we're hosting
      else if(!stricmp(argv[i], "-hostaddr"))       // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostaddr = argv[i+1];
         else
         {
            logprintf("You must specify a host address for the host to listen on (e.g. IP:Any:28000 or IP:192.169.1.100:5500)");
            exitGame(1);
         }
      }
      // Simulate packet loss 0 (none) - 1 (total)  [I think]
      else if(!stricmp(argv[i], "-loss"))          // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.loss = atof(argv[i+1]);
         else
         {
            logprintf("You must specify a loss rate between 0 and 1 with the -loss option");
            exitGame(1);
         }
      }
      // Simulate network lag
      else if(!stricmp(argv[i], "-lag"))           // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.lag = atoi(argv[i+1]);
         else
         {
            logprintf("You must specify a lag (in ms) with the -lag option");
            exitGame(1);
         }
      }
      // Run as a dedicated server
      else if(!stricmp(argv[i], "-dedicated"))     // additional arg optional
      {
         setParamsForDedicatedMode();

         if(hasAdditionalArg)
            gCmdLineSettings.dedicated = argv[i+1];
         else
            i--;     // Correct for the fact that we don't really have two args here...
      }
      // Specify user name
      else if(!stricmp(argv[i], "-name"))          // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.name = argv[i+1];
         else
         {
            logprintf("You must enter a nickname with the -name option");
            exitGame(1);
         }
      }
      // Specify password for accessing locked ser vers
      else if(!stricmp(argv[i], "-password"))      // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.password = argv[i+1];
         else
         {
            logprintf("You must enter a password with the -password option");
            exitGame(1);
         }
      }
      // Specify admin password for server
      else if(!stricmp(argv[i], "-adminpassword")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.adminPassword = argv[i+1];
         else
         {
            logprintf("You must specify an admin password with the -adminpassword option");
            exitGame(1);
         }
      }
      // Specify level change password for server
      else if(!stricmp(argv[i], "-levelchangepassword")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.levelChangePassword = argv[i+1];
         else
         {
            logprintf("You must specify an level-change password with the -levelchangepassword option");
            exitGame(1);
         }
      }
      // Specify to include all levels in levels folder -- not really needed any more, but can be used as a shortcut to tell game to ignore levels in INI file...
      else if(!stricmp(argv[i], "-alllevels"))     // no additional args
      {
         i--;  // compentsate for +=2 in for loop with single param
         gCmdLineSettings.alllevels = true;
      }
      // Read all levels in the specified subfolder
      else if(!stricmp(argv[i], "-leveldir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify a levels subfolder with the -leveldir option");
            exitGame(1);
         }

         gCmdLineSettings.levelDir = argv[i+1].getString();
      }

      // Specify list of levels...  all remaining params will be taken as level names
      else if(!stricmp(argv[i], "-levels"))     // additional arg(s) required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify one or more levels to load with the -levels option");
            exitGame(1);
         }

         // We'll overwrite our main level list directly, so if we're writing the INI for the first time,
         // we'll use the cmd line args to generate the INI Level keys, rather than the built-in defaults.
         for(S32 j = i+1; j < argc; j++)
            gCmdLineSettings.specifiedLevels.push_back(StringTableEntry(argv[j]));

         return;     // This param must be last, so no more args to process.  We can return.

      }
      // Specify name of the server as others will see it from the Join menu
      else if(!stricmp(argv[i], "-hostname"))   // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostname = argv[i+1];
         else
         {
            logprintf("You must specify a server name with the -hostname option");
            exitGame(1);
         }
      }
      else if(!stricmp(argv[i], "-hostdescr"))   // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostdescr = argv[i+1];
         else
         {
            logprintf("You must specify a description (use quotes) with the -hostdescr option");
            exitGame(1);
         }
      }
      // Change max players on server
      else if(!stricmp(argv[i], "-maxplayers")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.maxplayers = atoi(argv[i+1]);
         else
         {
            logprintf("You must specify the max number of players on your server with the -maxplayers option");
            exitGame(1);
         }
      }
      // Start in window mode
      else if(!stricmp(argv[i], "-window"))     // no additional args
      {
         i--;  // compentsate for +=2 in for loop with single param
         gCmdLineSettings.window = true;
      }
      // Start in fullscreen mode
      else if(!stricmp(argv[i], "-fullscreen")) // no additional args
      {
         i--;
         gCmdLineSettings.fullscreen = true;
      }
      // Specify position of window
      else if(!stricmp(argv[i], "-winpos"))     // 2 additional args required
      {
         if(has2AdditionalArgs)
         {
            gCmdLineSettings.xpos = atoi(argv[i+1]);
            gCmdLineSettings.ypos = atoi(argv[i+2]);
            i++;  // compentsate for +=2 in for loop with single param (because we're grabbing two)
         }
         else
         {
            logprintf("You must specify the x and y position of the window with the -winpos option");
            exitGame(1);
         }
      }
      // Specify width of the game window
      else if(!stricmp(argv[i], "-winwidth")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.winWidth = atoi(argv[i+1]);
         else
         {
            logprintf("You must specify the width of the game window with the -winwidth option");
            exitGame(1);
         }
      }
      else if(!stricmp(argv[i], "-help"))       // no additional args
      {
         i--;
         logprintf("See http://bitfighter.org/wiki/index.php?title=Command_line_parameters for information");
         exitGame(0);
      }
      // Highly speculative use of different joysticks
      else if(!stricmp(argv[i], "-usestick")) // additional arg required
      {
         if(hasAdditionalArg)
            gUseStickNumber = atoi(argv[i+1]);           /////////////////////////////////////////  TODO: should be part of gCmdLineSettings
         else
         {
            logprintf("You must specify the joystick you want to use with the -usestick option");
            exitGame(1);
         }
      }
   }

// Override some settings if we're compiling ZAP_DEDICATED
#ifdef ZAP_DEDICATED
   setParamsForDedicatedMode();
#endif
}

/*
void InitSdlVideo()
{
   // Information about the current video settings.
   const SDL_VideoInfo* info = NULL;

   // Flags we will pass into SDL_SetVideoMode.
   S32 flags = 0;

   // First, initialize SDL's video subsystem.
   if (SDL_Init(SDL_INIT_VIDEO) < 0)
   {
       // Failed, exit.
       logprintf("SDL Video initialization failed: %s", SDL_GetError( ));
       exitGame();
   }

   // Let's get some video information.
   info = SDL_GetVideoInfo( );

   if( !info ) {
       // This should probably never happen.
       logprintf("SDL Video query failed: %s", SDL_GetError());
       exitGame();
   }

   // We get the bpp we will request from
   // the display. On X11, VidMode can't change
   // resolution, so this is probably being overly
   // safe. Under Win32, ChangeDisplaySettings
   // can change the bpp.

   gBPP = info->vfmt->BitsPerPixel;

   // Now, we want to setup our requested
   // window attributes for our OpenGL window.
   // We want *at least* 5 bits of red, green
   // and blue. We also want at least a 16-bit
   // depth buffer.
   //
   // The last thing we do is request a double
   // buffered window. '1' turns on double
   // buffering, '0' turns it off.
   //
   // Note that we do not use SDL_DOUBLEBUF in
   // the flags to SDL_SetVideoMode. That does
   // not affect the GL attribute state, only
   // the standard 2D blitting setup.

   SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
   SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

   // We want to request that SDL provide us with an OpenGL window, possibly in a fullscreen video mode.
   // Note the SDL_DOUBLEBUF flag is not required to enable double buffering when setting an OpenGL
   // video mode. Double buffering is enabled or disabled using the SDL_GL_DOUBLEBUFFER attribute.
   flags = SDL_OPENGL | SDL_RESIZABLE ; // | SDL_FULLSCREEN;


   if(SDL_SetVideoMode(gScreenWidth, gScreenHeight, gBPP, flags ) == 0)
   {
      // This could happen for a variety of reasons,
      // including DISPLAY not being set, the specified
      // resolution not being available, etc.

      logprintf("SDL Video mode set failed: %s", SDL_GetError());
      exitGame();
   }

   SDL_WM_SetCaption(gWindowTitle, "Icon XXX");    // TODO: Fix icon here
}
*/


// Basically checks if the folder base exists, and if not, makes it a subdir of levels
// Typos on the user's part can lead to hilarity!
string getLevelsFolder(string base)
{
   // See if levelsFolder could refer to a standalone folder (rather than a subfolder of gLevelDir)
   struct stat st;
   if(stat(base.c_str(), &st) != 0 )
      return gLevelDir + "/" + base;      // It doesn't
   else
      return base;                        // It does
}


// Now integrate INI settings with those from the command line and process them
void processStartupParams()
{
   // These options can only be set on cmd line
   if(!gCmdLineSettings.server.empty())
      gBindAddress.set(gCmdLineSettings.server);

   if(!gCmdLineSettings.dedicated.empty())
      gBindAddress.set(gCmdLineSettings.dedicated);

   if(gCmdLineSettings.connect != "")
      gConnectAddress.set(gCmdLineSettings.connect);

   gSimulatedPacketLoss = gCmdLineSettings.loss;
   gSimulatedLag = gCmdLineSettings.lag;

   // Enable some logging...
   TNLLogEnable(LogConnectionProtocol, gIniSettings.logConnectionProtocol);
   TNLLogEnable(LogNetConnection, gIniSettings.logNetConnection);
   TNLLogEnable(LogEventConnection, gIniSettings.logEventConnection);
   TNLLogEnable(LogGhostConnection, gIniSettings.logGhostConnection);

   TNLLogEnable(LogNetInterface, gIniSettings.logNetInterface);
   TNLLogEnable(LogPlatform, gIniSettings.logPlatform);
   TNLLogEnable(LogNetBase, gIniSettings.logNetBase);
   TNLLogEnable(LogUDP, gIniSettings.logUDP);


   // These options can come either from cmd line or INI file
   if(gCmdLineSettings.name != "")
      gNameEntryUserInterface.setText(gCmdLineSettings.name.c_str());
   else if(gIniSettings.name != "")
      gNameEntryUserInterface.setText(gIniSettings.name.c_str());
   else
      gNameEntryUserInterface.setText(gIniSettings.lastName.c_str());


   if(gCmdLineSettings.password != "")
      gServerPassword = gCmdLineSettings.password.c_str();
   else if(gIniSettings.password != "")
      gServerPassword = gIniSettings.password.c_str();
   // else rely on gServerPassword default of ""

   if(gCmdLineSettings.adminPassword != "")
      gAdminPassword = gCmdLineSettings.adminPassword.c_str();
   else if(gIniSettings.adminPassword != "")
      gAdminPassword = gIniSettings.adminPassword.c_str();
   // else rely on gAdminPassword default of ""   i.e. no one can do admin tasks on the server

   if(gCmdLineSettings.levelChangePassword != "")
      gLevelChangePassword = gCmdLineSettings.levelChangePassword.c_str();
   else if(gIniSettings.levelChangePassword != "")
      gLevelChangePassword = gIniSettings.levelChangePassword.c_str();
   // else rely on gLevelChangePassword default of ""   i.e. no one can change levels on the server

   if(gIniSettings.levelDir != "")
      gLevelDir = gIniSettings.levelDir;
   else
      gIniSettings.levelDir = gLevelDir;     // So a good default will be written to the INI

   // This way, the main level dir can be specified in the INI, but it can either be overridden here,
   // or a subfolder can be specified, depending on what's in the leveldir param
   if(gCmdLineSettings.levelDir != "")
      gLevelDir = getLevelsFolder(gCmdLineSettings.levelDir);
   else
      gLevelDir = getLevelsFolder(gIniSettings.levelDir);
   // else leave gLevelDir at it's default setting, "levels"

   if(gCmdLineSettings.hostname != "")
      gHostName = gCmdLineSettings.hostname;
   else
      gHostName = gIniSettings.hostname;

   if(gCmdLineSettings.hostdescr != "")
      gHostDescr = gCmdLineSettings.hostdescr;
   else
      gHostDescr = gIniSettings.hostdescr;

   if(gCmdLineSettings.hostaddr != "")
      gBindAddress.set(gCmdLineSettings.hostaddr);
   else if(gIniSettings.hostaddr != "")
      gBindAddress.set(gIniSettings.hostaddr);
   // else stick with default defined earlier

   U32 maxplay;
   if (gCmdLineSettings.maxplayers > 0)
      maxplay = gCmdLineSettings.maxplayers;
   else
      maxplay = gIniSettings.maxplayers;
   if (maxplay < 0 || maxplay > 128)
      maxplay = 128;
   gMaxPlayers = (U32) maxplay;


   if(gCmdLineSettings.fullscreen)
      gIniSettings.fullscreen = true;    // Simply clobber the gINISettings copy
   else if(gCmdLineSettings.window)
      gIniSettings.fullscreen = false;

   if(gCmdLineSettings.xpos != -9999)
      gIniSettings.winXPos = gCmdLineSettings.xpos;
   if(gCmdLineSettings.ypos != -9999)
      gIniSettings.winYPos = gCmdLineSettings.ypos;
   if(gCmdLineSettings.winWidth > 0)
      gIniSettings.winSizeFact = max((F32) gCmdLineSettings.winWidth / (F32) UserInterface::canvasWidth, 0.15f);

   if(gCmdLineSettings.masterAddress != "")
      gMasterAddress.set(gCmdLineSettings.masterAddress);
   else
      gMasterAddress.set(gIniSettings.masterAddress);    // This will always have a value

   if(gCmdLineSettings.name != "")                       // We'll clobber the INI file setting.  Since this
      gIniSettings.name = gCmdLineSettings.name;         // setting is never saved, we won't mess up our INI


   // Note that we can be in both clientMode and serverMode (such as when we're hosting a game interactively)

   if(gCmdLineSettings.clientMode)               // Create ClientGame object
      gClientGame = new ClientGame(Address());   //   let the system figure out IP address and assign a port

   //LevelListLoader::buildLevelList();            // Get our level list squared away before we jump off to initHostGame() if that's what's going to happen

   if(gCmdLineSettings.serverMode)
      initHostGame(gBindAddress, false);         // Start hosting
   else if(gCmdLineSettings.connectRemote)       //       or
      joinGame(gConnectAddress, false, false);   // Connect to a game server (i.e. bypass master matchmaking)


   // Not immediately starting a connection...  start out with name entry or main menu
   if(!gCmdLineSettings.connectRemote && !gDedicatedServer)
   {
      if(gIniSettings.name == "")
         gNameEntryUserInterface.activate();
      else
      {
         gMainMenuUserInterface.activate();
         gReadyToConnectToMaster = true;         // Set elsewhere if in dedicated server mode
      }
   }
}



};  // namespace Zap


using namespace Zap;



#ifdef TNL_OS_XBOX
int zapmain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef TNL_OS_MAC_OSX
   // Move to the application bundle's path (RDW)
   moveToAppPath();
#endif

   gCmdLineSettings.init();      // Init cmd line settings struct
   gIniSettings.init();          // Init struct that holds INI settings

   //setDefaultLevelList();        // Levels we'll play, unless we're told otherwise

   Vector<TNL::StringPtr> theArgv;

   // Process some command line args that need to be handled early, like journaling options
   for(S32 i = 1; i < argc; i++)
   {
      //if(!stricmp(argv[i], "-createsampleini")) // Create sample INI file and exit
      //{
      //   gINI.Path("bitfighter.ini.sample");
      //   saveSettingsToINI();
      //   logprintf("Wrote bitfighter.ini.sample, for version %s/%d", ZAP_GAME_RELEASE, BUILD_VERSION);
      //   exitGame(0);
      //}
      if(!stricmp(argv[i], "-rules"))            // Print current rules and exit
      {
         GameType::printRules();
         exitGame(0);
      }
      else if(!stricmp(argv[i], "-jsave"))      // Write game to journal
      {
         if(i != argc - 1)
         {
            gZapJournal.record(argv[i+1]);
            i++;
         }
      }
      else if(!stricmp(argv[i], "-jplay"))      // Replay game from journal
      {
         if(i != argc - 1)
         {
            gZapJournal.load(argv[i+1]);
            i++;
         }
      }
      else
         theArgv.push_back(argv[i]);
   }  // End processing command line args

   gZapJournal.readCmdLineParams(theArgv);   // Process normal command line params, read INI, and start up
   gZapJournal.processNextJournalEntry();    // If we're replaying a journal, this will cause the cmd line params to be read from the saved journal

   gHostingModePhase = NotHosting;

   loadSettingsFromINI();                    // Read INI

   processStartupParams();                   // And process command lines and INI settings in a unified way

   SFXObject::init();

#ifndef ZAP_DEDICATED
   if(gClientGame)     // That is, we're starting up in interactive mode, as opposed to running a dedicated server
   {
      FXManager::init();                     // Get ready for sparks!!  C'mon baby!!
      InitJoystick();
      resetKeyStates();                      // Reset keyboard state mapping to show no keys depressed
      gAutoDetectedJoystickType = autodetectJoystickType();
      gOptionsMenuUserInterface.setJoystick(gAutoDetectedJoystickType);     // Will override INI settings, so process INI first

      glutInitWindowSize(gScreenWidth, gScreenHeight);                      // Does this actually do anything?  Seem to get same result, regardless of params!
      glutInit(&argc, argv);


      // On OS X, glutInit changes the working directory to the app
      // bundle's resource directory.  We don't want that. (RDW)
#ifdef TNL_OS_MAC_OSX
      moveToAppPath();
#endif
      // InitSdlVideo();      // Get our main SDL rendering window all set up
      // SDL_ShowCursor(0);   // Hide cursor

      glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
      glutCreateWindow(gWindowTitle);

      // Register keyboard/mouse event handlers -- see GLUT docs for details
      glutDisplayFunc(GLUT_CB_display);        // Called when GLUT thinks display needs to be redrawn
      glutReshapeFunc(GLUT_CB_reshape);        // Handle window reshape events
      glutPassiveMotionFunc(GLUT_CB_passivemotion);  // Handle mouse motion when button is not pressed
      glutMotionFunc(GLUT_CB_motion);          // Handle mouse motion when button is pressed
      glutKeyboardFunc(GLUT_CB_keydown);       // Handle key-down events for regular keys
      glutKeyboardUpFunc(GLUT_CB_keyup);       // Handle key-up events for regular keys
      glutSpecialFunc(GLUT_CB_specialkeydown); // Handle key-down events for special keys
      glutSpecialUpFunc(GLUT_CB_specialkeyup); // Handle key-up events for special keys
      glutMouseFunc(GLUT_CB_mouse);            // Handle mouse-clicks

      glutIdleFunc(idle);                      // Register our idle function.  This will get run whenever GLUT is idling
      glutSetCursor(GLUT_CURSOR_NONE);         // Turn off the cursor -- we'll turn this back on in the editor, or when the user tries to use mouse to work menus

      glMatrixMode(GL_PROJECTION);
      glOrtho(0, gScreenWidth, gScreenHeight, 0, 0, 1);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(gScreenWidth/2, gScreenHeight/2, 0);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glLineWidth(gDefaultLineWidth);

      atexit(onExit);
      gOptionsMenuUserInterface.actualizeScreenMode(true);     // Create a display window

      glutMainLoop();         // Launch GLUT on it's merry way.  It'll call back with events and when idling.
      // dedicatedServerLoop();  //    Instead, with SDL, loop forever, running the idle command endlessly

   }
   else                       // We're running a dedicated server so...
#endif
      dedicatedServerLoop();  //    loop forever, running the idle command endlessly

   return 0;
}

