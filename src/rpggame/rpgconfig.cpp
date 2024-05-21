#include "rpggame.h"

/*
This file is configures the following items

* scripts
* particle effects
* status effect groups
* item bases
* item uses
* recipes
* ammotypes
* character bases
* factions
* obstacles
* containers
* platforms
* triggers
* mapscripts

This file is also used to declare the following items (but disallows subsequent changes)

* Global Variables

*/

namespace game
{
	#define CHECK(x, c, vec, select) \
		x *loading ## x = NULL; \
		static x *check ## x () \
		{ \
			if(! loading ## x) \
				conoutf(CON_ERROR, "ERROR: " #c " not defined or being loaded"); \
			return loading ## x; \
		} \
		ICOMMAND(r_select_ ## c, "se", (const char *ref, uint *contents), \
			x *old = loading ## x; \
			loading ## x = NULL; \
			\
			reference *obj = rpgscript::searchstack(ref, false); \
			if(obj) \
			{ \
				loading ## x = select; \
				if(loading ## x && DEBUG_VCONF) \
					conoutf("DEBUG: successfully selected \"" #c "\" from reference %s", ref); \
			} \
			\
			if(!loading ##x) \
			{ \
				int idx = parseint(ref); \
				if(vec.inrange(idx)) \
				{ \
					if(DEBUG_VCONF) \
						conoutf("DEBUG: successfully selected \"" #c "\" using index %i", idx); \
					loading ## x = vec[idx]; \
				} \
			} \
			if(loading ## x) \
				execute(contents); \
			else \
				conoutf(CON_ERROR, "ERROR: unable to select reference %s as type " #c, ref); \
			loading ## x = old; \
		) \
		ICOMMAND(r_num_ ## c, "", (), \
			if(DEBUG_VCONF) \
				conoutf("DEBUG: r_num_" #c " requested, returning %i", vec.length()); \
			intret(vec.length()); \
		)

	CHECK(script, script, scripts, NULL)
	CHECK(effect, effect, effects, NULL)
	CHECK(statusgroup, status, statuses,
		obj->getveffect() ? statuses[obj->getveffect()->group] :
		obj->getaeffect() ? statuses[obj->getaeffect()->group] :
		NULL
	)
	CHECK(item_base, item, items,
		obj->getinv() ? items[obj->getinv()->base] :
		obj->getequip() ? items[obj->getequip()->base] :
		NULL
	)
	CHECK(recipe, recipe, recipes, NULL)
	CHECK(ammotype, ammo, ammotypes, NULL)
	CHECK(char_base, char, chars, obj->getchar())
	CHECK(faction, faction, factions,
		  obj->getchar() ? factions[obj->getchar()->faction] :
		  NULL
	)
	CHECK(obstacle_base, obstacle, obstacles, obj->getobstacle())
	CHECK(container_base, container, containers, obj->getcontainer())
	CHECK(platform_base, platform, platforms, obj->getplatform())
	CHECK(trigger_base, trigger, triggers, obj->gettrigger())
	CHECK(mapscript, mapscript, mapscripts, NULL)

	#undef CHECK

	use *loadinguse = NULL;
	static use *checkuse(int type)
	{
		if(!loadinguse)
			conoutf(CON_ERROR, "ERROR: use not defined or being loaded");
		else if(loadinguse->type < type)
			conoutf(CON_ERROR, "ERROR: trying to set an unavailable use property");
		else
			return loadinguse;
		return NULL;
	}
	ICOMMAND(r_select_item_use, "se", (const char *ref, uint *contents),
		use *old = loadinguse;
		loadinguse = NULL;
		reference *obj = rpgscript::searchstack(ref, false);

		if(obj && obj->getequip())
		{
			if(DEBUG_VCONF)
				conoutf("DEBUG: successfully selected \"use\" from reference %s", ref);
			loadinguse = items[obj->getequip()->base]->uses[obj->getequip()->use];
		}
		else if(loadingitem_base)
		{
			int idx = parseint(ref);
			if(loadingitem_base->uses.inrange(idx))
			{
				if(DEBUG_VCONF)
					conoutf("DEBUG: successfully selected \"use\" using index %i", idx);
				loadinguse = loadingitem_base->uses[idx];
			}
		}

		if(loadinguse) execute(contents);
		else conoutf(CON_ERROR, "ERROR: unable to select reference %s as type use", ref);
		loadinguse = old;
	)
	ICOMMAND(r_num_item_use, "", (),
		if(loadingitem_base)
			intret(loadingitem_base->uses.length());
		else intret(0);
	)

	/// examples
	/// NOTE: the following variable names are reserved: i, f, s, x, y, z
	/// #define START(n, f, a, b)  ICOMMAND(r_script ## n, f, a, b)
	/// #define INIT script *e = checkscript()       parent variable must always be named e, e->var is modified (see below for var)
	/// #define DEBUG "scripts[%i]"
	/// #define DEBUG_IND scripts.length() - 1

	#define INTN(name, var, min, max) \
		START(name, "i", (int *i), \
			INIT \
			if(!e) return; \
			e->var = clamp((int) max, (int) min, *i); \
			if(e->var != *i) \
				conoutf("WARNING: value provided for " DEBUG "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var != *i) \
				conoutf(DEBUG "->" #var " = %i (%.8X)", DEBUG_IND, e->var, e->var); \
		) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			if(DEBUG_VCONF) \
				conoutf("DEBUG: request for " #name " on %p, returning %i", e, e->var); \
			intret(e->var); \
		)
	#define INT(var, min, max) INTN(var, var, min, max)

	#define FLOATN(name, var, min, max) \
		START(name, "f", (float *f), \
			INIT \
			if(!e) return; \
			e->var = clamp((float) max, (float) min, *f); \
			if(e->var != *f) \
				conoutf("WARNING: value provided for " DEBUG "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var != *f) \
				conoutf(DEBUG "->" #var " = %g", DEBUG_IND, e->var); \
		) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			if(DEBUG_VCONF) \
				conoutf("DEBUG: request for " #name " on %p, returning %f", e, e->var); \
			floatret(e->var); \
		)
	#define FLOAT(var, min, max) FLOATN(var, var, min, max)

	#define STRINGN(name, var) \
		START(name, "C", (const char *s), \
			INIT \
			if(!e) return; \
			if(e->var) delete[] e->var; \
			e->var = newstring(s); \
			if(DEBUG_CONF) \
				conoutf(CON_DEBUG, DEBUG "->" #var " = %s", DEBUG_IND, e->var); \
		) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			if(DEBUG_VCONF) \
				conoutf("DEBUG: request for " #name " on %p, returning %s", e, e->var); \
			result(e->var ? e->var : ""); \
		)
	#define STRING(var) STRINGN(var, var)

	#define VECN(name, var, l1, l2, l3, h1, h2, h3) \
		START(name, "fff", (float *x, float *y, float *z), \
			INIT \
			if(!e) return; \
			e->var.x = clamp((float)h1, (float)l1, *x); \
			e->var.y = clamp((float)h2, (float)l2, *y); \
			e->var.z = clamp((float)h3, (float)l3, *z); \
			if(e->var.x != *x || e->var.y != *y || e->var.z != *z) \
				conoutf("WARNING: value provided for " DEBUG "->" #var " exceeded limits", DEBUG_IND); \
			if(DEBUG_CONF || e->var.x != *x || e->var.y != *y || e->var.z != *z) \
				conoutf(DEBUG "->" #var " = (%g, %g, %g)", DEBUG_IND, e->var.x, e->var.y, e->var.z); \
		) \
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			defformatstring(res)("%g %g %g", e->var.x, e->var.y, e->var.z); \
			if(DEBUG_VCONF) \
				conoutf("DEBUG: request for " #name " on %p, returning %s", e, res); \
			result(res); \
		)
	#define VEC(var, l1, l2, l3, h1, h2, h3) VECN(var, var, l1, l2, l3, h1, h2, h3)

	#define BOOLN(name, var) \
		START(name, "i", (int *i), \
			INIT \
			if(!e) return; \
			e->var = *i != 0; \
			if(DEBUG_CONF) \
				conoutf(CON_DEBUG, DEBUG "->" #var " = %i", DEBUG_IND, e->var); \
		)\
		START(name ## _get, "", (), \
			INIT \
			if(!e) return; \
			if(DEBUG_VCONF) \
				conoutf("DEBUG: request for " #name " on %p, returning %i", e, e->var); \
			intret(e->var); \
		)
	#define BOOL(var) BOOLN(var, var)

	#define STATREQ(var) \
		INTN(var ## _strength, var.attrs[STAT_STRENGTH], 1, 100) \
		INTN(var ## _endurance, var.attrs[STAT_ENDURANCE], 1, 100) \
		INTN(var ## _agility, var.attrs[STAT_AGILITY], 1, 100) \
		INTN(var ## _charisma, var.attrs[STAT_CHARISMA], 1, 100) \
		INTN(var ## _wisdom, var.attrs[STAT_WISDOM], 1, 100) \
		INTN(var ## _intelligence, var.attrs[STAT_INTELLIGENCE], 1, 100) \
		INTN(var ## _luck, var.attrs[STAT_LUCK], 1, 100) \
		\
		INTN(var ## _armour, var.skills[SKILL_ARMOUR], 0, 100) \
		INTN(var ## _diplomacy, var.skills[SKILL_DIPLOMACY], 0, 100) \
		INTN(var ## _magic, var.skills[SKILL_MAGIC], 0, 100) \
		INTN(var ## _marksman, var.skills[SKILL_MARKSMAN], 0, 100) \
		INTN(var ## _melee, var.skills[SKILL_MELEE], 0, 100) \
		INTN(var ## _stealth, var.skills[SKILL_STEALTH], 0, 100) \
		INTN(var ## _craft, var.skills[SKILL_CRAFT], 0, 100)

	#define STATS(var) \
		INTN(var ## _level, var.level, 1, 1000) \
		INTN(var ## _experience, var.experience, 0, 0x7FFFFFFF) \
		INTN(var ## _points, var.points, 0, 500) \
		INTN(var ## _strength, var.baseattrs[STAT_STRENGTH], 1, 100) \
		INTN(var ## _endurance, var.baseattrs[STAT_ENDURANCE], 1, 100) \
		INTN(var ## _agility, var.baseattrs[STAT_AGILITY], 1, 100) \
		INTN(var ## _charisma, var.baseattrs[STAT_CHARISMA], 1, 100) \
		INTN(var ## _wisdom, var.baseattrs[STAT_WISDOM], 1, 100) \
		INTN(var ## _intelligence, var.baseattrs[STAT_INTELLIGENCE], 1, 100) \
		INTN(var ## _luck, var.baseattrs[STAT_LUCK], 1, 100) \
		\
		INTN(var ## _armour, var.baseskills[SKILL_ARMOUR], 0, 100) \
		INTN(var ## _diplomacy, var.baseskills[SKILL_DIPLOMACY], 0, 100) \
		INTN(var ## _magic, var.baseskills[SKILL_MAGIC], 0, 100) \
		INTN(var ## _marksman, var.baseskills[SKILL_MARKSMAN], 0, 100) \
		INTN(var ## _melee, var.baseskills[SKILL_MELEE], 0, 100) \
		INTN(var ## _stealth, var.baseskills[SKILL_STEALTH], 0, 100) \
		INTN(var ## _craft, var.baseskills[SKILL_CRAFT], 0, 100) \
		\
		INTN(var ## _fire_thresh, var.bonusthresh[ATTACK_FIRE], -500, 500) \
		INTN(var ## _water_thresh, var.bonusthresh[ATTACK_WATER], -500, 500) \
		INTN(var ## _air_thresh, var.bonusthresh[ATTACK_AIR], -500, 500) \
		INTN(var ## _earth_thresh, var.bonusthresh[ATTACK_EARTH], -500, 500) \
		INTN(var ## _magic_thresh, var.bonusthresh[ATTACK_MAGIC], -500, 500) \
		INTN(var ## _slash_thresh, var.bonusthresh[ATTACK_SLASH], -500, 500) \
		INTN(var ## _blunt_thresh, var.bonusthresh[ATTACK_BLUNT], -500, 500) \
		INTN(var ## _pierce_thresh, var.bonusthresh[ATTACK_PIERCE], -500, 500) \
		\
		INTN(var ## _fire_resist, var.bonusresist[ATTACK_FIRE], -200, 100) \
		INTN(var ## _water_resist, var.bonusresist[ATTACK_WATER], -200, 100) \
		INTN(var ## _air_resist, var.bonusresist[ATTACK_AIR], -200, 100) \
		INTN(var ## _earth_resist, var.bonusresist[ATTACK_EARTH], -200, 100) \
		INTN(var ## _magic_resist, var.bonusresist[ATTACK_MAGIC], -200, 100) \
		INTN(var ## _slash_resist, var.bonusresist[ATTACK_SLASH], -200, 100) \
		INTN(var ## _blunt_resist, var.bonusresist[ATTACK_BLUNT], -200, 100) \
		INTN(var ## _pierce_resist, var.bonusresist[ATTACK_PIERCE], -200, 100) \
		\
		INTN(var ## _maxspeed, var.bonusmovespeed, 0, 100) \
		INTN(var ## _jumpvel, var.bonusjumpvel, 0, 100) \
		INTN(var ## _maxhealth, var.bonushealth, 0, 100000) \
		INTN(var ## _maxmana, var.bonusmana, 0, 100000) \
		INTN(var ## _crit, var.bonuscrit, -100, 100) \
		FLOATN(var ## _healthregen, var.bonushregen, 0, 1000) \
		FLOATN(var ## _manaregen, var.bonusmregen, 0, 1000) \

	ICOMMAND(r_script_say, "ss", (char *t, char *s),
		script *scr = checkscript();
		if(!scr) return;

		scr->dialogue.add(new rpgchat(newstring(t), compilecode(s)));

		if(DEBUG_CONF)
		{
			conoutf(CON_DEBUG, "script[%i]->dialogue[%i]->talk = \"%s\"", scripts.length() - 1, scr->dialogue.length() - 1, t);
			conoutf(CON_DEBUG, "script[%i]->dialogue[%i]->script = \"%s\"", scripts.length() - 1, scr->dialogue.length() - 1, s);
		}
	)

	ICOMMAND(r_script_signal, "ss", (const char *sig, const char *code),
		script *scr = checkscript();
		if(!scr) return;

		signal *listen = scr->listeners.access(sig);
		if(!listen)
		{
			const char *name = newstring(sig);
			listen = &scr->listeners.access(name, signal());
			listen->name = name;
			if(DEBUG_CONF)
				conoutf(CON_DEBUG, "DEBUG: scripts[%i], creating signal slot %p (%s)", scripts.find(scr), listen, sig);
		}
		else if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: scripts[%i], reusing signal slot %p (%s)", scripts.find(scr), listen, sig);

		if(DEBUG_CONF) conoutf(CON_DEBUG, "DEBUG: scripts[%i] = [\n%s\n]", scripts.find(scr), code);
		delete[] listen->code; listen->code = compilecode(code);
	)


	ICOMMAND(r_mapscript_signal, "ss", (const char *sig, const char *code),
		mapscript *scr = checkmapscript();
		if(!scr) return;

		signal *listen = scr->listeners.access(sig);
		if(!listen)
		{
			const char *name = newstring(sig);
			listen = &scr->listeners.access(name, signal());
			listen->name = name;
			if(DEBUG_CONF)
				conoutf(CON_DEBUG, "DEBUG: mapscripts[%i], creating signal slot %p (%s)", mapscripts.find(scr), listen, sig);
		}
		else if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: mapscripts[%i], reusing signal slot %p (%s)", mapscripts.find(scr), listen, sig);

		if(DEBUG_CONF) conoutf(CON_DEBUG, "DEBUG: mapscripts[%i] = [\n%s\n]", mapscripts.find(scr), code);
		delete[] listen->code; listen->code = compilecode(code);
	)

	#define START(n, f, a, b) ICOMMAND(r_effect_ ##n, f, a, b)
	#define INIT effect *e = checkeffect();
	#define DEBUG "effect[%i]"
	#define DEBUG_IND effects.find(e)

	INT(flags, 0, FX_MAX - 1)
	INT(decal, -1, DECAL_MAX - 1)

	STRING(mdl)
	INT(particle, 0, PART_LENS_FLARE - 1)
	INT(colour, 0, 0xFFFFFF)
	INT(fade, 1, 120000)
	INT(gravity, -10000, 100000)
	FLOAT(size, 0.01f, 100.0f)

	INT(lightflags, 0, 7)
	INT(lightfade, 0, 120000)
	INT(lightradius, 16, 2048)
	INT(lightinitradius, 16, 2048)
	VEC(lightcol, -1, -1, -1, 1, 1, 1)
	VEC(lightinitcol, -1, -1, -1, 1, 1, 1)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_status_ ##n, f, a, b)
	#define INIT statusgroup *e = checkstatusgroup();
	#define DEBUG "statusgroup[%i]"
	#define DEBUG_IND statuses.find(e)

	ICOMMAND(r_status_addgeneric, "iiif", (int *t, int *s, int *d, float *v),
		statusgroup *e = checkstatusgroup();
		if(!e) return;

		status_generic *g = new status_generic();
		e->effects.add(g);

		g->type = *t;
		g->duration = *d;
		g->strength = *s;
		g->variance = clamp(1.f, 0.f, *v);

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: adding generic effect (%i %i %i), to statusgroup %i", *t, *d, *s, DEBUG_IND);
	)

	ICOMMAND(r_status_addpolymorph, "siif", (char *m, int *st, int *d, float *v),
		statusgroup *e = checkstatusgroup();
		if(!e) return;

		status_polymorph *p = new status_polymorph();
		e->effects.add(p);
		p->type = STATUS_POLYMORPH;
		p->strength = *st;
		p->duration = *d;
		p->mdl = newstring(m);
		p->variance = clamp(1.f, 0.f, *v);

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: adding polymorph effect (%s), to statusgroup %i", m, DEBUG_IND);
	)

	ICOMMAND(r_status_addlight, "fffiif", (float *r, float *g, float *b, int *str, int *d, float *v),
		statusgroup *e = checkstatusgroup();
		if(!e) return;

		status_light *l = new status_light();
		e->effects.add(l);
		l->type = STATUS_LIGHT;

		l->colour.x = clamp((float) 1, (float) -1, *r);
		l->colour.y = clamp((float) 1, (float) -1, *g);
		l->colour.z = clamp((float) 1, (float) -1, *b);
		l->strength = clamp((int) 512, (int) 8, *str);
		l->duration = *d;
		l->variance = clamp(1.f, 0.f, *v);

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: adding light effect (%f %f %f --> %i), to statusgroup %i", l->colour.x, l->colour.y, l->colour.z, l->strength, DEBUG_IND);
	)

	ICOMMAND(r_status_addsignal, "siif", (char *s, int *str, int *d, float *v),
		INIT if(!e) return;

		status_signal *sig = new status_signal();
		e->effects.add(sig);
		sig->type = STATUS_SIGNAL;
		sig->strength = *str;
		sig->variance = clamp(1.f, 0.f, *v);
		sig->signal = newstring(s);
		sig->duration = *d;
	)

	ICOMMAND(r_status_addscript, "siif", (char *s, int *str, int *d, float *v),
		INIT if(!e) return;

		status_script *scr = new status_script();
		e->effects.add(scr);

		scr->type = STATUS_SCRIPT;
		scr->script = newstring(s);
		scr->strength = *str;
		scr->duration = *d;
		scr->variance = clamp(1.f, 0.f, *v);
	)

	ICOMMAND(r_status_get_effect, "i", (int *idx),
		statusgroup *e = checkstatusgroup();
		if(!e && e->effects.inrange(*idx)) return;
		status *s = e->effects[*idx];
		string str;
		switch(s->type)
		{
			case STATUS_LIGHT:
			{
				status_light  *ls = (status_light *) s;
				formatstring(str)("%i %i %i %f %f %f %f", STATUS_LIGHT, s->strength, s->duration, s->variance, ls->colour.x, ls->colour.y, ls->colour.z);
				break;
			}
			case STATUS_POLYMORPH:
			{
				status_polymorph *ps = (status_polymorph *) s;
				formatstring(str)("%i %i %i %f \"%s\"", STATUS_POLYMORPH, s->strength, s->duration, s->variance, ps->mdl);
				break;
			}
			case STATUS_SIGNAL:
			{
				status_signal *ss = (status_signal *) s;
				formatstring(str)("%i %i %i %f \"%s\"", s->type, s->strength, s->duration, s->variance, ss->signal);
				break;
			}
			case STATUS_SCRIPT: //return script?
			default:
				formatstring(str)("%i %i %i %f", s->type, s->strength, s->duration, s->variance);
				break;
		}
		result(str);
	)

	ICOMMAND(r_status_num_effect, "", (),
		INIT
		if(!e) return;
		intret(e->effects.length());
	)

	BOOL(friendly)
	STRING(icon)
	STRING(name)
	STRING(description)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_item_ ##n, f, a, b)
	#define INIT item_base *e = checkitem_base();
	#define DEBUG "item_base[%i]"
	#define DEBUG_IND items.find(e)

	ICOMMAND(r_item_use_new_consumable, "", (),
		item_base *e = checkitem_base();
		if(!e) return;

		loadinguse = (e->uses.add(new use(e->script)));
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: created new consumable use (%i) for item %i", e->uses.length() - 1, DEBUG_IND);
	)

	ICOMMAND(r_item_use_new_armour, "", (),
		item_base *e = checkitem_base();
		if(!e) return;

		loadinguse = (e->uses.add(new use_armour(e->script)));
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: created new armour use (%i) for item %i", e->uses.length() - 1, DEBUG_IND);
	)

	ICOMMAND(r_item_use_new_weapon, "", (),
		item_base *e = checkitem_base();
		if(!e) return;

		loadinguse = (e->uses.add(new use_weapon(e->script)));
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: created new weapon use (%i) for item %i", e->uses.length() - 1, DEBUG_IND);
	)


	STRING(name)
	STRING(icon)
	STRING(description)
	STRING(mdl)

	INT(script, 0, 0xFFFF)
	INT(type, 0, ITEM_MAX - 1)
	INT(flags, 0, item_base::F_MAX - 1)
	INT(worth, 0, 0xFFFFFF)
	FLOAT(weight, 0, 0xFFFF)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_item_use_ ##n, f, a, b)
	#define INIT CAST *e = (CAST *) checkuse(TYPE);
	#define DEBUG "item_base[%i]->uses[%i]"
	#define DEBUG_IND items.find(loadingitem_base), loadingitem_base->uses.find(loadinguse)

	#define CAST use
	#define TYPE USE_CONSUME

	START(type_get, "", (),
		INIT
		intret(e ? e->type : -1);
	)

	STRING(name)
	STRING(description)
	INT(script, 0, 0xFFFF)
	INT(cooldown, 0, 0xFFFF)
	INT(chargeflags, 0, CHARGE_MAX - 1)

	START(new_status, "iif", (int *st, int *el, float *m),
		INIT if(!e) return;
		e->effects.add(new inflict(*st, *el, *m));
	)

	#undef CAST
	#undef TYPE
	#define CAST use_armour
	#define TYPE USE_ARMOUR

	STRING(vwepmdl)
	STRING(hudmdl)
	INT(idlefx, -1, 0xFFFF)
	STATREQ(reqs)
	INT(slots, 0, SLOT_MAX - 1)
	INT(skill, -1, SKILL_MAX - 1)

	#undef CAST
	#undef TYPE
	#define CAST use_weapon
	#define TYPE USE_WEAPON

	INT(range, 0, 1024)
	INT(angle, 0, 360)
	INT(lifetime, 0, 0xFFFF)
	INT(gravity, -1000, 1000)
	INT(projeffect, -1, 0xFFFF)
	INT(traileffect, -1, 0xFFFF)
	INT(deatheffect, -1, 0xFFFF)
	INT(cost, 0, 0xFFFF)
	INT(pflags, 0, P_MAX - 1)
	INT(ammo, -3, 0xFFFF)
	INT(target, 0, T_MAX - 1)
	INT(radius, 0, 0xFFFF)
	INT(kickback, -0xFFFF, 0xFFFF)
	INT(recoil, -0xFFFF, 0xFFFF)
	INT(charge, 0, 0xFFFF)
	FLOAT(basecharge, 0, 100)
	FLOAT(mincharge, 0, 100)
	FLOAT(maxcharge, 0, 100)
	FLOAT(elasticity, 0, 1)
	FLOAT(speed, 0, 100)

	#undef CAST
	#undef TYPE

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_recipe_ ##n, f, a, b)
	#define INIT recipe *e = checkrecipe();
	#define DEBUG "recipe[%i]"
	#define DEBUG_IND recipes.find(e)

	ICOMMAND(r_recipe_add_ingredient, "ii", (int *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {conoutf(CON_ERROR, "ERROR: can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: added ingredient %i (%i) to recipe %i (%p)", *base, *qty, DEBUG_IND, e);

		e->ingredients.add(invstack(*base, *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_add_catalyst, "ii", (int *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {conoutf(CON_ERROR, "ERROR: can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: added catalyst %i (%i) to recipe %i (%p)", *base, *qty, DEBUG_IND, e);

		e->catalysts.add(invstack(*base, *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_add_product, "ii", (int *base, int *qty),
		recipe *e = checkrecipe();
		if(!e) return;
		if(*qty <= 0) {conoutf(CON_ERROR, "ERROR: can't add an ingredient with a quantity of <= 0"); return;}

		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: added product %i (%i) to recipe %i (%p)", *base, *qty, DEBUG_IND, e);

		e->products.add(invstack(*base, *qty));
		e->optimise()
	)

	ICOMMAND(r_recipe_get_ingredient, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->ingredients.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str)("%i %i", e->ingredients[*idx].base, e->ingredients[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_get_catalyst, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->catalysts.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str)("%i %i", e->catalysts[*idx].base, e->catalysts[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_get_product, "i", (int *idx),
		recipe *e = checkrecipe();
		if(!e || !e->products.inrange(*idx))
		{
			result("-1 0");
			return;
		}
		defformatstring(str)("%i %i", e->products[*idx].base, e->products[*idx].quantity);
		result(str);
	)

	ICOMMAND(r_recipe_num_ingredient, "", (),
		recipe *e = checkrecipe();
		if(!e) { intret(0); return; }
		intret(e->ingredients.length());
	)

	ICOMMAND(r_recipe_num_catalyst, "", (),
		recipe *e = checkrecipe();
		if(!e) { intret(0); return; }
		intret(e->catalysts.length());
	)

	ICOMMAND(r_recipe_num_product, "", (),
		recipe *e = checkrecipe();
		if(!e) { intret(0); return; }
		intret(e->products.length());
	)

	INT(flags, 0, recipe::MAX - 1)
	STATREQ(reqs)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_ammo_ ##n, f, a, b)
	#define INIT ammotype *e = checkammotype();
	#define DEBUG "ammotype[%i]"
	#define DEBUG_IND ammotypes.find(e)

	ICOMMAND(r_ammo_add_item, "i", (int *i),
		INIT
		if(!e) return;

		e->items.add(*i);
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: Classified item %i as member of ammo %i (%s)", *i, DEBUG_IND, e->name ? e->name : "unnamed");
	)

	ICOMMAND(r_ammo_num_item,  "", (),
		INIT
		if(!e) {intret(0); return;}

		intret(e->items.length());
	)

	ICOMMAND(r_ammo_get_item, "i", (int *idx),
		INIT
		if(!e && !e->items.inrange(*idx)) {intret(-1); return;}

		intret(e->items[*idx]);
	)

	STRING(name)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_char_ ##n, f, a, b)
	#define INIT char_base *e = checkchar_base();
	#define DEBUG "char_base[%i]"
	#define DEBUG_IND chars.find(e)

	//ICOMMAND(r_char_additem, "ii", (int *i, int *q)

	STRING(name)
	STRING(mdl)
	INT(script, 0, 0xFFFF)
	INT(faction, 0, 0xFFFF)
	STATS(base)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_faction_ ##n, f, a, b)
	#define INIT faction *e = checkfaction();
	#define DEBUG "faction[%i]"
	#define DEBUG_IND factions.find(e)

	ICOMMAND(r_faction_set_relation, "ii", (int *o, int *f),
		INIT
		if(!e) return;
		e->setrelation(*o, *f);
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: faction %i's liking of faction %i is now %i", DEBUG_IND, *o, *f);
	)

	ICOMMAND(r_faction_get_relation, "i", (int *o),
		INIT
		if (!e) return;

		intret(e->getrelation(*o));
	)

	STRING(name)
	STRING(logo)
	INT(base, 0, 100)

	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_obstacle_ ##n, f, a, b)
	#define INIT obstacle_base *e = checkobstacle_base();
	#define DEBUG "obstacle_base[%i]"
	#define DEBUG_IND obstacles.find(e)

	STRING(mdl)
	INT(weight, 0, 0xFFFF)
	INT(script, 0, 0xFFFF)
	INT(flags, 0, rpgobstacle::F_MAX - 1)


	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_container_ ##n, f, a, b)
	#define INIT container_base *e = checkcontainer_base();
	#define DEBUG "container_base[%i]"
	#define DEBUG_IND containers.find(e)

	//ICOMMAND(r_container_additem, "ii", (int *i, int *q)

	STRING(mdl)
	STRING(name)
	INT(capacity, 0, 0xFFFF)
	INT(script, 0, 0xFFFF)
	INT(lock, 0, 101)


	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_platform_ ##n, f, a, b)
	#define INIT platform_base *e = checkplatform_base();
	#define DEBUG "platform_base[%i]"
	#define DEBUG_IND platforms.find(e)

	STRING(mdl)
	INT(speed, 1, 1000)
	INT(flags, 0, rpgplatform::F_MAX - 1)
	INT(script, 0, 0xFFFF)

	START(addroute, "ii", (int *from, int *to),
		INIT
		vector<int> &detours = e->routes.access(*from, vector<int>());
		if(detours.find(*to) == -1)
		{
			detours.add(*to);
			if(DEBUG_CONF)
				conoutf(CON_DEBUG, "DEBUG: registered detour; " DEBUG "->routes[%i].add(%i)", DEBUG_IND, *from, *to);
		}
		else if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: route from %i to %i already registred for " DEBUG, *from, *to, DEBUG_IND);
	)


	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	#define START(n, f, a, b) ICOMMAND(r_trigger_ ##n, f, a, b)
	#define INIT trigger_base *e = checktrigger_base();
	#define DEBUG "trigger_base[%i]"
	#define DEBUG_IND triggers.find(e)


	STRING(mdl)
	STRING(name)
	INT(flags, 0, rpgtrigger::F_MAX - 1)
	INT(script, 0, 0xFFFF)


	#undef START
	#undef INIT
	#undef DEBUG
	#undef DEBUG_IND

	ICOMMAND(r_global_new, "i", (int *v),
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: created new global variable - variables[%i] = %i", variables.length(), *v);
		variables.add(*v);
		intret(variables.length() - 1);
	)

	ICOMMAND(r_tip_new, "s", (const char *s),
		if(DEBUG_CONF)
			conoutf(CON_DEBUG, "DEBUG: added tip...\n%s", s);
		tips.add(newstring(s));
	)
}
