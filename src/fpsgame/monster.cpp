// monster.h: implements AI for single player monsters, currently client only
#include "game.h"

extern int physsteps;

namespace game
{
    static vector<int> teleports;

    static const int TOTMFREQ = 14;
    const int NUMMONSTERTYPES = 7;

    const monstertype monstertypes[NUMMONSTERTYPES] =
    {
        { GUN_FIST,  4, 100, 3, 0,    100, 800,   50,  0,     5, S_PAINO, S_DIE1,   "Motyl",       "butterfly",                    "butterfly"},
        { GUN_BITE,  20, 800, 5, 0,   400, 1600, 100, 60,  4000, S_PAINO, S_DIE1,   "Smok",        "rpg/characters/dragon",        "Dragon"   },
        { GUN_FIST,  8, 800, 5, 0,    400, 1600, 100, 16,  4000, S_PAINO, S_DIE1,   "Golem",       "rpg/characters/golem",         "golem"    },
        { GUN_FIST,  12, 200, 5, 0,   400, 1600, 100, 16,   800, S_PAINO, S_DIE1,   "Grizzly",     "rpg/characters/grizzly",       "grizzly"  },
        { GUN_BITE,  25, 50, 5, 0,    400, 1600, 100,  2,    30, S_PAINO, S_DIE1,   "Szczur",      "rpg/characters/rat",           "rat"      },
        { GUN_FIST,  7, 150, 5, 0,    400, 1600, 100, 16,   200, S_PAINO, S_DIE1,   "Snagon",      "rpg/characters/snagon",        "snagon"   },
        { GUN_BITE,  28, 80, 5, 0,    400, 1600, 100,  6,   150, S_PAINO, S_DIE1,   "Wilk",        "rpg/characters/wolf",          "wolf"     },
    };

    VAR(skill, 1, 3, 10);
    VAR(killsendsp, 0, 1, 1);

    bool monsterhurt;
    vec monsterhurtpos;

    struct monster : fpsent
    {
        int monsterstate;                   // one of M_*, M_NONE means human

        int mtype, tag;                     // see monstertypes table
        fpsent *enemy;                      // monster wants to kill this entity
        float targetyaw;                    // monster wants to look in this direction
        int trigger;                        // millis at which transition to another monsterstate takes place
        vec attacktarget;                   // delayed attacks
        int anger;                          // how many times already hit by fellow monster
        physent *stacked;
        vec stackpos;

        monster(int _type, int _yaw, int _tag, int _state, int _trigger, int _move) :
            monsterstate(_state), tag(_tag),
            stacked(NULL),
            stackpos(0, 0, 0)
        {
            type = ENT_AI;
            respawn();
            if(_type>=NUMMONSTERTYPES || _type < 0)
            {
                conoutf(CON_WARN, "warning: unknown monster in spawn: %d", _type);
                _type = 0;
            }
            mtype = _type;
            const monstertype &t = monstertypes[mtype];
            eyeheight = 8.0f;
            aboveeye = 7.0f;
            radius *= t.bscale/10.0f;
            xradius = yradius = radius;
            eyeheight *= t.bscale/10.0f;
            aboveeye *= t.bscale/10.0f;
            weight = t.weight;
            if(_state!=M_SLEEP) spawnplayer(this);
            trigger = lastmillis+_trigger;
            targetyaw = yaw = (float)_yaw;
            move = _move;
            enemy = player1;
            gunselect = t.gun;
            maxspeed = (float)t.speed*4;
            health = t.health;
            armour = 0;
            loopi(NUMGUNS) ammo[i] = 10000;
            pitch = 0;
            roll = 0;
            state = CS_ALIVE;
            anger = 0;
            copystring(name, t.name);
        }

        void normalize_yaw(float angle)
        {
            while(yaw<angle-180.0f) yaw += 360.0f;
            while(yaw>angle+180.0f) yaw -= 360.0f;
        }

        // monster AI is sequenced using transitions: they are in a particular state where
        // they execute a particular behaviour until the trigger time is hit, and then they
        // reevaluate their situation based on the current state, the environment etc., and
        // transition to the next state. Transition timeframes are parametrized by difficulty
        // level (skill), faster transitions means quicker decision making means tougher AI.

        void transition(int _state, int _moving, int n, int r) // n = at skill 0, n/2 = at skill 10, r = added random factor
        {
            monsterstate = _state;
            move = _moving;
            n = n*130/100;
            trigger = lastmillis+n-skill*(n/16)+rnd(r+1);
        }

        void monsteraction(int curtime)           // main AI thinking routine, called every frame for every monster
        {
            if(enemy->state==CS_DEAD) { enemy = player1; anger = 0; }
            normalize_yaw(targetyaw);
            if(targetyaw>yaw)             // slowly turn monster towards his target
            {
                yaw += curtime*0.5f;
                if(targetyaw<yaw) yaw = targetyaw;
            }
            else
            {
                yaw -= curtime*0.5f;
                if(targetyaw>yaw) yaw = targetyaw;
            }
            float dist = enemy->o.dist(o);
            if(monsterstate!=M_SLEEP) pitch = asin((enemy->o.z - o.z) / dist) / RAD;

            if(blocked)                                                              // special case: if we run into scenery
            {
                blocked = false;
                if(!rnd(20000/monstertypes[mtype].speed))                            // try to jump over obstackle (rare)
                {
                    jumping = true;
                }
                else if(trigger<lastmillis && (monsterstate!=M_HOME || !rnd(5)))  // search for a way around (common)
                {
                    targetyaw += 90+rnd(180);                                        // patented "random walk" AI pathfinding (tm) ;)
                    transition(M_SEARCH, 1, 100, 1000);
                }
            }

            float enemyyaw = -atan2(enemy->o.x - o.x, enemy->o.y - o.y)/RAD;

            switch(monsterstate)
            {
                case M_PAIN:
                case M_ATTACKING:
                case M_SEARCH:
                    if(trigger<lastmillis) transition(M_HOME, 1, 100, 200);
                    break;

                case M_SLEEP:                       // state classic sp monster start in, wait for visual contact
                {
                    if(editmode) break;
                    normalize_yaw(enemyyaw);
                    float angle = (float)fabs(enemyyaw-yaw);
                    if(dist<32                   // the better the angle to the player, the further the monster can see/hear
                    ||(dist<64 && angle<135)
                    ||(dist<128 && angle<90)
                    ||(dist<256 && angle<45)
                    || angle<10
                    || (monsterhurt && o.dist(monsterhurtpos)<128))
                    {
                        vec target;
                        if(raycubelos(o, enemy->o, target))
                        {
                            transition(M_HOME, 1, 500, 200);
                            playsound(S_GRUNT1+rnd(2), &o);
                        }
                    }
                    break;
                }

                case M_AIMING:                      // this state is the delay between wanting to shoot and actually firing
                    if(trigger<lastmillis)
                    {
                        lastaction = 0;
                        attacking = true;
                        //shoot(this, attacktarget);
                        transition(M_ATTACKING, 0, 600, 0);
                    }
                    break;

                case M_HOME:                        // monster has visual contact, heads straight for player and may want to shoot at any time
                    targetyaw = enemyyaw;
                    if(trigger<lastmillis)
                    {
                        vec target;
                        if(!raycubelos(o, enemy->o, target))    // no visual contact anymore, let monster get as close as possible then search for player
                        {
                            transition(M_HOME, 1, 800, 500);
                        }
                        else
                        {
                            bool melee = false, longrange = false;
                            switch(monstertypes[mtype].gun)
                            {
                                case GUN_BITE:
                                case GUN_FIST: melee = true; break;
                                case GUN_RIFLE: longrange = true; break;
                            }
                            // the closer the monster is the more likely he wants to shoot,
                            if((!melee || dist<20) && !rnd(longrange ? (int)dist/12+1 : min((int)dist/12+1,6)) && enemy->state==CS_ALIVE)      // get ready to fire
                            {
                                attacktarget = target;
                                transition(M_AIMING, 0, monstertypes[mtype].lag, 10);
                            }
                            else                                                        // track player some more
                            {
                                transition(M_HOME, 1, monstertypes[mtype].rate, 0);
                            }
                        }
                    }
                    break;

            }

            if(move || maymove() || (stacked && (stacked->state!=CS_ALIVE || stackpos != stacked->o)))
            {
                vec pos = feetpos();
                loopv(teleports) // equivalent of player entity touch, but only teleports are used
                {
                    entity &e = *entities::ents[teleports[i]];
                    float dist = e.o.dist(pos);
                    if(dist<16) entities::teleport(teleports[i], this);
                }

                if(physsteps > 0) stacked = NULL;
                moveplayer(this, 1, true);        // use physics to move monster
            }
        }

        void monsterpain(int damage, fpsent *d)
        {
            if(d->type==ENT_AI)     // a monster hit us
            {
                if(this!=d)            // guard for RL guys shooting themselves :)
                {
                    anger++;     // don't attack straight away, first get angry
                    int _anger = d->type==ENT_AI && mtype==((monster *)d)->mtype ? anger/2 : anger;
                    if(_anger>=monstertypes[mtype].loyalty) enemy = d;     // monster infight if very angry
                }
            }
            else if(d->type==ENT_PLAYER) // player hit us
            {
                anger = 0;
                enemy = d;
                monsterhurt = true;
                monsterhurtpos = o;
            }
            //damageeffect(damage, this);
            if((health -= damage)<=0)
            {
                state = CS_DEAD;
                lastpain = lastmillis;
                playsound(monstertypes[mtype].diesound, &o);
                monsterkilled();
                //superdamage = -health;
                //superdamageeffect(vel, this);

                defformatstring(id)("monster_dead_%d", tag);
                if(identexists(id)) execute(id);
            }
            else
            {
                transition(M_PAIN, 0, monstertypes[mtype].pain, 200);      // in this state monster won't attack
                playsound(monstertypes[mtype].painsound, &o);
            }
        }
    };

    void stackmonster(monster *d, physent *o)
    {
        d->stacked = o;
        d->stackpos = o->o;
    }

    int nummonsters(int tag, int state)
    {
        int n = 0;
        loopv(monsters) if(monsters[i]->tag==tag && (monsters[i]->state==CS_ALIVE ? state!=1 : state>=1)) n++;
        return n;
    }
    ICOMMAND(nummonsters, "ii", (int *tag, int *state), intret(nummonsters(*tag, *state)));

    void preloadmonsters()
    {
        loopi(NUMMONSTERTYPES) preloadmodel(monstertypes[i].mdlname);
    }

    vector<monster *> monsters;

    int nextmonster, spawnremain, numkilled, monstertotal, mtimestart, remain;

    void spawnmonster()     // spawn a random monster according to freq distribution in DMSP
    {
        int n = rnd(TOTMFREQ), type;
        for(int i = 0; ; i++) if((n -= monstertypes[i].freq)<0) { type = i; break; }
        monsters.add(new monster(type, rnd(360), 0, M_SEARCH, 1000, 1));
    }

    void clearmonsters()     // called after map start or when toggling edit mode to reset/spawn all monsters to initial state
    {
        removetrackedparticles();
        removetrackeddynlights();
        loopv(monsters) delete monsters[i];
        cleardynentcache();
        monsters.shrink(0);
        numkilled = 0;
        monstertotal = 0;
        spawnremain = 0;
        remain = 0;
        monsterhurt = false;
        if(m_dmsp)
        {
            nextmonster = mtimestart = lastmillis+10000;
            monstertotal = spawnremain = skill*10;
        }
        else if(m_classicsp)
        {
            mtimestart = lastmillis;
            loopv(entities::ents)
            {
                extentity &e = *entities::ents[i];
                if(e.type!=MONSTER) continue;
                monster *m = new monster(e.attr[1], e.attr[0], e.attr[2], M_SLEEP, 100, 0);
                monsters.add(m);
                m->o = e.o;
                entinmap(m);
                updatedynentcache(m);
                monstertotal++;
            }
        }
        teleports.setsize(0);
        if(m_dmsp || m_classicsp)
        {
            loopv(entities::ents) if(entities::ents[i]->type==TELEPORT) teleports.add(i);
        }
    }

    void endsp(bool allkilled)
    {
        conoutf(CON_GAMEINFO, allkilled ? "\f2you have cleared the map!" : "\f2znalazles wyjscie!");
        monstertotal = 0;
        game::addmsg(N_FORCEINTERMISSION, "r");
    }
    ICOMMAND(endsp, "", (), endsp(false));


    void monsterkilled()
    {
        numkilled++;
        player1->frags = numkilled;
        remain = monstertotal-numkilled;
        if(remain>0 && remain<=5) conoutf(CON_GAMEINFO, "\f2only %d monster(s) remaining", remain);
    }

    void updatemonsters(int curtime)
    {
        if(m_dmsp && spawnremain && lastmillis>nextmonster)
        {
            if(spawnremain--==monstertotal) { conoutf(CON_GAMEINFO, "\f2The invasion has begun!"); playsound(S_V_FIGHT); }
            nextmonster = lastmillis+1000;
            spawnmonster();
        }

        if(killsendsp && monstertotal && !spawnremain && numkilled==monstertotal) endsp(true);

        bool monsterwashurt = monsterhurt;

        loopv(monsters)
        {
            if(monsters[i]->state==CS_ALIVE) monsters[i]->monsteraction(curtime);
            else if(monsters[i]->state==CS_DEAD)
            {
                if(lastmillis-monsters[i]->lastpain<2000)
                {
                    //monsters[i]->move = 0;
                    monsters[i]->move = monsters[i]->strafe = 0;
                    moveplayer(monsters[i], 1, true);
                }
            }
        }

        if(monsterwashurt) monsterhurt = false;
    }

    void rendermonsters()
    {
        loopv(monsters)
        {
            monster &m = *monsters[i];
            if(m.state!=CS_DEAD || lastmillis-m.lastpain<10000)
            {
                modelattach vwep[2];
                vwep[0] = modelattach("tag_weapon", monstertypes[m.mtype].vwepname, ANIM_VWEP_IDLE|ANIM_LOOP, 0);
                float fade = 1;
                if(m.state==CS_DEAD) fade -= clamp(float(lastmillis - (m.lastpain + 9000))/1000, 0.0f, 1.0f);
                renderclient(&m, monstertypes[m.mtype].mdlname, vwep, 0, m.monsterstate==M_ATTACKING ? -ANIM_ATTACK1 : 0, 300, m.lastaction, m.lastpain, fade);
            }
        }
    }

    void suicidemonster(monster *m)
    {
        m->monsterpain(400, player1);
    }

    void hitmonster(int damage, monster *m, fpsent *at, const vec &vel, int gun)
    {
        m->monsterpain(damage, at);
    }

    void spsummary(int accuracy)
    {
        conoutf(CON_GAMEINFO, "\f2--- single player time score: ---");
        int pen, score = 0;
        pen = ((lastmillis-maptime)*100)/(1000*getvar("gamespeed")); score += pen; if(pen) conoutf(CON_GAMEINFO, "\f2time taken: %d seconds (%d simulated seconds)", pen, (lastmillis-maptime)/1000);
        pen = player1->deaths*60; score += pen; if(pen) conoutf(CON_GAMEINFO, "\f2time penalty for %d deaths (1 minute each): %d seconds", player1->deaths, pen);
        pen = remain*10;          score += pen; if(pen) conoutf(CON_GAMEINFO, "\f2time penalty for %d monsters remaining (10 seconds each): %d seconds", remain, pen);
        pen = (10-skill)*20;      score += pen; if(pen) conoutf(CON_GAMEINFO, "\f2time penalty for lower skill level (20 seconds each): %d seconds", pen);
        pen = 100-accuracy;       score += pen; if(pen) conoutf(CON_GAMEINFO, "\f2time penalty for missed shots (1 second each %%): %d seconds", pen);
        defformatstring(aname)("bestscore_%s", getclientmap());
        const char *bestsc = getalias(aname);
        int bestscore = *bestsc ? parseint(bestsc) : score;
        if(score<bestscore) bestscore = score;
        defformatstring(nscore)("%d", bestscore);
        alias(aname, nscore);
        conoutf(CON_GAMEINFO, "\f2TOTAL SCORE (time + time penalties): %d seconds (best so far: %d seconds)", score, bestscore);
    }
}

