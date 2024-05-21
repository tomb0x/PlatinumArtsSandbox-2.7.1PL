#include "sspgame.h"

namespace game
{
	vector<proj *> projs;
	vector<projectile *> projectiles;

	ICOMMAND(addproj, "siiiii", (const char *s, int *u, int *w, int *x, int *y, int *z), projs.add(new proj(s, *u, *w, *x, *y, *z)))
	ICOMMAND(resetprojs, "", (), projs.deletecontents();)

	bool intersect(dynent *d, const vec &from, const vec &to, float &dist)   // if lineseg hits entity bounding box
	{
		vec bottom(d->o), top(d->o);
		bottom.z -= d->eyeheight;
		top.z += d->aboveeye;
		return linecylinderintersect(from, to, bottom, top, d->radius, dist);
	}

	dynent *intersectclosest(const vec &from, const vec &to, sspent *at, float &bestdist, float maxdist)
	{
		dynent *best = NULL;
		bestdist = maxdist;
		loopi(numdynents())
		{
			dynent *o = iterdynents(i);
			if(o==at || o->state != CS_ALIVE) continue;
			float dist;
			if(!intersect(o, from, to, dist)) continue;
			if(dist < bestdist)
			{
				best = o;
				bestdist = dist;
			}
		}
		return best;
	}

	void setprojdecal(int *idx, int *rad, int *col)
	{
		if(!projs.length()) {conoutf("Error: no projectiles defined, define some with /addproj"); return;}

		proj &p = *projs[projs.length()-1];
		p.didx = *idx;
		p.drad = *rad;
		p.dcol = bvec((*col&0xFF0000)/0x010000, (*col&0x00FF00)/0x000100, *col&0xFF);
	}
	COMMAND(setprojdecal, "iii");

	void addproj(int p, sspent *d)
	{
		if(!projs.inrange(p)) return;

		projectiles.add(new projectile(p, vec(d->o.x, d->o.y, d->o.z - d->eyeheight/2), d->yaw, d->pitch, d));
	}


	void clearprojs()
	{
		projectiles.deletecontents();
	}

	void updateprojs()
	{
		if(player1->attacking && pickups.inrange(player1->gunselect))
		{
			pickup_weapon &p =* ((pickup_weapon *) pickups[player1->gunselect]);
			if(player1->shootmillis + p.cooldown < lastmillis)
			{
				player1->shootmillis = lastmillis;
				addproj(p.projectile, player1);
				playsound(p.sound, &player1->o);
			}
		}
		loopvrev(projectiles)
		{ //do simplistic flying first, add gravity effects, timers, and so forth, later
			projectile *p = projectiles[i];
			proj *prj = projs[p->prj];

			if(prj->speed > 10000) //assume bullet-ish for high speeds
			{
				vec oldpos = p->o;
				float dist = raycubepos(oldpos, p->d, p->o, 0, RAY_CLIPMAT|RAY_ALPHAPOLY);
				float vicdist;

				sspent *victim = (sspent *) intersectclosest(oldpos, p->o, p->owner, vicdist, 1);
				if(victim)
				{
					p->o.sub(vec(p->d).mul((1 - vicdist) * dist));

					if(debug)
					{
						defformatstring(ds)("%p", victim);
						particle_textcopy(p->o, ds, PART_TEXT, 2000, 0xFF0000, 4, -5);
					}

					//FIXME splash damage
					victim->takedamage(prj->damage);
				}

				adddecal(prj->didx, p->o, p->d.neg(), prj->drad, prj->dcol);

				if(debug)
					particle_splash(PART_SPARK, 10, 1000, p->o, 0xFF0000, 1, 200);

				delete projectiles[i]; projectiles.remove(i);
			}
			else if(prj->speed>500) //lower speeds means it's rocketish
			{
				bool deleted = false;

				float dist = (prj->speed * curtime) / 1000.0f;
				vec pos;
				float newdist = raycubepos(p->o, p->d, pos, 0, RAY_CLIPMAT|RAY_ALPHAPOLY);

				if(newdist > dist)
				{
					pos = vec(p->o).add(vec(p->d).mul(dist));
				}
				else
				{
					dist = newdist;
					deleted = true;
				}

				sspent *victim = (sspent *) intersectclosest(p->o, pos, p->owner, newdist, 1);

				if(victim)
				{
					deleted = true;
					pos.sub(vec(p->d).mul((1 - newdist) * dist));

					if(debug)
					{
						defformatstring(ds)("%p", victim);
						particle_textcopy(pos, ds, PART_TEXT, 2000, 0xFF0000, 4, -5);
					}

					//TODO splash damage
					victim->takedamage(prj->damage);
				}

				p->o = pos;

				if(deleted)
				{
					if(debug)
						particle_splash(PART_SPARK, 10, 1000, p->o, 0xFF0000, 1, 200);

					adddecal(prj->didx, p->o, p->d.neg(), prj->drad, prj->dcol);
					delete projectiles[i]; projectiles.remove(i);
					continue;
				}

				if(debug)
					particle_splash(PART_SPARK, 10, 1000, p->o, 0xFFBF3A, 1, 200);
			}
			else if(prj->speed) //hand grenad -ish, bouncers, they are affected by jumppads
			{
				conoutf("hand grenade physics not implemented yet, deleting proj");
				delete projectiles[i]; projectiles.remove(i);;
			}
			else //mines or traps
			{
				conoutf("mines and traps not done yet, deleting proj"); //need a timer, 15 seconds?
				delete projectiles[i]; projectiles.remove(i);;
			}
		}
	}

	void renderprojs()
	{
		loopv(projectiles)
		{
			projectile &p = *projectiles[i];
			float yaw, pitch;
			vectoyawpitch(p.d, yaw, pitch);

			rendermodel(NULL, projs[p.prj]->mdl, 0, p.o, yaw, pitch, 0, MDL_SHADOW);
		}
	}

	void adddynlights()
	{

	}

	void writeprojectiles(stream *f)
	{
		f->printf("resetprojs\n\n");
		loopv(projs)
		{
			proj &p = *projs[i];
			f->printf("addproj %s %i %i %i %i %i\n", p.mdl, p.damage, p.radius, p.force, p.travelsound, p.speed);
			f->printf("setprojdecal %i %i 0x%.2X%.2X%.2X\n", p.didx, p.drad, p.dcol.x, p.dcol.y, p.dcol.z);
		}
	}
}

