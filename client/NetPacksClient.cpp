#include "../lib/NetPacks.h"
#include "../CCallback.h"
#include "Client.h"
#include "CPlayerInterface.h"
#include "CGameInfo.h"
#include "../lib/Connection.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/map.h"
#include "../lib/VCMIDirs.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CSoundBase.h"
#include "mapHandler.h"
#include "GUIClasses.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "CConfigHandler.h"
#include "SDL_Extensions.h"
#include "CBattleInterface.h"
#include "../hch/CCampaignHandler.h"

//macro to avoid code duplication - calls given method with given arguments if interface for specific player is present
#define INTERFACE_CALL_IF_PRESENT(player,function,...) 	\
		if(vstd::contains(cl->playerint,player))		\
			cl->playerint[player]->function(__VA_ARGS__);

/*
 * NetPacksClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void SetResources::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,receivedResource,-1,-1);
}

void SetResource::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,receivedResource,resid,val);
}

void SetPrimSkill::applyCl( CClient *cl )
{
	const CGHeroInstance *h = GS(cl)->getHero(id);
	if(!h)
	{
		tlog1 << "Cannot find hero with ID " << id << std::endl;
		return;
	}
	INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroPrimarySkillChanged,h,which,val);
}

void SetSecSkill::applyCl( CClient *cl )
{
	const CGHeroInstance *h = GS(cl)->getHero(id);
	if(!h)
	{
		tlog1 << "Cannot find hero with ID " << id << std::endl;
		return;
	}
	INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroSecondarySkillChanged,h,which,val);
}

void HeroVisitCastle::applyCl( CClient *cl )
{
	if(start() && !garrison() && vstd::contains(cl->playerint,GS(cl)->getHero(hid)->tempOwner))
	{
		cl->playerint[GS(cl)->getHero(hid)->tempOwner]->heroVisitsTown(GS(cl)->getHero(hid),GS(cl)->getTown(tid));
	}
}

void ChangeSpells::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void SetMana::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(hid);
	if(vstd::contains(cl->playerint,h->tempOwner))
		cl->playerint[h->tempOwner]->heroManaPointsChanged(h);
}

void SetMovePoints::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(hid);

	if (cl->IGameCallback::getSelectedHero(LOCPLINT->playerID) == h)//if we have selected that hero
	{
		GS(cl)->calculatePaths(h, *cl->pathInfo);
	}

	if(vstd::contains(cl->playerint,h->tempOwner))
		cl->playerint[h->tempOwner]->heroMovePointsChanged(h);
}

void FoWChange::applyCl( CClient *cl )
{
	if(!vstd::contains(cl->playerint,player))
		return;

	if(mode)
		cl->playerint[player]->tileRevealed(tiles);
	else
		cl->playerint[player]->tileHidden(tiles);

	cl->updatePaths();
}

void SetAvailableHeroes::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void ChangeStackCount::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner,garrisonChanged,sl.army);
}

void SetStackType::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner,garrisonChanged,sl.army);
}

void EraseStack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner,garrisonChanged,sl.army);
}

void SwapStacks::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl1.army->tempOwner,garrisonChanged,sl1.army);
	if(sl1.army != sl2.army)
		INTERFACE_CALL_IF_PRESENT(sl2.army->tempOwner,garrisonChanged,sl2.army);
}

void InsertNewStack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner,garrisonChanged,sl.army);
}

void RebalanceStacks::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(src.army->tempOwner,garrisonChanged,src.army);
	if(src.army != dst.army)
		INTERFACE_CALL_IF_PRESENT(dst.army->tempOwner,garrisonChanged,dst.army);
}

void GiveBonus::applyCl( CClient *cl )
{
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(id);
			INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroBonusChanged, h, *h->bonuses.back(),true);
		}
		break;
	case PLAYER:
		{
			const PlayerState *p = GS(cl)->getPlayer(id);
			INTERFACE_CALL_IF_PRESENT(id, playerBonusChanged, *p->bonuses.back(), true);
		}
		break;
	}
}

void ChangeObjPos::applyFirstCl( CClient *cl )
{
	CGObjectInstance *obj = GS(cl)->map->objects[objid];
	if(flags & 1)
		CGI->mh->hideObject(obj);
}
void ChangeObjPos::applyCl( CClient *cl )
{
	CGObjectInstance *obj = GS(cl)->map->objects[objid];
	if(flags & 1)
		CGI->mh->printObject(obj);

	cl->updatePaths();
}

void PlayerEndsGame::applyCl( CClient *cl )
{
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
		i->second->gameOver(player,	victory);


// 	if(!CPlayerInterface::howManyPeople)
// 		cl->terminate = true;
}

void RemoveBonus::applyCl( CClient *cl )
{
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(id);
			INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroBonusChanged, h, bonus,false);
		}
		break;
	case PLAYER:
		{
			const PlayerState *p = GS(cl)->getPlayer(id);
			INTERFACE_CALL_IF_PRESENT(id, playerBonusChanged, bonus, false);
		}
		break;
	}
}

void UpdateCampaignState::applyCl( CClient *cl )
{
	cl->stopConnection();
	if(camp->mapsRemaining.size())
		cl->proposeNextMission(camp);
	else
		cl->finishCampaign(camp);
}

void RemoveObject::applyFirstCl( CClient *cl )
{
	const CGObjectInstance *o = cl->getObj(id);
	CGI->mh->hideObject(o);

	int3 pos = o->visitablePos();
	//notify interfaces about removal
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		if(i->first >= PLAYER_LIMIT) continue;
		if(GS(cl)->getPlayerTeam(i->first)->fogOfWarMap[pos.x][pos.y][pos.z])
		{
			i->second->objectRemoved(o);
		}
	}
}

void RemoveObject::applyCl( CClient *cl )
{
	if(cl->pathInfo->hero && cl->pathInfo->hero->id != id)
		GS(cl)->calculatePaths(cl->pathInfo->hero, *cl->pathInfo);
}

void TryMoveHero::applyFirstCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(id);

	//check if playerint will have the knowledge about movement - if not, directly update maphandler
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		if(i->first >= PLAYER_LIMIT)
			continue;
		TeamState *t = GS(cl)->getPlayerTeam(i->first);
		if((t->fogOfWarMap[start.x-1][start.y][start.z] || t->fogOfWarMap[end.x-1][end.y][end.z])
				&& GS(cl)->getPlayer(i->first)->human)
			humanKnows = true;
	}

	if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK  ||  !humanKnows)
		CGI->mh->removeObject(h);


	if(result == DISEMBARK)
		CGI->mh->printObject(h->boat);
}

void TryMoveHero::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(id);

	if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK)
		CGI->mh->printObject(h);

	if(result == EMBARK)
		CGI->mh->hideObject(h->boat);

	int player = h->tempOwner;

	if(vstd::contains(cl->playerint,player))
	{
		cl->playerint[player]->tileRevealed(fowRevealed);
	}

	//notify interfaces about move
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		if(i->first >= PLAYER_LIMIT) continue;
		TeamState *t = GS(cl)->getPlayerTeam(i->first);
		if(t->fogOfWarMap[start.x-1][start.y][start.z] || t->fogOfWarMap[end.x-1][end.y][end.z])
		{
			i->second->heroMoved(*this);
		}
	}

	if(!humanKnows) //maphandler didn't get update from playerint, do it now
	{				//TODO: restructure nicely
		CGI->mh->printObject(h);
	}
}

// void SetGarrisons::applyCl( CClient *cl )
// {
// 	for(std::map<ui32,CCreatureSet>::iterator i = garrs.begin(); i!=garrs.end(); i++)
// 		if(vstd::contains(cl->playerint,cl->getOwner(i->first)))
// 			cl->playerint[cl->getOwner(i->first)]->garrisonChanged(cl->getObj(i->first));
// }

void NewStructures::applyCl( CClient *cl )
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	BOOST_FOREACH(si32 id, bid)
	{
		if(id==13) //fort or capitol
		{
			town->defInfo = GS(cl)->capitols[town->subID];
		}
		if(id ==7)
		{
			town->defInfo = GS(cl)->forts[town->subID];
		}
		if(vstd::contains(cl->playerint,town->tempOwner))
			cl->playerint[town->tempOwner]->buildChanged(town,id,1);
	}
}
void RazeStructures::applyCl (CClient *cl)
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	BOOST_FOREACH(si32 id, bid)
	{
		if (id == 13) //fort or capitol
		{
			town->defInfo = GS(cl)->forts[town->subID];
		}
		if(vstd::contains (cl->playerint,town->tempOwner))
			cl->playerint[town->tempOwner]->buildChanged (town,id,2);
	}
}

void SetAvailableCreatures::applyCl( CClient *cl )
{
	const CGDwelling *dw = static_cast<const CGDwelling*>(cl->getObj(tid));

	//inform order about the change

	int p = -1;
	if(dw->ID == 106) //War Machines Factory is not flaggable, it's "owned" by visitor
		p = cl->getTile(dw->visitablePos())->visitableObjects.back()->tempOwner;
	else
		p = dw->tempOwner;

	INTERFACE_CALL_IF_PRESENT(p, availableCreaturesChanged, dw);
}

void SetHeroesInTown::applyCl( CClient *cl )
{
	CGTownInstance *t = GS(cl)->getTown(tid);
	if(vstd::contains(cl->playerint,t->tempOwner))
		cl->playerint[t->tempOwner]->heroInGarrisonChange(t);
}

void SetHeroArtifacts::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(hid);
	CGameInterface *player = (vstd::contains(cl->playerint,h->tempOwner) ? cl->playerint[h->tempOwner] : NULL);
	if(!player)
		return;

	//h->recreateArtBonuses();
	player->heroArtifactSetChanged(h);

// 	BOOST_FOREACH(Bonus bonus, gained)
// 	{
// 		player->heroBonusChanged(h,bonus,true);
// 	}
// 	BOOST_FOREACH(Bonus bonus, lost)
// 	{
// 		player->heroBonusChanged(h,bonus,false);
// 	}
}

void HeroRecruited::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->map->heroes.back();
	if(h->subID != hid)
	{
		tlog1 << "Something wrong with hero recruited!\n";
	}

	CGI->mh->initHeroDef(h);
	CGI->mh->printObject(h);
		
	if(vstd::contains(cl->playerint,h->tempOwner))
	{
		cl->playerint[h->tempOwner]->heroCreated(h);
		if(const CGTownInstance *t = GS(cl)->getTown(tid))
			cl->playerint[h->tempOwner]->heroInGarrisonChange(t);
	}
}

void GiveHero::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(id);
	CGI->mh->initHeroDef(h);
	CGI->mh->printObject(h);
	cl->playerint[h->tempOwner]->heroCreated(h);
}

void GiveHero::applyFirstCl( CClient *cl )
{
	CGI->mh->hideObject(GS(cl)->getHero(id));
}

void InfoWindow::applyCl( CClient *cl )
{
	std::vector<Component*> comps;
	for(size_t i=0;i<components.size();i++) 
	{
		comps.push_back(&components[i]);
	}
	std::string str;
	text.toString(str);

	if(vstd::contains(cl->playerint,player))
		cl->playerint[player]->showInfoDialog(str,comps,(soundBase::soundID)soundID);
	else
		tlog2 << "We received InfoWindow for not our player...\n";
}

void SetObjectProperty::applyCl( CClient *cl )
{
	//inform all players that see this object
	for(std::map<ui8,CGameInterface *>::const_iterator it = cl->playerint.begin(); it != cl->playerint.end(); ++it)
	{
		if(GS(cl)->isVisible(GS(cl)->map->objects[id], it->first))
			INTERFACE_CALL_IF_PRESENT(it->first, objectPropertyChanged, this);
	}
}

void HeroLevelUp::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(heroid);
	if(vstd::contains(cl->playerint,h->tempOwner))
	{
		boost::function<void(ui32)> callback = boost::function<void(ui32)>(boost::bind(&CCallback::selectionMade,LOCPLINT->cb,_1,id));
		cl->playerint[h->tempOwner]->heroGotLevel(const_cast<const CGHeroInstance*>(h),static_cast<int>(primskill),skills, callback);
	}
}

void BlockingDialog::applyCl( CClient *cl )
{
	std::string str;
	text.toString(str);

	if(vstd::contains(cl->playerint,player))
		cl->playerint[player]->showBlockingDialog(str,components,id,(soundBase::soundID)soundID,selection(),cancel());
	else
		tlog2 << "We received YesNoDialog for not our player...\n";
}

void GarrisonDialog::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(hid);
	const CArmedInstance *obj = static_cast<const CArmedInstance*>(cl->getObj(objid));

	if(!vstd::contains(cl->playerint,h->getOwner()))
		return;

	boost::function<void()> callback = boost::bind(&CCallback::selectionMade,LOCPLINT->cb,0,id);
	cl->playerint[h->getOwner()]->showGarrisonDialog(obj,h,removableUnits,callback);
}

void BattleStart::applyCl( CClient *cl )
{
	CPlayerInterface * att, * def;
	if(vstd::contains(cl->playerint, info->side1) && cl->playerint[info->side1]->human)
		att = static_cast<CPlayerInterface*>( cl->playerint[info->side1] );
	else
		att = NULL;

	if(vstd::contains(cl->playerint, info->side2) && cl->playerint[info->side2]->human)
		def = static_cast<CPlayerInterface*>( cl->playerint[info->side2] );
	else
		def = NULL;


	new CBattleInterface(info->belligerents[0], info->belligerents[1], info->heroes[0], info->heroes[1], genRect(600, 800, (conf.cc.resx - 800)/2, (conf.cc.resy - 600)/2), att, def);

	if(vstd::contains(cl->playerint,info->side1))
		cl->playerint[info->side1]->battleStart(info->belligerents[0], info->belligerents[1], info->tile, info->heroes[0], info->heroes[1], 0);

	if(vstd::contains(cl->playerint,info->side2))
		cl->playerint[info->side2]->battleStart(info->belligerents[0], info->belligerents[1], info->tile, info->heroes[0], info->heroes[1], 1);
}

void BattleNextRound::applyFirstCl(CClient *cl)
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleNewRoundFirst(round);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleNewRoundFirst(round);
}

void BattleNextRound::applyCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleNewRound(round);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleNewRound(round);
}

void BattleSetActiveStack::applyCl( CClient *cl )
{
	CStack * activated = GS(cl)->curB->getStack(stack);
	int playerToCall = -1; //player that will move activated stack
	if( activated->hasBonusOfType(Bonus::HYPNOTIZED) )
	{
		playerToCall = ( GS(cl)->curB->side1 == activated->owner ? GS(cl)->curB->side2 : GS(cl)->curB->side1 );
	}
	else
	{
		playerToCall = activated->owner;
	}
	if( vstd::contains(cl->playerint, playerToCall) )
		boost::thread( boost::bind(&CClient::waitForMoveAndSend, cl, playerToCall) );
}

void BattleResult::applyFirstCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleEnd(this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleEnd(this);
}

void BattleStackMoved::applyFirstCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStackMoved,stack,tile,distance,ending);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStackMoved,stack,tile,distance,ending);
}

void BattleStackAttacked::applyCl( CClient *cl )
{
	std::vector<BattleStackAttacked> bsa;
	bsa.push_back(*this);

	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStacksAttacked,bsa);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStacksAttacked,bsa);
}

void BattleAttack::applyFirstCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleAttack(this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleAttack(this);
	for (int g=0; g<bsa.size(); ++g)
	{
		for (int z=0; z<bsa[g].healedStacks.size(); ++z)
		{
			bsa[g].healedStacks[z].applyCl(cl);
		}
	}
}

void BattleAttack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStacksAttacked,bsa);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStacksAttacked,bsa);
}

void StartAction::applyFirstCl( CClient *cl )
{
	cl->curbaction = new BattleAction(ba);
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->actionStarted(&ba);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->actionStarted(&ba);
}

void BattleSpellCast::applyCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleSpellCast(this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleSpellCast(this);

	if(id >= 66 && id <= 69) //elemental summoning
	{
		if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
			cl->playerint[GS(cl)->curB->side1]->battleNewStackAppeared(GS(cl)->curB->stacks.size() - 1);
		if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
			cl->playerint[GS(cl)->curB->side2]->battleNewStackAppeared(GS(cl)->curB->stacks.size() - 1);
	}
}

void SetStackEffect::applyCl( CClient *cl )
{
	BattleSpellCast sc;
	sc.id = effect.id;
	sc.side = 3; //doesn't matter
	sc.skill = effect.val;

	//informing about effects
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleStacksEffectsSet(*this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleStacksEffectsSet(*this);
}

void StacksInjured::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStacksAttacked,stacks);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStacksAttacked,stacks);
}

void BattleResultsApplied::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player1,battleResultsApplied);
	INTERFACE_CALL_IF_PRESENT(player2,battleResultsApplied);
}

void StacksHealedOrResurrected::applyCl( CClient *cl )
{
	std::vector<std::pair<ui32, ui32> > shiftedHealed;
	for(int v=0; v<healedStacks.size(); ++v)
	{
		shiftedHealed.push_back(std::make_pair(healedStacks[v].stackID, healedStacks[v].healedHP));
	}
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1, battleStacksHealedRes, shiftedHealed, lifeDrain, drainedFrom);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2, battleStacksHealedRes, shiftedHealed, lifeDrain, drainedFrom);
}

void ObstaclesRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed obstacles
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1, battleObstaclesRemoved, obstacles);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2, battleObstaclesRemoved, obstacles);
}

void CatapultAttack::applyCl( CClient *cl )
{
	//inform interfaces about catapult attack
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1, battleCatapultAttacked, *this);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2, battleCatapultAttacked, *this);
}

void BattleStacksRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed stacks
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1, battleStacksRemoved, *this);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2, battleStacksRemoved, *this);
}

CGameState* CPackForClient::GS( CClient *cl )
{
	return cl->gs;
}

void EndAction::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,actionFinished,cl->curbaction);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,actionFinished,cl->curbaction);

	delete cl->curbaction;
	cl->curbaction = NULL;
}

void PackageApplied::applyCl( CClient *cl )
{
	ui8 player = GS(cl)->currentPlayer;
	INTERFACE_CALL_IF_PRESENT(player, requestRealized, this);
	if(cl->waitingRequest.get())
		cl->waitingRequest.setn(false);
}

void SystemMessage::applyCl( CClient *cl )
{
	std::ostringstream str;
	str << "System message: " << text;

	tlog4 << str.str() << std::endl;
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void PlayerBlocked::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,playerBlocked,reason);
}

void YourTurn::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,yourTurn);
}

void SaveGame::applyCl(CClient *cl)
{
	CSaveFile save(GVCMIDirs.UserPath + "/Games/" + fname + ".vcgm1");
	save << *cl;
}

void PlayerMessage::applyCl(CClient *cl)
{
	std::ostringstream str;
	str << "Player "<<(int)player<<" sends a message: " << text;

	tlog4 << str.str() << std::endl;
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void SetSelection::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
		return;

	//CPackForClient::GS(cl)->calculatePaths(h, *cl->pathInfo);
}

void ShowInInfobox::applyCl(CClient *cl)
{
	SComponent sc(c);
	text.toString(sc.description);
	if(vstd::contains(cl->playerint, player) && cl->playerint[player]->human)
	{
		static_cast<CPlayerInterface*>(cl->playerint[player])->showComp(sc);
	}
}


void AdvmapSpellCast::applyCl(CClient *cl)
{
	cl->playerint[caster->getOwner()]->advmapSpellCast(caster, spellID);
}

void OpenWindow::applyCl(CClient *cl)
{
	switch(window)
	{
	case EXCHANGE_WINDOW:
		{
			const CGHeroInstance *h = cl->getHero(id1);
			const CGObjectInstance *h2 = cl->getHero(id2);
			assert(h && h2);
			INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroExchangeStarted, id1, id2);
		}
		break;
	case RECRUITMENT_FIRST:
	case RECRUITMENT_ALL:
		{
			const CGDwelling *dw = dynamic_cast<const CGDwelling*>(cl->getObj(id1));
			const CArmedInstance *dst = dynamic_cast<const CArmedInstance*>(cl->getObj(id2));
			INTERFACE_CALL_IF_PRESENT(dst->tempOwner,showRecruitmentDialog, dw, dst, window == RECRUITMENT_FIRST ? 0 : -1);
		}
		break;
	case SHIPYARD_WINDOW:
		{
			const IShipyard *sy = IShipyard::castFrom(cl->getObj(id1));
			INTERFACE_CALL_IF_PRESENT(sy->o->tempOwner, showShipyardDialog, sy);
		}
		break;
	case THIEVES_GUILD:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(id1);
			GH.pushInt( new CThievesGuildWindow(obj) );
		}
		break;
	case UNIVERSITY_WINDOW:
		{
			//displays University window (when hero enters University on adventure map)
			const IMarket *market = IMarket::castFrom(cl->getObj(id1));
			const CGHeroInstance *hero = cl->getHero(id2);
			INTERFACE_CALL_IF_PRESENT(hero->tempOwner,showUniversityWindow, market, hero);
		}
		break;
	case MARKET_WINDOW:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(id1);
			const CGHeroInstance *hero = cl->getHero(id2);
			const IMarket *market = IMarket::castFrom(obj);
			INTERFACE_CALL_IF_PRESENT(cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, showMarketWindow, market, hero);
		}
		break;
	case HILL_FORT_WINDOW:
		{
			//displays Hill fort window
			const CGObjectInstance *obj = cl->getObj(id1);
			const CGHeroInstance *hero = cl->getHero(id2);
			INTERFACE_CALL_IF_PRESENT(cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, showHillFortWindow, obj, hero);
		}
		break;
	case PUZZLE_MAP:
		{
			INTERFACE_CALL_IF_PRESENT(id1, showPuzzleMap);
		}
		break;
	case TAVERN_WINDOW:
		const CGObjectInstance *obj1 = cl->getObj(id1),
								*obj2 = cl->getObj(id2);
		INTERFACE_CALL_IF_PRESENT(obj1->tempOwner, showTavernWindow, obj2);
		break;
	}

}

void CenterView::applyCl(CClient *cl)
{
	INTERFACE_CALL_IF_PRESENT (player, centerView, pos, focusTime);
}

void NewObject::applyCl(CClient *cl)
{
	cl->updatePaths();

	const CGObjectInstance *obj = cl->getObj(id);
	//notify interfaces about move
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		//TODO: check if any covered tile is visible
		if(i->first >= PLAYER_LIMIT) continue;
		if(GS(cl)->getPlayerTeam(i->first)->fogOfWarMap[obj->pos.x][obj->pos.y][obj->pos.z])
		{
			i->second->newObject(obj);
		}
	}
}

void SetAvailableArtifacts::applyCl(CClient *cl)
{
	if(id < 0) //artifact merchants globally
	{
		for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
			i->second->availableArtifactsChanged(NULL);
	}
	else
	{
		const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(cl->getObj(id));
		assert(bm);
		INTERFACE_CALL_IF_PRESENT(cl->getTile(bm->visitablePos())->visitableObjects.back()->tempOwner, availableArtifactsChanged, bm);
	}
}

void TradeComponents::applyCl(CClient *cl)
{///Shop handler
	switch (CGI->mh->map->objects[objectid]->ID)
	{
		case 7: //Black Market
			break;
		case 95: //Tavern
			break;
		case 97: //Den of Thieves
			break;
		case 221: //Trading Post
			break;
		default: 
			tlog2 << "Shop type not supported! \n";
	}
}
