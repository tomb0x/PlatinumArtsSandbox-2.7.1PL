#include "rpggame.h"

extern bool reloadtexture(const char *name); //texture.cpp

namespace rpgio
{
	#define GAME_VERSION 27
	#define GAME_MAGIC "RPGS"

	/**
		SAVING STUFF
			STRINGS - use writestring(stream *, char *)
			VECTORS - write an int corresponding to the number of elements, then write the elements
			POINTERS - convert to reference, if this is not possible don't save it
			VEC - use writevec macro, you need to write all 3 coordinates independantly

		LOADING STUFF
			STRINGS - use readstring(stream *)
			VECTORS - read the next int, and use that as the basis of knowing how many elements to load
			POINTERS - read the reference and convert it back to a pointer of the intended object. If this can't be done reliably don't save it
			VEC - use readvec macro, you need to read all 3 coordinates independantly

		REMINDERS
			HOW MANY - don't forget to indicate how many times you need to repeat the loop; there should be a amcro that helps with this
			ORDER - you must read and write items in the same order
			LITTLE ENDIAN - pullil add getlil for single values and lilswap for arrays and objects are going to be your bestest friends
			POINTERS - they change each run, remember that, for convention, use -1 for players
			ARGUMENTS - as a argument to a function, ORDER IS NOT RESPECTED, use with extreme care
			TIME - the difference of lastmillis is taken and applied on save and load - use countdown timers instead if possible.

		ORDER
			they are to coincide with the order of the item structs, and before the map functions, the order in which crap is stored there
	*/

	#define readvec(v) \
	v.x = f->getlil<float>(); \
	v.y = f->getlil<float>(); \
	v.z = f->getlil<float>(); \
	if(DEBUG_IO) \
		conoutf("DEBUG: Read vec (%f, %f, %f) from file", v.x, v.y, v.z);

	#define writevec(v) \
	f->putlil(v.x); \
	f->putlil(v.y); \
	f->putlil(v.z); \
	if(DEBUG_IO) \
		conoutf("DEBUG: Wrote vec (%f, %f, %f) to file", v.x, v.y, v.z);

	struct saveheader
	{
		char magic[4];
		int sversion; //save
		int gversion; //game
	};

	bool abort = false;
	mapinfo *lastmap = NULL;

	rpgent *entfromnum(int num)
	{
		if(num == -2)
			return NULL;
		if(num == -1)
			return game::player1;
		else if(num >= lastmap->objs.length())
		{
			conoutf("WARNING: invalid entity num (%i), possible corruption", num);
			return NULL;
		}
		return lastmap->objs[num];
	}

	int enttonum(rpgent *ent)
	{
		if(ent == NULL)
			return -2;
		if(ent == game::player1)
			return -1;

		int i = lastmap->objs.find(game::player1);
		if(i != -1)
		{
			int num = lastmap->objs.find(ent);
			if(num > i)
				num--;
			return num;
		}
		else
			return lastmap->objs.find(ent);
	}

	struct reference
	{
		int ind;
		mapinfo *map;
		rpgent *&ref;
		reference(int i, mapinfo *m, rpgent *&r) : ind(i), map(m), ref(r) {}
		~reference() {}
	};
	vector<reference> updates;

	const char *readstring(stream *f)
	{
		int len = f->getlil<int>();
		char *s = NULL;
		if(len)
		{
			s = new char[len];
			f->read(s, len);
			if(DEBUG_IO)
				conoutf(CON_DEBUG, "DEBUG: Read \"%s\" from file (%i)", s, len);
		}

		return s;
	}

	void writestring(stream *f, const char *s)
	{
		int len = s ? strlen(s) + 1 : 0;
		f->putlil(len);

		if(!len) return;
		f->write(s, len);
		if(DEBUG_IO)
			conoutf(CON_DEBUG, "DEBUG: Wrote \"%s\" to file (%i)", s, len);
	}

	void readfaction(stream *f, faction *fact)
	{
		int num = f->getlil<int>();
		loopi(num)
		{
			int val = f->getlil<int>();
			if(!fact->relations.inrange(num))
				fact->relations.add(val);
			else
				fact->relations[num] = val;
		}
	}

	void writefaction(stream *f, faction *fact)
	{
		f->putlil(fact->relations.length());
		loopv(fact->relations)
			f->putlil(fact->relations[i]);
	}

	void readrecipe(stream *f, recipe *rec)
	{
		rec->flags |= f->getlil<int>();
	}

	void writerecipe(stream *f, recipe *rec)
	{
		f->putlil( rec->flags & recipe::SAVE);
	}

	rpgent *readent(stream *f, rpgent *ent = NULL)
	{
		int type = f->getlil<int>();
		if(ent)
			type = ent->type();

		switch(type)
		{
			case ENT_CHAR:
			{
				if(!ent) ent = new rpgchar();
				rpgchar *loading = (rpgchar *) ent;

				delete[] loading->name;
				delete[] loading->mdl;

				loading->name = readstring(f);
				loading->mdl = readstring(f);
				
				#define x(var, type) loading->base.var = f->getlil<type>();
	
				x(experience, int)
				x(level, int)
				x(points, int)
				loopi(STAT_MAX) x(baseattrs[i], short)
				loopi(SKILL_MAX) x(baseskills[i], short)
				loopi(STAT_MAX) x(deltaattrs[i], short)
				loopi(SKILL_MAX) x(deltaskills[i], short)
				
				loopi(ATTACK_MAX) x(bonusthresh[i], short)
				loopi(ATTACK_MAX) x(bonusresist[i], short)
				x(bonushealth, int)
				x(bonusmana, int)
				x(bonusmovespeed, int)
				x(bonusjumpvel, int)
				x(bonuscarry, int)
				x(bonuscrit, int)
				x(bonushregen, float)
				x(bonusmregen, float)
				
				loopi(ATTACK_MAX) x(deltathresh[i], short)
				loopi(ATTACK_MAX) x(deltaresist[i], short)
				x(deltahealth, int)
				x(deltamana, int)
				x(deltamovespeed, int)
				x(deltajumpvel, int)
				x(deltacarry, int)
				x(deltacrit, int)
				x(deltahregen, float)
				x(deltamregen, float)
					
				#undef x

				int items = f->getlil<int>();
				loopi(items)
				{
					int b = f->getlil<int>();
					int q = f->getlil<int>();
					if(q) loading->modinv(b, q, false); //technically this facilitates dupes, but cleans out empty entries
				}

				loading->script = f->getlil<int>();
				loading->faction = f->getlil<int>();

				int equiplen = f->getlil<int>();
				loopi(equiplen)
				{
					int base = f->getlil<int>();
					int use = f->getlil<int>();
					loading->modinv(base, 1, false);
					loading->equip(base, use); //validate equips
				}

				loading->health = f->getlil<float>();
				loading->mana = f->getlil<float>();
				loading->lastaction = f->getlil<int>() + lastmillis;

				int num = f->getlil<int>();
				loopi(num)
				loading->route.add(f->getlil<ushort>());

				break;
			}
			case ENT_ITEM:
			{
				if(!ent) ent = new rpgitem();
				rpgitem *loading = (rpgitem *) ent;

				loading->base = f->getlil<int>();
				loading->quantity = f->getlil<int>();

				break;
			}
			case ENT_OBSTACLE:
			{
				if(!ent) ent = new rpgobstacle();
				rpgobstacle *loading = (rpgobstacle *) ent;

				delete loading->mdl;
				loading->mdl = readstring(f);
				loading->weight = f->getlil<int>();
				loading->script = f->getlil<int>();
				loading->flags = f->getlil<int>();

				break;
			}
			case ENT_CONTAINER:
			{
				if(!ent) ent = new rpgcontainer();
				rpgcontainer *loading = (rpgcontainer *) ent;

				delete[] loading->mdl;
				delete[] loading->name;

				loading->mdl = readstring(f);
				loading->name = readstring(f);

				int items = f->getlil<int>();
				loopi(items)
				{
					int b = f->getlil<int>();
					int q = f->getlil<int>();
					if(q) loading->modinv(b, q, false); //technically this facilitates dupes, but cleans out empty entries
				}

				loading->capacity = f->getlil<int>();
				loading->script = f->getlil<int>();
				loading->lock = f->getlil<int>();

				break;
			}
			case ENT_PLATFORM:
			{
				if(!ent) ent = new rpgplatform();
				rpgplatform *loading = (rpgplatform *) ent;

				delete[] loading->mdl;

				loading->mdl = readstring(f);
				loading->speed = f->getlil<int>();
				loading->script = f->getlil<int>();
				loading->flags = f->getlil<int>();

				int steps = f->getlil<int>();
				loopi(steps)
				{
					vector<int> &detours = loading->routes.access(f->getlil<int>(), vector<int>());
					int routes = f->getlil<int>();
					loopj(routes) detours.add(f->getlil<int>());
				}


				loading->target = f->getlil<int>();

				break;
			}
			case ENT_TRIGGER:
			{
				if(!ent) ent = new rpgtrigger();
				rpgtrigger *loading = (rpgtrigger *) ent;

				delete[] loading->mdl;
				delete[] loading->name;

				loading->mdl = readstring(f);
				loading->name = readstring(f);
				loading->script = f->getlil<int>();
				loading->flags = f->getlil<int>();
				loading->lasttrigger = f->getlil<int>();

				break;
			}
			default:
				conoutf(CON_ERROR, "ERROR: unknown entity type %i", type);
				abort = true;
				return NULL;
		}

		int numeffs = f->getlil<int>();
		loopi(numeffs)
		{
			victimeffect *eff = new victimeffect();
			ent->seffects.add(eff);

			updates.add(reference(f->getlil<int>(), lastmap, eff->owner));
			eff->group = f->getlil<int>();
			eff->elem = f->getlil<int>();
			int numstat = f->getlil<int>();
			loopj(numstat)
			{
				int type = f->getlil<int>();
				status *st = NULL;
				switch(type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = new status_polymorph();
						st = poly;

						poly->mdl = readstring(f);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = new status_light();
						st = light;

						readvec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = new status_script();
						st = scr;

						scr->script = readstring(f);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = new status_signal();
						st = sig;

						sig->signal = readstring(f);
						break;
					}
					default:
						st = new status_generic();
						break;
				}
				st->type = type;
				st->duration = f->getlil<int>();
				st->remain = f->getlil<int>();
				st->strength = f->getlil<int>();
				st->variance = f->getlil<float>();

				eff->effects.add(st);
			}
		}

		ent->eyeheight = f->getlil<float>();
		readvec(ent->o);
		ent->newpos = ent->o;
		readvec(ent->vel);
		readvec(ent->falling);

		ent->yaw = f->getlil<float>();
		ent->pitch = f->getlil<float>();
		ent->roll = f->getlil<float>();

		ent->timeinair = f->getlil<int>();

		ent->state = f->getchar();
		ent->editstate = f->getchar();

		ent->lastjump = f->getlil<int>() + lastmillis;

		return ent;
	}

	void writeent(stream *f, rpgent *d)
	{
		f->putlil(d->type());

		switch(d->type())
		{
			case ENT_CHAR:
			{
				rpgchar *saving = (rpgchar *) d;

				writestring(f, saving->name);
				writestring(f, saving->mdl);
				
				#define x(var) f->putlil(saving->base.var);
	
				x(experience)
				x(level)
				x(points)
				loopi(STAT_MAX) x(baseattrs[i])
				loopi(SKILL_MAX) x(baseskills[i])
				loopi(STAT_MAX) x(deltaattrs[i])
				loopi(SKILL_MAX) x(deltaskills[i])
				
				loopi(ATTACK_MAX) x(bonusthresh[i])
				loopi(ATTACK_MAX) x(bonusresist[i])
				x(bonushealth)
				x(bonusmana)
				x(bonusmovespeed)
				x(bonusjumpvel)
				x(bonuscarry)
				x(bonuscrit)
				x(bonushregen)
				x(bonusmregen)
				
				loopi(ATTACK_MAX) x(deltathresh[i])
				loopi(ATTACK_MAX) x(deltaresist[i])
				x(deltahealth)
				x(deltamana)
				x(deltamovespeed)
				x(deltajumpvel)
				x(deltacarry)
				x(deltacrit)
				x(deltahregen)
				x(deltamregen)
					
				#undef x

				f->putlil(saving->inventory.length());
				loopv(saving->inventory)
				{
					f->putlil(saving->inventory[i]->base);
					f->putlil(saving->inventory[i]->quantity);
				}

				f->putlil(saving->script);
				f->putlil(saving->faction);

				f->putlil(saving->equipped.length());
				loopv(saving->equipped)
				{
					f->putlil(saving->equipped[i]->base);
					f->putlil(saving->equipped[i]->use);
				}

				f->putlil(saving->health);
				f->putlil(saving->mana);
				f->putlil(saving->lastaction - lastmillis);

				f->putlil(saving->route.length());
				loopv(saving->route)
				f->putlil(saving->route[i]);

				break;
			}
			case ENT_ITEM:
			{
				rpgitem *saving = (rpgitem *) d;

				f->putlil(saving->base);
				f->putlil(saving->quantity);

				break;
			}
			case ENT_OBSTACLE:
			{
				rpgobstacle *saving = (rpgobstacle *) d;

				writestring(f, saving->mdl);
				f->putlil(saving->weight);
				f->putlil(saving->script);
				f->putlil(saving->flags);

				break;
			}
			case ENT_CONTAINER:
			{
				rpgcontainer *saving = (rpgcontainer *) d;

				writestring(f, saving->mdl);
				writestring(f, saving->name);

				f->putlil(saving->inventory.length());
				loopv(saving->inventory)
				{
					f->putlil(saving->inventory[i]->base);
					f->putlil(saving->inventory[i]->quantity);
				}
				f->putlil(saving->capacity);
				f->putlil(saving->script);
				f->putlil(saving->lock);

				break;
			}
			case ENT_PLATFORM:
			{
				rpgplatform *saving = (rpgplatform *) d;

				writestring(f, saving->mdl);
				f->putlil(saving->speed);
				f->putlil(saving->script);
				f->putlil(saving->flags);

				int num = 0;
				enumerate(saving->routes, vector<int>, tmp, num++);
				f->putlil(num);

				enumeratekt(saving->routes, int, stop, vector<int>, routes,
					f->putlil(stop);
					f->putlil(routes.length());
					loopvj(routes)
						f->putlil(routes[j]);
				);

				f->putlil(saving->target);

				break;
			}
			case ENT_TRIGGER:
			{
				rpgtrigger *saving = (rpgtrigger *) d;

				writestring(f, saving->mdl);
				writestring(f, saving->name);
				f->putlil(saving->script);
				f->putlil(saving->flags);
				f->putlil(saving->lasttrigger);

				break;
			}
			default:
				conoutf(CON_ERROR, "ERROR: unsupported ent type %i, aborting", d->type());
				return;
		}

		f->putlil(d->seffects.length());
		loopv(d->seffects)
		{
			f->putlil(enttonum(d->seffects[i]->owner));
			f->putlil(d->seffects[i]->group);
			f->putlil(d->seffects[i]->elem);
			f->putlil(d->seffects[i]->effects.length());

			loopvj(d->seffects[i]->effects)
			{
				status *st = d->seffects[i]->effects[j];
				f->putlil(st->type);
				switch(st->type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = (status_polymorph *) st;

						writestring(f, poly->mdl);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = (status_light *) st;

						writevec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = (status_script *) st;

						writestring(f, scr->script);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = (status_signal *) st;

						writestring(f, sig->signal);
						break;
					}
				}
				f->putlil(st->duration);
				f->putlil(st->remain);
				f->putlil(st->strength);
				f->putlil(st->variance);
			}
		}

		f->putlil(d->eyeheight);
		writevec(d->o);
		writevec(d->vel);
		writevec(d->falling);

		f->putlil(d->yaw);
		f->putlil(d->pitch);
		f->putlil(d->roll);

		f->putlil(d->timeinair);

		f->putchar(d->state);
		f->putchar(d->editstate);

		f->putlil(d->lastjump - lastmillis);
	}

	mapinfo *readmap(stream *f)
	{
		const char *name = readstring(f);
		mapinfo *loading = game::accessmap(name);
		lastmap = loading;

		loading->name = name;
		loading->script = f->getlil<int>();
		loading->flags = f->getlil<int>();
		loading->loaded = f->getchar();

		int numobjs = f->getlil<int>(),
		numactions = f->getlil<int>(),
		numprojs = f->getlil<int>(),
		numaeffects = f->getlil<int>(),
		numblips = f->getlil<int>();

		loopi(numobjs)
		loading->objs.add(readent(f));

		loopvrev(updates)
		{
			if(updates[i].map == lastmap)
				updates[i].ref = entfromnum(updates[i].ind);
		}

		loopi(numactions)
		{
			action *act = NULL;
			int type = f->getlil<int>();

			switch(type)
			{
				case ACTION_TELEPORT:
				{
					int ent = f->getlil<int>();
					if(!entfromnum(ent))
					{//how'd that happen?
					conoutf("WARNING: loaded teleport loadaction for invalid ent? ignoring");
					f->getlil<int>();
					continue;
					}
					act = new action_teleport(entfromnum(ent), f->getlil<int>());
					break;
				}
				case ACTION_SPAWN:
				{
					int tag = f->getlil<int>(),
						ent = f->getlil<int>(),
						id = f->getlil<int>(),
						amount = f->getlil<int>(),
						qty = f->getlil<int>();
						act = new action_spawn(tag, ent, id, amount, qty);
						break;
				}
				case ACTION_SCRIPT:
				{
					act = new action_script(readstring(f));
					break;
				}
			}

			loading->loadactions.add(act);
		}

		loopi(numprojs)
		{
			projectile *p = new projectile();

			p->owner = (rpgchar *) entfromnum(f->getlil<int>());
			if(p->owner->type() != ENT_CHAR)
				p->owner = NULL;

			p->item = f->getlil<int>();
			p->use = f->getlil<int>();
			p->ammo = f->getlil<int>();
			p->ause = f->getlil<int>();

			readvec(p->o); readvec(p->dir); readvec(p->emitpos);
			p->lastemit = 0; //should emit immediately
			p->gravity = f->getlil<int>();
			p->deleted = f->getchar();

			p->pflags = f->getlil<int>();
			p->time = f->getlil<int>();
			p->dist = f->getlil<int>();

			p->projfx = f->getlil<int>();
			p->trailfx = f->getlil<int>();
			p->deathfx = f->getlil<int>();
			p->radius = f->getlil<int>();
			p->elasticity = f->getlil<float>();
			p->charge = f->getlil<float>();
			p->chargeflags = f->getlil<int>();

			loading->projs.add(p);
		}

		loopi(numaeffects)
		{
			areaeffect *aeff = new areaeffect();
			loading->aeffects.add(aeff);

			readvec(aeff->o);
			aeff->owner = entfromnum(f->getlil<int>());
			aeff->lastemit = 0; //should emit immediately
			aeff->group = f->getlil<int>();
			aeff->elem = f->getlil<int>();
			aeff->radius = f->getlil<int>();
			aeff->fx = f->getlil<int>();

			int numstat = f->getlil<int>();
			loopj(numstat)
			{
				int type = f->getlil<int>();
				status *st = NULL;
				switch(type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = new status_polymorph();
						st = poly;

						poly->mdl = readstring(f);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = new status_light();
						st = light;

						readvec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = new status_script();
						st = scr;

						scr->script = readstring(f);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = new status_signal();
						st = sig;

						sig->signal = readstring(f);
						break;
					}
					default:
						st = new status_generic();
						break;
				}
				st->type = type;
				st->duration = f->getlil<int>();
				st->remain = f->getlil<int>();
				st->strength = f->getlil<int>();
				st->variance = f->getlil<float>();

				aeff->effects.add(st);
			}
		}

		loopi(numblips)
		{
			///FIXME finalize blip structure and write me
		}

		lastmap = NULL;
		return loading;
	}

	void writemap(stream *f, mapinfo *saving)
	{
		lastmap = saving;

		writestring(f, saving->name);
		f->putlil(saving->script);
		f->putlil(saving->flags);
		f->putchar(saving->loaded);

		f->putlil(saving->objs.length());
		f->putlil(saving->loadactions.length());
		f->putlil(saving->projs.length());
		f->putlil(saving->aeffects.length());
		f->putlil(saving->blips.length());

		loopv(saving->objs)
		{
			writeent(f, saving->objs[i]);
		}

		loopv(saving->loadactions)
		{
			f->putlil(saving->loadactions[i]->type());

			switch(saving->loadactions[i]->type())
			{
				case ACTION_TELEPORT:
				{
					action_teleport *act = (action_teleport *) saving->loadactions[i];
					f->putlil(enttonum(act->ent));
					f->putlil(act->dest);

					break;
				}
				case ACTION_SPAWN:
				{
					action_spawn *spw = (action_spawn *) saving->loadactions[i];
					f->putlil(spw->tag);
					f->putlil(spw->ent);
					f->putlil(spw->id);
					f->putlil(spw->amount);
					f->putlil(spw->qty);

					break;
				}
				case ACTION_SCRIPT:
					writestring(f, ((action_script *) saving->loadactions[i])->script);
					break;
			}
		}

		loopv(saving->projs)
		{
			projectile *p = saving->projs[i];
			f->putlil(enttonum(p->owner));
			f->putlil(p->item);
			f->putlil(p->use);
			f->putlil(p->ammo);
			f->putlil(p->ause);
			writevec(p->o); writevec(p->dir); writevec(p->emitpos);
			//f->putlil(p->lastemit);
			f->putlil(p->gravity);
			f->putchar(p->deleted);

			f->putlil(p->pflags);
			f->putlil(p->time);
			f->putlil(p->dist);

			f->putlil(p->projfx);
			f->putlil(p->trailfx);
			f->putlil(p->deathfx);
			f->putlil(p->radius);

			f->putlil(p->elasticity);
			f->putlil(p->charge);
			f->putlil(p->chargeflags);
		}

		loopv(saving->aeffects)
		{
			areaeffect *aeff = saving->aeffects[i];

			f->putlil(enttonum(aeff->owner));
			writevec(aeff->o);
			f->putlil(aeff->group);
			f->putlil(aeff->elem);
			f->putlil(aeff->radius);
			f->putlil(aeff->fx);

			f->putlil(aeff->effects.length());
			loopvj(aeff->effects)
			{
				status *st = aeff->effects[i];
				f->putlil(st->type);
				switch(st->type)
				{
					case STATUS_POLYMORPH:
					{
						status_polymorph *poly = (status_polymorph *) st;

						writestring(f, poly->mdl);
						break;
					}
					case STATUS_LIGHT:
					{
						status_light *light = (status_light *) st;

						writevec(light->colour);
						break;
					}
					case STATUS_SCRIPT:
					{
						status_script *scr = (status_script *) st;

						writestring(f, scr->script);
						break;
					}
					case STATUS_SIGNAL:
					{
						status_signal *sig = (status_signal *) st;

						writestring(f, sig->signal);
						break;
					}
				}
				f->putlil(st->duration);
				f->putlil(st->remain);
				f->putlil(st->strength);
				f->putlil(st->variance);
			}
		}

		loopv(saving->blips)
		{
			///FIXME finalize blip structure and write me
		}
		lastmap = NULL;
	}

	//don't mind the ::blah, just a namespace collision with rpgio:: when we want it from <anonymous>::
	void writereferences(stream *f, vector<mapinfo *> &maps, hashtable<const char*, ::reference> &stack)
	{
		int num = 0;
		enumerate(stack, ::reference , saving,
			num++;
			if(!saving.ref) saving.type = ::reference::T_INVALID;
		);
		f->putlil(num);

		enumerate(stack, ::reference, saving,
			writestring(f, saving.name);

			char type = saving.type;
			switch(type)
			{
				case ::reference::T_CHAR:
				case ::reference::T_ITEM:
				case ::reference::T_OBSTACLE:
				case ::reference::T_CONTAINER:
				case ::reference::T_PLATFORM:
				case ::reference::T_TRIGGER:
				{
					int map = -1;
					int ent = -1;
					if(saving.ref == game::player1) { map = ent = -1; }
					else loopvj(maps)
					{
						ent = maps[j]->objs.find(saving.ref);
						if(ent >= 0) {map = j; break;}
						else ent--;
					}
					if(map == -1 && ent == -1 && saving.ref != game::player1)
					{
						conoutf(CON_ERROR, "WARNING: char/item/object reference \"%s\" points to non-player entity that does not exist");
						f->putchar(::reference::T_INVALID);
						continue;
					}

					if(DEBUG_IO)
						conoutf(CON_DEBUG, "DEBUG: writing reference %s as rpgent reference: %i %i", saving.name, map, ent);
					f->putchar(type);
					f->putlil(map);
					f->putlil(ent);

					break;
				}
				case ::reference::T_MAP:
				{
					f->putchar(type);
					int map = maps.find(saving.ref);
					f->putlil(map);
					if(DEBUG_IO) conoutf(CON_DEBUG, "DEBUG: writing reference %s as map reference: %i", saving.name, map);
					break;
				}
				default:
					conoutf(CON_ERROR, "ERROR: unsupported reference type %i for reference %s, saving as NULL", saving.type, saving.name);
				case ::reference::T_INV:
				case ::reference::T_EQUIP:
					type = ::reference::T_INVALID;
				case ::reference::T_INVALID:
					if(DEBUG_IO) conoutf(CON_DEBUG, "DEBUG: writing null reference %s", saving.name);
					f->putchar(type);
					break;
			}
		)
	}

	void readreferences(stream *f, vector<mapinfo *> &maps, hashtable<const char*, ::reference> &stack)
	{
		static ::reference dummy("");
		int num = f->getlil<int>();
		loopi(num)
		{
			const char *name = readstring(f);
			::reference *loading = &stack.access(name, dummy);
			if(dummy.name != loading->name)
				delete[] name;
			else
				loading->name = name;

			char type = f->getchar();
			switch(type)
			{
				case ::reference::T_CHAR:
				case ::reference::T_ITEM:
				case ::reference::T_OBSTACLE:
				case ::reference::T_CONTAINER:
				case ::reference::T_PLATFORM:
				case ::reference::T_TRIGGER:
				{
					int map = f->getlil<int>();
					int ent = f->getlil<int>();

					if(map == -1 && ent == -1)
					{
						if(DEBUG_IO) conoutf(CON_DEBUG, "DEBUG: reading player char reference %s", loading->name);
						loading->setref(game::player1);
					}
					else if(maps.inrange(map) && maps[map]->objs.inrange(ent))
					{
						if(DEBUG_IO) conoutf(CON_DEBUG, "DEBUG: reading valid rpgent reference %s: %i %i", loading->name, map, ent);
						loading->setref(maps[map]->objs[ent]);
					}
					else if(DEBUG_IO) conoutf(CON_DEBUG, "DEBUG: reading in valid rpgent reference %s: %i %i", loading->name, map, ent);

					break;
				}
				case ::reference::T_MAP:
				{
					int map = f->getlil<int>();
					if(DEBUG_IO) conoutf(CON_DEBUG, "DEBUG: reading map reference %s: %i", loading->name, map);
					if(maps.inrange(map)) loading->setref(maps[map]);
					break;
				}
				case ::reference::T_INV:
				case ::reference::T_EQUIP:
				case ::reference::T_INVALID:
					if(DEBUG_IO) conoutf(CON_DEBUG, "DEBUG: reading null referene %s", loading->name);
					loading->setnull();
					break;
				default:
					conoutf(CON_ERROR, "ERROR: unsupported reference type %i for reference %s, this will cause issues; aborting", type, loading->name);
					abort = true;
					return;
			}
		}
	}

	void readdelayscript(stream *f, vector<mapinfo *> &maps, delayscript *loading)
	{
		readreferences(f, maps, loading->refs);
		loading->script = readstring(f);
		loading->remaining = f->getlil<int>();
	}

	void writedelayscript(stream *f, vector<mapinfo *> &maps, delayscript *saving)
	{
		writereferences(f, maps, saving->refs);
		writestring(f, saving->script);
		f->putlil(saving->remaining);
	}

	void loadgame(const char *name)
	{
		defformatstring(file)("data/rpg/saves/%s.sgz", name);
		stream *f = opengzfile(file, "rb");

		if(!f)
		{
			conoutf(CON_ERROR, "ERROR: unable to read file: %s", file);
			return;
		}

		saveheader hdr;
		f->read(&hdr, sizeof(saveheader));
		lilswap(&hdr.sversion, 2);

		if(hdr.sversion != GAME_VERSION || strncmp(hdr.magic, GAME_MAGIC, 4))
		{
			conoutf(CON_ERROR, "ERROR: Unsupported version or corrupt save: %i (%i) - %4.4s (%s)", hdr.sversion, GAME_VERSION, hdr.magic, GAME_MAGIC);
			delete f;
			return;
		}
		if(DEBUG_IO)
			conoutf(CON_DEBUG, "DEBUG: supported save: %i  %4.4s", hdr.sversion, hdr.magic);

		const char *data = readstring(f);
		game::newgame(data, true);
		delete[] data;
		abort = false;

		if(game::compatversion > hdr.gversion)
		{
			conoutf(CON_ERROR, "ERROR: saved game is of game version %i, last compatible version is %i; aborting", hdr.gversion, game::compatversion);
			delete f;
			localdisconnect();
			return;
		}

		const char *curmap = readstring(f);
		if(!curmap)
		{
			delete f;
			conoutf(CON_ERROR, "ERROR: no game/map in progress? aborting");
			localdisconnect();
			return;
		}

		lastmap = game::accessmap(curmap);
		readent(f, game::player1);

		int num;
		#define READ(m, b) \
			num = f->getlil<int>(); \
			loopi(num) \
			{ \
				if(abort) break; \
				if(f->end()) \
				{ \
					conoutf(CON_ERROR, "ERROR: unexpected EoF, aborting"); \
					abort = true; break; \
				} \
				if(DEBUG_IO) \
					conoutf(CON_DEBUG, "DEBUG: reading " #m " %i of %i", i + 1, num); \
				b; \
			}

		READ(hotkey,
			int b = f->getlil<int>();
			int u = f->getlil<int>();
			game::hotkeys.add(equipment(b, u));
		)

		READ(faction,
			if(!game::factions.inrange(i)) game::factions.add(new faction());
			readfaction(f, game::factions[i]);
		);
		READ(recipe,
			if(!game::recipes.inrange(i)) game::recipes.add(new recipe());
			readrecipe(f, game::recipes[i]);
		);
		READ(variable,
			if(!game::variables.inrange(i)) game::variables.add(0);
			game::variables[i] = f->getlil<int>();
		);
		vector<mapinfo *> maps;
		READ(mapinfo, maps.add(readmap(f)));

		READ(reference stack,
			if(!rpgscript::stack.inrange(i)) rpgscript::pushstack();
			readreferences(f, maps, *rpgscript::stack[i]);
		)

		READ(delayscript stack,
			rpgscript::delaystack.add(new delayscript());
			readdelayscript(f, maps, rpgscript::delaystack[i]);
		)

		#undef READ

		delete f;
		updates.shrink(0);

		if(abort)
		{
			conoutf(CON_ERROR, "ERROR: aborted - something went seriously wrong");
			localdisconnect();
		}

		game::transfer = true;
		game::openworld(curmap);
		delete[] curmap;
	}
	COMMAND(loadgame, "s");

	void savegame(const char *name)
	{
		if(!game::mapdata || !game::curmap)
		{
			conoutf(CON_ERROR, "ERROR: No game in progress, can't save");
			return;
		}
		else if(!game::cansave())
		{
			conoutf("You may not save at this time");
			return;
		}

		defformatstring(file)("data/rpg/saves/%s.sgz.tmp", name);
		stream *f = opengzfile(path(file), "wb");

		if(!f)
		{
			conoutf(CON_ERROR, "ERROR: failed to create savegame");
			return;
		}

		saveheader hdr;
		hdr.sversion = GAME_VERSION;
		hdr.gversion = game::gameversion;
		memcpy(hdr.magic, GAME_MAGIC, 4);

		lilswap(&hdr.sversion, 2);
		f->write(&hdr, sizeof(saveheader));

		writestring(f, game::data);
		writestring(f, game::curmap->name);
		writeent(f, game::player1);
		game::curmap->objs.removeobj(game::player1);

		#define WRITE(m, v, b) \
			f->putlil(v.length()); \
			loopv(v) \
			{ \
				if(DEBUG_IO) \
					conoutf(CON_DEBUG, "DEBUG: Writing " #m " %i of %i", i + 1, v.length()); \
				b; \
			}

		WRITE(hotkey, game::hotkeys,
			f->putlil(game::hotkeys[i].base);
			f->putlil(game::hotkeys[i].use);
		)

		WRITE(faction, game::factions,
			writefaction(f, game::factions[i]);
		)
		WRITE(recipe, game::recipes,
			writerecipe(f, game::recipes[i]);
		)
		WRITE(variable, game::variables,
			f->putlil(game::variables[i]);
		)

		vector<mapinfo *> maps;
		enumerate(*game::mapdata, mapinfo, map, maps.add(&map););
		WRITE(map, maps, writemap(f, maps[i]));

		WRITE(reference stack, rpgscript::stack,
			writereferences(f, maps, *rpgscript::stack[i]);
		)

		WRITE(delayscript stack, rpgscript::delaystack,
			writedelayscript(f, maps, rpgscript::delaystack[i]);
		)

		game::curmap->objs.add(game::player1);
		DELETEP(f);

		string actual, final;
		formatstring(actual)("%s", findfile(file, "wb"));
		copystring(final, actual, strlen(actual) - 3);
		rename(actual, final);

		conoutf("Game saved successfully to data/rpg/%s.sgz", name);

		copystring(file + strlen(file) - 8, ".png", 5);
		scaledscreenshot(file, 2, 256, 256);
		copystring(file + strlen(file) - 4, "", 1);
		reloadtexture(file);
	}
	COMMAND(savegame, "s");
}