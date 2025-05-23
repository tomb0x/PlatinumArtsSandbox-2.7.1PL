#include "engine.h"

static hashtable<const char *, font> fonts;
static font *fontdef = NULL;
static int fontdeftex = 0;

font *curfont = NULL;
int curfonttex = 0;

void newfont(char *name, char *tex, int *defaultw, int *defaulth)
{
    font *f = fonts.access(name);
    if(!f)
    {
        name = newstring(name);
        f = &fonts[name];
        f->name = name;
    }

    f->texs.shrink(0);
    f->texs.add(textureload(tex));
    f->chars.shrink(0);
    f->charoffset = '!';
    f->defaultw = *defaultw;
    f->defaulth = *defaulth;

    fontdef = f;
    fontdeftex = 0;
}

void fontoffset(char *c)
{
    if(!fontdef) return;

    fontdef->charoffset = c[0];
}

void fonttex(char *s)
{
    if(!fontdef) return;

    Texture *t = textureload(s);
    loopv(fontdef->texs) if(fontdef->texs[i] == t) { fontdeftex = i; return; }
    fontdeftex = fontdef->texs.length();
    fontdef->texs.add(t);
}

void fontchar(int *x, int *y, int *w, int *h, int *offsetx, int *offsety, int *advance)
{
    if(!fontdef) return;

    font::charinfo &c = fontdef->chars.add();
    c.x = *x;
    c.y = *y;
    c.w = *w ? *w : fontdef->defaultw;
    c.h = *h ? *h : fontdef->defaulth;
    c.offsetx = *offsetx;
    c.offsety = *offsety;
    c.advance = *advance ? *advance : c.offsetx + c.w;
    c.tex = fontdeftex;
}

void fontskip(int *n)
{
    if(!fontdef) return;
    loopi(max(*n, 1))
    {
        font::charinfo &c = fontdef->chars.add();
        c.x = c.y = c.w = c.h = c.offsetx = c.offsety = c.advance = c.tex = 0;
    }
}

COMMANDN(font, newfont, "ssii");
COMMAND(fontoffset, "s");
COMMAND(fonttex, "s");
COMMAND(fontchar, "iiiiiii");
COMMAND(fontskip, "i");

bool setfont(const char *name)
{
    font *f = fonts.access(name);
    if(!f) return false;
    curfont = f;
    return true;
}
ICOMMAND(setfont, "s", (const char *name),
    if(!setfont(name))
        conoutf("font %s does not exist", name);
)

static vector<font *> fontstack;

void pushfont()
{
    fontstack.add(curfont);
}

bool popfont()
{
    if(fontstack.empty()) return false;
    curfont = fontstack.pop();
    return true;
}

void gettextres(int &w, int &h)
{
    if(w < MINRESW || h < MINRESH)
    {
        if(MINRESW > w*MINRESH/h)
        {
            h = h*MINRESW/w;
            w = MINRESW;
        }
        else
        {
            w = w*MINRESH/h;
            h = MINRESH;
        }
    }
}

#define PIXELTAB (4*curfont->defaultw)

int text_width(const char *str) { //@TODO deprecate in favour of text_bounds(..)
    int width, height;
    text_bounds(str, width, height);
    return width;
}
ICOMMAND(text_width, "s", (char *s), intret(text_width(s)))

void tabify(const char *str, int *numtabs)
{
    vector<char> tabbed;
    tabbed.put(str, strlen(str));
    int w = text_width(str), tw = max(*numtabs, 0)*PIXELTAB;
    while(w < tw)
    {
        tabbed.add('\t');
        w = ((w+PIXELTAB)/PIXELTAB)*PIXELTAB;
    }
    tabbed.add('\0');
    result(tabbed.getbuf());
}

COMMAND(tabify, "si");

void draw_textf(const char *fstr, int left, int top, ...)
{
    defvformatstring(str, top, fstr);
    draw_text(str, left, top);
}

static int draw_char(Texture *&tex, int c, int x, int y)
{
    font::charinfo &info = curfont->chars[c-curfont->charoffset];
    if(tex != curfont->texs[info.tex])
    {
        xtraverts += varray::end();
        tex = curfont->texs[info.tex];
        glBindTexture(GL_TEXTURE_2D, tex->id);
    }

    float tc_left    = info.x / float(tex->xs);
    float tc_top     = info.y / float(tex->ys);
    float tc_right   = (info.x + info.w) / float(tex->xs);
    float tc_bottom  = (info.y + info.h) / float(tex->ys);

    x += info.offsetx;
    y += info.offsety;

    varray::attrib<float>(x,          y         ); varray::attrib<float>(tc_left,  tc_top   );
    varray::attrib<float>(x + info.w, y         ); varray::attrib<float>(tc_right, tc_top   );
    varray::attrib<float>(x + info.w, y + info.h); varray::attrib<float>(tc_right, tc_bottom);
    varray::attrib<float>(x,          y + info.h); varray::attrib<float>(tc_left,  tc_bottom);

    return info.advance;
}

//stack[sp] is current color index
static void text_color(char c, char *stack, int size, int &sp, bvec colour, int a)
{
    if(c=='s') // save color
    {
        c = stack[sp];
        if(sp<size-1) stack[++sp] = c;
    }
    else
    {
        xtraverts += varray::end();
        if(c=='r') c = stack[(sp > 0) ? --sp : sp]; // restore color
        else stack[sp] = c;
        switch(c)
        {
            case '0': colour = bvec( 64, 255, 128); break; // green: player talk
            case '1': colour = bvec( 96, 160, 255); break; // blue: "echo" command
            case '2': colour = bvec(255, 192,  64); break; // yellow: gameplay messages
            case '3': colour = bvec(255,  64,  64); break; // red: important errors
            case '4': colour = bvec(128, 128, 128); break; // gray
            case '5': colour = bvec(192,  64, 192); break; // magenta
            case '6': colour = bvec(255, 128,   0); break; // orange
	    case '7': colour = bvec(255, 255, 255); break; // white
            // provided color: everything else

            case 'A': colour = bvec(255, 224, 192); break; //apricot
            case 'B': colour = bvec(192,  96,   0); break; //brown
            case 'C': colour = bvec(255, 224,  96); break; //corn
            case 'D': colour = bvec(64,  160, 192); break; //dodger blue
            case 'E': colour = bvec(96 , 224, 128); break; //emerald
            case 'F': colour = bvec(255,  64, 255); break; //fuchsia
            case 'G': colour = bvec(255, 224,  64); break; //gold
            case 'H': colour = bvec(224, 160, 255); break; //heliotrope
            case 'I': colour = bvec(128,  64, 192); break; //indigo
            case 'J': colour = bvec(  0, 192, 128); break; //jade
            case 'K': colour = bvec(192, 176, 144); break; //khaki
            case 'L': colour = bvec(255, 224,  32); break; //lemon
            case 'M': colour = bvec(160, 255, 160); break; //mint
            case 'N': colour = bvec(255, 224, 160); break; //navajo white
            case 'O': colour = bvec(160, 160,  64); break; //olive
            case 'P': colour = bvec(255, 192, 208); break; //pink
            //case 'Q': nothing
            case 'R': colour = bvec(255,  64, 128); break; //rose
            case 'S': colour = bvec(224, 224, 224); break; //silver
            case 'T': colour = bvec( 64, 224, 224); break; //turquoise
            case 'U': colour = bvec( 32,  32, 192); break; //ultramarine
            case 'V': colour = bvec(192,  96, 224); break; //violet
            case 'W': colour = bvec(255, 224, 176); break; //wheat
            //case 'X': nothing
            case 'Y': colour = bvec(255, 255,  96); break; //yellow
            case 'Z': colour = bvec(224, 192, 160); break; //zinnwaldite
        }
        glColor4ub(colour.x, colour.y, colour.z, a);
    }
}

#define TEXTSKELETON \
    int y = 0, x = 0;\
    int i;\
    for(i = 0; str[i]; i++)\
    {\
        TEXTINDEX(i)\
        int c = uchar(str[i]);\
        if(c=='\t')      { x = ((x+PIXELTAB)/PIXELTAB)*PIXELTAB; TEXTWHITE(i) }\
        else if(c==' ')  { x += curfont->defaultw; TEXTWHITE(i) }\
        else if(c=='\n') { TEXTLINE(i) x = 0; y += FONTH; }\
        else if(c=='\f') { if(str[i+1]) { i++; TEXTCOLOR(i) }}\
        else if(curfont->chars.inrange(c-curfont->charoffset))\
        {\
            int cw = curfont->chars[c-curfont->charoffset].advance;\
            if(cw <= 0) continue;\
            if(maxwidth != -1)\
            {\
                int j = i;\
                int w = cw;\
                for(; str[i+1]; i++)\
                {\
                    int c = uchar(str[i+1]);\
                    if(c=='\f') { if(str[i+2]) i++; continue; }\
                    if(i-j > 16) break;\
                    if(!curfont->chars.inrange(c-curfont->charoffset)) break;\
                    int cw = curfont->chars[c-curfont->charoffset].advance;\
                    if(cw <= 0 || w + cw >= maxwidth) break;\
                    w += cw;\
                }\
                if(x + w >= maxwidth && j!=0) { TEXTLINE(j-1) x = 0; y += FONTH; }\
                TEXTWORD\
            }\
            else\
            { TEXTCHAR(i) }\
        }\
    }

//all the chars are guaranteed to be either drawable or color commands
#define TEXTWORDSKELETON \
                for(; j <= i; j++)\
                {\
                    TEXTINDEX(j)\
                    int c = uchar(str[j]);\
                    if(c=='\f') { if(str[j+1]) { j++; TEXTCOLOR(j) }}\
                    else { int cw = curfont->chars[c-curfont->charoffset].advance; TEXTCHAR(j) }\
                }

int text_visible(const char *str, int hitx, int hity, int maxwidth)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx) if(y+FONTH > hity && x >= hitx) return idx;
    #define TEXTLINE(idx) if(y+FONTH > hity) return idx;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += cw; TEXTWHITE(idx)
    #define TEXTWORD TEXTWORDSKELETON
    TEXTSKELETON
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
    return i;
}

//inverse of text_visible
void text_pos(const char *str, int cursor, int &cx, int &cy, int maxwidth)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; break; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += cw;
    #define TEXTWORD TEXTWORDSKELETON if(i >= cursor) break;
    cx = INT_MIN;
    cy = 0;
    TEXTSKELETON
    if(cx == INT_MIN) { cx = x; cy = y; }
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void text_bounds(const char *str, int &width, int &height, int maxwidth)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx) if(x > width) width = x;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += cw;
    #define TEXTWORD x += w;
    width = 0;
    TEXTSKELETON
    height = y + FONTH;
    TEXTLINE(_)
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void draw_text(const char *str, int left, int top, int r, int g, int b, int a, int cursor, int maxwidth)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx) if(usecolor) text_color(str[idx], colorstack, sizeof(colorstack), colorpos, color, a);
    #define TEXTCHAR(idx) draw_char(tex, c, left+x, top+y); x += cw;
    #define TEXTWORD TEXTWORDSKELETON
    char colorstack[10];
    colorstack[0] = 'c'; //indicate user color
    bvec color(r, g, b);
    int colorpos = 0, cx = INT_MIN, cy = 0;
    bool usecolor = true;
    if(a < 0) { usecolor = false; a = -a; }
    Texture *tex = curfont->texs[0];
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glColor4ub(color.x, color.y, color.z, a);
    varray::enable();
    varray::defattrib(varray::ATTRIB_VERTEX, 2, GL_FLOAT);
    varray::defattrib(varray::ATTRIB_TEXCOORD0, 2, GL_FLOAT);
    varray::begin(GL_QUADS);
    TEXTSKELETON
    xtraverts += varray::end();
    if(cursor >= 0 && (totalmillis/250)&1)
    {
        glColor4ub(r, g, b, a);
        if(cx == INT_MIN) { cx = x; cy = y; }
        if(maxwidth != -1 && cx >= maxwidth) { cx = 0; cy += FONTH; }
        draw_char(tex, '_', left+cx, top+cy);
        xtraverts += varray::end();
    }
    varray::disable();
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void reloadfonts()
{
    enumerate(fonts, font, f,
        loopv(f.texs) if(!reloadtexture(*f.texs[i])) fatal("failed to reload font texture");
    );
}

