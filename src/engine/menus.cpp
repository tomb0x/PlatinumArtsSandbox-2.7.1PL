// menus.cpp: ingame menu system (also used for scores and serverlist)

#ifndef NEWGUI

#include "engine.h"

HVARP(guititlecolour, 0, 0xDDFF00, 0xFFFFFF);
HVARP(guibuttoncolour, 0, 0xFFFF33, 0xFFFFFF);
HVARP(guitextcolour, 0, 0x00FF64, 0xFFFFFF);
HVARP(guislidercolour, 0, 0x00E4FF, 0xFFFFFF);
HVARP(guicheckboxcolour, 0, 0xFFFF33, 0xFFFFFF);


static vec menupos;
static int menustart = 0;
static int menutab = 1;
static g3d_gui *cgui = NULL;

extern int guirollovercolour;

void resetcolours (){
	guititlecolour = 0xDDFF00;
	guibuttoncolour = 0xFFFF33;
	guitextcolour = 0x00FF64;
	guislidercolour = 0x00E4FF;
	guicheckboxcolour = 0xFFFF33;
	guirollovercolour = 0x00FFC0;
}

COMMANDN(guiresetcolours, resetcolours, "");

struct menu : g3d_callback
{
    char *name, *header;
    uint *contents, *onclear;

    menu() : name(NULL), header(NULL), contents(NULL), onclear(NULL) {}

    void gui(g3d_gui &g, bool firstpass)
    {
        cgui = &g;
        cgui->start(menustart, 0.03f, &menutab);
        cgui->tab(header ? header : name, guititlecolour);
        execute(contents);
        cgui->end();
        cgui = NULL;
    }

    virtual void clear()
    {
        DELETEA(onclear);
    }
};

struct delayedupdate
{
    enum
    {
        INT,
        FLOAT,
        STRING,
        ACTION
    } type;
    ident *id;
    union
    {
        int i;
        float f;
        char *s;
    } val;
    delayedupdate() : type(ACTION), id(NULL) { val.s = NULL; }
    ~delayedupdate() { if(type == STRING || type == ACTION) DELETEA(val.s); }

    void schedule(const char *s) { type = ACTION; val.s = newstring(s); }
    void schedule(ident *var, int i) { type = INT; id = var; val.i = i; }
    void schedule(ident *var, float f) { type = FLOAT; id = var; val.f = f; }
    void schedule(ident *var, char *s) { type = STRING; id = var; val.s = newstring(s); }

    int getint() const
    {
        switch(type)
        {
            case INT: return val.i;
            case FLOAT: return int(val.f);
            case STRING: return int(strtol(val.s, NULL, 0));
            default: return 0;
        }
    }

    float getfloat() const
    {
        switch(type)
        {
            case INT: return float(val.i);
            case FLOAT: return val.f;
            case STRING: return float(parsefloat(val.s));
            default: return 0;
        }
    }

    const char *getstring() const
    {
        switch(type)
        {
	    case INT: return intstr(val.i);
            case FLOAT: return intstr(int(floor(val.f)));
            case STRING: return val.s;
            default: return "";
        }
    }

    void run()
    {
        if(type == ACTION) { if(val.s) execute(val.s); }
        else if(id) switch(id->type)
        {
            case ID_VAR: setvarchecked(id, getint()); break;
            case ID_FVAR: setfvarchecked(id, getfloat()); break;
            case ID_SVAR: setsvarchecked(id, getstring()); break;
            case ID_ALIAS: alias(id->name, getstring()); break;
        }
    }
};

static hashtable<const char *, menu> guis;
static vector<menu *> guistack;
static vector<delayedupdate> updatelater;
static bool shouldclearmenu = true, clearlater = false;

VARP(menudistance,  16, 40,  256);
VARP(menuautoclose, 32, 120, 4096);

vec menuinfrontofplayer()
{
    vec dir;
    vecfromyawpitch(camera1->yaw, 0, 1, 0, dir);
    dir.mul(menudistance).add(camera1->o);
    dir.z -= player->eyeheight-1;
    return dir;
}

void popgui()
{
    menu *m = guistack.pop();
    m->clear();
}

void removegui(menu *m)
{
    loopv(guistack) if(guistack[i]==m)
    {
        guistack.remove(i);
        m->clear();
        return;
    }
}

void pushgui(menu *m, int pos = -1)
{
    if(guistack.empty())
    {
        menupos = menuinfrontofplayer();
        g3d_resetcursor();
    }
    if(pos < 0) guistack.add(m);
    else guistack.insert(pos, m);
    if(pos < 0 || pos==guistack.length()-1)
    {
        menutab = 1;
        menustart = totalmillis;
    }
}

void restoregui(int pos)
{
    int clear = guistack.length()-pos-1;
    loopi(clear) popgui();
    menutab = 1;
    menustart = totalmillis;
}

void showgui(const char *name)
{
    menu *m = guis.access(name);
    if(!m) return;
    int pos = guistack.find(m);
    if(pos<0) pushgui(m);
    else restoregui(pos);
}

bool activegui(const char *name)
{
	if(!guistack.length()) return false;
	return !strcmp(guistack.last()->name, name);
}

int numgui()
{
	return guistack.length();
}

int cleargui(int n)
{
    int clear = guistack.length();
    if(mainmenu && !isconnected(true) && clear > 0 && guistack[0]->name && !strcmp(guistack[0]->name, "main"))
    {
        clear--;
        if(!clear) return 1;
    }
    if(n>0) clear = min(clear, n);
    loopi(clear) popgui();
    if(!guistack.empty()) restoregui(guistack.length()-1);
    return clear;
}

void guionclear(char *action)
{
    if(guistack.empty()) return;
    menu *m = guistack.last();
    DELETEA(m->onclear);
    if(action[0]) m->onclear = compilecode(action);
}

void guifont(const char *name, uint *contents)
{
    pushfont();
    setfont(name);
    execute(contents);
    popfont();
}

void guistayopen(uint *contents)
{
    bool oldclearmenu = shouldclearmenu;
    shouldclearmenu = false;
    execute(contents);
    shouldclearmenu = oldclearmenu;
}

void guinoautotab(uint *contents)
{
    if(!cgui) return;
    cgui->allowautotab(false);
    execute(contents);
    cgui->allowautotab(true);
}

//@DOC name and icon are optional
void guibutton(char *name, char *action, char *icon, int *colour)
{
    if(!cgui) return;
    bool hideicon = !strcmp(icon, "0");
    int ret = cgui->button(name, *colour ? *colour : guibuttoncolour, hideicon ? NULL : (icon[0] ? icon : (strstr(action, "showgui") ? "menu" : "action")));
    if(ret&G3D_UP)
    {
        updatelater.add().schedule(action[0] ? action : name);
        if(shouldclearmenu) clearlater = true;
    }
    else if(ret&G3D_ROLLOVER)
    {
        alias("guirollovername", name);
        alias("guirolloveraction", action);
    }
}

void guiimage(char *path, char *action, float *scale, int *overlaid, char *alt)
{
    if(!cgui) return;
    Texture *t = textureload(path, 0, true, false);
    if(t==notexture)
    {
        if(alt[0]) t = textureload(alt, 0, true, false);
        if(t==notexture) return;
    }
    int ret = cgui->image(t, *scale, *overlaid!=0);
    if(ret&G3D_UP)
    {
        if(*action)
        {
            updatelater.add().schedule(action);
            if(shouldclearmenu) clearlater = true;
        }
    }
    else if(ret&G3D_ROLLOVER)
    {
        alias("guirolloverimgpath", path);
        alias("guirolloverimgaction", action);
    }
}

void guicolor(int *color)
{
    if(cgui)
    {
        defformatstring(desc)("0x%06X", *color);
        cgui->text(desc, *color, NULL);
    }
}
void guitextbox(char *text, int *width, int *height, int *color)
{
    if(cgui && text[0]) cgui->textbox(text, *width ? *width : 12, *height ? *height : 1, *color ? *color : guitextcolour);
}

void guitext(char *name, char *icon, int *colour)
{
    bool hideicon = !strcmp(icon, "0");
    if(cgui) cgui->text(name, *colour ? *colour : (!hideicon && icon[0] ? guibuttoncolour : guitextcolour), hideicon ? NULL : (icon[0] ? icon : "info"));
}

void guititle(char *name, int *colour)
{
    if(cgui) cgui->title(name, *colour ? *colour : guititlecolour);
}

void guitab(char *name, int *colour)
{
    if(cgui) cgui->tab(name, *colour ? *colour : guititlecolour);
}

void guibar()
{
    if(cgui) cgui->separator();
}

void guistrut(float *strut, int *alt)
{
	if(cgui)
	{
		if(!*alt) cgui->pushlist();
		cgui->strut(*strut);
		if(!*alt) cgui->poplist();
	}
}

template<class T> static void updateval(char *var, T val, char *onchange)
{
    ident *id = newident(var);
    updatelater.add().schedule(id, val);
    if(onchange[0]) updatelater.add().schedule(onchange);
}

static int getval(char *var)
{
    ident *id = getident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return int(*id->storage.f);
        case ID_SVAR: return parseint(*id->storage.s);
        case ID_ALIAS: return id->getint();
        default: return 0;
    }
}

static float getfval(char *var)
{
    ident *id = getident(var);
    if(!id) return 0;
    switch(id->type)
    {
        case ID_VAR: return *id->storage.i;
        case ID_FVAR: return *id->storage.f;
        case ID_SVAR: return parsefloat(*id->storage.s);
        case ID_ALIAS: return id->getfloat();
        default: return 0;
    }
}

static const char *getsval(char *var)
{
    ident *id = getident(var);
    if(!id) return "";
    switch(id->type)
    {
        case ID_VAR: return intstr(*id->storage.i);
        case ID_FVAR: return floatstr(*id->storage.f);
        case ID_SVAR: return *id->storage.s;
        case ID_ALIAS: return id->getstr();
        default: return "";
    }
}

void guislider(char *var, int *min, int *max, char *onchange, int *colour, int *reverse)
{
	if(!cgui) return;
    int oldval = getval(var), val = oldval, vmin = *max ? *min : getvarmin(var), vmax = *max ? *max : getvarmax(var);
    cgui->slider(val, vmin, vmax, *colour ? *colour : guislidercolour, NULL, *reverse ? true : false);
    if(val != oldval) updateval(var, val, onchange);
}

void guilistslider(char *var, char *list, char *onchange, int *colour, int *reverse)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = getval(var), oldoffset = vals.length()-1, offset = oldoffset;
    loopv(vals) if(val <= vals[i]) { oldoffset = offset = i; break; }
    defformatstring(label)("%d", val);
    cgui->slider(offset, 0, vals.length()-1, *colour ? *colour : guislidercolour, label, *reverse ? true: false);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
}

void guinameslider(char *var, char *names, char *list, char *onchange, int *colour)
{
    if(!cgui) return;
    vector<int> vals;
    list += strspn(list, "\n\t ");
    while(*list)
    {
        vals.add(parseint(list));
        list += strcspn(list, "\n\t \0");
        list += strspn(list, "\n\t ");
    }
    if(vals.empty()) return;
    int val = getval(var), oldoffset = vals.length()-1, offset = oldoffset;
    loopv(vals) if(val <= vals[i]) { oldoffset = offset = i; break; }
    char *label = indexlist(names, offset);
    cgui->slider(offset, 0, vals.length()-1, *colour ? *colour : guititlecolour, label);
    if(offset != oldoffset) updateval(var, vals[offset], onchange);
    delete[] label;
}

void guicheckbox(char *name, char *var, float *on, float *off, char *onchange, int *colour)
{
    bool enabled = getfval(var)!=*off;
    if(cgui && cgui->button(name, *colour ? *colour : guicheckboxcolour, enabled ? "checkbox_on" : "checkbox_off")&G3D_UP)
    {
        updateval(var, enabled ? *off : (*on || *off ? *on : 1.0f), onchange);
    }
}

void guiradio(char *name, char *var, float *n, char *onchange, int *colour)
{
    bool enabled = getfval(var)==*n;
    if(cgui && cgui->button(name, *colour ? *colour : guicheckboxcolour, enabled ? "radio_on" : "radio_off")&G3D_UP)
    {
        if(!enabled) updateval(var, *n, onchange);
    }
}

void guibitfield(char *name, char *var, int *mask, char *onchange, int *colour)
{
    int val = getval(var);
    bool enabled = (val & *mask) != 0;
    if(cgui && cgui->button(name, *colour ? *colour : guibuttoncolour, enabled ? "checkbox_on" : "checkbox_off")&G3D_UP)
    {
        updateval(var, enabled ? val & ~*mask : val | *mask, onchange);
    }
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guifield(char *var, int *maxlength, char *onchange, int *colour)
{
    if(!cgui) return;
    const char *initval = getsval(var);
    char *result = cgui->field(var, *colour ? *colour : guibuttoncolour, *maxlength ? *maxlength : 12, 0, initval);
    if(result) updateval(var, result, onchange);
}

//-ve maxlength indicates a wrapped text field of any (approx 260 chars) length, |maxlength| is the field width
void guieditor(char *name, int *maxlength, int *height, int *mode, int *colour)
{
    if(!cgui) return;
    cgui->field(name, *colour ? *colour : guibuttoncolour, *maxlength ? *maxlength : 12, *height, NULL, *mode<=0 ? EDITORFOREVER : *mode);
    //returns a non-NULL pointer (the currentline) when the user commits, could then manipulate via text* commands
}

//-ve length indicates a wrapped text field of any (approx 260 chars) length, |length| is the field width
void guikeyfield(char *var, int *maxlength, char *onchange)
{
    if(!cgui) return;
    const char *initval = getsval(var);
    char *result = cgui->keyfield(var, guibuttoncolour, *maxlength ? *maxlength : -8, 0, initval);
    if(result) updateval(var, result, onchange);
}


void guilist(uint *contents)
{
    if(!cgui) return;
    cgui->pushlist();
    execute(contents);
    cgui->poplist();
}

void guialign(int *align, uint *contents)
{
    if(!cgui) return;
    cgui->pushlist(clamp(*align, -1, 1));
    execute(contents);
    cgui->poplist();
}

void newgui(char *name, char *contents, char *header)
{
    menu *m = guis.access(name);
    if(!m)
    {
        name = newstring(name);
        m = &guis[name];
        m->name = name;
    }
    else
    {
        DELETEA(m->header);
        DELETEA(m->contents);
    }
    m->header = header && header[0] ? newstring(header) : NULL;
    m->contents = compilecode(contents);
}

void guiservers()
{
    extern char *showservers(g3d_gui *cgui);
    if(cgui)
    {
        char *command = showservers(cgui);
        if(command)
        {
            updatelater.add().schedule(command);
            if(shouldclearmenu) clearlater = true;
        }
    }
}

COMMAND(newgui, "sss");
COMMAND(guibutton, "sssi");
COMMAND(guitext, "ssi");
COMMAND(guiservers, "s");
ICOMMAND(cleargui, "i", (int *n), intret(cleargui(*n)));
COMMAND(showgui, "s");
COMMAND(guionclear, "s");
COMMAND(guifont, "se");
COMMAND(guistayopen, "e");
COMMAND(guinoautotab, "e");

COMMAND(guilist, "e");
COMMAND(guialign, "ie");
COMMAND(guititle, "si");
COMMAND(guibar,"");
COMMAND(guistrut,"fi");
COMMAND(guiimage,"ssfis");
COMMAND(guislider,"siisii");
COMMAND(guilistslider, "sssii");
COMMAND(guinameslider, "ssssi");
COMMAND(guiradio,"ssfsi");
COMMAND(guibitfield, "ssisi");
COMMAND(guicheckbox, "ssffsi");
COMMAND(guitab, "si");
COMMAND(guifield, "sisi");
COMMAND(guikeyfield, "sis");
COMMAND(guieditor, "siiii");
COMMAND(guicolor, "i");
COMMAND(guitextbox, "siii");

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

static struct applymenu : menu
{
    void gui(g3d_gui &g, bool firstpass)
    {
        if(guistack.empty()) return;
        g.start(menustart, 0.03f);
        g.text("zmienione ustawienia:", guitextcolour, "info");
        loopv(needsapply) g.text(needsapply[i].desc, guitextcolour, "info");
        g.separator();
        g.text("akceptujesz zmiany teraz?", guitextcolour, "info");
        if(g.button("tak", guibuttoncolour, "action")&G3D_UP)
        {
            int changetypes = 0;
            loopv(needsapply) changetypes |= needsapply[i].type;
            if(changetypes&CHANGE_GFX) updatelater.add().schedule("resetgl");
            if(changetypes&CHANGE_SOUND) updatelater.add().schedule("resetsound");
            clearlater = true;
        }
        if(g.button("nie", guibuttoncolour, "action")&G3D_UP)
            clearlater = true;
        g.end();
    }

    void clear()
    {
        menu::clear();
        needsapply.shrink(0);
    }
} applymenu;

VARP(applydialog, 0, 1, 1);

static bool processingmenu = false;

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
    needsapply.add(change(type, desc));
    if(needsapply.length() && guistack.find(&applymenu) < 0)
        pushgui(&applymenu, processingmenu ? max(guistack.length()-1, 0) : -1);
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
    if(needsapply.empty()) removegui(&applymenu);
}

void menuprocess()
{
    processingmenu = true;
    int wasmain = mainmenu, level = guistack.length();
    loopv(updatelater) updatelater[i].run();
    updatelater.shrink(0);
    if(wasmain > mainmenu || clearlater)
    {
        if(wasmain > mainmenu || level==guistack.length())
        {
            loopvrev(guistack)
            {
                menu *m = guistack[i];
                if(m->onclear)
                {
                    uint *action = m->onclear;
                    m->onclear = NULL;
                    execute(action);
                    delete[] action;
                }
            }
            cleargui(level);
        }
        clearlater = false;
    }
    if(mainmenu && !isconnected(true) && guistack.empty()) showgui("main");
    processingmenu = false;
}

VAR(mainmenu, 1, 1, 0);

void clearmainmenu()
{
    if(mainmenu && (isconnected() || haslocalclients()))
    {
        mainmenu = 0;
        if(!processingmenu) cleargui();
    }
}

void g3d_mainmenu()
{
    if(!guistack.empty())
    {
        extern int usegui2d;
        if(!mainmenu && !usegui2d && camera1->o.dist(menupos) > menuautoclose) cleargui();
        else g3d_addgui(guistack.last(), menupos, GUI_2D | GUI_FOLLOW);
    }
}

#endif

