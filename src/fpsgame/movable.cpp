// movable.cpp: implements physics for inanimate models
#include "game.h"

extern int physsteps;

namespace game
{
    enum
    {
        BOXWEIGHT = 25,
        BARRELHEALTH = 50,
        BARRELWEIGHT = 25,
        PLATFORMWEIGHT = 1000,
        PLATFORMSPEED = 8,
        EXPLODEDELAY = 200
    };

    vector<int> teleports;

    struct movable : dynent
    {
        int etype, mapmodel, health, weight, exploding, tag, dir;
        physent *stacked;
        vec stackpos;

        movable(const entity &e) :
            etype(e.type),
            mapmodel(e.attr[1]),
            health(e.type==BARREL ? (e.attr[3] ? e.attr[3] : BARRELHEALTH) : 0),
            weight(e.type==PLATFORM || e.type==ELEVATOR ? PLATFORMWEIGHT : (e.attr[2] ? e.attr[2] : (e.type==BARREL ? BARRELWEIGHT : BOXWEIGHT))),
            exploding(0),
            tag(e.type==PLATFORM || e.type==ELEVATOR ? e.attr[2] : 0),
            dir(e.type==PLATFORM || e.type==ELEVATOR ? (e.attr[3] < 0 ? -1 : 1) : 0),
            stacked(NULL),
            stackpos(0, 0, 0)
        {
            state = CS_ALIVE;
            type = ENT_INANIMATE;
            yaw = e.attr[0];
            if(e.type==PLATFORM || e.type==ELEVATOR)
            {
                maxspeed = e.attr[3] ? fabs(float(e.attr[3])) : PLATFORMSPEED;
                if(tag) vel = vec(0, 0, 0);
                else if(e.type==PLATFORM) { vecfromyawpitch(yaw, 0, 1, 0, vel); vel.mul(dir*maxspeed); }
                else vel = vec(0, 0, dir*maxspeed);
            }

            const char *mdlname = mapmodelname(e.attr[1]);
            if(mdlname) setbbfrommodel(this, mdlname);
        }

        void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
        {
            if(etype!=BOX && etype!=BARREL) return;
            vec push(dir);
            push.mul(80*damage/weight);
            vel.add(push);
        }

        void explode(dynent *at)
        {
            state = CS_DEAD;
            exploding = 0;
            //game::explode(true, (fpsent *)at, o, this, guns[GUN_BARREL].damage, GUN_BARREL);
        }

        void damaged(int damage, fpsent *at, int gun = -1)
        {
            if(etype!=BARREL || state!=CS_ALIVE || exploding) return;
            health -= damage;
            if(health>0) return;
            if(gun==GUN_BARREL) exploding = lastmillis + EXPLODEDELAY;
            else explode(at);
        }

        void suicide()
        {
            state = CS_DEAD;
            if(etype==BARREL) explode(player1);
        }
    };

    vector<movable *> movables;

    void clearmovables()
    {
        if(movables.length())
        {
            cleardynentcache();
            movables.deletecontents();
        }
        if(!m_dmsp && !m_classicsp) return;
	    teleports.setsize(0);
        loopv(entities::ents)
        {
            const entity &e = *entities::ents[i];
            if(e.type!=BOX && e.type!=BARREL && e.type!=PLATFORM && e.type!=ELEVATOR)
	    	{
				if(e.type==TELEPORT)
					teleports.add(i);
				continue;
	    	}
            movable *m = new movable(e);
            movables.add(m);
            m->o = e.o;
            entinmap(m);
            updatedynentcache(m);
        }
    }

    void triggerplatform(int tag, int newdir)
    {
        newdir = max(-1, min(1, newdir));
        loopv(movables)
        {
            movable *m = movables[i];
            if(m->state!=CS_ALIVE || (m->etype!=PLATFORM && m->etype!=ELEVATOR) || m->tag!=tag) continue;
            if(!newdir)
            {
                if(m->tag) m->vel = vec(0, 0, 0);
                else m->vel.neg();
            }
            else
            {
                if(m->etype==PLATFORM) { vecfromyawpitch(m->yaw, 0, 1, 0, m->vel); m->vel.mul(newdir*m->dir*m->maxspeed); }
                else m->vel = vec(0, 0, newdir*m->dir*m->maxspeed);
            }
        }
    }
    ICOMMAND(platform, "ii", (int *tag, int *newdir), triggerplatform(*tag, *newdir));

    void stackmovable(movable *d, physent *o)
    {
        d->stacked = o;
        d->stackpos = o->o;
    }

    void updatemovables(int curtime)
    {
        if(!curtime) return;
        loopv(movables)
        {
            movable *m = movables[i];
            if(m->state!=CS_ALIVE) continue;
            if(m->etype==PLATFORM || m->etype==ELEVATOR)
            {
                if(m->vel.iszero()) continue;
                for(int remaining = curtime; remaining>0;)
                {
                    int step = min(remaining, 20);
                    remaining -= step;
                    if(!moveplatform(m, vec(m->vel).mul(step/1000.0f)))
                    {
                        if(m->tag) { m->vel = vec(0, 0, 0); break; }
                        else m->vel.neg();
                    }
                }
            }
            else if(m->exploding && lastmillis >= m->exploding)
            {
                m->explode(m);
                adddecal(DECAL_BURN, m->o, vec(0, 0, 1), RL_DAMRAD/2);
            }
            else if(m->maymove() || (m->stacked && (m->stacked->state!=CS_ALIVE || m->stackpos != m->stacked->o)))
            {
                if(physsteps > 0) m->stacked = NULL;
                moveplayer(m, 1, true);
            }
	    loopvj(teleports)
	    {
	    	extentity &e = *entities::getents()[teleports[j]];
		if(vec(e.o.x, e.o.y, 0).dist(vec(m->o.x, m->o.y, 0)) <= m->radius &&
			vec(0, 0, e.o.z).dist(vec(0,0, m->o.z - 0.5 * m->eyeheight)) <= 0.5 * m->eyeheight)
		{
			loopvk(entities::getents()) {extentity &f = *entities::getents()[k]; if(f.type == TELEDEST && f.attr[1] == e.attr[0]) {m->o = m->newpos = f.o; m->yaw = f.attr[0]; break;}}
		}
	    }
        }
    }

    void rendermovables()
    {
        loopv(movables)
        {
            movable &m = *movables[i];
            if(m.state!=CS_ALIVE) continue;
            vec o = m.feetpos();
            const char *mdlname = mapmodelname(m.mapmodel);
            if(!mdlname) continue;
			rendermodel(NULL, mdlname, ANIM_MAPMODEL|ANIM_LOOP, o, m.yaw, 0, 0, MDL_LIGHT | MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED, &m);
        }
    }

    void suicidemovable(movable *m)
    {
        m->suicide();
    }

    void hitmovable(int damage, movable *m, fpsent *at, const vec &vel, int gun)
    {
        m->hitpush(damage, vel, at, gun);
        m->damaged(damage, at, gun);
    }
}

