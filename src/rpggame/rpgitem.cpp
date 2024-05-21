#include "rpggame.h"

///base functions
void rpgitem::resetmdl()
{
	temp.mdl = game::items.inrange(base) ? game::items[base]->mdl : "rc";
}

void rpgitem::update()
{
	resetmdl();
	temp.alpha = 1;
	temp.light = vec4(0, 0, 0, 0);

	moveplayer(this, 2, false);
	entities::touchents(this);
}

void rpgitem::render(bool mainpass)
{
	rendermodel(&light, temp.mdl ? temp.mdl : (game::items.inrange(base) ? game::items[base]->mdl : "rc") , ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, 0, MDL_SHADOW|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_LIGHT|MDL_DYNLIGHT, NULL, NULL, 0, 0, temp.alpha);
}

const char *rpgitem::getname() const
{
	if(game::items.inrange(base))
	{
		item_base *i = game::items[base];
		if(i->name)
			return i->name;
	}

	return "Tough Cookie";
}

int rpgitem::getscript()
{
	if(game::items.inrange(base))
		return game::items[base]->script;
	conoutf("WARNING: item %p uses non existant item base %i", this, base);
	return -1;
}

int rpgitem::getitemcount(int i)
{
	if(i == base)
		return quantity;
	return 0;
}

float rpgitem::getweight()
{
	if(game::items.inrange(base))
		return quantity * game::items[base]->weight;
	return 0;
}

void rpgitem::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
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

	vel.add(dir.mul(weapon->kickback + (ammo ? ammo->kickback : 0)));

	rpgent::getsignal("hit", false, attacker);
}