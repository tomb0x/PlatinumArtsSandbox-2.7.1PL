#include "game.h"
#include "servmode.h"

namespace game
{
    void parseoptions(vector<const char *> &args)
    {
        loopv(args)
#ifndef STANDALONE
            if(!game::clientoption(args[i]))
#endif
            if(!server::serveroption(args[i])) {
                conoutf(CON_ERROR, "unknown command-line option: %s", args[i]);
            }
    }
	const char *gameident() { return "movie"; }
}

extern ENetAddress masteraddress;

namespace server
{
    struct server_entity            // server side version of "entity" type
    {
        int type;
        int spawntime;
        char spawned;
        int uid;
    };

    static const int DEATHMILLIS = 300;

    struct clientinfo;

    struct gameevent
    {
        virtual ~gameevent() {}

        virtual bool flush(clientinfo *ci, int fmillis);
        virtual void process(clientinfo *ci) {}

        virtual bool keepable() const { return false; }
    };

    struct timedevent : gameevent
    {
        int millis;

        bool flush(clientinfo *ci, int fmillis);
    };

    struct hitinfo
    {
        int target;
        int lifesequence;
        union
        {
            int rays;
            float dist;
        };
        vec dir;
    };

    struct shotevent : timedevent
    {
        int id, gun;
        vec from, to;
        vector<hitinfo> hits;

        void process(clientinfo *ci);
    };

    struct explodeevent : timedevent
    {
        int id, gun;
        vector<hitinfo> hits;

        bool keepable() const { return true; }

        void process(clientinfo *ci);
    };

    struct pickupevent : gameevent
    {
        int ent;

        void process(clientinfo *ci);
    };

    struct gamestate : dynentstate
    {
        vec o;
        int state, editstate;
//        int lastspawn;
        int lasttimeplayed, timeplayed;

//        gamestate() : state(CS_DEAD), editstate(CS_DEAD), lastspawn(-1) {}
        gamestate() : state(CS_ALIVE), editstate(CS_ALIVE){} //changed by Protagoras

        bool isalive(int gamemillis)
        {
            return state==CS_ALIVE; // || (state==CS_DEAD && gamemillis - lastdeath <= DEATHMILLIS);
        }

        bool waitexpired(int gamemillis)
        {
        	return false;
        }

        void reset()
        {
            if(state!=CS_SPECTATOR) state = editstate = CS_ALIVE; //changed by Protagoras
            timeplayed = 0;
            respawn();
        }

        void respawn()
        {
            dynentstate::respawn();
            o = vec(-1e10f, -1e10f, -1e10f);
        }

        void reassign()
        {
            respawn();
        }
    };

    struct clientinfo
    {
        int clientnum, ownernum, connectmillis, sessionid;
        string name, mapvote;
        int playermodel;
        int modevote;
        int privilege;
        bool connected, local, timesync;
        int gameoffset, lastevent;
        gamestate state;
        vector<gameevent *> events;
        vector<uchar> position, messages;
        int posoff, poslen, msgoff, msglen;
        vector<clientinfo *> bots;
        uint authreq;
        string authname;
        int ping, botreinit;
        string clientmap;
        int mapcrc;
        bool warned, gameclip;

        clientinfo() { reset(); }
        ~clientinfo() { events.deletecontents(); }

        void addevent(gameevent *e)
        {
            if(state.state==CS_SPECTATOR || events.length()>100) delete e;
            else events.add(e);
        }

        void mapchange()
        {
            mapvote[0] = 0;
            state.reset();
            events.deletecontents();
            timesync = false;
            lastevent = 0;
            clientmap[0] = '\0';
            mapcrc = 0;
            warned = false;
            gameclip = false;
        }

        void reassign()
        {
            state.reassign();
            events.deletecontents();
            timesync = false;
            lastevent = 0;
        }

        void reset()
        {
            name[0] = 0;
            playermodel = -1;
            privilege = PRIV_NONE;
            connected = local = false;
            authreq = 0;
            position.setsize(0);
            messages.setsize(0);
            ping = 0;
            botreinit = 0;
            mapchange();
        }

        int geteventmillis(int servmillis, int clientmillis)
        {
            if(!timesync || (events.empty() && state.waitexpired(servmillis)))
            {
                timesync = true;
                gameoffset = servmillis - clientmillis;
                return servmillis;
            }
            else return gameoffset + clientmillis;
        }
    };

    struct worldstate
    {
        int uses;
        vector<uchar> positions, messages;
    };

    struct ban
    {
        int time;
        uint ip;
    };

    namespace botmgr
    {
        extern void removebot(clientinfo *ci);
        extern void clearbot();
        extern void checkbot();
        extern void reqadd(clientinfo *ci);
        extern void reqdel(clientinfo *ci);
        extern 	void deletebot(clientinfo *ci);
        extern void setbotlimit(clientinfo *ci, int limit);
        extern void setbotbalance(clientinfo *ci, bool balance);
        extern void changemap();
        extern void addclient(clientinfo *ci);
        extern void changeteam(clientinfo *ci);
    }

    #define MM_MODE 0xF
    #define MM_AUTOAPPROVE 0x1000
    #define MM_PRIVSERV (MM_MODE | MM_AUTOAPPROVE)
    #define MM_PUBSERV ((1<<MM_OPEN) | (1<<MM_VETO))
    #define MM_COOPSERV (MM_AUTOAPPROVE | MM_PUBSERV | (1<<MM_LOCKED))
    //#define print if(dedicated == 2) conoutf
    #ifdef STANDALONE
        #define print conoutf
    #else
        #define print if(dedicated == 2) conoutf
    #endif

    bool notgotitems = true;        // true when map has changed and waiting for clients to send item
    int gamemode = 1;
    int gamemillis = 0, gamelimit = 0;
    bool gamepaused = false;

    string smapname = "";
    int interm = 0, minremain = 0;
    bool mapreload = false;
    enet_uint32 lastsend = 0;
    int mastermode = MM_OPEN, mastermask = MM_PRIVSERV;
    int currentmaster = -1;
    bool masterupdate = false;

	//offtools (see game.h)
	mapdata upload;
	bool serverupload = false;

    vector<uint> allowedips;
    vector<ban> bannedips;
    vector<clientinfo *> connects, clients, bots;
    vector<worldstate *> worldstates;
    bool reliablemessages = false;

    struct demofile
    {
        string info;
        uchar *data;
        int len;
    };

    #define MAXDEMOS 5
    vector<demofile> demos;

    bool demonextmatch = false;
    stream *demotmp = NULL, *demorecord = NULL, *demoplayback = NULL;
    int nextplayback = 0, demomillis = 0;

    #define SERVMODE 1
//    #include "capture.h"
//    #include "ctf.h"

//    captureservmode capturemode;
//    ctfservmode ctfmode;
    servmode *smode = new defaultservmode;

    SVAR(serverdesc, "");
    SVAR(serverpass, "");
    SVAR(adminpass, "");
    VARF(publicserver, 0, 0, 2, {
		switch(publicserver)
		{
			case 0: default: mastermask = MM_PRIVSERV; break;
			case 1: mastermask = MM_PUBSERV; break;
			case 2: mastermask = MM_COOPSERV; break;
		}
	});
    SVAR(servermotd, "");

    void *newclientinfo() { return new clientinfo; }
    void deleteclientinfo(void *ci) { delete (clientinfo *)ci; }

    clientinfo *getinfo(int n)
    {
        if(n < MAXCLIENTS) return (clientinfo *)getclientinfo(n);
        n -= MAXCLIENTS;
        return bots.inrange(n) ? bots[n] : NULL;
    }

    vector<server_entity> sents;

    int lastmapuid = -1;

    int msgsizelookup(int msg)
    {
        static int sizetable[NUMSV] = { -1 };
        if(sizetable[0] < 0)
        {
            memset(sizetable, -1, sizeof(sizetable));
            for(const int *p = msgsizes; *p >= 0; p += 2) sizetable[p[0]] = p[1];
        }
        return msg >= 0 && msg < NUMSV ? sizetable[msg] : -1;
    }

    const char *modename(int n, const char *unknown)
    {
        if(m_valid(n)) return gamemodes[n - STARTGAMEMODE].name;
        return unknown;
    }

    const char *mastermodename(int n, const char *unknown)
    {
        return (n>=MM_START && size_t(n-MM_START)<sizeof(mastermodenames)/sizeof(mastermodenames[0])) ? mastermodenames[n-MM_START] : unknown;
    }

    const char *privname(int type)
    {
        switch(type)
        {
            case PRIV_ADMIN: return "admin";
            case PRIV_MASTER: return "master";
            default: return "unknown";
        }
    }

    void sendservmsg(const char *s) { sendf(-1, 1, "ris", SV_SERVMSG, s); }

    void resetitems()
    {
        sents.setsize(0);
    }

    bool serveroption(const char *arg)
    {
        if(arg[0]=='-') switch(arg[1])
        {
            case 'n': setsvar("serverdesc", &arg[2]); return true;
            case 'y': setsvar("serverpass", &arg[2]); return true;
            case 'p': setsvar("adminpass", &arg[2]); return true;
            case 'o': setvar("publicserver", atoi(&arg[2])); return true;
            case 'g': setvar("serverbotlimit", atoi(&arg[2])); return true;
			case 'e': setvar("serverupload", serverupload = atoi(&arg[2]) ); return true;
        }
        return false;
    }

    void serverinit()
    {
        smapname[0] = '\0';
        resetitems();
    }

    int numclients(int exclude = -1, bool nospec = true, bool noai = true)
    {
        int n = 0;
        loopv(clients) if(i!=exclude && (!nospec || clients[i]->state.state!=CS_SPECTATOR) && (!noai || clients[i]->state.clienttype == CLIENT_PLAYER)) n++;
        return n;
    }

    bool duplicatename(clientinfo *ci, char *name)
    {
        if(!name) name = ci->name;
        loopv(clients) if(clients[i]!=ci && !strcmp(name, clients[i]->name)) return true;
        return false;
    }

    const char *colorname(clientinfo *ci, char *name = NULL)
    {
        if(!name) name = ci->name;
        if(name[0] && !duplicatename(ci, name) && ci->state.clienttype == CLIENT_PLAYER) return name;
        static string cname[3];
        static int cidx = 0;
        cidx = (cidx+1)%3;
        formatstring(cname[cidx])(ci->state.clienttype == CLIENT_PLAYER ? "%s \fs\f5(%d)\fr" : "%s \fs\f5[%d]\fr", name, ci->clientnum);
        return cname[cidx];
    }

    bool canspawnitem(int type) { return !m_noitems && (type>=I_SHELLS && type<=I_QUAD && (!m_noammo || type<I_SHELLS || type>I_CARTRIDGES)); }

    int spawntime(int type)
    {
        if(m_classicsp) return INT_MAX;
        int np = numclients(-1, true, false);
        np = np<3 ? 4 : (np>4 ? 2 : 3);         // spawn times are dependent on number of players
        int sec = 0;
        switch(type)
        {
            case I_SHELLS:
            case I_BULLETS:
            case I_ROCKETS:
            case I_ROUNDS:
            case I_GRENADES:
            case I_CARTRIDGES: sec = np*4; break;
            case I_HEALTH: sec = np*5; break;
            case I_GREENARMOUR:
            case I_YELLOWARMOUR: sec = 20; break;
            case I_BOOST:
            case I_QUAD: sec = 40+rnd(40); break;
        }
        return sec*1000;
    }

    bool pickup(int i, int sender)         // server side item pickup, acknowledge first client that gets it
    {
        if(minremain<=0 || !sents.inrange(i) || !sents[i].spawned) return false;
        clientinfo *ci = getinfo(sender);
        if(!ci || (!ci->local && !ci->state.canpickup(sents[i].type))) return false;
        sents[i].spawned = false;
        sents[i].spawntime = spawntime(sents[i].type);
        sendf(-1, 1, "ri3", SV_ITEMACC, i, sender);
        ci->state.pickup(sents[i].type);
        return true;
    }

    void writedemo(int chan, void *data, int len)
    {
        if(!demorecord) return;
        int stamp[3] = { gamemillis, chan, len };
        lilswap(stamp, 3);
        demorecord->write(stamp, sizeof(stamp));
        demorecord->write(data, len);
    }

    void recordpacket(int chan, void *data, int len)
    {
        writedemo(chan, data, len);
    }

    void enddemorecord()
    {
        if(!demorecord) return;

        DELETEP(demorecord);

        if(!demotmp) return;

        int len = demotmp->size();
        if(demos.length()>=MAXDEMOS)
        {
            delete[] demos[0].data;
            demos.remove(0);
        }
        demofile &d = demos.add();
        time_t t = time(NULL);
        char *timestr = ctime(&t), *trim = timestr + strlen(timestr);
        while(trim>timestr && isspace(*--trim)) *trim = '\0';
        formatstring(d.info)("%s: %s, %s, %.2f%s", timestr, modename(gamemode), smapname, len > 1024*1024 ? len/(1024*1024.f) : len/1024.0f, len > 1024*1024 ? "MB" : "kB");
        defformatstring(msg)("demo \"%s\" recorded", d.info);
        sendservmsg(msg);
        d.data = new uchar[len];
        d.len = len;
        demotmp->seek(0, SEEK_SET);
        demotmp->read(d.data, len);
        DELETEP(demotmp);
    }

    int welcomepacket(packetbuf &p, clientinfo *ci);
    void sendwelcome(clientinfo *ci);

    void setupdemorecord()
    {
        if(!m_mp(gamemode) || m_edit) return;

        demotmp = opentempfile("demorecord", "w+b");
        if(!demotmp) return;

        stream *f = opengzfile(NULL, "wb", demotmp);
        if(!f) { DELETEP(demotmp); return; }

        sendservmsg("recording demo");

        demorecord = f;

        demoheader hdr;
        memcpy(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic));
        hdr.version = DEMO_VERSION;
        hdr.protocol = PROTOCOL_VERSION;
        lilswap(&hdr.version, 2);
        demorecord->write(&hdr, sizeof(demoheader));

        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        welcomepacket(p, NULL);
        writedemo(1, p.buf, p.len);
    }

    void listdemos(int cn)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putint(p, SV_SENDDEMOLIST);
        putint(p, demos.length());
        loopv(demos) sendstring(demos[i].info, p);
        sendpacket(cn, 1, p.finalize());
    }

    void cleardemos(int n)
    {
        if(!n)
        {
            loopv(demos) delete[] demos[i].data;
            demos.setsize(0);
            sendservmsg("cleared all demos");
        }
        else if(demos.inrange(n-1))
        {
            delete[] demos[n-1].data;
            demos.remove(n-1);
            defformatstring(msg)("cleared demo %d", n);
            sendservmsg(msg);
        }
    }

    void senddemo(int cn, int num)
    {
        if(!num) num = demos.length();
        if(!demos.inrange(num-1)) return;
        demofile &d = demos[num-1];
        sendf(cn, 2, "rim", SV_SENDDEMO, d.len, d.data);
    }

    void enddemoplayback()
    {
        if(!demoplayback) return;
        DELETEP(demoplayback);

        loopv(clients) sendf(clients[i]->clientnum, 1, "ri3", SV_DEMOPLAYBACK, 0, clients[i]->clientnum);

        sendservmsg("demo playback finished");

        loopv(clients) sendwelcome(clients[i]);
    }

    void setupdemoplayback()
    {
        if(demoplayback) return;
        demoheader hdr;
        string msg;
        msg[0] = '\0';
        defformatstring(file)("%s.dmo", smapname);
        demoplayback = opengzfile(file, "rb");
        if(!demoplayback) formatstring(msg)("could not read demo \"%s\"", file);
        else if(demoplayback->read(&hdr, sizeof(demoheader))!=sizeof(demoheader) || memcmp(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic)))
            formatstring(msg)("\"%s\" is not a demo file", file);
        else
        {
            lilswap(&hdr.version, 2);
            if(hdr.version!=DEMO_VERSION) formatstring(msg)("demo \"%s\" requires an %s version of Cube 2: Sauerbraten", file, hdr.version<DEMO_VERSION ? "older" : "newer");
            else if(hdr.protocol!=PROTOCOL_VERSION) formatstring(msg)("demo \"%s\" requires an %s version of Cube 2: Sauerbraten", file, hdr.protocol<PROTOCOL_VERSION ? "older" : "newer");
        }
        if(msg[0])
        {
            DELETEP(demoplayback);
            sendservmsg(msg);
            return;
        }

        formatstring(msg)("playing demo \"%s\"", file);
        sendservmsg(msg);

        demomillis = 0;
        sendf(-1, 1, "ri3", SV_DEMOPLAYBACK, 1, -1);

        if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
        {
            enddemoplayback();
            return;
        }
        lilswap(&nextplayback, 1);
    }

    void readdemo()
    {
        if(!demoplayback || gamepaused) return;
        demomillis += curtime;
        while(demomillis>=nextplayback)
        {
            int chan, len;
            if(demoplayback->read(&chan, sizeof(chan))!=sizeof(chan) ||
               demoplayback->read(&len, sizeof(len))!=sizeof(len))
            {
                enddemoplayback();
                return;
            }
            lilswap(&chan, 1);
            lilswap(&len, 1);
            ENetPacket *packet = enet_packet_create(NULL, len, 0);
            if(!packet || demoplayback->read(packet->data, len)!=len)
            {
                if(packet) enet_packet_destroy(packet);
                enddemoplayback();
                return;
            }
            sendpacket(-1, chan, packet);
            if(!packet->referenceCount) enet_packet_destroy(packet);
            if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
            {
                enddemoplayback();
                return;
            }
            lilswap(&nextplayback, 1);
        }
    }

    void stopdemo()
    {
        if(m_demo) enddemoplayback();
        else enddemorecord();
    }

    void pausegame(bool val)
    {
        if(gamepaused==val) return;
        gamepaused = val;
        sendf(-1, 1, "rii", SV_PAUSEGAME, gamepaused ? 1 : 0);
    }

    void hashpassword(int cn, int sessionid, const char *pwd, char *result, int maxlen)
    {
        char buf[2*sizeof(string)];
        formatstring(buf)("%d %d ", cn, sessionid);
        copystring(&buf[strlen(buf)], pwd);
        if(!hashstring(buf, result, maxlen)) *result = '\0';
    }

    bool checkpassword(clientinfo *ci, const char *wanted, const char *given)
    {
        string hash;
        hashpassword(ci->clientnum, ci->sessionid, wanted, hash, sizeof(hash));
        return !strcmp(hash, given);
    }

    void revokemaster(clientinfo *ci)
    {
        ci->privilege = PRIV_NONE;
        if(ci->state.state==CS_SPECTATOR && !ci->local) botmgr::removebot(ci);
    }

    void setmaster(clientinfo *ci, bool val, const char *pass = "", const char *authname = NULL)
    {
        if(authname && !val) return;
        const char *name = "";
        if(val)
        {
            bool haspass = adminpass[0] && checkpassword(ci, adminpass, pass);
            if(ci->privilege)
            {
                if(!adminpass[0] || haspass==(ci->privilege==PRIV_ADMIN)) return;
            }
            else if(ci->state.state==CS_SPECTATOR && !haspass && !authname && !ci->local) return;
            loopv(clients) if(ci!=clients[i] && clients[i]->privilege)
            {
                if(haspass) clients[i]->privilege = PRIV_NONE;
                else if((authname || ci->local) && clients[i]->privilege<=PRIV_MASTER) continue;
                else return;
            }
            if(haspass) ci->privilege = PRIV_ADMIN;
            else if(!authname && !(mastermask&MM_AUTOAPPROVE) && !ci->privilege && !ci->local)
            {
                sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "This server requires you to use the \"/auth\" command to gain master.");
                return;
            }
            else
            {
                if(authname)
                {
                    loopv(clients) if(ci!=clients[i] && clients[i]->privilege<=PRIV_MASTER) revokemaster(clients[i]);
                }
                ci->privilege = PRIV_MASTER;
            }
            name = privname(ci->privilege);
        }
        else
        {
            if(!ci->privilege) return;
            name = privname(ci->privilege);
            revokemaster(ci);
        }
        mastermode = MM_OPEN;
        allowedips.setsize(0);
        string msg;
        if(val && authname)
	{
		formatstring(msg)("%s claimed %s as '\fs\f5%s\fr'", colorname(ci), name, authname);
		print(msg);
	}
        else
	{
		formatstring(msg)("%s %s %s", colorname(ci), val ? "claimed" : "relinquished", name);
		print(msg);
	}
        sendservmsg(msg);
        currentmaster = val ? ci->clientnum : -1;
        masterupdate = true;
        if(gamepaused)
        {
            int admins = 0;
            loopv(clients) if(clients[i]->privilege >= PRIV_ADMIN || clients[i]->local) admins++;
            if(!admins) pausegame(false);
        }
    }

    int checktype(int type, clientinfo *ci)
    {
        if(ci && ci->local) return type;
        // only allow edit messages in coop-edit mode
        if(type>=SV_EDITENT && type<=SV_EDITVAR && !m_edit) return -1;
        // server only messages
        static int servtypes[] = { SV_SERVINFO, SV_INITCLIENT, SV_WELCOME, SV_MAPRELOAD, SV_SERVMSG, SV_SPAWNSTATE, SV_FORCEDEATH, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_CDIS, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_BASESCORE, SV_BASEINFO, SV_BASEREGEN, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_SENDMAP, SV_DROPFLAG, SV_SCOREFLAG, SV_RETURNFLAG, SV_RESETFLAG, SV_INVISFLAG, SV_CLIENT, SV_AUTHCHAL, SV_INITBOT };
        if(ci) loopi(sizeof(servtypes)/sizeof(int)) if(type == servtypes[i]) return -1;
        return type;
    }

    void cleanworldstate(ENetPacket *packet)
    {
        loopv(worldstates)
        {
            worldstate *ws = worldstates[i];
            if(ws->positions.inbuf(packet->data) || ws->messages.inbuf(packet->data)) ws->uses--;
            else continue;
            if(!ws->uses)
            {
                delete ws;
                worldstates.remove(i);
            }
            break;
        }
    }

    void addclientstate(worldstate &ws, clientinfo &ci)
    {
        if(ci.position.empty()) ci.posoff = -1;
        else
        {
            ci.posoff = ws.positions.length();
            loopvj(ci.position) ws.positions.add(ci.position[j]);
            ci.poslen = ws.positions.length() - ci.posoff;
            ci.position.setsize(0);
        }
        if(ci.messages.empty()) ci.msgoff = -1;
        else
        {
            ci.msgoff = ws.messages.length();
            ucharbuf p = ws.messages.reserve(16);
            putint(p, SV_CLIENT);
            putint(p, ci.clientnum);
            putuint(p, ci.messages.length());
            ws.messages.addbuf(p);
            loopvj(ci.messages) ws.messages.add(ci.messages[j]);
            ci.msglen = ws.messages.length() - ci.msgoff;
            ci.messages.setsize(0);
        }
    }

    bool buildworldstate()
    {
        worldstate &ws = *new worldstate;
        loopv(clients)
        {
            clientinfo &ci = *clients[i];
            if(ci.state.clienttype != CLIENT_PLAYER) continue;
            addclientstate(ws, ci);
            loopv(ci.bots)
            {
                clientinfo &bi = *ci.bots[i];
                addclientstate(ws, bi);
                if(bi.posoff >= 0)
                {
                    if(ci.posoff < 0) { ci.posoff = bi.posoff; ci.poslen = bi.poslen; }
                    else ci.poslen += bi.poslen;
                }
                if(bi.msgoff >= 0)
                {
                    if(ci.msgoff < 0) { ci.msgoff = bi.msgoff; ci.msglen = bi.msglen; }
                    else ci.msglen += bi.msglen;
                }
            }
        }
        int psize = ws.positions.length(), msize = ws.messages.length();
        if(psize) recordpacket(0, ws.positions.getbuf(), psize);
        if(msize) recordpacket(1, ws.messages.getbuf(), msize);
        loopi(psize) { uchar c = ws.positions[i]; ws.positions.add(c); }
        loopi(msize) { uchar c = ws.messages[i]; ws.messages.add(c); }
        ws.uses = 0;
        if(psize || msize) loopv(clients)
        {
            clientinfo &ci = *clients[i];
            if(ci.state.clienttype != CLIENT_PLAYER) continue;
            ENetPacket *packet;
            if(psize && (ci.posoff<0 || psize-ci.poslen>0))
            {
                packet = enet_packet_create(&ws.positions[ci.posoff<0 ? 0 : ci.posoff+ci.poslen],
                                            ci.posoff<0 ? psize : psize-ci.poslen,
                                            ENET_PACKET_FLAG_NO_ALLOCATE);
                sendpacket(ci.clientnum, 0, packet);
                if(!packet->referenceCount) enet_packet_destroy(packet);
                else { ++ws.uses; packet->freeCallback = cleanworldstate; }
            }

            if(msize && (ci.msgoff<0 || msize-ci.msglen>0))
            {
                packet = enet_packet_create(&ws.messages[ci.msgoff<0 ? 0 : ci.msgoff+ci.msglen],
                                            ci.msgoff<0 ? msize : msize-ci.msglen,
                                            (reliablemessages ? ENET_PACKET_FLAG_RELIABLE : 0) | ENET_PACKET_FLAG_NO_ALLOCATE);
                sendpacket(ci.clientnum, 1, packet);
                if(!packet->referenceCount) enet_packet_destroy(packet);
                else { ++ws.uses; packet->freeCallback = cleanworldstate; }
            }
        }
        reliablemessages = false;
        if(!ws.uses)
        {
            delete &ws;
            return false;
        }
        else
        {
            worldstates.add(&ws);
            return true;
        }
    }

    bool sendpackets(bool force)
    {
        if(clients.empty() || (!hasnonlocalclients() && !demorecord)) return false;
        enet_uint32 curtime = enet_time_get()-lastsend;
        if(curtime<33 && !force) return false;
        bool flush = buildworldstate();
        lastsend += curtime - (curtime%33);
        return flush;
    }

    template<class T>
    void sendstate(gamestate &gs, T &p)
    {
    	//send gamestate
    }

    void spawnstate(clientinfo *ci)
    {
        gamestate &gs = ci->state;
        gs.spawnstate(gamemode);
    }

    void sendspawn(clientinfo *ci)
    {
//        gamestate &gs = ci->state;
        spawnstate(ci);

//        defformatstring(dbg)("DEBUG: server::sendspawn send SV_SPAWNSTATE cl: %d, state: %d", ci->clientnum, ci->state.state);
//        sendf(-1, 1, "ris", SV_SERVMSG, dbg);

        sendf(ci->ownernum, 1, "rii", SV_SPAWNSTATE, ci->clientnum);
//        gs.lastspawn = gamemillis;
    }

    void sendwelcome(clientinfo *ci)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        int chan = welcomepacket(p, ci);
        sendpacket(ci->clientnum, chan, p.finalize());
    }

    //Notify new clients about existing player or bots
    //TODO: *no tests here, let client handle the type:
    //      clienttype is either bot || player controltype is CONTROL_REMOTE
    //      *remove SV_INITBOT, add clienttype, ownernum to SV_INITCLIENT
    void putinitclient(clientinfo *ci, packetbuf &p)
    {
        if(ci->state.clienttype != CLIENT_PLAYER)
        {
            putint(p, SV_INITBOT);
            putint(p, ci->clientnum);
            putint(p, ci->ownernum);
            putint(p, ci->state.clienttype);
            putint(p, ci->playermodel);
//            defformatstring(dbg)("DEBUG: server::putinitclient SV_INITBOT cl: %d", ci->clientnum);
//            sendf(-1, 1, "ris", SV_SERVMSG, dbg);
        }
        else
        {
            putint(p, SV_INITCLIENT);
            putint(p, ci->clientnum);
            sendstring(ci->name, p);
            putint(p, ci->playermodel);
//            defformatstring(dbg)("DEBUG: server::putinitclient SV_INITCLIENT cl: %d, state: %d", ci->clientnum, ci->state.state);
//            sendf(-1, 1, "ris", SV_SERVMSG, dbg);
        }
    }

    //Notify new clients about other clients (see putinitclient)
    void welcomeinitclient(packetbuf &p, int exclude = -1)
    {
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(!ci->connected || ci->clientnum == exclude) continue;

            putinitclient(ci, p);
        }
    }

    //Welcome Packet on connect, sends about whats going on
    int welcomepacket(packetbuf &p, clientinfo *ci)
    {
        int hasmap = (m_edit && (clients.length()>1 || (ci && ci->local))) || (smapname[0] && (minremain>0 || (ci && ci->state.state==CS_SPECTATOR) || numclients(ci && ci->local ? ci->clientnum : -1)));
        putint(p, SV_WELCOME);
        putint(p, hasmap);
        if(hasmap)
        {
            putint(p, SV_MAPCHANGE);
            sendstring(smapname, p);
            putint(p, gamemode);
            putint(p, notgotitems ? 1 : 0);
            if(!ci || (m_timed && smapname[0]))
            {
                putint(p, SV_TIMEUP);
                putint(p, minremain);
            }
            if(!notgotitems)
            {
                putint(p, SV_ITEMLIST);
                loopv(sents) if(sents[i].spawned)
                {
                    putint(p, i);
                    putint(p, sents[i].type);
                }
                putint(p, -1);
            }
        }
        if(gamepaused)
        {
            putint(p, SV_PAUSEGAME);
            putint(p, 1);
        }
        if(ci && (m_demo || m_mp(gamemode)) && ci->state.state!=CS_SPECTATOR)
        {
//            if(smode && !smode->canspawn(ci, true))
//            {
//                ci->state.state = CS_DEAD;
//                putint(p, SV_FORCEDEATH);
//                putint(p, ci->clientnum);
//                sendf(-1, 1, "ri2x", SV_FORCEDEATH, ci->clientnum, ci->clientnum);
//            }
//            else
//            {
                gamestate &gs = ci->state;
                spawnstate(ci);
//                defformatstring(dbg)("DEBUG: server::welcomepacket send SV_SPAWNSTATE cl: %d", ci->clientnum);
//                sendf(-1, 1, "ris", SV_SERVMSG, dbg);
                putint(p, SV_SPAWNSTATE);
                putint(p, ci->clientnum);
                sendstate(gs, p);
//            }
        }
        if(ci && ci->state.state==CS_SPECTATOR)
        {
            putint(p, SV_SPECTATOR);
            putint(p, ci->clientnum);
            putint(p, 1);
            sendf(-1, 1, "ri3x", SV_SPECTATOR, ci->clientnum, 1, ci->clientnum);
        }
        if(!ci || clients.length()>1)
        {
        	//TODO: check SV_RESUME
            putint(p, SV_RESUME);
            loopv(clients)
            {
                clientinfo *oi = clients[i];
//                defformatstring(dbg)("DEBUG: server::welcomepacket SV_SV_RESUME cl: %d, state: %d", oi->clientnum, oi->state.state);
//                sendf(-1, 1, "ris", SV_SERVMSG, dbg);

                if(ci && oi->clientnum==ci->clientnum) continue;
                putint(p, oi->clientnum);
                putint(p, oi->state.state);
//                putint(p, oi->state.frags);
//                putint(p, oi->state.quadmillis);
//                sendstate(oi->state, p); //TODO: smode->sendstate();
            }
            putint(p, -1);
            welcomeinitclient(p, ci ? ci->clientnum : -1);
        }
        if(smode) smode->initclient(ci, p, true);
        return 1;
    }

    void sendresume(clientinfo *ci)
    {
    	//TODO: correct SV_RESUME
//        defformatstring(dbg)("DEBUG: server::sendresume ci: %d, state: %d", ci->clientnum, ci->state);
//        sendf(-1, 1, "ris", SV_SERVMSG, dbg);

        gamestate &gs = ci->state;
                sendf(-1, 1, "ri3i", SV_RESUME, ci->clientnum, gs.state, -1);
//        sendf(-1, 1, "ri2i9vi", SV_RESUME, ci->clientnum,
//            gs.state, gs.frags, gs.quadmillis,
//            gs.lifesequence,
//            gs.health, gs.maxhealth,
//            gs.armour, gs.armourtype,
//            gs.gunselect, GUN_PISTOL-GUN_SG+1, &gs.ammo[GUN_SG], -1);
    }

    void sendinitclient(clientinfo *ci)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putinitclient(ci, p);
        sendpacket(-1, 1, p.finalize(), ci->clientnum);
    }

    void changemap(const char *s, int mode)
    {
        stopdemo();
        pausegame(false);
        if(smode) smode->reset(false);
        botmgr::clearbot();

        mapreload = false;
        gamemode = mode;
        gamemillis = 0;
        minremain = m_overtime ? 15 : 10;
        gamelimit = minremain*60000;
        interm = 0;
        copystring(smapname, s);
        resetitems();
        notgotitems = true;
        upload.reset();
        lastmapuid = -1;

        loopv(clients)
        {
            clientinfo *ci = clients[i];
            ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
        }

        if(!m_mp(gamemode)) kicknonlocalclients(DISC_PRIVATE);

//        if(m_teammode) autoteam();

//        if(m_capture) smode = &capturemode;
//        else if(m_ctf) smode = &ctfmode;
//        else smode = NULL;
//        smode = &defaultservmode;
        if(smode) smode->reset(false);

        if(m_timed && smapname[0]) sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            ci->mapchange();
            ci->state.lasttimeplayed = lastmillis;
            if(m_mp(gamemode) && ci->state.state!=CS_SPECTATOR) sendspawn(ci);
        }

        botmgr::changemap();

        if(m_demo)
        {
            if(clients.length()) setupdemoplayback();
        }
        else if(demonextmatch)
        {
            demonextmatch = false;
            setupdemorecord();
        }
    }

    struct votecount
    {
        char *map;
        int mode, count;
        votecount() {}
        votecount(char *s, int n) : map(s), mode(n), count(0) {}
    };

    void checkvotes(bool force = false)
    {
        vector<votecount> votes;
        int maxvotes = 0;
        loopv(clients)
        {
            clientinfo *oi = clients[i];
            if(oi->state.state==CS_SPECTATOR && !oi->privilege && !oi->local) continue;
            if(oi->state.clienttype!=CLIENT_PLAYER) continue;
            maxvotes++;
            if(!oi->mapvote[0]) continue;
            votecount *vc = NULL;
            loopvj(votes) if(!strcmp(oi->mapvote, votes[j].map) && oi->modevote==votes[j].mode)
            {
                vc = &votes[j];
                break;
            }
            if(!vc) vc = &votes.add(votecount(oi->mapvote, oi->modevote));
            vc->count++;
        }
        votecount *best = NULL;
        loopv(votes) if(!best || votes[i].count > best->count || (votes[i].count == best->count && rnd(2))) best = &votes[i];
        if(force || (best && best->count > maxvotes/2))
        {
            if(demorecord) enddemorecord();
            if(best && (best->count > (force ? 1 : maxvotes/2)))
            {
                sendservmsg(force ? "vote passed by default" : "vote passed by majority");
                sendf(-1, 1, "risii", SV_MAPCHANGE, best->map, best->mode, 1);
		print((force ? "admin/master forced map change to %s" : "map change vote passed for %s" ), best->map);
                changemap(best->map, best->mode);
            }
            else
            {
                mapreload = true;
                if(clients.length()) sendf(-1, 1, "ri", SV_MAPRELOAD);
            }
        }
    }

    void forcemap(const char *map, int mode)
    {
        stopdemo();
        if(hasnonlocalclients() && !mapreload)
        {
            defformatstring(msg)("local player forced %s on map %s", modename(mode), map);
            sendservmsg(msg);
        }
        sendf(-1, 1, "risii", SV_MAPCHANGE, map, mode, 1);
        changemap(map, mode);
    }

    void vote(char *map, int reqmode, int sender)
    {
        clientinfo *ci = getinfo(sender);
        if(!ci || (ci->state.state==CS_SPECTATOR && !ci->privilege && !ci->local) || (!ci->local && !m_mp(reqmode))) return;
        copystring(ci->mapvote, map);
        ci->modevote = reqmode;
        if(!ci->mapvote[0]) return;
        if(ci->local || mapreload || (ci->privilege && mastermode>=MM_VETO))
        {
            if(demorecord) enddemorecord();
            if((!ci->local || hasnonlocalclients()) && !mapreload)
            {
                defformatstring(msg)("%s forced %s on map %s", ci->privilege && mastermode>=MM_VETO ? privname(ci->privilege) : "local player", modename(ci->modevote), ci->mapvote);
                sendservmsg(msg);
            }
            sendf(-1, 1, "risii", SV_MAPCHANGE, ci->mapvote, ci->modevote, 1);
            changemap(ci->mapvote, ci->modevote);
        }
        else
        {
            defformatstring(msg)("%s suggests %s on map %s (select map to vote)", colorname(ci), modename(reqmode), map);
            sendservmsg(msg);
            checkvotes();
        }
    }

    void checkintermission()
    {
        if(minremain>0)
        {
            minremain = gamemillis>=gamelimit ? 0 : (gamelimit - gamemillis + 60000 - 1)/60000;
            sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
            if(!minremain && smode) smode->intermission();
        }
        if(!interm && minremain<=0) interm = gamemillis+10000;
    }

    void startintermission() { gamelimit = min(gamelimit, gamemillis); checkintermission(); }

    void dodamage(clientinfo *target, clientinfo *actor, int damage, int gun, const vec &hitpush = vec(0, 0, 0))
    {
//        gamestate &ts = target->state;
//        ts.dodamage(damage);
//        actor->state.damage += damage;
//        sendf(-1, 1, "ri6", SV_DAMAGE, target->clientnum, actor->clientnum, damage, ts.armour, ts.health);
//        if(target!=actor && !hitpush.iszero())
//        {
//            ivec v = vec(hitpush).rescale(DNF);
//            sendf(ts.health<=0 ? -1 : target->ownernum, 1, "ri7", SV_HITPUSH, target->clientnum, gun, damage, v.x, v.y, v.z);
//        }
//        if(ts.health<=0)
//        {
//            target->state.deaths++;
////            if(actor!=target && isteam(actor->team, target->team)) actor->state.teamkills++;
////            int fragvalue = smode ? smode->fragvalue(target, actor) : (target==actor || isteam(target->team, actor->team) ? -1 : 1);
//            int fragvalue = smode ? smode->fragvalue(target, actor) : (target==actor ? -1 : 1);
//            actor->state.frags += fragvalue;
//            if(fragvalue>0)
//            {
//                int friends = 0, enemies = 0; // note: friends also includes the fragger
////                if(m_teammode) loopv(clients) if(strcmp(clients[i]->team, actor->team)) enemies++; else friends++;
////                else { friends = 1; enemies = clients.length()-1; }
//                friends = 1; enemies = clients.length()-1;
//                actor->state.effectiveness += fragvalue*friends/float(max(enemies, 1));
//            }
//            sendf(-1, 1, "ri4", SV_DIED, target->clientnum, actor->clientnum, actor->state.frags);
//            target->position.setsizenodelete(0);
//            if(smode) smode->died(target, actor);
//            ts.state = CS_DEAD;
//            ts.lastdeath = gamemillis;
//            // don't issue respawn yet until DEATHMILLIS has elapsed
//            // ts.respawn();
//        }
    }

    void explodeevent::process(clientinfo *ci)
    {
//        gamestate &gs = ci->state;
//        switch(gun)
//        {
//            case GUN_RL:
//                if(!gs.rockets.remove(id)) return;
//                break;
//
//            case GUN_GL:
//                if(!gs.grenades.remove(id)) return;
//                break;
//
//            default:
//                return;
//        }
//        loopv(hits)
//        {
//            hitinfo &h = hits[i];
//            clientinfo *target = getinfo(h.target);
//            if(!target || target->state.state!=CS_ALIVE || h.lifesequence!=target->state.lifesequence || h.dist<0 || h.dist>RL_DAMRAD) continue;
//
//            bool dup = false;
//            loopj(i) if(hits[j].target==h.target) { dup = true; break; }
//            if(dup) continue;
//
//            int damage = guns[gun].damage;
//            if(gs.quadmillis) damage *= 4;
//            damage = int(damage*(1-h.dist/RL_DISTSCALE/RL_DAMRAD));
//            if(gun==GUN_RL && target==ci) damage /= RL_SELFDAMDIV;
//            dodamage(target, ci, damage, gun, h.dir);
//        }
    }

    void shotevent::process(clientinfo *ci)
    {
//        gamestate &gs = ci->state;
//        int wait = millis - gs.lastshot;
//        if(!gs.isalive(gamemillis) ||
//           wait<gs.gunwait ||
//           gun<GUN_FIST || gun>GUN_PISTOL ||
//           gs.ammo[gun]<=0)
//            return;
//        if(gun!=GUN_FIST) gs.ammo[gun]--;
//        gs.lastshot = millis;
//        gs.gunwait = guns[gun].attackdelay;
//        sendf(-1, 1, "ri9x", SV_SHOTFX, ci->clientnum, gun,
//                int(from.x*DMF), int(from.y*DMF), int(from.z*DMF),
//                int(to.x*DMF), int(to.y*DMF), int(to.z*DMF),
//                ci->ownernum);
//        gs.shotdamage += guns[gun].damage*(gs.quadmillis ? 4 : 1)*(gun==GUN_SG ? SGRAYS : 1);
//        switch(gun)
//        {
//            case GUN_RL: gs.rockets.add(id); break;
//            case GUN_GL: gs.grenades.add(id); break;
//            default:
//            {
//                int totalrays = 0, maxrays = gun==GUN_SG ? SGRAYS : 1;
//                loopv(hits)
//                {
//                    hitinfo &h = hits[i];
//                    clientinfo *target = getinfo(h.target);
//                    if(!target || target->state.state!=CS_ALIVE || h.lifesequence!=target->state.lifesequence || h.rays<1) continue;
//
//                    totalrays += h.rays;
//                    if(totalrays>maxrays) continue;
//                    int damage = h.rays*guns[gun].damage;
//                    if(gs.quadmillis) damage *= 4;
//                    dodamage(target, ci, damage, gun, h.dir);
//                }
//                break;
//            }
//        }
    }

    void pickupevent::process(clientinfo *ci)
    {
//        gamestate &gs = ci->state;
//        if(m_mp(gamemode) && !gs.isalive(gamemillis)) return;
//        pickup(ent, ci->clientnum);
    }

    bool gameevent::flush(clientinfo *ci, int fmillis)
    {
        process(ci);
        return true;
    }

    bool timedevent::flush(clientinfo *ci, int fmillis)
    {
        if(millis > fmillis) return false;
        else if(millis >= ci->lastevent)
        {
            ci->lastevent = millis;
            process(ci);
        }
        return true;
    }

    void clearevent(clientinfo *ci)
    {
        delete ci->events.remove(0);
    }

    void flushevents(clientinfo *ci, int millis)
    {
        while(ci->events.length())
        {
            gameevent *ev = ci->events[0];
            if(ev->flush(ci, millis)) clearevent(ci);
            else break;
        }
    }

    void processevents()
    {
        loopv(clients)
        {
            clientinfo *ci = clients[i];
//            if(curtime>0 && ci->state.quadmillis) ci->state.quadmillis = max(ci->state.quadmillis-curtime, 0);
            flushevents(ci, gamemillis);
        }
    }

    void cleartimedevents(clientinfo *ci)
    {
        int keep = 0;
        loopv(ci->events)
        {
            if(ci->events[i]->keepable())
            {
                if(keep < i)
                {
                    for(int j = keep; j < i; j++) delete ci->events[j];
                    ci->events.remove(keep, i - keep);
                    i = keep;
                }
                keep = i+1;
                continue;
            }
        }
        while(ci->events.length() > keep) delete ci->events.pop();
        ci->timesync = false;
    }

    bool ispaused() { return gamepaused; }

    void serverupdate()
    {
        if(!gamepaused) gamemillis += curtime;

        if(m_demo) readdemo();
        else if(!gamepaused && minremain>0)
        {
            processevents();
            if(curtime)
            {
                loopv(sents) if(sents[i].spawntime) // spawn entities when timer reached
                {
                    int oldtime = sents[i].spawntime;
                    sents[i].spawntime -= curtime;
                    if(sents[i].spawntime<=0)
                    {
                        sents[i].spawntime = 0;
                        sents[i].spawned = true;
                        sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
                    }
                    else if(sents[i].spawntime<=10000 && oldtime>10000 && (sents[i].type==I_QUAD || sents[i].type==I_BOOST))
                    {
                        sendf(-1, 1, "ri2", SV_ANNOUNCE, sents[i].type);
                    }
                }
            }
            botmgr::checkbot();
            if(smode) smode->update();
        }

        while(bannedips.length() && bannedips[0].time-totalmillis>4*60*60000) bannedips.remove(0);
        loopv(connects) if(totalmillis-connects[i]->connectmillis>15000) disconnect_client(connects[i]->clientnum, DISC_TIMEOUT);

        if(masterupdate)
        {
            clientinfo *m = currentmaster>=0 ? getinfo(currentmaster) : NULL;
            sendf(-1, 1, "ri3", SV_CURRENTMASTER, currentmaster, m ? m->privilege : 0);
            masterupdate = false;
        }

        if(!gamepaused && m_timed && smapname[0] && gamemillis-curtime>0 && gamemillis/60000!=(gamemillis-curtime)/60000) checkintermission();
        if(interm && gamemillis>interm)
        {
            if(demorecord) enddemorecord();
            interm = 0;
            checkvotes(true);
        }
    }

    struct crcinfo
    {
        int crc, matches;

        crcinfo() {}
        crcinfo(int crc, int matches) : crc(crc), matches(matches) {}

        static bool compare(const crcinfo &x, const crcinfo &y) { return x.matches > y.matches; }
    };

    void checkmaps(int req = -1)
    {
        if(m_edit || !smapname[0]) return;
        vector<crcinfo> crcs;
        int total = 0, unsent = 0, invalid = 0;
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(ci->state.state==CS_SPECTATOR || ci->state.clienttype != CLIENT_PLAYER) continue;
            total++;
            if(!ci->clientmap[0])
            {
                if(ci->mapcrc < 0) invalid++;
                else if(!ci->mapcrc) unsent++;
            }
            else
            {
                crcinfo *match = NULL;
                loopvj(crcs) if(crcs[j].crc == ci->mapcrc) { match = &crcs[j]; break; }
                if(!match) crcs.add(crcinfo(ci->mapcrc, 1));
                else match->matches++;
            }
        }
        if(total - unsent < min(total, 4)) return;
        crcs.sort(crcinfo::compare);
        string msg;
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(ci->state.state==CS_SPECTATOR || ci->state.clienttype != CLIENT_PLAYER || ci->clientmap[0] || ci->mapcrc >= 0 || (req < 0 && ci->warned)) continue;
            formatstring(msg)("%s has modified map \"%s\"", colorname(ci), smapname);
            sendf(req, 1, "ris", SV_SERVMSG, msg);
            if(req < 0) ci->warned = true;
        }
        if(crcs.empty() || crcs.length() < 2) return;
        loopv(crcs)
        {
            crcinfo &info = crcs[i];
            if(i || info.matches <= crcs[i+1].matches) loopvj(clients)
            {
                clientinfo *ci = clients[j];
                if(ci->state.state==CS_SPECTATOR || ci->state.clienttype != CLIENT_PLAYER || !ci->clientmap[0] || ci->mapcrc != info.crc || (req < 0 && ci->warned)) continue;
                formatstring(msg)("%s has modified map \"%s\"", colorname(ci), smapname);
                sendf(req, 1, "ris", SV_SERVMSG, msg);
                if(req < 0) ci->warned = true;
            }
        }
    }

    void sendservinfo(clientinfo *ci)
    {
        sendf(ci->clientnum, 1, "ri5", SV_SERVINFO, ci->clientnum, PROTOCOL_VERSION, ci->sessionid, serverpass[0] ? 1 : 0);
    }

    void noclients()
    {
        bannedips.setsize(0);
        botmgr::clearbot();
    }

    void localconnect(int n)
    {
        clientinfo *ci = getinfo(n);
        ci->clientnum = ci->ownernum = n;
        ci->connectmillis = totalmillis;
        ci->sessionid = (rnd(0x1000000)*((totalmillis%10000)+1))&0xFFFFFF;
        ci->local = true;

        connects.add(ci);
        sendservinfo(ci);
    }

    void localdisconnect(int n)
    {
        if(m_demo) enddemoplayback();
        clientdisconnect(n);
    }

    int clientconnect(int n, uint ip)
    {
        clientinfo *ci = getinfo(n);
        ci->clientnum = ci->ownernum = n;
        ci->connectmillis = totalmillis;
        ci->sessionid = (rnd(0x1000000)*((totalmillis%10000)+1))&0xFFFFFF;

        connects.add(ci);
        if(!m_mp(gamemode)) return DISC_PRIVATE;
        sendservinfo(ci);
        return DISC_NONE;
    }

    void clientdisconnect(int n)
    {
        clientinfo *ci = getinfo(n);
        if(ci->connected)
        {
            if(ci->privilege) setmaster(ci, false);
            if(smode) smode->leavegame(ci, true);
            ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
//            savescore(ci);
            sendf(-1, 1, "ri2", SV_CDIS, n);

//            defformatstring(dbg)("DEBUG: server::clientdisconnect send SV_CDIS cl: %d", ci->clientnum);
//            sendf(-1, 1, "ris", SV_SERVMSG, dbg);

            clients.removeobj(ci);
            botmgr::removebot(ci);
            if(!numclients(-1, false, true)) noclients(); // bans clear when server empties
        }
        else connects.removeobj(ci);
    }

    int reserveclients() { return 3; }

    int allowconnect(clientinfo *ci, const char *pwd)
    {
        if(ci->local) return DISC_NONE;
        if(!m_mp(gamemode)) return DISC_PRIVATE;
        if(serverpass[0])
        {
            if(!checkpassword(ci, serverpass, pwd)) return DISC_PRIVATE;
            return DISC_NONE;
        }
        if(adminpass[0] && checkpassword(ci, adminpass, pwd)) return DISC_NONE;
        if(numclients(-1, false, true)>=maxclients) return DISC_MAXCLIENTS;
        uint ip = getclientip(ci->clientnum);
        loopv(bannedips) if(bannedips[i].ip==ip) return DISC_IPBAN;
        if(mastermode>=MM_PRIVATE && allowedips.find(ip)<0) return DISC_PRIVATE;
        return DISC_NONE;
    }

    bool allowbroadcast(int n)
    {
        clientinfo *ci = getinfo(n);
        return ci && ci->connected;
    }

    clientinfo *findauth(uint id)
    {
        loopv(clients) if(clients[i]->authreq == id) return clients[i];
        return NULL;
    }

    void authfailed(uint id)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = 0;
    }

    void authsucceeded(uint id)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = 0;
        setmaster(ci, true, "", ci->authname);
    }

    void authchallenged(uint id, const char *val)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        sendf(ci->clientnum, 1, "risis", SV_AUTHCHAL, "", id, val);
    }

    uint nextauthreq = 0;

    void tryauth(clientinfo *ci, const char *user)
    {
        if(!nextauthreq) nextauthreq = 1;
        ci->authreq = nextauthreq++;
        filtertext(ci->authname, user, false, 100);
        if(!requestmasterf("reqauth %u %s\n", ci->authreq, ci->authname))
        {
            ci->authreq = 0;
            sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "not connected to authentication server");
        }
    }

    void answerchallenge(clientinfo *ci, uint id, char *val)
    {
        if(ci->authreq != id) return;
        for(char *s = val; *s; s++)
        {
            if(!isxdigit(*s)) { *s = '\0'; break; }
        }
        if(!requestmasterf("confauth %u %s\n", id, val))
        {
            ci->authreq = 0;
            sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "not connected to authentication server");
        }
    }

    void processmasterinput(const char *cmd, int cmdlen, const char *args)
    {
        uint id;
        string val;
        if(sscanf(cmd, "failauth %u", &id) == 1)
            authfailed(id);
        else if(sscanf(cmd, "succauth %u", &id) == 1)
            authsucceeded(id);
        else if(sscanf(cmd, "chalauth %u %s", &id, val) == 2)
            authchallenged(id, val);
    }

    void receivefile(int sender, uchar *data, int len)
    {
	//we upload map data and cfg's, monitored by "mapdata upload"
		ucharbuf p(data, len);
        int type = getint(p);
		char name[MAXTRANS];
		getstring(name, p);
		data += p.length();
		len -= p.length();

		if(!len) return;

		switch(type)
        {
            case SV_UPLOADMAP:
            {
				clientinfo *ci = getinfo(sender);
				if(ci->state.state==CS_SPECTATOR && !ci->privilege && !ci->local) return;

				if(!upload.status || upload.finished()) upload.init(ci->clientnum, name);
				else if(!upload.check(ci->clientnum, name)) return;

				if(!m_edit || len > 1024*1024) { conoutf("map is too large"); return; }

				upload.map = opentempfile("mapdata", "w+b");
				if(!upload.map) {
					sendf(sender, 1, "ris", SV_SERVMSG, "failed to open temporary file for map");
					upload.reset();
					return;
				}
				upload.map->write(data, len);

				upload.status |= MD_GOTMAP;

				if(upload.finished()) {
					defformatstring(msg)("[%s uploaded map to server, \"/getmap\" to receive it]", colorname(ci));
#ifdef STANDALONE
						conoutf("received map from %s(%i)", ci->name, ci->clientnum);
#endif
					sendservmsg(msg);
				}

				break;
			}
			case SV_UPLOADCFG:
			{
				clientinfo *ci = getinfo(sender);
				if(ci->state.state==CS_SPECTATOR && !ci->privilege && !ci->local) return;

				if(!upload.status || upload.finished()) upload.init(ci->clientnum, name);
				else if(!upload.check(ci->clientnum, name)) return;

				upload.cfg = opentempfile("cfgdata", "w+b");
				if(!upload.cfg) {
					sendf(sender, 1, "ris", SV_SERVMSG, "failed to open temporary file for cfg");
					upload.reset();
					return;
				}
				upload.cfg->write(data, len);

				upload.status |= MD_GOTCFG;

				if(upload.finished()) {
					defformatstring(msg)("[%s uploaded map to server, \"/getmap\" to receive it]", colorname(ci));
#ifdef STANDALONE
						conoutf("received map from %s(%i)", ci->name, ci->clientnum);
#endif
					sendservmsg(msg);
				}
				break;
			}
		}
    }

    void parsepacket(int sender, int chan, packetbuf &p)     // has to parse exactly each byte of the packet
    {
        if(sender<0) return;
        char text[MAXTRANS];
        int type;
        clientinfo *ci = sender>=0 ? getinfo(sender) : NULL, *cq = ci, *cm = ci;
        if(ci && !ci->connected)
        {
            if(chan==0) return;
            else if(chan!=1 || getint(p)!=SV_CONNECT) { disconnect_client(sender, DISC_TAGT); return; }
            else
            {
                getstring(text, p);
                filtertext(text, text, false, MAXNAMELEN);
                if(!text[0]) copystring(text, "unnamed");
                copystring(ci->name, text, MAXNAMELEN+1);

                getstring(text, p);
                int disc = allowconnect(ci, text);
                if(disc)
                {
                    disconnect_client(sender, disc);
                    return;
                }

                ci->playermodel = getint(p);

                if(m_demo) enddemoplayback();

                connects.removeobj(ci);
                clients.add(ci);

                ci->connected = true;
                if(mastermode>=MM_LOCKED) ci->state.state = CS_SPECTATOR;
                if(currentmaster>=0) masterupdate = true;
                ci->state.lasttimeplayed = lastmillis;

                sendwelcome(ci);
                sendresume(ci);
                sendinitclient(ci);

                botmgr::addclient(ci);

                if(m_demo) setupdemoplayback();

                if(servermotd[0]) sendf(sender, 1, "ris", SV_SERVMSG, servermotd);
            }
        }
        else if(chan==2)
        {
            receivefile(sender, p.buf, p.maxlen);
            return;
        }

        if(p.packet->flags&ENET_PACKET_FLAG_RELIABLE) reliablemessages = true;
        #define QUEUE_AI clientinfo *cm = cq;
        #define QUEUE_MSG { if(cm && (!cm->local || demorecord || hasnonlocalclients())) while(curmsg<p.length()) cm->messages.add(p.buf[curmsg++]); }
        #define QUEUE_BUF(size, body) { \
            if(cm && (!cm->local || demorecord || hasnonlocalclients())) \
            { \
                curmsg = p.length(); \
                ucharbuf buf = cm->messages.reserve(size); \
                { body; } \
                cm->messages.addbuf(buf); \
            } \
        }
        #define QUEUE_INT(n) QUEUE_BUF(5, putint(buf, n))
        #define QUEUE_UINT(n) QUEUE_BUF(4, putuint(buf, n))
        #define QUEUE_STR(text) QUEUE_BUF(2*strlen(text)+1, sendstring(text, buf))

        int curmsg;
        while((curmsg = p.length()) < p.maxlen) switch(type = checktype(getint(p), ci))
        {
            case SV_POS:
            {

                int pcn = getint(p);
                clientinfo *cp = getinfo(pcn);
                if(cp && pcn != sender && cp->ownernum != sender) cp = NULL;
                vec pos;
                loopi(3) pos[i] = getuint(p)/DMF;
                getuint(p);
                loopi(5) getint(p);
                int physstate = getuint(p);
                if(physstate&0x20) loopi(2) getint(p);
                if(physstate&0x10) getint(p);
                getuint(p);

                if(cp)
                {
                    if((!ci->local || demorecord || hasnonlocalclients()) && (cp->state.state==CS_ALIVE || cp->state.state==CS_EDITING))
                    {
                        cp->position.setsize(0);
                        while(curmsg<p.length()) cp->position.add(p.buf[curmsg++]);
                    }
                    cp->state.o = pos;
                    cp->gameclip = (physstate&0x80)!=0;
                }
                break;
            }

            case SV_FROMBOT:
            {
                int qcn = getint(p);
                if(qcn < 0) cq = ci;
                else
                {
                    cq = getinfo(qcn);
                    if(cq && qcn != sender && cq->ownernum != sender) cq = NULL;
                }
                break;
            }

            case SV_EDITMODE:
            {
                int val = getint(p);
                if(!ci->local && !m_edit) break;
                if(val ? ci->state.state!=CS_ALIVE && ci->state.state!=CS_DEAD : ci->state.state!=CS_EDITING) break;
                if(smode)
                {
                    if(val) smode->leavegame(ci);
                    else smode->entergame(ci);
                }
                if(val)
                {
                    ci->state.editstate = ci->state.state;
                    ci->state.state = CS_EDITING;
                    ci->events.setsize(0);
                }
                else ci->state.state = ci->state.editstate;
                QUEUE_MSG;
                break;
            }

            case SV_MAPCRC:
            {
                getstring(text, p);
                int crc = getint(p);
                if(!ci) break;
                if(strcmp(text, smapname))
                {
                    if(ci->clientmap[0])
                    {
                        ci->clientmap[0] = '\0';
                        ci->mapcrc = 0;
                    }
                    else if(ci->mapcrc > 0) ci->mapcrc = 0;
                    break;
                }
                copystring(ci->clientmap, text);
                ci->mapcrc = text[0] ? crc : 1;
                checkmaps();
		print("(%s(%i)) map CRC %s: %i", ci->name, ci->clientnum, ci->clientmap, ci->mapcrc);
                break;
            }

            case SV_CHECKMAPS:
                checkmaps(sender);
                break;

            case SV_TRYSPAWN:
//                if(!ci || !cq || cq->state.state!=CS_DEAD || cq->state.lastspawn>=0 || (smode && !smode->canspawn(cq))) break;
                if(!ci || !cq || cq->state.state!=CS_DEAD || (smode && !smode->canspawn(cq))) break;
                if(!ci->clientmap[0] && !ci->mapcrc)
                {
                    ci->mapcrc = -1;
                    checkmaps();
                }
//                defformatstring(dbg)("DEBUG: server::parsepacket SV_TRYSPAWN cl: %d on: %d, clienttype: %d, state: %d", cq->clientnum, cq->ownernum, cq->state.clienttype, cq->state.state);
//                sendf(-1, 1, "ris", SV_SERVMSG, dbg);

                cleartimedevents(cq);
                sendspawn(cq);
                break;

            case SV_SPAWN:
            {
//                defformatstring(dbg2)("1DEBUG: server::parsepacket SV_SPAWN");
//                sendf(-1, 1, "ris", SV_SERVMSG, dbg2);
                if(!cq || (cq->state.state!=CS_ALIVE && cq->state.state!=CS_DEAD) ) break;

                cq->state.state = CS_ALIVE;
                if(smode) smode->spawned(cq);
//
//                defformatstring(dbg)("2DEBUG: server::parsepacket SV_SPAWN cl: %d on: %d, state: %d, clienttype: %d", cq->clientnum, cq->ownernum, cq->state.state, cq->state.clienttype);
//                sendf(-1, 1, "ris", SV_SERVMSG, dbg);

                QUEUE_AI;
                QUEUE_BUF(100,
                {
                    putint(buf, SV_SPAWN);
                    sendstate(cq->state, buf);
                });
//                defformatstring(dbg1)("3DEBUG: server::parsepacket QUEUE SV_SPAWN cl: %d on: %d, clienttype: %d", cq->clientnum, cq->ownernum, cq->state.clienttype);
//                sendf(-1, 1, "ris", SV_SERVMSG, dbg1);
                break;
            }

            case SV_SHOOT:
            {
                shotevent *shot = new shotevent;
                shot->id = getint(p);
                shot->millis = cq ? cq->geteventmillis(gamemillis, shot->id) : 0;
                shot->gun = getint(p);
                loopk(3) shot->from[k] = getint(p)/DMF;
                loopk(3) shot->to[k] = getint(p)/DMF;
                int hits = getint(p);
                loopk(hits)
                {
                    if(p.overread()) break;
                    hitinfo &hit = shot->hits.add();
                    hit.target = getint(p);
                    hit.lifesequence = getint(p);
                    hit.rays = getint(p);
                    loopk(3) hit.dir[k] = getint(p)/DNF;
                }
                if(cq) cq->addevent(shot);
                else delete shot;
                break;
            }

            case SV_EXPLODE:
            {
                explodeevent *exp = new explodeevent;
                int cmillis = getint(p);
                exp->millis = cq ? cq->geteventmillis(gamemillis, cmillis) : 0;
                exp->gun = getint(p);
                exp->id = getint(p);
                int hits = getint(p);
                loopk(hits)
                {
                    if(p.overread()) break;
                    hitinfo &hit = exp->hits.add();
                    hit.target = getint(p);
                    hit.lifesequence = getint(p);
                    hit.dist = getint(p)/DMF;
                    loopk(3) hit.dir[k] = getint(p)/DNF;
                }
                if(cq) cq->addevent(exp);
                else delete exp;
                break;
            }

            case SV_ITEMPICKUP:
            {
                int n = getint(p);
                if(!cq) break;
                pickupevent *pickup = new pickupevent;
                pickup->ent = n;
                cq->addevent(pickup);
                break;
            }

	    case SV_TEXT:
	    {
		    int mtype = getint(p);
		    getstring(text, p);
		    filtertext(text, text);

		    if(mtype&CHAT_TEAM)
		    {

			    if(!ci || !cq || (ci->state.state==CS_SPECTATOR && !ci->local && !ci->privilege) ) break;
//			    if(!ci || !cq || (ci->state.state==CS_SPECTATOR && !ci->local && !ci->privilege) || !m_teammode || !cq->team[0]) break;
			    loopv(clients)
			    {
				    clientinfo *t = clients[i];
				    if(t==cq || t->state.state==CS_SPECTATOR || t->state.clienttype != CLIENT_PLAYER ) continue;
//				    if(t==cq || t->state.state==CS_SPECTATOR || t->state.clienttype != CLIENT_PLAYER || strcmp(cq->team, t->team)) continue;
				    sendf(t->clientnum, 1, "riiis", SV_TEXT, cq->clientnum, mtype, text);
			    }
			    if(mtype&CHAT_EMOTE)
			    {
				    print(" *%s(%i) %s", colorname(ci), ci->clientnum, text);
			    }
			    else
				    print(" <%s(%i)> %s", colorname(ci), ci->clientnum, text);
		    }
		    else //it would probably be TEXT... otherwise
		    {
			    loopv(clients)
			    {
				    	clientinfo *t = clients[i];
					if(t==ci || t->state.clienttype != CLIENT_PLAYER) continue;
					sendf(t->clientnum, 1, "riiis", SV_TEXT, ci->clientnum, mtype, text);
			    }
			    if(mtype&CHAT_EMOTE)
			    {
			    	print("*%s(%i) %s", colorname(ci), ci->clientnum, text);
			    }
			    else
				    print("<%s(%i)> %s", colorname(ci), ci->clientnum, text);
		    }
		    break;
	    }

            case SV_SWITCHNAME:
            {
                QUEUE_MSG;
		string tmp; copystring(tmp, ci->name);
                getstring(text, p);
                filtertext(ci->name, text, false, MAXNAMELEN);
                if(!ci->name[0]) copystring(ci->name, "unnamed");
                QUEUE_STR(ci->name);
		print("%s(%i) changed name to %s", tmp, ci->clientnum, ci->name);
                break;
            }

            case SV_SWITCHMODEL:
            {
                cq->playermodel = getint(p);
//                defformatstring(dbg)("DEBUG: server::parsepacket SV_SWITCHMODEL cl: %d, bot: %d state: %d", ci->clientnum, cq->clientnum, ci->state.state);
//        		sendf(-1, 1, "ris", SV_SERVMSG, dbg);
                //sendf(-1, 1, "riii", SV_SWITCHMODEL, cq->clientnum, cq->playermodel);
                //ci->playermodel = getint(p);
                QUEUE_MSG;
                break;
            }

//            case SV_SWITCHTEAM:
//            {
//                getstring(text, p);
//                filtertext(text, text, false, MAXTEAMLEN);
//                if(strcmp(ci->team, text))
//                {
//                    if(m_teammode && smode && !smode->canchangeteam(ci, ci->team, text))
//                        sendf(sender, 1, "riis", SV_SETTEAM, sender, ci->team);
//                    else
//                    {
//                        if(smode && ci->state.state==CS_ALIVE) smode->changeteam(ci, ci->team, text);
//                        copystring(ci->team, text);
//                        botmgr::changeteam(ci);
//                        sendf(-1, 1, "riis", SV_SETTEAM, sender, ci->team);
//                    }
//		    print("%s changed teams to %s", ci->name, ci->team);
//                }
//                break;
//            }

            case SV_MAPVOTE:
            case SV_MAPCHANGE:
            {
                getstring(text, p);
                filtertext(text, text, false);
                int reqmode = getint(p);
                if(type!=SV_MAPVOTE && !mapreload) break;
                vote(text, reqmode, sender);
                break;
            }

            case SV_ITEMLIST:
            {
                if((ci->state.state==CS_SPECTATOR && !ci->privilege && !ci->local) || !notgotitems || strcmp(ci->clientmap, smapname)) { while(getint(p)>=0 && !p.overread()) getint(p); break; }
                int n;
                while((n = getint(p))>=0 && n<MAXENTS && !p.overread())
                {
                    server_entity se = { NOTUSED, 0, false, -1 };
                    while(sents.length()<=n) sents.add(se);
                    sents[n].type = getint(p);
                    if(canspawnitem(sents[n].type))
                    {
                        if(m_mp(gamemode) && (sents[n].type==I_QUAD || sents[n].type==I_BOOST)) sents[n].spawntime = spawntime(sents[n].type);
                        else sents[n].spawned = true;
                    }
                }
                notgotitems = false;
                break;
            }

            case SV_EDITENT:
            {
            	int uid = getint(p);
            	int i = getint(p);
            	loopk(3) getint(p);
            	int type = getint(p);
            	loopk(8) getint(p);
            	if(!ci || ci->state.state==CS_SPECTATOR) break;
            	QUEUE_MSG;
//            	bool canspawn = canspawnitem(type);
//            	if(i<MAXENTS && (sents.inrange(i) || canspawnitem(type)))
//            	{
//            		server_entity se = { NOTUSED, 0, false, -1 };
//            		while(sents.length()<=i) sents.add(se);
//            		sents[i].type = type;
//            		if(canspawn ? !sents[i].spawned : sents[i].spawned)
//            		{
//            			sents[i].spawntime = canspawn ? 1 : 0;
//            			sents[i].spawned = false;
//            		}
//            	}
                if(uid < 0 && i<MAXENTS)
                {

                    server_entity se = { NOTUSED, 0, false, -1 };
                    while(sents.length()<=i) sents.add(se);
                    sents[i].type = type;
//            		if(canspawn ? !sents[i].spawned : sents[i].spawned)
//            		{
//            			sents[i].spawntime = canspawn ? 1 : 0;
//            			sents[i].spawned = false;
//            		}
            		lastmapuid++;
                    sents[i].uid = lastmapuid;
                    sendf(-1, 1, "riii", SV_MAPUID, i, sents[i].uid);
                }
            	break;
            }
            case SV_MAPENTITIES:
            {
                if((ci->state.state==CS_SPECTATOR && !ci->privilege && !ci->local) || strcmp(ci->clientmap, smapname)) { while(getint(p)>=-1 && !p.overread()) getint(p); break; }
                int n;
                while((n = getint(p))>=-1 && n<MAXENTS && !p.overread())
                {
                    int uid = getint(p);
                    //we already have set this
                    if(uid >= 0 && uid < lastmapuid) continue;

                    uid < 0 ? uid = ++lastmapuid : lastmapuid = uid;

                    server_entity se = { NOTUSED, 0, false, uid };
                    sents.add(se);
                    //NOTE: send map uid - is this necessary
                    if(uid >= 0) sendf(-1, 1, "riii", SV_MAPUID, n, uid);
                }
                break;

            }

            case SV_EDITVAR:
            {
                int type = getint(p);
                getstring(text, p);
		string tmp; copystring(tmp, text);

		int a = 0;
		float b = 0;

		switch(type)
		{
			case ID_VAR: a = getint(p); break;
			case ID_FVAR: b = getfloat(p); break;
			case ID_SVAR: getstring(text, p); break;
		}

		if(!ci || ci->state.state==CS_SPECTATOR)
			break;

		switch(type)
		{
			case ID_VAR:
			{
				print("%s(%i) set map variable \"%s\" to  %i", ci->name, ci->clientnum, tmp, a);
				break;
			}
			case ID_FVAR:
			{
				print("%s(%i) set map variable \"%s\" to  %f", ci->name, ci->clientnum, tmp, b);
				break;
			}
			case ID_SVAR:
				print("%s(%i) set map variable \"%s\" to  %s", ci->name, ci->clientnum, tmp, text);
				break;
		}
		QUEUE_MSG;
		break;
	}

            case SV_PING:
                sendf(sender, 1, "i2", SV_PONG, getint(p));
                break;

            case SV_CLIENTPING:
            {
                int ping = getint(p);
                if(ci)
                {
                    ci->ping = ping;
                    loopv(ci->bots) ci->bots[i]->ping = ping;
                }
                QUEUE_MSG;
                break;
            }

            case SV_MASTERMODE:
            {
                int mm = getint(p);
                if((ci->privilege || ci->local) && mm>=MM_OPEN && mm<=MM_PRIVATE)
                {
                    if((ci->privilege>=PRIV_ADMIN || ci->local) || (mastermask&(1<<mm)))
                    {
                        mastermode = mm;
                        allowedips.setsize(0);
                        if(mm>=MM_PRIVATE)
                        {
                            loopv(clients) allowedips.add(getclientip(clients[i]->clientnum));
                        }
                        defformatstring(s)("mastermode is now %s (%d)", mastermodename(mastermode), mastermode);
			print("%s(%i) set mastermode to %s (%d)", ci->name, ci->clientnum, mastermodename(mastermode), mastermode);
                        sendservmsg(s);
                    }
                    else
                    {
                        defformatstring(s)("mastermode %d is disabled on this server", mm);
                        sendf(sender, 1, "ris", SV_SERVMSG, s);
                    }
                }
                break;
            }

            case SV_CLEARBANS:
            {
                if(ci->privilege || ci->local)
                {
                    bannedips.setsize(0);
                    sendservmsg("cleared all bans");
		    print("%s(%i) cleared bans", ci->name, ci->clientnum);
                }
                break;
            }

            case SV_KICK:
            {
                int victim = getint(p);
                if((ci->privilege || ci->local) && ci->clientnum!=victim && getclientinfo(victim)) // no bots
                {
                    ban &b = bannedips.add();
                    b.time = totalmillis;
                    b.ip = getclientip(victim);
                    allowedips.removeobj(b.ip);
		    print("%s(%i) kicked %s(%i)", ci->name, ci->clientnum, ((clientinfo *)getclientinfo(victim))->name, victim);
                    disconnect_client(victim, DISC_KICK);
                }
                break;
            }

            case SV_SPECTATOR:
            {
                int spectator = getint(p), val = getint(p);
                if(!ci->privilege && !ci->local && (spectator!=sender || (ci->state.state==CS_SPECTATOR && mastermode>=MM_LOCKED))) break;
                clientinfo *spinfo = (clientinfo *)getclientinfo(spectator); // no bots
                if(!spinfo || (spinfo->state.state==CS_SPECTATOR ? val : !val)) break;

                if(spinfo->state.state!=CS_SPECTATOR && val)
                {
//                    if(spinfo->state.state==CS_ALIVE) suicide(spinfo);
                    if(smode) smode->leavegame(spinfo);
                    spinfo->state.state = CS_SPECTATOR;
                    spinfo->state.timeplayed += lastmillis - spinfo->state.lasttimeplayed;
                    if(!spinfo->local && !spinfo->privilege) botmgr::removebot(spinfo);


		    if(spectator!=sender)
		    {
			    print("%s(%i) turned %s(%i) into a spectator", ci->name, ci->clientnum, spinfo->name, spinfo->clientnum);
		    }
	            else
			    print("%s(%i) turned himself into a spectator", ci->name, ci->clientnum);
                }
                else if(spinfo->state.state==CS_SPECTATOR && !val)
                {
                    spinfo->state.state = CS_DEAD;
                    spinfo->state.respawn();
                    spinfo->state.lasttimeplayed = lastmillis;
                    botmgr::addclient(spinfo);
                    if(spinfo->clientmap[0] || spinfo->mapcrc) checkmaps();
		    if(spectator!=sender)
		    {
			    print("%s(%i) kicked %s(%i) out of spectator mode", ci->name, ci->clientnum, spinfo->name, spinfo->clientnum);
		    }
		    else print("%s(%i) kicked himself out of spectator mode", ci->name, ci->clientnum);
                }
                sendf(-1, 1, "ri3", SV_SPECTATOR, spectator, val);
                break;
            }

//            case SV_SETTEAM:
//            {
//                int who = getint(p);
//                getstring(text, p);
//                filtertext(text, text, false, MAXTEAMLEN);
//                if(!ci->privilege && !ci->local) break;
//                clientinfo *wi = getinfo(who);
//                if(!wi || !strcmp(wi->team, text)) break;
//                if(!smode || smode->canchangeteam(wi, wi->team, text))
//                {
//                    if(smode && wi->state.state==CS_ALIVE)
//                        smode->changeteam(wi, wi->team, text);
//                    copystring(wi->team, text, MAXTEAMLEN+1);
//		    if(who!=sender)
//		    {
//			    print("%s(%i) forced %s(%i) onto team %s", ci->name, ci->clientnum, wi->name, wi->clientnum, wi->team);
//		    }
//		    else
//			    print("%s(%i) changed to team %s", ci->name, ci->clientnum, wi->team);
//                }
//                botmgr::changeteam(wi);
//                sendf(-1, 1, "riis", SV_SETTEAM, who, wi->team);
//                break;
//            }

            case SV_FORCEINTERMISSION:
                if(ci->local && !hasnonlocalclients()) startintermission();
                break;

            case SV_RECORDDEMO:
            {
                int val = getint(p);
                if(ci->privilege<PRIV_ADMIN && !ci->local) break;
                demonextmatch = val!=0;
                defformatstring(msg)("demo recording is %s for next match", demonextmatch ? "enabled" : "disabled");
                sendservmsg(msg);
                break;
            }

            case SV_STOPDEMO:
            {
                if(ci->privilege<PRIV_ADMIN && !ci->local) break;
                stopdemo();
                break;
            }

            case SV_CLEARDEMOS:
            {
                int demo = getint(p);
                if(ci->privilege<PRIV_ADMIN && !ci->local) break;
                cleardemos(demo);
                break;
            }

            case SV_LISTDEMOS:
                if(!ci->privilege && !ci->local && ci->state.state==CS_SPECTATOR) break;
                listdemos(sender);
                break;

            case SV_GETDEMO:
            {
                int n = getint(p);
                if(!ci->privilege  && !ci->local && ci->state.state==CS_SPECTATOR) break;
                senddemo(sender, n);
                break;
            }

            case SV_GETMAP:
                if(upload.finished())
                {
                    sendf(sender, 1, "ris", SV_SERVMSG, "server sending map...");
					defformatstring(id)("%d", lastmillis);
                    sendfile(sender, 2, upload.cfg, "ris", SV_SENDCFG, id);
					sendfile(sender, 2, upload.map, "ris", SV_SENDMAP, id);
                }
                else sendf(sender, 1, "ris", SV_SERVMSG, "no map to send");
                break;

            case SV_NEWMAP:
            {
                int size = getint(p);
                if(!ci->privilege && !ci->local && ci->state.state==CS_SPECTATOR) break;
                if(size>=0)
                {
                    smapname[0] = '\0';
                    resetitems();
                    notgotitems = false;
                    if(smode) smode->reset(true);
                }
                QUEUE_MSG;
                break;
            }

            case SV_SETMASTER:
            {
                int val = getint(p);
                getstring(text, p);
                setmaster(ci, val!=0, text);
                // don't broadcast the master password
                break;
            }

            case SV_BOTLIMIT:
            {
                int limit = getint(p);
                if(ci) botmgr::setbotlimit(ci, limit);
                break;
            }

            case SV_BOTBALANCE:
            {
                int balance = getint(p);
                if(ci) botmgr::setbotbalance(ci, balance!=0);
                break;
            }

            case SV_AUTHTRY:
            {
                string desc, name;
                getstring(desc, p, sizeof(desc)); // unused for now
                getstring(name, p, sizeof(name));
                if(!desc[0]) tryauth(ci, name);
                break;
            }

            case SV_AUTHANS:
            {
                string desc, ans;
                getstring(desc, p, sizeof(desc)); // unused for now
                uint id = (uint)getint(p);
                getstring(ans, p, sizeof(ans));
                if(!desc[0]) answerchallenge(ci, id, ans);
                break;
            }

            case SV_PAUSEGAME:
            {
                int val = getint(p);
                if(ci->privilege<PRIV_ADMIN && !ci->local) break;
                pausegame(val > 0);
                break;
            }

			case SV_TEXTUREREQUEST:
			{
				getstring(text, p);
				int slot = getint(p);
				if(!serverupload) return;
				stream* file = openrawfile(text, "rb");
				if(file)
				{
					defformatstring(texmsg)("server - sending texture: %s", text);
					sendf(sender, 1, "ris", SV_SERVMSG, texmsg);
					sendfile(sender, 2, file, "risi", SV_SENDTEXTURE, text, slot);
				}
				else
				{
					defformatstring(err)("server - texture: %s not found", text);
					sendf(sender, 1, "ris", SV_SERVMSG, err);
				}
				break;
			}

            //__offtools__ attachments: attach stuff
			case SV_ATTACH:
			{
				int part = getint(p);
				getstring(text, p);
				int rule = getint(p);

                //defformatstring(dbg)("DEBUG: server::parsepacket SV_ATTACH cl: %d, bot: %d, %d %s, %d", ci->clientnum, cq->clientnum, part, text, rule);
        		//sendf(-1, 1, "ris", SV_SERVMSG, dbg);
                //sendf(-1, 1, "riii", SV_SWITCHMODEL, cq->clientnum, cq->playermodel);
                //ci->playermodel = getint(p);
                QUEUE_MSG;


//				QUEUE_MSG;
//				int part = getint(p);
//				getstring(text, p);
//				int animrule = getint(p);
//				QUEUE_INT(part);
//				QUEUE_STR(text);
//				QUEUE_INT(animrule);
				break;
			}

            //__offtools__ attachments: detach stuff
			case SV_DETACH:
			{
				QUEUE_MSG;
				int part = getint(p);
				QUEUE_INT(part);
				break;
			}

			case SV_REQUESTBOT:
            {
                botmgr::reqadd(ci);
                break;
            }

			case SV_DELETEBOT:
            {
                //botmgr::reqdel(ci);
                clientinfo* bot = getinfo(getint(p));
//                defformatstring(dbg)("DEBUG: server::parsepacket SV_DELETEBOT cl: %d, bot cn: %d", ci->clientnum, bot->clientnum);
                botmgr::deletebot(bot);
//                sendf(-1, 1, "ris", SV_SERVMSG, dbg);
                break;
            }

//            #define PARSEMESSAGES 1
//            #include "capture.h"
//            #include "ctf.h"
//            #undef PARSEMESSAGES

            default:
            {
                int size = server::msgsizelookup(type);
                if(size==-1) { disconnect_client(sender, DISC_TAGT); return; }
                if(size>0) loopi(size-1) getint(p);
                if(ci && cq && (ci != cq || ci->state.state!=CS_SPECTATOR)) { QUEUE_AI; QUEUE_MSG; }
                break;
            }
        }
    }

    int laninfoport() { return MOVIECUBE_LANINFO_PORT; }
    int serverinfoport(int servport) { return servport < 0 ? MOVIECUBE_SERVINFO_PORT : servport+1; }
    int serverport(int infoport) { return infoport < 0 ? MOVIECUBE_SERVER_PORT : infoport-1; }
    const char *defaultmaster() { return "master.sandboxgamemaker.com"; }
    int masterport() { return MOVIECUBE_MASTER_PORT; }

    #include "extinfo.h"

    void serverinforeply(ucharbuf &req, ucharbuf &p)
    {
        if(!getint(req))
        {
            extserverinforeply(req, p);
            return;
        }

        putint(p, numclients(-1, false, true));
        putint(p, 5);                   // number of attrs following
        putint(p, PROTOCOL_VERSION);    // a // generic attributes, passed back below
        putint(p, gamemode);            // b
        putint(p, minremain);           // c
        putint(p, maxclients);
        putint(p, serverpass[0] ? MM_PASSWORD : (!m_mp(gamemode) ? MM_PRIVATE : (mastermode || mastermask&MM_AUTOAPPROVE ? mastermode : MM_AUTH)));
        sendstring(smapname, p);
        sendstring(serverdesc, p);
        sendserverinforeply(p);
    }

    bool servercompatible(char *name, char *sdec, char *map, int ping, const vector<int> &attr, int np)
    {
        return attr.length() && attr[0]==PROTOCOL_VERSION;
    }

	int numchannels() { return 3; }


    #include "botmgr.h"
}

