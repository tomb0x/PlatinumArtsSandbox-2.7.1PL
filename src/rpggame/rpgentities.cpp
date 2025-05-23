#include "rpggame.h"

namespace entities
{
	vector<extentity *> ents;
	vector<extentity *> &getents() { return ents; }

	vector<int> intents;

	bool mayattach(extentity &e) { return false; }
	bool attachent(extentity &e, extentity &a) { return false; }
	int extraentinfosize() {return 0;}

	void animatemapmodel(const extentity &e, int &anim, int &basetime)
	{
		anim = ANIM_MAPMODEL|ANIM_LOOP;
	}

	extentity *newentity() { return new rpgentity(); }
	void deleteentity(extentity *e) { intents.setsize(0); delete (rpgentity *)e; }

	void genentlist() //filters all interactive ents
	{
		loopv(ents)
		{
			const int t = ents[i]->type;
			if(t == JUMPPAD)
				intents.add(i);
		}
	}

	void touchents(rpgent *d)
	{
		loopv(intents)
		{
			extentity &e = *ents[intents[i]];

			switch(e.type)
			{
				case JUMPPAD:
					if(lastmillis - d->lasttouch < 500 || d->o.dist(e.o) > (e.attr[3] ? e.attr[3] : 12)) break;

					d->falling = vec(0, 0, 0);
					d->vel.add(vec(e.attr[2], e.attr[1], e.attr[0]).mul(10));

					d->lasttouch = lastmillis;
					break;
			}
		}
	}

	void clearents()
	{
		ents.deletecontents();
		intents.setsize(0);
		//loopvrev(ents) { delete (rpgentity *) ents[i];}
		//ents.shrink(0);
	}

	const char *entmodel(const entity &e)
	{
		switch(e.type)
		{
			case CRITTER:
				if(game::chars.inrange(e.attr[1]))
					return game::chars[e.attr[1]]->mdl;
				break;
			case ITEM:
				if(game::items.inrange(e.attr[1]))
					return game::items[e.attr[1]]->mdl;
				break;
			case OBSTACLE:
				if(game::obstacles.inrange(e.attr[1]))
					return game::obstacles[e.attr[1]]->mdl;
				break;
			case CONTAINER:
				if(game::containers.inrange(e.attr[1]))
					return game::containers[e.attr[1]]->mdl;
				break;
			case PLATFORM:
				if(game::platforms.inrange(e.attr[1]))
					return game::platforms[e.attr[1]]->mdl;
				break;
			case TRIGGER:
				if(game::triggers.inrange(e.attr[1]))
					return game::triggers[e.attr[1]]->mdl;
				break;
		}
		return NULL;
	}

	FVARP(entpreviewalpha, 0, .4, 1);

	void renderentities()
	{
		loopv(ents)
		{
			extentity &e = *ents[i];
			const char *mdl = entmodel(e);
			//extern void rendermodel(entitylight *light, const char *mdl, int anim, const vec &o, float yaw = 0, float pitch = 0, float roll = 0, int cull = MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED | MDL_LIGHT, dynent *d = NULL, modelattach *a = NULL, int basetime = 0, int basetime2 = 0, float trans = 1, int colour = 0xFFFFFF);
			if(mdl && entpreviewalpha)
			{
				rendermodel(
					&e.light, mdl, (e.type == CRITTER ? ANIM_IDLE : ANIM_MAPMODEL)|ANIM_LOOP,
					e.o, e.attr[0] + (e.type == CRITTER ? 90 : 0), 0, 0,
					MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED | MDL_LIGHT,
					NULL, NULL, 0, 0, entpreviewalpha
				);
			}
		}
	}

	void startmap()
	{
		loopv(ents)
		{
			extentity &e = *ents[i];
			switch(e.type)
			{
				case CRITTER:
					spawn(e, e.attr[1], ENT_CHAR, 1);
					break;
				case ITEM:
					spawn(e, e.attr[1], ENT_ITEM, e.attr[2]);
					break;
				case OBSTACLE:
					spawn(e, e.attr[1], ENT_OBSTACLE, 1);
					break;
				case CONTAINER:
					spawn(e, e.attr[1], ENT_CONTAINER, 1);
					break;
				case PLATFORM:
					spawn(e, e.attr[1], ENT_PLATFORM, 1);
					break;
				case TRIGGER:
					spawn(e, e.attr[1], ENT_TRIGGER, 1);
					break;
			}
		}
	}

	void spawn(extentity &e, int ind, int type, int qty)
	{
		rpgent *ent = NULL;
		switch(type)
		{
			case ENT_CHAR:
			{
				if(DEBUG_ENT)
					conoutf(CON_DEBUG, "DEBUG: Creating creature and instancing to type %i", ind);

				rpgchar *d = new rpgchar();
				ent = d;

				if(game::chars.inrange(ind))
					game::chars[ind]->transfer(*d);
				else
					conoutf(CON_ERROR, "ERROR: creature type %i does not exist", ind);

				d->health = d->base.getmaxhp();
				d->mana = d->base.getmaxmp();

				break;
			}
			case ENT_ITEM:
			{
				if(DEBUG_ENT)
					conoutf(CON_DEBUG, "DEBUG: Creating item and instancing to type %i", ind);

				rpgitem *d = new rpgitem();
				ent = d;

				if(game::items.inrange(ind))
				{
					d->base = ind;
					d->quantity = qty <= 0 ? 1 : qty;
				}
				else
					conoutf(CON_ERROR, "ERROR: item type %i does not exist", ind);

				break;
			}
			case ENT_OBSTACLE:
			{
				if(DEBUG_ENT)
					conoutf(CON_DEBUG, "DEBUG: Creating obstacle and instancing to type %i", ind);

				rpgobstacle *d = new rpgobstacle();
				ent = d;

				if(game::obstacles.inrange(ind))
					game::obstacles[ind]->transfer(*d);
				else
					conoutf(CON_ERROR, "ERROR: obstacle type %i does not exist", ind);

				break;
			}
			case ENT_CONTAINER:
			{
				if(DEBUG_ENT)
					conoutf(CON_DEBUG, "DEBUG: Creating container and instancing to type %i", ind);

				rpgcontainer *d = new rpgcontainer();
				ent = d;

				if(game::containers.inrange(ind))
					game::containers[ind]->transfer(*d);
				else
					conoutf(CON_ERROR, "ERROR: container type %i does not exist", ind);

				break;
			}
			case ENT_PLATFORM:
			{
				if(DEBUG_ENT)
					conoutf(CON_DEBUG, "DEBUG: Creating platform and instancing to type %i", ind);

				rpgplatform *d = new rpgplatform();
				ent = d;

				if(game::platforms.inrange(ind))
					game::platforms[ind]->transfer(*d);
				else
					conoutf(CON_ERROR, "ERROR: platform type %i does not exist", ind);

				break;
			}
			case ENT_TRIGGER:
			{
				if(DEBUG_ENT)
					conoutf(CON_DEBUG, "DEBUG: Creating trigger and instancing to type %i", ind);

				rpgtrigger *d = new rpgtrigger();
				ent = d;

				if(game::triggers.inrange(ind))
					game::triggers[ind]->transfer(*d);
				else
					conoutf(CON_ERROR, "ERROR: trigger type %i does not exist", ind);

				break;
			}
		}

		ent->resetmdl();
		setbbfrommodel(ent, ent->temp.mdl);
		ent->o = e.o;

		if(e.type == SPAWN && e.attr[1])
		{
			ent->o.add(vec(rnd(360) * RAD, 0).mul(rnd(e.attr[1])));
			entinmap(ent);
		}
		else
		{
			ent->o.z += ent->eyeheight;
			ent->resetinterp();
		}

		ent->yaw = e.attr[0];

		game::curmap->objs.add(ent);
		ent->getsignal("spawn", false);
	}

	void teleport(rpgent *d, int dest)
	{
		loopv(ents)
		{
			if(ents[i]->type == TELEDEST && ents[i]->attr[1] == dest)
			{
				d->yaw = ents[i]->attr[0];
				d->pitch = 0;
				d->o = ents[i]->o; entinmap(d, true);
				d->vel = vec(0, 0, 0);
				updatedynentcache(d);
				return;
			}
		}
		conoutf("ERROR: no teleport destination %i", dest);
	}

	void entradius(extentity &e, bool &color)
	{
		switch(e.type)
		{
			case TELEDEST:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, 12);

				break;
			}
			case JUMPPAD:
			{
				vec dir(e.attr[2], e.attr[1], e.attr[0]); dir.normalize();
				renderentarrow(e, dir, e.attr[3] ? e.attr[3] : 12);
				renderentsphere(e, e.attr[3] ? e.attr[3] : 12);

				break;
			}
			case CHECKPOINT:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, 12);
				renderentsphere(e, e.attr[3] ? e.attr[3] : 12);

				break;
			}
			case CRITTER:
			case ITEM:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, 12);
				break;
			}
			case SPAWN:
			{
				vec dir(e.attr[0] * RAD, 0);
				renderentarrow(e, dir, e.attr[1] ? e.attr[1] : 12);
				renderentring(e, e.attr[1], 0);

				break;
			}
			case LOCATION:
			{
				renderentring(e, e.attr[1], 0);
				break;
			}
			case CAMERA:
			{
				vec dir(e.attr[1] * RAD, e.attr[2] * RAD);
				renderentarrow(e, dir, 12);

				break;
			}
		}
	}

	bool printent(extentity &e, char *buf)
	{
		return false;
	}

	void fixentity(extentity &e)
	{
		switch(e.type)
		{
			case TELEDEST:
			case CHECKPOINT:
			case CRITTER:
			case ITEM:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
			case SPAWN:
				e.attr.pop();
				e.attr.insert(0, game::player1->yaw);
		}
	}

	void renderhelpertext(extentity &e, int &colour, vec &pos, string &tmp)
	{
		switch(e.type)
		{
			case TELEDEST:
				pos.z += 3.0f;
				formatstring(tmp)("Yaw: %i\nTag: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case JUMPPAD:
				pos.z += 6.0f;
				formatstring(tmp)("Z: %i\nY: %i\nX: %i\nRadius: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2],
					e.attr[3]
				);
				break;
			case CHECKPOINT:
				pos.z += 1.5f;
				formatstring(tmp)("Yaw: %i",
					e.attr[0]
				);
				break;
			case SPAWN:
			{
				pos.z += 4.5f;
				formatstring(tmp)("Yaw: %i\nRadius: %i\nTag: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2]
				);
				break;
			}
			case LOCATION:
			{
				pos.z += 3.0f;
				formatstring(tmp)("Tag: %i\nRadius: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			}
			case BLIP:
				pos.z += 3.0f;
				formatstring(tmp)("Texture?: %i\nTag: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case CAMERA:
				pos.z += 6.0f;
				formatstring(tmp)("Tag: %i\nYaw: %i\nPitch: %i\nRoll: %i",
					e.attr[0],
					e.attr[1],
					e.attr[2],
					e.attr[3]
				);
				break;
			case CRITTER:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
				pos.z += 3.0f;
				formatstring(tmp)("Yaw: %i\nIndex: %i",
					e.attr[0],
					e.attr[1]
				);
				break;
			case ITEM:
				pos.z += 4.5f;
				formatstring(tmp)("Yaw: %i\nIndex: %i\nQuantity: %i",
					e.attr[0],
					e.attr[1],
					max(1, e.attr[2])
				);
		}
	}

	const char *entname(int i)
	{
		static const char *entnames[] =
		{
			"none?", "light", "mapmodel", "playerstart", "envmap", "particles", "sound", "spotlight",
			"teledest", "jumppad", "checkpoint", "spawn", "location", "reserved", "blip", "camera", "platformroute", "critter", "item", "obstacle", "container", "platform", "trigger"
		};
		return i>=0 && size_t(i)<sizeof(entnames)/sizeof(entnames[0]) ? entnames[i] : "";
	}

	const int numattrs(int type)
	{
		static const int num[] =
		{
			2, //teledest
			4, //jumppad
			1, //checkpoint
			3, //spawn
			2, //location
			0, //reserved
			2, //blip
			4, //camera
			1, //platformroute
			2, //critter
			3, //item
			2, //obstacle
			2, //container
			2, //platform
			2, //trigger
		};

		type -= ET_GAMESPECIFIC;
		return type >= 0 && size_t(type) < sizeof(num)/sizeof(num[0]) ? num[type] : 0;
	}

	const char *entnameinfo(entity &e)
	{
		return "";
	}

	bool radiusent(extentity &e)
	{
		switch(e.type)
		{
			case LIGHT:
			case ENVMAP:
			case MAPSOUND:
			case JUMPPAD:
			case SPAWN:
			case LOCATION:
				return true;
				break;
			default:
				return false;
				break;
		}
	}

	bool dirent(extentity &e)
	{
		switch(e.type)
		{
			case MAPMODEL:
			case PLAYERSTART:
			case SPOTLIGHT:
			case TELEDEST:
			case CHECKPOINT:
			case JUMPPAD:
			case CRITTER:
			case ITEM:
			case OBSTACLE:
			case CONTAINER:
			case PLATFORM:
			case TRIGGER:
			case SPAWN:
			case CAMERA:
				return true;
				break;
			default:
				return false;
				break;
		}
	}

	int *getmodelattr(extentity &e)
	{
		return NULL;
	}

	bool checkmodelusage(extentity &e, int i)
	{
		return false;
	}

	//mapmodels to be purely decorative
	void rumble(const extentity &e) {}
	void trigger(extentity &e){}
	void editent(int i, bool local) {}
	void writeent(entity &e, char *buf) {}

	void readent(entity &e, char *buf, int ver)
	{
		if(ver <= 30) switch(e.type)
		{
			case TELEDEST:
			case SPAWN:
				e.attr[0] = (int(e.attr[0])+180)%360;
				break;
		}
	}

	float dropheight(entity &e)
	{
		if (e.type==MAPMODEL) return 0.0f;
		return 4.0f;
	}
}