#include "rpggame.h"
void rpgobstacle::update()
{
	resetmdl();
	temp.light = vec4(0, 0, 0, 0);
	temp.alpha = 1;

	if(!(flags&F_STATIONARY))
	{
		moveplayer(this, 2, false);
		entities::touchents(this);
	}
}

void rpgobstacle::render(bool mainpass)
{
	rendermodel(&light, temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, 0, MDL_SHADOW|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_LIGHT|MDL_DYNLIGHT, NULL, NULL, 0, 0, temp.alpha);
}

void rpgobstacle::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
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

	getsignal("hit", false, attacker);
}
