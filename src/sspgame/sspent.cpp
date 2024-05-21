#include "sspgame.h"

#define PAINIMMUNITY 2000

bool sspchar::stool()
{
	bool val = false;
	if(state == CS_DEAD)
		return false;

	loopv(game::sspobjs)
	{
		sspent *ob = game::sspobjs[i];
		if(state == CS_DEAD || ob == this || (ob->etype != ENT_CHAR && ob->etype != ENT_ENEMY))
			continue;

		float xdelta = xradius + ob->xradius, ydelta = yradius + ob->yradius;

		vec bottom = ob->feetpos();

		float x = abs(bottom.x - o.x);
		float y = abs(bottom.y - o.y);
		float z = abs(bottom.z - o.z);
		//if you must know, abs() doens't want to work properly inside the if()

		if(	x < xdelta &&
			y < ydelta &&
			z <= aboveeye &&
			ob->vel.z + ob->falling.z <= 0
		)
		{
			val = true;
			ob->vel.z = 150;
			ob->falling.z = 0;
		}

	}
	return val;
}

bool sspchar::update()
{
	if(state == CS_EDITING)
	{
		moveplayer(this, 10, true);
		return true;
	}


	if(state == CS_DEAD && ragdoll)
		moveragdoll(this);
	else
	{
		if(state == CS_ALIVE)
		{
			entities::checkitems(this);

			bool left = game::closestyaw(camera1->yaw - yaw) == 180 || game::closestyaw(camera1->yaw - yaw) == 90;

			strafe = 0;
			if(game::mousetarget)
			{
				if(!(k_left ^ k_right)) //both true or both false
					move = 0;
				else if (k_left)
				{
					move = left ? 1 : -1;
				}
				else if(k_right)
				{
					move = left ? -1 : 1;
				}
			}
			else
			{
				if(k_left ^ k_right)
				{
					if((!left && k_left) || (left && k_right))
						yaw += 180;

					move = 1;
				}
				else
					move = 0;
			}
		}
		else
		{
			move = strafe = 0;
		}
		moveplayer(this, 10, state == CS_ALIVE);
	}

	return true;
}

void sspchar::takedamage(int amount, bool immunity)
{
	if(immunity && lastpain + PAINIMMUNITY > lastmillis) return;

	if(armour)
		armour = ARM_NONE;
	else
		health -= amount;

	if(health <= 0 && state != CS_DEAD)
	{
		state = CS_DEAD;
		game::eventimages.add(new eventimage("data/ssp/hud/died", 2000));
	}

	lastpain = lastmillis;
}

void sspchar::render()
{
	if(state == CS_EDITING && !isthirdperson())
		return;

	int anim = ANIM_ATTACK1+gunselect,
	delay = 300,
	hold = ANIM_HOLD1|ANIM_LOOP;

	if(game::intermission && state!=CS_DEAD)
	{
		if (game::secsremain > 0)
			hold = anim = ANIM_WIN|ANIM_LOOP;
		else
			hold = anim = ANIM_LOSE|ANIM_LOOP;
	}
	modelattach a[4];
	int ai = 0;
	if(state==CS_ALIVE)
	{
		if(game::pickups.inrange(gunselect))
		{
			a[ai].m = NULL;
			a[ai].name = ((pickup_weapon *) game::pickups[gunselect])->attachmdl;
			a[ai].tag = "tag_weapon";
			a[ai].anim = ANIM_VWEP_IDLE|ANIM_LOOP;
			a[ai].basetime = 0;
			ai++;
		}
		if(game::pickups.inrange(armourvec))
		{
			a[ai].m = NULL;
			a[ai].name = ((pickup_armour *) game::pickups[armourvec])->attachmdl;
			a[ai].tag = "tag_shield";
			a[ai].anim = ANIM_VWEP_IDLE|ANIM_LOOP;
			a[ai].basetime = 0;
			ai++;
		}
		if(powerup>=0)
		{
			//TODO
			//ai++;
		}
	}
	a[ai].name = NULL;

	if(lastpain + PAINIMMUNITY > lastmillis && (lastmillis % 100) > 50) return; //flicker if hurt
		renderclient(this, "rc", a[0].tag ? a : NULL, hold, anim, delay, lastaction, game::intermission ? 0 : lastpain, 1.0f, true);
}

bool sspmonster::update()
{
	if(state == CS_DEAD && ragdoll)
		moveragdoll(this);
	else
	{
		move &= state == CS_ALIVE;
		strafe &= state == CS_ALIVE;
		moveplayer(this, 4, state == CS_ALIVE);
	}
	if(state == CS_ALIVE)
	{
		game::checkmonsteritems(this);
	}
	return !(state == CS_DEAD && lastpain + PAINIMMUNITY < lastmillis);
}

void sspmonster::takedamage(int amount, bool immunity)
{
	if(immunity && lastpain + PAINIMMUNITY > lastmillis) return;

	health -= amount;
	if(health <= 0)
		state = CS_DEAD;

	lastpain = lastmillis;
}

void sspmonster::render()
{
	if(game::monstertypes.inrange(mtype))
	{
		if(lastpain + PAINIMMUNITY > lastmillis && (lastmillis % 100) > 50) return; //flicker if hurt
			renderclient(this, game::monstertypes[mtype]->mdl, NULL, 0, ANIM_HOLD1|ANIM_LOOP, 300, lastaction, lastpain);

	}
}

bool sspitem::update()
{
	//ONLY the player can pickup pickups
	vec pos = game::player1->o; pos.z -= game::player1->eyeheight / 2;
	float dist = o.dist(pos);

	if(dist <= (game::player1->armour == ARM_ATTRACT ? 48 : 12))
	{
		vec delta = pos; delta.sub(o);

		delta.mul(curtime / ( 100.0f * max<float>(dist, 1)));
		o.add(delta);

		dist -= delta.magnitude();

		if(game::player1->armour==ARM_ATTRACT)
		{
			regularshape(PART_LIGHTNING, 8, 0x007FFF, 31, 2, 200, vec(o.x+rnd(8)-4, o.y+rnd(8)-4, o.z+rnd(8)), .25, 0, 0 ); //make it look pretty
		}

		if(dist < 8 && entities::pickuppowerup(game::player1, this))
		{
			return false;
		}
	}
	return deathtime ? deathtime > lastmillis : true;
}

void sspitem::render()
{
	if(editmode || !game::pickups.inrange(index))
		return;

	if(deathtime && deathtime - lastmillis < 2000)
	{
		if((lastmillis % 200) > 100)
			return;
	}

	rendermodel(&light, game::pickups[index]->mdl,
		ANIM_MAPMODEL|ANIM_LOOP, vec(o).add(vec(0, 0, (float)(1 + sin(lastmillis/300.0)))),
		lastmillis/10.0f, 0, 0, MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_LIGHT
	);
}

bool sspbox::update()
{
	if(explode && explode > lastmillis)
	{
		//explosion here
		return false;
	}

	vec pos = feetpos(), &pl = game::player1->o;

	float xdelta = xradius + game::player1->xradius;
	float ydelta = yradius + game::player1->yradius;

	float x = abs(pos.x - pl.x);
	float y = abs(pos.y - pl.y);
	float z = abs(pos.z - pl.z);

	if(game::player1->vel.z + game::player1->falling.z > 0 &&
		x < xdelta &&
		y < ydelta &&
		z <= game::player1->aboveeye + (game::player1->vel.z + game::player1->falling.z) * curtime / 800.0f
	)
	{
		game::player1->vel.z = 0;
		lasttouch = lastmillis;

		defformatstring(ds)("box_%i", script);
		if(identexists(ds)) execute(ds);

		if(!inventory.length())
			return true;


		if(flags & BOX_PINJATA)
		{
			while(inventory.length())
			{
				sspitem *item = new sspitem();
				item->init(vec(o).add(vec(0, 0, 4)), inventory[0], false, lastmillis + 10000);
				entities::items.add(item);

				inventory.remove(0);
			}
		}
		else
		{
			sspitem *item = new sspitem();
			item->init(vec(o).add(vec(0, 0, 4)), inventory[0], false, lastmillis + 10000);
			entities::items.add(item);

			inventory.remove(0);
		}

		if(flags & BOX_DESTROY && !inventory.length())
		{
			if(flags & BOX_EXPLODE)
			{
				explode = lastmillis + 3000; // explode in 3 seconds
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

void sspbox::takedamage(int amount, bool immunity)
{
	if(!explode)
		lasttouch = lastmillis;

	defformatstring(ds)("box_%i", script);
	if(identexists(ds)) execute(ds);

	if(!inventory.length())
		return;


	if(flags & BOX_PINJATA)
	{
		while(inventory.length())
		{
			sspitem *item = new sspitem();
			item->init(vec(o).add(vec(0, 0, 4)), inventory[0], 0, false, lastmillis + 50000);
			entities::items.add(item);

			inventory.remove(0);
		}
	}
	else
	{
		sspitem *item = new sspitem();
		item->init(vec(o).add(vec(0, 0, 4)), inventory[0], false, lastmillis + 50000);
		entities::items.add(item);

		inventory.remove(0);
	}

	if(flags & BOX_DESTROY && !inventory.length())
	{
		if(flags & BOX_EXPLODE)
		{
			explode = lastmillis + 3000; // explode in 3 seconds
		}
		else
		{
			return;
		}
	}
}

void sspbox::render()
{
	if(editmode)
		return;

	vec pos = o;
	float yaw = 0;
	if(lasttouch + 500 > lastmillis)
	{
		yaw += (lasttouch + 500 - lastmillis) * 360.0f / 500.0f;
		pos.z += radius * sin((lastmillis - lasttouch) / 125.0f * PI) * (lasttouch + 500 - lastmillis) / 1000.0f;
	}
	rendermodel(&light, mapmodelname(mdl), ANIM_MAPMODEL|ANIM_LOOP, pos, yaw, 0, 0,
		MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_LIGHT
	);
}