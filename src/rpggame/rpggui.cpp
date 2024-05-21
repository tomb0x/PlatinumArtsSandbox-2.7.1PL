#include "rpggame.h"

#ifdef NEWGUI
//extern bool UI::activegui(const char *name);
//extern int UI::numui();
#else
extern bool activegui(const char *name);
extern int numgui();
#endif

using namespace rpgscript;
using namespace game;

namespace rpggui
{
	#ifdef NEWGUI

	bool open()
	{
		// return !editmode && mapdata && curmap && UI::numgui();
		return false;
	}

	void forcegui()
	{
		if(editmode || !mapdata || !talker->getent()) return;
		/*
		if(!UI::numgui()) execute("showchat");
		else if(!activegui("chat"))
		{
			|-- close all UIs here --|
			execute("showchat");
		}

		*/
	}

	void refreshgui()
	{
		/*
		if(UI::hideui("chat"))
			execute("showchat");
		*/
		//reopen GUI
	}

	#else

	bool open() //forces pause when GUIs are open
	{
		return !editmode && mapdata && curmap && numgui();
	}

	void forcegui() //basically forces chat GUI
	{
		if(editmode || !mapdata || !talker->getent()) return;

		if(!numgui()) showgui("chat");
		else if (!activegui("chat"))
		{
			while(numgui())
			{
				cleargui(1);
				if(activegui("chat"))
					break;
			}
			if(!numgui())
				showgui("chat");
		}

	}

	void refreshgui() {}


	#endif

	ICOMMAND(r_get_dialogue, "", (),
		if(!talker || !talker->getent()) return;
		script *scr = scripts[talker->getent()->getscript()];
		if(scr->dialogue.inrange(scr->pos)) result(scr->dialogue[scr->pos]->talk);
	)

	ICOMMAND(r_num_response, "", (),
		if(!talker || !talker->getent()) {intret(0); return;}
		script *scr = scripts[talker->getent()->getscript()];
		if(scr->dialogue.inrange(scr->pos)) intret(scr->dialogue[scr->pos]->dests.length());
		else intret(0);
	)

	ICOMMAND(r_get_response, "i", (int *n),
		if(!talker || !talker->getent()) {result(""); return;}
		script *scr = scripts[talker->getent()->getscript()];
		if(scr->dialogue.inrange(scr->pos) && scr->dialogue[scr->pos]->dests.inrange(*n))
			result(scr->dialogue[scr->pos]->dests[*n]->talk);
		else
			result("");
	)

	void chattrigger(int n)
	{
		if(!talker || !talker->getent()) return;
		script *scr = scripts[talker->getent()->getscript()];

		if(scr->dialogue.inrange(scr->pos) && scr->dialogue[scr->pos]->dests.inrange(n))
		{
			rpgchat *chat = scr->dialogue[scr->pos];
			scr->pos = clamp(scr->dialogue.length() - 1, -2, chat->dests[n]->dest);
			if(scr->pos == -2)
			{
				scr->pos = -1; //FIXME
				conoutf("Trade GUI not implemented yet :(");
			}

			if(chat->dests[n]->script) execute(chat->dests[n]->script);
			chat->close();

			if(talker && talker->getent() && scr != scripts[talker->getent()->getscript()])
				scr = scripts[talker->getent()->getscript()];

			if(scr->pos >= 0)
			{
				if(scr->dialogue[scr->pos]->dests.length())
					scr->dialogue[scr->pos]->close();
				scr->dialogue[scr->pos]->open();

				if(!scr->dialogue[scr->pos]->dests.length())
				{
					conoutf("ERROR: dialogue node %i has no destinations; closing", scr->pos);
					scr->pos = -1;
				}
			}
			if(scr->pos == -1)
			{
				#ifdef NEWGUI
				//UI::hideui("chat"); //FIXME ENABLE ME!!!
				#else
				cleargui(1);
				#endif
				talker->setnull();
			}
		}
	}

	ICOMMAND(r_trigger_response, "i", (int *n),
		chattrigger(*n);
	)

	ICOMMAND(r_inv_numitems, "", (),
		int num = 0;
		loopv(player1->inventory)
			if(player1->inventory[i]->quantity) num++;

		intret(num);
	)

	void hotkey(int n)
	{
		if(talker->getent())
			chattrigger(n);
	}
}