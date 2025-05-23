#include "rpggame.h"

void rpgplatform::update()
{
	resetmdl();
	temp.light = vec4(0, 0, 0, 0);
	temp.alpha = 1;

	if(!entities::ents.inrange(target) || entities::ents[target]->type != PLATFORMROUTE)
	{
		target = -1;
		float closest;
		loopv(entities::ents)
		{
			extentity &e = *entities::ents[i];
			if(e.type == PLATFORMROUTE && routes.access(e.attr[0]) && (target < 0 || e.o.dist(o) < closest))
			{
				target = i;
				closest = e.o.dist(o);
			}
		}
	}
	if(flags & F_ACTIVE)
	{
		float delta = speed * curtime / 1000.f;
		while(target >= 0 && delta > 0)
		{
			float dist = min(delta, entities::ents[target]->o.dist(o));
			if(dist > 0) //avoid divide by 0 for normalisation
			{
				delta -= dist;
				vec move = vec(entities::ents[target]->o).sub(o).normalize().mul(dist);

				o.add(move);
				loopv(stack) {stack[i]->o.add(move); stack[i]->resetinterp();}
			}

			if(delta <= 0) break;

			vector<int> &detours = *routes.access(entities::ents[target]->attr[0]);
			target = -1;

			//we pick a random one; we test up to n times if we fail to get a destination
			loopi(detours.length())
			{
				int tag = detours[rnd(detours.length())];
				loopvj(entities::ents)
				{
					extentity &e = *entities::ents[j];
					if(e.type == PLATFORMROUTE && e.attr[0] == tag)
					{
						target = j;
						break;
					}
				}
				if(target >= 0) break;
			}
		}
	}

	stack.setsize(0);
}

void rpgplatform::render(bool mainpass)
{
	rendermodel(&light, temp.mdl, ANIM_MAPMODEL|ANIM_LOOP, vec(o).sub(vec(0, 0, eyeheight)), yaw, pitch, 0, MDL_SHADOW|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_LIGHT|MDL_DYNLIGHT, NULL, NULL, 0, 0, temp.alpha);
}

void rpgplatform::hit(rpgent *attacker, use_weapon *weapon, use_weapon *ammo, float mul, int flags, vec dir)
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
