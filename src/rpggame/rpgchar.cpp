#include "rpggame.h"

///USE apply functionality - limited to characters and derived

void use::apply(rpgchar *user)
{
	loopv(effects)
		user->seffects.add(new victimeffect(user, effects[i], chargeflags, 1));
}

void use_armour::apply(rpgchar *user)
{
	float base, extra;
	user->base.skillpotency(skill, base, extra);

	loopv(effects)
	{
		if(!game::statuses.inrange(effects[i]->status)) continue;
		statusgroup *sg = game::statuses[effects[i]->status];
		int resist = 0;
		int thresh = 0;
		if(!sg->friendly)
		{
			resist = user->base.getresistance(effects[i]->element);
			thresh = user->base.getthreshold(effects[i]->element);
		}

		loopvj(sg->effects)
		{
			if(sg->effects[j]->duration >= 0)
				conoutf(CON_ERROR, "ERROR: statuses[%i]->effect[%i] has a non negative duration, these will not work properly and some will be ridiculously overpowered", effects[i]->status, j);
			sg->effects[j]->update(user, user, resist, thresh, base, extra);
		}
	}
}

void use_weapon::apply(rpgchar *user) {} // does nothing, do it in the character update loop

///base functions
bool rpgchar::checkammo(equipment &eq, equipment *quiver, bool remove)
{
	//it is assumed it's valid
	use_weapon *wep = (use_weapon *) game::items[eq.base]->uses[eq.use];
	switch(wep->ammo)
	{
		case -3:
			if(base.experience < wep->cost)
			{
				if(this == game::player1) conoutf("You lack sufficient exp to perform this action");
				else if (DEBUG_AI) conoutf(CON_DEBUG, "DEBUG: AI attack interrupted for %p, too little exp", this);
				return false;
			}
			if(remove) base.experience -= wep->cost;
			return true;
		case -2: case -1:
		{
			if((wep->ammo == -1 ? mana : health) < wep->cost)
			{
				if(this == game::player1) conoutf(wep->ammo == -1 ? "You lack the mana to cast the full spell" : "Doing this would be suicide");
				else if (DEBUG_AI) conoutf(CON_DEBUG, "DEBUG: AI attack interrupted for %p, too little %s", this, wep->ammo == -1 ? "mana" :"health");
				return false;
			}
			if(remove) (wep->ammo == -1 ? mana : health) -= wep->cost;
			return true;
		}
		default:
		{
			if(!game::ammotypes.inrange(wep->ammo))
			{
				conoutf(CON_ERROR, "ERROR: entity %p trying to attack with weapon using invalid ammotype %i; out of range", wep->ammo);
				return false;
			}
			ammotype *at = game::ammotypes[wep->ammo];
			int base = -1;

			if(quiver)     base = at->items.find(quiver->base);
			if(base == -1) base = at->items.find(eq.base);
			if(base == -1)
			{
				if(this == game::player1) conoutf("You have the wrong ammo equipped for the current weapon");
				else if (DEBUG_AI) conoutf(CON_DEBUG, "DEBUG: AI attack interrupted for %p, wrong ammo equipped", this);
				return false;
			}

			if(getitemcount(at->items[base]) < wep->cost)
			{
				if(this == game::player1) conoutf("You have too little ammo remaining to attack again");
				else if (DEBUG_AI) conoutf(CON_DEBUG, "DEBUG: AI attack interrupted for %p, too little ammo", this);
				return false;
			}

			if(remove)
			{
				modinv(at->items[base], -wep->cost, false);
				if(getitemcount(at->items[base]) == 0) return false; //this is the remove ammo pass - let it be known the quiver is no longer valid
			}
			return true;
		}
	}
}

VAR(lefthand, 1, 0, -1);

void rpgchar::doattack(equipment *eleft, equipment *eright, equipment *quiver)
{
	use_weapon *left = eleft ? (use_weapon *) game::items[eleft->base]->uses[eleft->use] : NULL,
	           *right = eright ? (use_weapon *) game::items[eright->base]->uses[eright->use] : NULL,
	           *ammo = NULL;

	if(charge && !primary && !secondary)
	{
		if((left && lastprimary) || (left == right && lastsecondary))
			attack = left;
		else
			attack = right;
	}
	else if((primary || secondary) && left && left == right)
		attack = right;
	else if((primary && left) ^ (secondary && right))
		attack = (primary && left) ? left : right;

	lastprimary = primary; lastsecondary = secondary;

	//we're attacking and can attack, oh noes!
	if(attack)
	{
		lefthand = attack == left;

		if(attack->cost)
		{
			if(!checkammo(attack == left ? *eleft : *eright, quiver))
			{
				lastaction = lastmillis + 100;
				primary = secondary = 0;
				return; //we lack the catalysts, don't attack
			}
			if(quiver && (!game::ammotypes.inrange(attack->ammo) || -1 == game::ammotypes[attack->ammo]->items.find(quiver->base))) quiver = NULL;
		}
		ammo = (use_weapon *) ((quiver && attack->ammo >= 0) ? game::items[quiver->base]->uses[quiver->use] : NULL);

		float mult = attack->maxcharge;
		if(attack->charge)
		{
			if(primary || secondary)
			{
				charge += curtime;
				return; //we're charging, don't attack... yet
			}

			mult = min<float>((float)charge / attack->charge, attack->maxcharge);
			charge = 0;

			if(mult < attack->mincharge)
				return;
		}

		//HACK
		//propogate an attack signal to self and all items within instead of this
		{
			invstack *wep = NULL, *amm = NULL;
			loopv(inventory)
			{
				if((attack == left && inventory[i]->base == eleft->base) || (right && inventory[i]->base == eright->base))
					wep = inventory[i];
				else if (ammo && inventory[i]->base == quiver->base)
					amm = inventory[i];
			}

			if(wep) wep->getsignal("attacksound", false, this, (attack == left ? eleft : eright)->use);
			if(amm) amm->getsignal("attacksound", false, this, quiver->use);
			getsignal("attacksound", false, this); //mainly for grunts
		}

		{
			float potency, extra;
			base.skillpotency(attack->skill, potency, extra);
			mult += attack->basecharge;
			mult = mult * (potency + extra);
		}

		int flags = attack->chargeflags | (ammo ? ammo->chargeflags : 0);

		switch(attack->target)
		{
			case T_SINGLE:
			case T_MULTI:
			case T_AREA:
			{
				projectile *p = game::curmap->projs.add(new projectile());
				p->init(this, attack == left ? eleft : eright, attack->ammo >= 0 ? quiver : NULL, 0, mult);
				break;
			}

			case T_SELF:
				hit(this, attack, ammo, mult, flags, vec(0, 0, 1));
				if(game::effects.inrange(attack->deatheffect))
					game::effects[attack->deatheffect]->drawaura(this, mult, effect::DEATH, 0);
				break;

			case T_SMULTI:
			{
				//UNIMPLEMENTED
				break;
			}
			case T_SAREA:
			{
				//UNIMPLEMENTED
				break;
			}

			case T_HORIZ:
			case T_VERT:
			{
				vec perp;
				if(attack->target == T_HORIZ)
					perp = vec(yaw * RAD, (pitch + 90) * RAD);
				else
					perp = vec((yaw + 90) * RAD, 0);

				vec dir = vec(yaw * RAD, pitch * RAD).rotate(attack->angle / 2.f * RAD, perp);
				vec orig = vec(o).sub(vec(0, 0, eyeheight / 2));
				vector<rpgent *> hit;
				vector<vec> hits;
				int reach = (attack->range + (ammo ? ammo->range : 0)) * (flags & CHARGE_TRAVEL ? mult : 1) + radius;

				loopi(attack->angle + 1)
				{
					vec ray = vec(dir).mul(reach).add(orig);
					loopvj(game::curmap->objs)
					{
						rpgent *obj = game::curmap->objs[j];
						float dist;
						if(obj != this && game::intersect(obj, orig, ray, dist) && dist <= 1)
						{
							if(hit.find(obj) == -1) hit.add(obj);
							hits.add(vec(ray).sub(orig).mul(dist));
						}
					}

					dir.rotate(-1 * RAD, perp);
				}

				loopv(hit)
					hit[i]->hit(this, attack, ammo, mult, flags, vec(yaw * RAD, pitch * RAD));

				loopv(hits)
				{
					vec pos = vec(hits[i]).add(orig);
					if(game::effects.inrange(attack->deatheffect))
						game::effects[attack->deatheffect]->drawsplash(pos, vec(0, 0, 0), 5, 1, effect::DEATH, 1);
				}
				if(game::effects.inrange(attack->traileffect))
					game::effects[attack->traileffect]->drawcircle(this, attack, mult, effect::TRAIL_SINGLE, 0);

				break;
			}

			case T_CONE:
			{
				//UNIMPLEMENTED
				break;
			}
		}
		//recoil
		vel.add(vec(yaw * RAD, pitch * RAD).mul( -1 * (attack->recoil + (ammo ? ammo->recoil : 0))));

		if(attack->cost)
			checkammo(attack == left ? *eleft : *eright, quiver, true); //remove ammo

		lastaction = lastmillis + attack->cooldown + (ammo ? ammo->cooldown : 0);
	}
}

void rpgchar::resetmdl()
{
	temp.mdl = mdl;
}

///REMEMBER route IS REVERSED
void rpgchar::doai(equipment *eleft, equipment *eright, equipment *quiver)
{
	//TODO 1 - minimise friendly fire; try to avoid killing allies
	//TODO 2 - ranged attackers/spellcasters; keep distance/retreat
	//TODO 3 - ranged attacks; dodge
	//TODO 4 - use objects (ie teleports) to follow the player

	if(aiflags & AI_MOVE)
	{
		int end = ai::closestwaypoint(dest) - ai::waypoints.getbuf();
		if(!route.length() || route[0] != end)
		{
			route.setsize(0);
			int start = ai::closestwaypoint(feetpos()) - ai::waypoints.getbuf();
			ai::findroute(start, end, route);
		}

		if(route.length())
		{
			waypoint &first = ai::waypoints[route[route.length() - 1]];
			if(first.o.dist(feetpos()) <= 4)
				route.pop();
			else if(route.length() > 1)
			{
				waypoint &second = ai::waypoints[route[route.length() - 2]];
				const vec feet = feetpos();
				if(first.o.dist(second.o) >= feet.dist(second.o) || feet.dist(second.o) < feet.dist(second.o))
					route.pop();
			}
		}
	}

	if(target && aiflags & (AI_ATTACK | AI_ALERT))
	{
		vec dir = vec(lastknown).sub(feetpos());
		dir.z += target->eyeheight - eyeheight;
		vectoyawpitch(dir, yaw, pitch);
	}
	else
	{
		if(route.length() > 1)
		{
			vec dir = vec(ai::waypoints[route[route.length() - 2]].o).sub(feetpos());
			dir.mul(curtime / 200.f / dir.magnitude());
			dir.add(vec(yaw * RAD, pitch * RAD).mul(1 - (curtime / 200.0f)));
			//some basic interpolation
			//200 chosen as it's the maximum interval between updates
			vectoyawpitch(dir, yaw, pitch);
		}
	}

	primary = secondary = false;
	if(target && aiflags & AI_ALERT)
	{
		bool left = true;
		if(eleft)
		{
			attack = (use_weapon *) game::items[eleft->base]->uses[eleft->use];
			if(attack->type < USE_WEAPON || !attack->effects.length() || game::statuses[attack->effects[0]->status]->friendly)
				attack = NULL;
		}
		if(!attack && eright)
		{
			left = false;
			attack = (use_weapon *) game::items[eright->base]->uses[eright->use];
			if(attack->type < USE_WEAPON || !attack->effects.length() || game::statuses[attack->effects[0]->status]->friendly)
				attack = NULL;
		}


		if(attack)
		{
			if(attack->charge)
			{
				if(left) {primary = true;}
				else {secondary = true;}

				if((float) charge / attack->charge >= attack->maxcharge && cansee(target))
					primary = secondary = false;
			}
			else if(cansee(target) && (attack->range + radius + target->radius) >= o.dist(target->o))
			{
				if(left) primary = true;
				else secondary = true;
			}
		}
		attack = NULL;
	}

	if(aiflags & AI_MOVE && route.length())
	{
		vec dir = vec(ai::waypoints[route.last()].o).sub(feetpos());
		dir.z = 0; dir.normalize();
		dir.rotate_around_z(-yaw * RAD);

		if(fabs(dir.y) >= .7)
			move = dir.y < 0 ? -1 : 1;
		if(fabs(dir.x) >= .7)
			strafe = dir.x < 0 ? -1 : 1;
	}
}

void rpgchar::update()
{
	resetmdl();
	temp.alpha = 1;
	temp.light = vec4(0, 0, 0, 0);
	attack = NULL;

	if(state == CS_DEAD)
	{
		move = strafe = 0;
	}
	else
	{
		base.setspeeds(maxspeed, jumpvel);
		mana =   min<float>(base.getmaxmp(),   mana + (base.getmpregen() * curtime / 1000.0f));
		health = min<float>(base.getmaxhp(), health + (base.gethpregen() * curtime / 1000.0f));

		if(health < 0)
			die(NULL);
	}

	if(state == CS_DEAD)
	{
		if(ragdoll)
			moveragdoll(this);
		else
			moveplayer(this, this == game::player1 ? 8 : 2, false);

		base.resetdeltas();
		return;
	}

		//handle equipment
	equipment *eleft = NULL, //primary
	          *eright = NULL, //secondary
	          *quiver = NULL;
	loopv(equipped)
	{
		use *usable = game::items[equipped[i]->base]->uses[equipped[i]->use];
		usable->apply(this);
		if(usable->type == USE_WEAPON)
		{
			use_weapon *wep = (use_weapon *) usable;
			if(wep->slots & SLOT_LHAND)
				eleft = equipped[i];
			if(wep->slots & SLOT_RHAND)
				eright = equipped[i];
			if(wep->slots & SLOT_QUIVER)
				quiver = equipped[i];
		}
	}

	///AI STUFF - player can use it during cutscenes
	if(this != game::player1 || camera::cutscene)
	{
		move = strafe = jumping = 0;
		aiflags = 0;
		directives.sort(directive::compare);
		loopv(directives)
		{
			if(!directives[i]->update(this))
			{
				delete directives[i]; directives.remove(i); i--;
			}
		}

		doai(eleft, eright, quiver);
	}
	else if(directives.length())
	{
		directives.deletecontents();
		move = strafe = jumping = 0;
	}

	moveplayer(this, this == game::player1 ? 8 : 2, true);
	entities::touchents(this);

	if(lastmillis >= lastaction)
		doattack(eleft, eright, quiver);

	base.resetdeltas();
}

void rpgchar::render(bool mainpass)
{
	int lastaction = 0,
	anim = ANIM_ATTACK1,
	delay = 300,
	hold = ANIM_HOLD1;

	vector<modelattach> attachments;
	vec *emitter = emitters;

	loopv(equipped)
	{
		use_armour *use = (use_armour *) game::items[equipped[i]->base]->uses[equipped[i]->use];
		if(use->type < USE_ARMOUR || !use->vwepmdl || !use->slots) continue;

		const char *tag = NULL;
		if(use->slots & SLOT_LHAND) tag = "tag_lhand";
		else if(use->slots & SLOT_RHAND) tag = "tag_rhand";
		else if(use->slots & SLOT_LEGS) tag = "tag_legs";
		else if(use->slots & SLOT_ARMS) tag = "tag_arms";
		else if(use->slots & SLOT_TORSO) tag = "tag_torso";
		else if(use->slots & SLOT_HEAD) tag = "tag_head";
		else if(use->slots & SLOT_FEET) tag = "tag_feet";
		else if(use->slots & SLOT_QUIVER) tag = "tag_quiver";
		attachments.add(modelattach(tag, use->vwepmdl));

		if(use->idlefx >= 0 && mainpass)
		{
			attachments.add(modelattach("tag_partstart", emitter));
			attachments.add(modelattach("tag_partend", emitter + 1));
			emitter += 2;
		}
	}
	attachments.add(modelattach()); //delimitor

	renderclient(this, temp.mdl ? temp.mdl : mdl, attachments.buf, hold, anim, delay, lastaction, state!=CS_DEAD ? 0 : 0 /* lastpain */, temp.alpha, true);

	emitter = emitters;
	if(mainpass) loopv(equipped)
	{
		use_armour *use = (use_armour *) game::items[equipped[i]->base]->uses[equipped[i]->use];
		if(use->type < USE_ARMOUR || !use->vwepmdl || use->idlefx < 0 || !use->slots)
			continue;

		game::effects[use->idlefx]->drawwield(emitter[0], emitter[1], 1, effect::TRAIL);
		emitter[0] = emitter[1] = vec(-1, -1, -1);
		emitter += 2;
	}
}

const char *rpgchar::getname() const {return name ? name : "Shirley";}

///Character/AI
void rpgchar::givexp(int xp)
{
	int level = base.level;
	base.givexp(xp);
	if(level != base.level)
		getsignal("level");
}

void rpgchar::equip(int i, int u)
{
	if(primary || secondary || lastprimary || lastsecondary)
	{
		conoutf("You can't equip items while attacking!");
		return;
	}
	else if(!game::items.inrange(i) || !game::items[i]->uses.inrange(u))
	{
		conoutf(CON_ERROR, "ERROR: unable to equip item %i with use %i; itself or the use does not exist", i, u);
		return;
	}

	invstack *item = NULL;
	loopvj(inventory)
	{
		if(inventory[j]->base == i)
		{
			item = inventory[j];
			break;
		}
	}
	if(!item || !item->quantity)
	{
		conoutf(CON_ERROR, "ERROR: user contains no instances of item %i", i);
		return;
	}
	else if(game::items[i]->uses[u]->type < USE_ARMOUR)
	{
		conoutf(CON_ERROR, "ERROR: invalid use type, needs to be of type weapon or armour to equip");
		return;
	}

	use_armour *usecase = (use_armour *) game::items[i]->uses[u];

	if(!usecase->reqs.meet(base))
	{
		conoutf("You cannot wield this item! You do not meet the requirements!");
		return;
	}

	if(usecase->slots)
		dequip(-1, usecase->slots);

	equipped.add(new equipment(i, u));
	item->getsignal("equip", false, this, u);
	item->quantity--;
}

bool rpgchar::dequip(int i, int slots)
{
	if(primary || secondary || lastprimary || lastsecondary)
	{
		conoutf("You can't dequip items while attacking!");
		return false;
	}
	int rem = 0;
	loopvj(equipped)
	{
		invstack *item = NULL;
		loopvk(inventory)
		{
			if(inventory[k]->base == equipped[j]->base)
			{
				item = inventory[k];
				break;
			}
		}
		//this should not happen
		if(!item)
		{
			item = new invstack(equipped[j]->base, 0);
			inventory.add(item);
		}


		use_armour *arm = (use_armour *) game::items[equipped[j]->base]->uses[equipped[j]->use];
		if((i == -1 ? true : equipped[j]->base == i) && (slots ? arm->slots & slots : true))
		{
			item->quantity++;
			delete equipped.remove(j--);
			rem++;
		}
	}
	return rem != 0;
}

void rpgchar::die(rpgent *killer)
{
	if(DEBUG_ENT)
		conoutf(CON_DEBUG, "DEBUG: ent %p killed by %p%s", this, killer, state != CS_ALIVE ? "; already dead?" : "");
	if(state != CS_ALIVE)
		return;

	if(killer)
		killer->givexp((killer == this ? -100 : 25) * base.level);

	state = CS_DEAD;
	health = 0;
	route.setsize(0);
	getsignal("death", true, killer); //in case the player has an item in his inventory that will revive him
}

void rpgchar::revive(bool spawn)
{
	if(state != CS_DEAD)
		return;

	health = base.getmaxhp();
	mana = base.getmaxmp();
	state = CS_ALIVE;
	seffects.deletecontents();

	physent::reset();
	cleanragdoll(this);
	if(spawn) findplayerspawn(this, -1);
	else entinmap(this);
}

extern int friendlyfire;

VAR(hit_friendly, 1, 0, -1);
VAR(hit_total, 1, 0, -1);

void rpgchar::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
{
	hit_friendly = 1;
	hit_total = 0;
	loopv(weapon->effects)
	{
		if(!game::statuses.inrange(weapon->effects[i]->status)) continue;
		if(!game::statuses[weapon->effects[i]->status]->friendly) hit_friendly = 0;
		//TODO friendly fire checks here
		seffects.add(new victimeffect(attacker, weapon->effects[i], weapon->chargeflags, mul));
		hit_total += game::statuses[weapon->effects[i]->status]->strength() * mul;
	}

	if(ammo) loopv(ammo->effects)
	{
		if(!game::statuses.inrange(ammo->effects[i]->status)) continue;
		if(!game::statuses[ammo->effects[i]->status]->friendly) hit_friendly = 0;
		//TODO friendly fire checks here
		seffects.add(new victimeffect(attacker, ammo->effects[i], weapon->chargeflags, mul));
		hit_total += game::statuses[ammo->effects[i]->status]->strength() * mul;
	}

	vel.add(dir.mul(weapon->kickback + (ammo ? ammo->kickback : 0)));
	if(this == game::player1 && !hit_friendly && hit_total) damagecompass(hit_total, attacker->o);
	getsignal("hit", false, attacker);
}

///Inventory
int rpgchar::modinv(int b, int q, bool spawn)
{
	invstack *item = NULL;

	loopv(inventory)
	{
		if(inventory[i]->base == b)
		{
			item = inventory[i];
			break;
		}
	}

	if(!item)
		item = inventory.add(new invstack(b, 0));

	if(q > 0)
	{
		//factor weight into this?
		//int added = q;
		//int weight = getweight();

		item->quantity += q;
		return q;
	}
	else if (q < 0)
	{
		int removed = 0;
		if(item->quantity >= -q)
		{
			removed = -q;
			item->quantity += q;
		}
		else
		{
			removed = item->quantity;
			item->quantity = 0;

			loopvrev(equipped)
			{
				if(equipped[i]->base == b)
				{
					delete equipped.remove(i);
					if((++removed) == -q) break;
				}
			}
		}

		if(spawn)
		{
			rpgitem *drop = new rpgitem();
			game::curmap->objs.add(drop);

			drop->base = b;
			drop->quantity = removed;
			drop->o = drop->newpos = vec(yaw * RAD, pitch * RAD).mul(radius * 2).add(o);
			entinmap(drop);
		}

		return removed;
	}

	return 0;
}

int rpgchar::pickup(rpgitem *item)
{
	int add = item->quantity;
	if(game::items.inrange(item->base) && game::items[item->base]->weight)
		add = clamp<float>(add, 0, (base.getmaxcarry() * 2 - getweight()) / game::items[item->base]->weight);

	modinv(item->base, add, false);
	item->quantity -= add;

	return add;
}

int rpgchar::getitemcount(int i)
{
	int count = 0;
	loopvj(inventory)
	{
		if(inventory[j]->base == i)
			count += inventory[j]->quantity;
	}
	loopvj(equipped)
	{
		if(equipped[j]->base == i)
			count++;
	}

	return count;
}

float rpgchar::getweight()
{
	float ret = 0;
	loopv(inventory)
	{
		if(game::items.inrange(inventory[i]->base))
			ret += inventory[i]->quantity * game::items[inventory[i]->base]->weight;
	}
	loopv(equipped)
	{
		if(game::items.inrange(equipped[i]->base))
			ret += game::items[equipped[i]->base]->weight;
	}
	return ret;
}

extern int fog;
extern int waterfog;

bool rpgchar::cansee(rpgent *d)
{
	//TODO     add in modifiers for sound, movement, light, invisibility and perception

	//at present this will only see if the entity is within this creature's LoS
	vec dir = vec(d->o).sub(o).normalize();
	vec view = vec(yaw * RAD, pitch * RAD);

	if(dir.dot(view) >= .25) // fov == ~150
	{
		vec pos;
		float dist = raycubepos(o, dir, pos, 0, RAY_ALPHAPOLY|RAY_CLIPMAT);
		if(physstate == PHYS_FLOAT || d->physstate == PHYS_FLOAT)
			dist = min<float>(waterfog, dist);
		dist = min<float>(fog, dist);

		if(o.dist_to_bb(d->feetpos(), d->abovehead()) <= dist)
			return true;
	}

	return false;
}
