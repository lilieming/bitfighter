//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelFilesForTesting.h"
#include "TestUtils.h"

#include "ChatHelper.h"
#include "ClientGame.h"
#include "gameType.h"
#include "ServerGame.h"
#include "UIManager.h"
#include "UIMenus.h"

#include "tnlNetConnection.h"

#include "gtest/gtest.h"

using namespace TNL;
using namespace std;

namespace Zap {

   void letTeamChangeAbuseTimerExpire(GamePair &gamePair)
   {
      gamePair.idle(ServerGame::MaxTimeDelta, GameType::SwitchTeamsDelay / ServerGame::MaxTimeDelta + 1);
   }


   S32 countAdmins(GamePair &gamePair)
   {
      S32 ct = 0;
      for(S32 i = 0; i < gamePair.getClientCount(); i++)
         if(gamePair.getClient(i)->hasAdmin())
            ct++;
      return ct;
   }


   S32 findFirstAdmin(GamePair &gamePair)
   {
      for(S32 i = 0; i < gamePair.getClientCount(); i++)
         if(gamePair.getClient(i)->hasAdmin())
            return i;
      return -1;
   }


   TEST(TeamChangingTests, testTeamLocking)
   {
      GamePair gamePair(getLevelCodeForItemPropagationTests(""), 2); // This level has 2 teams; good for testing team changing

      ServerGame *server = gamePair.server;
      ClientInfo *info0 = gamePair.getClient(0)->getClientInfo();
      ClientInfo *info1 = gamePair.getClient(1)->getClientInfo();

      // Clients can't demote themselves, so we'll have to force this on the server
      server->getClientInfo(0)->setRole(ClientInfo::RoleNone);
      server->getClientInfo(1)->setRole(ClientInfo::RoleNone);
      gamePair.idle(5, 5);
      ASSERT_FALSE(info0->isAdmin());
      ASSERT_FALSE(info1->isAdmin());

      S32 team0 = gamePair.getClient(0)->getCurrentTeamIndex();
      S32 team1 = gamePair.getClient(1)->getCurrentTeamIndex();

      ASSERT_NE(team0, team1) << "Players start on different teams";

      gamePair.getClient(0)->changeOwnTeam(team1);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team1) << "Players now on same team";

      letTeamChangeAbuseTimerExpire(gamePair);
      gamePair.getClient(0)->changeOwnTeam(team0);
      gamePair.getClient(1)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      ASSERT_EQ(team0, gamePair.getClient(0)->getCurrentTeamIndex()) << "Players now on same team";
      ASSERT_EQ(team0, gamePair.getClient(1)->getCurrentTeamIndex()) << "Players now on same team";

      letTeamChangeAbuseTimerExpire(gamePair);
      gamePair.getClient(1)->changeOwnTeam(team1);
      gamePair.idle(5, 5);
      ASSERT_EQ(team0, gamePair.getClient(0)->getCurrentTeamIndex());
      ASSERT_EQ(team1, gamePair.getClient(1)->getCurrentTeamIndex());

      ///// Ok, teams now back to their starting position.  Try locking teams and ensuring players can't change.
      server->setTeamsLocked(true);
      letTeamChangeAbuseTimerExpire(gamePair);
      gamePair.getClient(0)->changeOwnTeam(team1);
      gamePair.getClient(1)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      EXPECT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "Player should not have changed teams when locked";
      EXPECT_EQ(gamePair.getClient(1)->getCurrentTeamIndex(), team1);
      // Try to bypass client-side checks and "hack" server
      gamePair.getClient(0)->getGameType()->c2sChangeTeams(team1);
      gamePair.getClient(1)->getGameType()->c2sChangeTeams(team0);
      gamePair.idle(5, 5);
      EXPECT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "Player should not have changed teams when locked";
      EXPECT_EQ(gamePair.getClient(1)->getCurrentTeamIndex(), team1);

      EXPECT_TRUE(gamePair.getClient(0)->areTeamsLocked());
      EXPECT_TRUE(gamePair.getClient(1)->areTeamsLocked());

      ///// New player joins while teams are locked... he should see that teams are locked and not be able to change.
      string newPlayerName = "New Player";
      ClientGame *newClient = gamePair.addClient(newPlayerName, team0);
      ClientInfo *newInfo = newClient->getClientInfo();
      gamePair.idle(5, 5);
      EXPECT_EQ(team0, newClient->getCurrentTeamIndex());
      newClient->changeOwnTeam(team1);
      gamePair.idle(5, 5);
      EXPECT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "Should not have changed teams... teams are locked";
      EXPECT_TRUE(newClient->areTeamsLocked());

      ///// Player quits, then rejoins -- should be reassigned to old team
      EXPECT_TRUE(server->areTeamsLocked());
      gamePair.idle(5, 5);
      EXPECT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0);
      EXPECT_EQ(gamePair.getClient(1)->getCurrentTeamIndex(), team1);
      ASSERT_NE(team0, team1);
      ASSERT_EQ(3, server->getClientCount());
      gamePair.getClient(0)->closeConnectionToGameServer();
      gamePair.getClient(1)->closeConnectionToGameServer();
      gamePair.idle(5, 5);
      ASSERT_EQ(1, server->getClientCount());
      gamePair.addClient(gamePair.getClient(1));
      gamePair.idle(5, 5);
      gamePair.addClient(gamePair.getClient(0));
      gamePair.idle(5, 5);
      ASSERT_EQ(3, server->getClientCount());
      EXPECT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0);
      EXPECT_EQ(gamePair.getClient(1)->getCurrentTeamIndex(), team1);
      // Retry same thing, but readd clients in a different order -- should get same result

      // Pressing escape should disconnect the game, and drop back to the main menu interface
      gamePair.getClient(0)->getUIManager()->getCurrentUI()->onKeyDown(KEY_ESCAPE);
      gamePair.getClient(1)->getUIManager()->getCurrentUI()->onKeyDown(KEY_ESCAPE);

      gamePair.getClient(0)->closeConnectionToGameServer();
      gamePair.getClient(1)->closeConnectionToGameServer();
      gamePair.idle(5, 5);
      ASSERT_EQ(1, server->getClientCount());
      gamePair.addClient(gamePair.getClient(0));
      gamePair.idle(5, 5);
      gamePair.addClient(gamePair.getClient(1));
      gamePair.idle(5, 5);
      ASSERT_EQ(3, server->getClientCount());
      EXPECT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0);
      EXPECT_EQ(gamePair.getClient(1)->getCurrentTeamIndex(), team1);

      ///// Unlock teams again -- everyone can change
      server->setTeamsLocked(false);
      letTeamChangeAbuseTimerExpire(gamePair);
      EXPECT_FALSE(newClient->areTeamsLocked());
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "We should be on team0";
      gamePair.getClient(0)->changeOwnTeam(team1);
      letTeamChangeAbuseTimerExpire(gamePair);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team1);
      gamePair.getClient(0)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "Back on team0";

      ///// Neither player is an admin, so neither can lock teams themselves
      // Somewhere along the way, order of clients on the server got out of whack... it's not a problem, but
      // we need to demote all the players to ensure we've really nailed the first two, which we need for the 
      // subsequent tests.
      for(S32 i = 0; i < gamePair.getClientCount(); i++)
         server->getClientInfo(i)->setRole(ClientInfo::RoleLevelChanger);  // Not quite admin!
      gamePair.idle(5, 5);
      ASSERT_EQ(0, countAdmins(gamePair));
      ASSERT_FALSE(server->areTeamsLocked());

      gamePair.runChatCmd(0, "/lockteams");   // User chat command
      gamePair.idle(5, 5);
      ASSERT_FALSE(server->areTeamsLocked());
      EXPECT_FALSE(gamePair.getClient(0)->areTeamsLocked()) << "Set should have failed, client should still see teams as unlocked";
      EXPECT_FALSE(gamePair.getClient(1)->areTeamsLocked());

      // Make player an admin
      server->getClientInfo(0)->setRole(ClientInfo::RoleAdmin);
      gamePair.idle(5, 5);

      S32 admin = findFirstAdmin(gamePair);
      EXPECT_NE(-1, admin) << "Couldn't find our admin!";
      gamePair.runChatCmd(admin, "/lockteams");   // User chat command
      gamePair.idle(5, 5);
      EXPECT_TRUE(server->areTeamsLocked());
      EXPECT_TRUE(gamePair.getClient(0)->areTeamsLocked());
      EXPECT_TRUE(gamePair.getClient(1)->areTeamsLocked());

      // Demote player
      server->getClientInfo(0)->setRole(ClientInfo::RoleLevelChanger);
      gamePair.idle(5, 5);
      EXPECT_FALSE(gamePair.getClient(admin)->hasAdmin());
      gamePair.runChatCmd(0, "/unlockteams");   // User chat command
      gamePair.idle(5, 5);
      EXPECT_TRUE(server->areTeamsLocked()) << "Unlock should not have worked -- player has no privs";
      EXPECT_TRUE(gamePair.getClient(0)->areTeamsLocked());
      EXPECT_TRUE(gamePair.getClient(1)->areTeamsLocked());

      // Promote player again, and retry
      server->getClientInfo(0)->setRole(ClientInfo::RoleOwner);
      gamePair.idle(5, 5);
      EXPECT_TRUE(gamePair.getClient(admin)->hasAdmin());
      gamePair.runChatCmd(admin, "/unlockteams");   // User chat command
      gamePair.idle(5, 5);
      EXPECT_FALSE(server->areTeamsLocked()) << "Unlock should have worked -- player is admin";
      EXPECT_FALSE(gamePair.getClient(0)->areTeamsLocked()) << "Clients should see it too";
      EXPECT_FALSE(gamePair.getClient(1)->areTeamsLocked());

      // Relock, and demote player 0
      gamePair.runChatCmd(admin, "/lockteams");   // User chat command
      gamePair.idle(5, 5);
      server->getClientInfo(0)->setRole(ClientInfo::RoleNone);
      gamePair.idle(5, 5);

      ///// Admin quits and quickly rejoins -- teams should not unlock automatically (15s grace period)
      ASSERT_EQ(-1, findFirstAdmin(gamePair)) << "Expect no admins in game at the moment";
      server->getClientInfo(2)->setRole(ClientInfo::RoleAdmin);
      gamePair.idle(5, 5);
      admin = findFirstAdmin(gamePair);
      EXPECT_EQ(1, countAdmins(gamePair)) << "Now we have an admin";
      EXPECT_TRUE(server->areTeamsLocked());

      // We have one admin, server is locked... when suddenly...
      // Admin quits.  No admins in game for 15 seconds == teams unlock
      S32 oldTeam = gamePair.getClient(admin)->getCurrentTeamIndex();
      string adminPlayerName = gamePair.getClient(admin)->getClientInfo()->getName().getString();
      ASSERT_TRUE(gamePair.getClient(admin)->hasAdmin());
      //gamePair.getClient(admin)->closeConnectionToGameServer();
      gamePair.removeClient(admin);
      S32 time = FIVE_SECONDS;
      gamePair.idle(100, time / 100);    // 5 seconds
      ASSERT_LT(time, +TeamHistoryManager::LockedTeamsNoAdminsGracePeriod) << "Tests only work if this is true... time = time idling since admin quit";
      EXPECT_EQ(0, countAdmins(gamePair)) << "Our admin quit, so we should have none in the game";

      // Admin rejoins
      newClient = gamePair.addClient(adminPlayerName);     // He's baaaack!
      newInfo = newClient->getClientInfo();
      gamePair.idle(5, 5);
      ASSERT_TRUE(newClient->hasAdmin()) << "With GamePair, players join as Owners";
      EXPECT_TRUE(server->areTeamsLocked()) << "Teams should still be locked";
      EXPECT_EQ(oldTeam, newClient->getCurrentTeamIndex()) << "Should have rejoined same team (teams are locked, after all!)";

      ///// Admin quits and stays quitted -- teams should unlock automatically after 60 seconds      
      ASSERT_EQ(1, countAdmins(gamePair));
      admin = findFirstAdmin(gamePair);
      gamePair.runChatCmd(0, "/lockteams");   // User chat command
      gamePair.idle(5, 5);
      EXPECT_TRUE(server->areTeamsLocked());

      // Admin quits
      gamePair.removeClient(admin);
      gamePair.idle(500, 50);    // 25 seconds
      ASSERT_EQ(0, countAdmins(gamePair));
      EXPECT_EQ(2, server->getClientInfos()->size());
      gamePair.idle(1000, 40);   // 40 more seconds

      // Teams should be unlocked, and clients should know about it
      EXPECT_FALSE(server->areTeamsLocked());
      gamePair.idle(5, 5);
      for(S32 i = 0; i < gamePair.getClientCount(); i++)
         EXPECT_FALSE(gamePair.getClient(i)->areTeamsLocked()) << "Client " + itos(i) + " thinks teams are locked";

      ASSERT_GT(65 * 1000, +TeamHistoryManager::LockedTeamsForgetClientTime) << "Tests only work if this is true... 65 = time idling since admin quit";

      ///// Admin quits, but another admin quickly joins... teams stay locked
      // TODO: Test this!

      ///// Verify that admins can reshuffle teams while they are locked (need admin perms to use shuffle command)
      // Promote first player to admin
      EXPECT_EQ(0, countAdmins(gamePair));
      admin = 0;
      gamePair.server->getSettings()->setAdminPassword("admin_pw", false);    // Make sure we have a PW
      string pwcmd = "/password " + gamePair.server->getSettings()->getAdminPassword();
      gamePair.runChatCmd(admin, pwcmd.c_str());
      gamePair.idle(5, 5);
      ASSERT_TRUE(gamePair.getClient(admin)->hasAdmin());
      EXPECT_EQ(1, countAdmins(gamePair));
      gamePair.runChatCmd(admin, "/lockteams");
      gamePair.idle(5, 5);
      EXPECT_TRUE(server->areTeamsLocked());
      ASSERT_TRUE(gamePair.getClient(admin)->getConnectionToServer()) << "Needs to be true in shuffle cmd";

      gamePair.runChatCmd(admin, "/shuffle");

      // Press space to shuffle
      // Press enter to accept
      gamePair.idle(5, 5);
      // 
      // TODO: Figure this out
      // All players quit -- game unlocks immediately
      server->getClientInfo(0)->setRole(ClientInfo::RoleAdmin);
      gamePair.idle(5, 5);
      EXPECT_EQ(1, countAdmins(gamePair));
      admin = findFirstAdmin(gamePair);
      gamePair.runChatCmd(admin, "/lockteams");   // User chat command, requires admin privs
      gamePair.idle(5, 5);
      EXPECT_TRUE(server->areTeamsLocked());
      gamePair.removeAllClients();
      gamePair.idle(5, 5);
      EXPECT_FALSE(server->areTeamsLocked());
      // Adding another player should change nothing
      gamePair.addClient("Last");
      gamePair.idle(5, 5);
      EXPECT_FALSE(server->areTeamsLocked());
   }


   TEST(TeamChangingTests, TestTeamRememberingAndPlacement)
   {
      // Create game with 3 teams; assign two players to same team
      string levelCode = getMultiTeamLevelCode(3);
      GamePair gamePair = GamePair(levelCode, 2);
      ServerGame *server = gamePair.server;

      gamePair.getClient(0)->changeOwnTeam(1);
      gamePair.idle(5, 5);
      ASSERT_EQ(2, server->getClientInfos()->size());
      ASSERT_EQ(1, server->getClientInfos()->get(0)->getTeamIndex());      // Both players on team 1
      ASSERT_EQ(1, server->getClientInfos()->get(1)->getTeamIndex());

      // Change game -- verify that players changed teams
      gamePair.runChatCmd(0, "/restart");
      gamePair.idle(5, 5);
      // Not sure which team players will be on, but should not be on same team
      ASSERT_NE(server->getClientInfos()->get(0)->getTeamIndex(), server->getClientInfos()->get(1)->getTeamIndex());

      // Reset players to same team, lock teams
      gamePair.getClient(0)->changeOwnTeam(2);
      gamePair.getClient(1)->changeOwnTeam(2);
      gamePair.idle(5, 5);
      ASSERT_EQ(2, server->getClientInfos()->size());
      ASSERT_EQ(2, server->getClientInfos()->get(0)->getTeamIndex());
      ASSERT_EQ(2, server->getClientInfos()->get(1)->getTeamIndex());
      gamePair.runChatCmd(0, "/lockteams");
      gamePair.idle(5, 5);
      ASSERT_TRUE(server->areTeamsLocked());

      // Change game -- are teams the same?  Should be!
      gamePair.runChatCmd(0, "/restart");
      gamePair.idle(5, 5);
      ASSERT_EQ(2, server->getClientInfos()->get(0)->getTeamIndex());
      ASSERT_EQ(2, server->getClientInfos()->get(1)->getTeamIndex());
   }

   TEST(TeamChangingTests, TestTeamRememberingAndPlacement2)
   {
      ///// Make sure players stay on their teams when teams are locked and changing to levels with different # teams:
      //    Put player on each team, and 3 on one team (unnatural config)
      Vector<string> levelCodes;
      levelCodes.push_back(getMultiTeamLevelCode(3));
      levelCodes.push_back(getMultiTeamLevelCode(2));

      GamePair gamePair = GamePair(levelCodes, 5);
      ServerGame *server = gamePair.server;
      ASSERT_EQ(3, server->getTeamCount());
      gamePair.getClient(0)->changeOwnTeam(0);
      gamePair.getClient(1)->changeOwnTeam(1);
      gamePair.getClient(2)->changeOwnTeam(1);
      gamePair.getClient(3)->changeOwnTeam(1);
      gamePair.getClient(4)->changeOwnTeam(2);
      gamePair.idle(5, 5);
      gamePair.runChatCmd(0, "/lockteams");
      gamePair.idle(5, 5);
      server->countTeamPlayers();
      ASSERT_EQ(1, server->getTeam(0)->getPlayerCount());      // Just to be sure!
      ASSERT_EQ(3, server->getTeam(1)->getPlayerCount());
      ASSERT_EQ(1, server->getTeam(2)->getPlayerCount());
      ASSERT_TRUE(server->areTeamsLocked());
      // Set players such that player on team 2 is weakest
      server->getClientInfo(4)->addDeath();
      // Now change game to level with just two teams -- player on team 3 should now be on team 2
      gamePair.runChatCmd(0, "/next");
      gamePair.idle(5, 5);
      ASSERT_EQ(2, server->getTeamCount());
      ASSERT_TRUE(server->areTeamsLocked());
      // Switch back to level with 3 teams
      gamePair.runChatCmd(0, "/prev");
      gamePair.idle(5, 5);
      ASSERT_EQ(3, server->getTeamCount());
      ASSERT_TRUE(server->areTeamsLocked());
      // Should be back to original config
      server->countTeamPlayers();
      ASSERT_EQ(1, server->getTeam(0)->getPlayerCount());
      ASSERT_EQ(3, server->getTeam(1)->getPlayerCount());
      ASSERT_EQ(1, server->getTeam(2)->getPlayerCount());
      // Restart level; team config should not change
      gamePair.runChatCmd(0, "/restart");
      gamePair.idle(5, 5);
      server->countTeamPlayers();
      ASSERT_EQ(3, server->getTeamCount());
      ASSERT_EQ(1, server->getTeam(0)->getPlayerCount());
      ASSERT_EQ(3, server->getTeam(1)->getPlayerCount());
      ASSERT_EQ(1, server->getTeam(2)->getPlayerCount());

      // Unlock teams, restart level; team config should change
      gamePair.runChatCmd(0, "/unlockteams");
      gamePair.idle(5, 5);
      gamePair.runChatCmd(0, "/restart");
      gamePair.idle(5, 5);
      server->countTeamPlayers();
      ASSERT_FALSE(server->areTeamsLocked());
      ASSERT_EQ(3, server->getTeamCount());
      ASSERT_EQ(2, server->getTeam(0)->getPlayerCount()) << "Expect different config than before";
      ASSERT_EQ(2, server->getTeam(1)->getPlayerCount());
      ASSERT_EQ(1, server->getTeam(2)->getPlayerCount());

      ///// Make sure locked teams aren't overly sticky:
      //    Relock teams, restart level; team config should not change, but it should be new config
      gamePair.runChatCmd(0, "/lockteams");
      gamePair.idle(5, 5);
      gamePair.runChatCmd(0, "/restart");
      gamePair.idle(5, 5);
      server->countTeamPlayers();
      ASSERT_TRUE(server->areTeamsLocked());
      ASSERT_EQ(2, server->getTeam(0)->getPlayerCount());
      ASSERT_EQ(2, server->getTeam(1)->getPlayerCount());
      ASSERT_EQ(1, server->getTeam(2)->getPlayerCount());
   }


   TEST(TeamChangingTests, TestTeamSwitchingAbuse)
   {
      GamePair gamePair(getLevelCodeForItemPropagationTests(""), 2); // This level has 2 teams; good for testing team changing

      ServerGame *server = gamePair.server;

      ClientInfo *info0 = gamePair.getClient(0)->getClientInfo();
      ClientInfo *info1 = gamePair.getClient(1)->getClientInfo();

      S32 team0 = gamePair.getClient(0)->getCurrentTeamIndex();
      S32 team1 = gamePair.getClient(1)->getCurrentTeamIndex();

      ASSERT_NE(team0, team1) << "Players start on different teams";

      ///// Verify admin players can swith at will in two-player game
      // Players start as admins in GamePairs
      gamePair.getClient(0)->changeOwnTeam(team1);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team1);

      gamePair.getClient(0)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "Admins can switch at will";

      gamePair.getClient(0)->changeOwnTeam(team1);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team1);

      gamePair.getClient(0)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "See?  Any time they want!";


      ///// But non-admins can't
      // Demote players to verify timer works
      server->getClientInfo(0)->setRole(ClientInfo::RoleNone);
      gamePair.idle(5, 5);

      gamePair.getClient(0)->changeOwnTeam(team1);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team1) << "Players now on same team";

      gamePair.getClient(0)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team1) << "Should not be able to switch so soon!";

      letTeamChangeAbuseTimerExpire(gamePair);

      gamePair.getClient(0)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0) << "Can switch when timer expires";


      ///// When only one player, even non-admins can switch at will (also verifies fix to bug where non-admins could not
      //    switch after switching teams in a 2-player game, and the other player left)
      gamePair.removeClient(1);
      ASSERT_EQ(1, server->getClientCount());
      gamePair.getClient(0)->changeOwnTeam(team1);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team1);

      gamePair.getClient(0)->changeOwnTeam(team0);
      gamePair.idle(5, 5);
      ASSERT_EQ(gamePair.getClient(0)->getCurrentTeamIndex(), team0);
   }
}
