#include "rpggame.h"

void rpgcontainer::update()
{
	resetmdl();
	temp.light = vec4(0, 0, 0, 0);
	temp.alpha = 1;
	magelock = 0; // IT'S MAGIC!
}

void rpgcontainer::render(bool mainpass)
{
	rendermodel(&light, temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, 0, MDL_SHADOW|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_LIGHT|MDL_DYNLIGHT, NULL, NULL, 0, 0, temp.alpha);
}

int rpgcontainer::modinv(int b, int q, bool spawn)
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
		//factor capacity into this?
		//int added = q;
		//int weight = getweight();

		item->quantity += q;
		return q;
	}
	else if (q < 0)
	{
		int removed = min(item->quantity, -q);
		item->quantity -= removed;

		if(spawn)
		{
			rpgitem *drop = new rpgitem();
			game::curmap->objs.add(drop);

			drop->base = b;
			drop->quantity = removed;
			drop->o = drop->newpos = vec(o).add(vec(rnd(360) * RAD, 0 * RAD).mul(radius * 2));
		}

		return removed;
	}

	return 0;
}

int rpgcontainer::getitemcount(int i)
{
	loopvj(inventory)
	{
		if(inventory[j]->base == i)
			return inventory[j]->quantity;
	}
	return 0;
}

float rpgcontainer::getweight()
{
	float ret = 0;
	loopv(inventory)
	{
		if(game::items.inrange(inventory[i]->base))
			ret += inventory[i]->quantity * game::items[inventory[i]->base]->weight;
	}

	return ret;
}

void rpgcontainer::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
{
	loopv(weapon->effects)
	{
		if(!game::statuses.inrange(weapon->effects[i]->status)) continue;
		seffects.add(new victimeffect(attacker, weapon->effects[i], weapon->chargeflags, mul));
	}

	if(ammo) loopv(ammo->effects)
	{
		if(!game::statuses.inrange(ammo->effects[i]->status)) continue;
		seffects.add(new victimeffect(attacker, ammo->effects[i], weapon->chargeflags, mul));
	}

	getsignal("hit", false, attacker);
}
