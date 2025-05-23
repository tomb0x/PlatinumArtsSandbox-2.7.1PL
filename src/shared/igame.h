// the interface the engine uses to run the gameplay module

namespace entities
{
    extern void editent(int i, bool local);
    extern const char *entnameinfo(entity &e);
    extern const char *entname(int i);
    extern const int numattrs(int type);
    extern int extraentinfosize();
    extern void writeent(entity &e, char *buf);
    extern void readent(entity &e, char *buf, int ver);
    extern float dropheight(entity &e);
    extern void fixentity(extentity &e);
    extern void entradius(extentity &e, bool &color);
    extern bool mayattach(extentity &e);
    extern bool attachent(extentity &e, extentity &a);
    extern bool printent(extentity &e, char *buf);
    extern extentity *newentity();
    extern void deleteentity(extentity *e);
    extern void clearents();
    extern vector<extentity *> &getents();
    extern bool dirent(extentity &e);
    extern bool radiusent(extentity &e);
    extern void renderhelpertext(extentity &e, int &colour, vec &pos, string &tmp);
    extern const char *entmodel(const entity &e);
    extern void animatemapmodel(const extentity &e, int &anim, int &basetime);
    extern int *getmodelattr(extentity &e);
    extern bool checkmodelusage(extentity &e, int i);
}

namespace game
{
    extern void parseoptions(vector<const char *> &args);

    extern void gamedisconnect(bool cleanup);
    extern void parsepacketclient(int chan, packetbuf &p);
    extern void connectattempt(const char *name, const char *password, const ENetAddress &address);
    extern void connectfail();
    extern void gameconnect(bool _remote);
    extern bool allowedittoggle();
    extern void edittoggled(bool on);
    extern void writeclientinfo(stream *f);
    extern void toserver(char *text);
    extern void changemap(const char *name);
    extern void forceedit(const char *name);
    extern bool ispaused();

    extern const char *gameident();
    extern const char *autoexec();
    extern const char *savedservers();
    extern void loadconfigs();

    extern void updateworld();
    extern void initclient();
    extern void physicstrigger(physent *d, bool local, int floorlevel, int waterlevel, int material = 0);
    extern void bounced(physent *d, const vec &surface);
    extern void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0);
    extern void vartrigger(ident *id);
    extern void dynentcollide(physent *d, physent *o, const vec &dir);
    extern const char *getclientmap();
    extern const char *getmapinfo();
    extern void resetgamestate();
    extern void suicide(physent *d);
    extern void newmap(int size);
    extern void startmap(const char *name);
    extern void preload();
    extern float abovegameplayhud(int w, int h);
    extern void gameplayhud(int w, int h);
    extern bool canjump();
    extern bool allowmove(physent *d);
    extern dynent *iterdynents(int i);
    extern int numdynents();
    extern void rendergame(bool mainpass);
    extern void renderavatar();
    extern void writegamedata(vector<char> &extras);
    extern void readgamedata(vector<char> &extras);
    extern int clipconsole(int w, int h);
    extern const char *defaultcrosshair(int index);
    extern int selectcrosshair(float &r, float &g, float &b);
    extern void lighteffects(dynent *d, vec &color, vec &dir);
    extern void setupcamera();
    extern bool detachcamera();
    extern bool collidecamera();
    extern void adddynlights();
    extern void writemapdata(stream *f);
    extern void particletrack(physent *owner, vec &o, vec &d);
    extern void dynlighttrack(physent *owner, vec &o, vec &hud);
    extern bool needminimap();
    #ifndef NEWGUI
    extern void g3d_gamemenus();
    extern bool serverinfostartcolumn(g3d_gui *g, int i);
    extern void serverinfoendcolumn(g3d_gui *g, int i);
    extern bool serverinfoentry(g3d_gui *g, int i, const char *name, int port, const char *desc, const char *map, int ping, const vector<int> &attr, int np);
    #endif
    extern bool allowdoublejump();
    extern bool showenthelpers();
    extern void setwindowcaption();

    //__offtools__ - disfunc fix this
    extern void texturefailed(char *file, int slot);
    extern void mmodelfailed(const char *name, int idx);
    extern void mapfailed(const char *name);

    //__offtools__
    extern void registeranimation(char *dir, char* anim, int num);

    //return false if you do not wish to override the associated code in game - return true otherwise
    extern bool mousemove(int &dx, int &dy, float &cursens);
    extern bool recomputecamera(physent *&camera1, physent &tempcamera, bool &detachedcamera, float &thirdpersondistance);
}

namespace server
{
    extern void *newclientinfo();
    extern void deleteclientinfo(void *ci);
    extern void serverinit();
    extern int reserveclients();
    extern int numchannels();
    extern void clientdisconnect(int n);
    extern int clientconnect(int n, uint ip);
    extern void localdisconnect(int n);
    extern void localconnect(int n);
    extern bool allowbroadcast(int n);
    extern void parsepacket(int sender, int chan, packetbuf &p);
    extern void sendservmsg(const char *s);
    extern void recordpacket(int chan, void *data, int len);
    extern bool sendpackets(bool force = false);
    extern void serverinforeply(ucharbuf &req, ucharbuf &p);
    extern void serverupdate();
    extern bool servercompatible(char *name, char *sdec, char *map, int ping, const vector<int> &attr, int np);
    extern int laninfoport();
    extern int serverinfoport(int servport = -1);
    extern int serverport(int infoport = -1);
    extern const char *defaultmaster();
    extern int masterport();
    extern void processmasterinput(const char *cmd, int cmdlen, const char *args);
    extern bool ispaused();
}
