#include "rpggame.h"

using namespace game;

/*
	Reference is a purely script class, so we define it here so we can have some debug stuff
*/

#define temporary(x) ((x).getequip() || (x).getveffect() || (x).getaeffect())

rpgent *reference::getent()
{
	if(type >= T_CHAR && type <= T_TRIGGER) return (rpgent *) ref;
	return NULL;
}
rpgchar *reference::getchar()
{
	if(type == T_CHAR) return (rpgchar *) ref;
	return NULL;
}
rpgitem *reference::getitem()
{
	if(type == T_ITEM) return (rpgitem *) ref;
	return NULL;
}
rpgobstacle *reference::getobstacle()
{
	if(type == T_OBSTACLE) return (rpgobstacle *) ref;
	return NULL;
}
rpgcontainer *reference::getcontainer()
{
	if(type == T_CONTAINER) return (rpgcontainer *) ref;
	return NULL;
}
rpgplatform *reference::getplatform()
{
	if(type == T_PLATFORM) return (rpgplatform *) ref;
	return NULL;
}
rpgtrigger *reference::gettrigger()
{
	if(type == T_TRIGGER) return (rpgtrigger *) ref;
	return NULL;
}
invstack *reference::getinv()
{
	if(type == T_INV || type == T_ITEM) return (invstack *) ref;
	return NULL;
}
equipment *reference::getequip()
{
	if(type == T_EQUIP) return (equipment *) ref;
	return NULL;
}
mapinfo *reference::getmap()
{
	if(type == T_MAP) return (mapinfo *) ref;
	return NULL;
}
victimeffect *reference::getveffect()
{
	if(type == T_VEFFECT) return (victimeffect *) ref;
	return NULL;
}
areaeffect *reference::getaeffect()
{
	if(type == T_AEFFECT) return (areaeffect *) ref;
	return NULL;
}

void reference::setref(rpgchar *d)
{
	ref = d; type = T_CHAR;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type char", name, ref);
}
void reference::setref(rpgitem *d)
{
	ref = d; type = T_ITEM;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type item", name, ref);
}
void reference::setref(rpgobstacle *d)
{
	ref = d; type = T_OBSTACLE;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type obstacle", name, ref);
}
void reference::setref(rpgcontainer *d)
{
	ref = d; type = T_CONTAINER;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type container", name, ref);
}
void reference::setref(rpgplatform *d)
{
	ref = d; type = T_PLATFORM;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type platform", name, ref);
}
void reference::setref(rpgtrigger *d)
{
	ref = d; type = T_TRIGGER;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type trigger", name, ref);
}
void reference::setref(rpgent *d)
{
	if(d) switch(d->type())
	{
		case ENT_CHAR: setref((rpgchar *) d); return;
		case ENT_ITEM: setref((rpgitem *) d); return;
		case ENT_OBSTACLE: setref((rpgobstacle *) d); return;
		case ENT_CONTAINER: setref((rpgcontainer *) d); return;
		case ENT_PLATFORM: setref((rpgplatform *) d); return;
		case ENT_TRIGGER: setref((rpgtrigger *) d); return;
	}
	setnull();
}
void reference::setref(invstack *d)
{
	ref = d; type = T_INV;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type inv", name, ref);
}
void reference::setref(equipment *d)
{
	ref = d; type = T_EQUIP;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with volatile type equip", name, ref);
}
void reference::setref(mapinfo *d)
{
	ref = d; type = T_MAP;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with type map", name, ref);
}
void reference::setref(victimeffect *d)
{
	ref = d; type = T_VEFFECT;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with volatile type victemeffect", name, ref);
}
void reference::setref(areaeffect *d)
{
	ref = d; type = T_AEFFECT;
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference %s set to %p with volatile type areaeffect", name, ref);
}

void reference::setnull() {ref = NULL; type = T_INVALID; if(DEBUG_VSCRIPT) conoutf("DEBUG: reference %s set to null", name);}

template<typename T>
reference::reference(const char *n, T d) : name (newstring(n))
{
	setref(d);
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: adding reference %s; %p %i", name, ref, type);
}
reference::reference(const char *n) : name (newstring(n)), ref(NULL), type(T_INVALID) {
	if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: adding null reference %s", name);
}

reference::~reference()
{
	if(name && DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: freeing reference %s; %p %i", name, ref, type);
	delete[] name;
}

/*
	Some signal stuff
*/

void invstack::getsignal(const char *sig, bool prop, rpgent *sender, int use)
{
	if(DEBUG_VSCRIPT)
		conoutf(CON_DEBUG, "DEBUG: item %p received signal %s with sender %p and use %i; propagating: %s", this, sig, sender, use, prop ? "yes" : "no");

	item_base *item = NULL;
	if(game::items.inrange(base))
		item = game::items[base];

	if(item)
	{
		signal *listen = NULL;
		if(use >= 0 && item->uses.inrange(use) && game::scripts.inrange(item->uses[use]->script))
			listen = game::scripts[item->uses[use]->script]->listeners.access(sig);
		if(!listen)
			listen = game::scripts[item->script]->listeners.access(sig);

		if(listen)
			rpgscript::doitemscript(this, sender, listen->code);
	}
}

void rpgent::getsignal(const char *sig, bool prop, rpgent *sender)
{
	if(DEBUG_VSCRIPT)
		conoutf(CON_DEBUG, "DEBUG: entity %p received signal %s with sender %p; propagating: %s", this, sig, sender, prop ? "yes" : "no");

	if(game::scripts.inrange(getscript()))
	{
		signal *listen = game::scripts[getscript()]->listeners.access(sig);
		if(listen)
			rpgscript::doentscript(this, sender, listen->code);
	}
	if(prop && (type() == ENT_CHAR || type() == ENT_CONTAINER))
	{
		vector<invstack *> &inv = type() == ENT_CHAR ? ((rpgchar *) this)->inventory : ((rpgcontainer *) this)->inventory;
		loopv(inv)
		{
			if(inv[i]->quantity > 0)
				inv[i]->getsignal(sig, prop, this, -1);
			if(type() == ENT_CHAR)
			{
				vector<equipment*> &equips = ((rpgchar *) this)->equipped;
				loopvj(equips) if(equips[j]->base == inv[i]->base)
					inv[i]->getsignal(sig, prop, this, equips[j]->use);
			}
		}
	}
}

void mapinfo::getsignal(const char *sig, bool prop, rpgent *sender)
{
	if(DEBUG_VSCRIPT)
		conoutf(CON_DEBUG, "DEBUG: map %p received signal %s with sender %p; propagating: %s", this, sig, sender, prop ? "yes" : "no");

	if(game::mapscripts.inrange(script))
	{
		signal *listen = game::mapscripts[script]->listeners.access(sig);
		if(listen)
			rpgscript::domapscript(this, sender, listen->code);
	}
	if(prop) loopv(objs)
	{
		objs[i]->getsignal(sig, prop, sender);
	}
}

bool delayscript::update()
{
	if((remaining -= curtime) > 0 || camera::cutscene)
		return true;

	rpgscript::stack.add(&refs);
	execute(script);
	rpgscript::stack.pop();

	return false;
}

/*
	start the actual script stuff!
*/

namespace rpgscript
{
	/**
		HOW IT WORKS

		The system is stack based, whenever a script command is executed, the stack is pushed, the script executed then the stack popped.
		The system defines several references, player, hover and curmap; these are special references that should only be modified by the code here.
		This system must be 100% type safe and relatively ignorant of them.
	*/

	vector<rpgent *> obits; //r_destroy
	vector<hashtable<const char*, reference> *> stack;
	vector<delayscript *> delaystack;
	reference *player = NULL;
	reference *hover = NULL;
	reference *map = NULL;
	reference *talker = NULL;
	reference *looter = NULL;
	reference *trader = NULL;

	void pushstack()
	{
		if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: pushing new stack...");
		stack.add(new hashtable<const char*, reference>(128));
		if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: new depth is %i", stack.length());
	}

	void popstack()
	{
		if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: popping stack...");
		if(stack.length() > 1)
		{
			delete stack.pop();
			if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: new depth is %i", stack.length());
		}
		else
			conoutf(CON_ERROR, "ERROR: sandbox just tried to pop the reference stack, but it is too shallow (stack.length <= 1)");
	}

	ICOMMAND(r_stack, "e", (uint *body),
		if(!stack.length()) return; //assume no game in progress
		pushstack();
		execute(body);
		popstack();
	)

	reference *searchstack(const char *name, bool create)
	{
		static reference dummy("");

		if(!stack.length())
		{
			conoutf(CON_ERROR, "ERROR: no stack");
			return NULL;
		}
		loopvrev(stack)
		{
			reference *ref = stack[i]->access(name);
			if(ref)
			{
				if(DEBUG_VSCRIPT)
					conoutf(CON_DEBUG, "DEBUG: reference \"%s\" found, returning %p (%i, %p)", name, ref, ref->type, ref->ref);
				return ref;
			}
		}
		if(create)
		{
			if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference \"%s\" not found, creating", name);
			const char *refname = newstring(name);
			reference *ref = &stack.last()->access(refname, dummy);
			ref->name = refname;
			return ref;

		}
		if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: reference \"%s\" not found", name);
		return NULL;
	}

	template<typename T>
	reference *registerref(const char *name, T ref)
	{
		if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: registering reference %p to name %s", ref, name);
		reference *r = searchstack(name, true);
		if(r) r->setref(ref);
		return r;
	}

	template<typename T>
	reference *registertemp(const char *name, T ref)
	{
		static reference dummy("");

		if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: registering end-of-stack reference %p to name %s", ref, name);
		reference *r = NULL;
		if(stack.length())
		{
			r = stack.last()->access(name);
			if(!r)
			{
				const char *n = newstring(name);
				r = &stack.last()->access(n, dummy);
				r->name = n;
			}
			r->setref(ref);
		}
		return r;
	}

	void copystack(hashtable<const char *, reference> &dst)
	{
		static reference dummy("");
		loopvj(stack)
		{
			enumerate(*stack[j], reference, ref,
				reference *newref = dst.access(ref.name);
				if(!newref)
				{
					const char *n = newstring(ref.name);
					newref = &dst.access(n, dummy);
					newref->name = n;
				}
				newref->setnull();
				if(!temporary(ref))
				{
					newref->type = ref.type;
					newref->ref = ref.ref;
				}
			)
		}
	}

	ICOMMAND(r_sleep, "is", (int *rem, const char *scr),
		delayscript *ds = delaystack.add(new delayscript());
		ds->remaining = *rem;
		ds->script = newstring(scr);
		copystack(ds->refs);
	)


	void clean()
	{
		stack.deletecontents();
		obits.setsize(0);
		player = map = talker = hover = NULL;
	}

	void init()
	{
		pushstack();
		player = registerref("player", player1);
		hover = searchstack("hover", true);
		talker = searchstack("talker", true);
		map = registerref("curmap", curmap);
	}

	void removereferences(void *ptr, bool references)
	{
		obits.removeobj((rpgent *)ptr);
		enumerate(*mapdata, mapinfo, map,
			map.objs.removeobj((rpgent *)ptr);
			loopvj(map.loadactions)
			{
				if(map.loadactions[j]->type() != ACTION_TELEPORT || ((action_teleport *) map.loadactions[j])->ent != ptr)
					continue;
				delete map.loadactions[j];
				map.loadactions.remove(j);
				j--;
			}
		);
		if(references) loopvj(stack)
		{
			enumerate(*stack[j], reference, ref,
				if(ref.ref == ptr)
					ref.setnull();
			);
		}
	}

	void changemap()
	{
		map->setref(curmap);
		player->setref(player1); //just in case someone's decided to be stupid...
		talker->setnull();
		hover->setnull();
	}

	void update()
	{
		obits.removeobj(game::player1);
		while(obits.length())
		{
			rpgent *ent = obits[0];
			removereferences(ent);
			delete ent;
		}
		//clear volatile references
		loopvj(stack)
		{
			enumerate(*stack[j], reference, ref,
				if(temporary(ref)) ref.setnull()
			);
		}

		loopv(delaystack)
		{
			if(!delaystack[i]->update())
				delete delaystack.remove(i--);
		}

		vec dir = vec(worldpos).sub(player1->o);
		if(dir.magnitude() > player1->radius + 8)
		{
			dir.normalize().mul(player1->radius + 8);
		}
		dir.add(player1->o);

		float dist;
		rpgent *h = intersectclosest(player1->o, dir, player1, dist, 1);

		if(hover->ref != h) hover->setref(h);
	}

	void doitemscript(invstack *invokee, rpgent *invoker, uint *code)
	{
		if(!invokee || !code) return;
		if(DEBUG_VSCRIPT)
			conoutf(CON_DEBUG, "DEBUG: conditions met, executing code with invokee %p and invoker %p", invokee, invoker);

		pushstack();

		registertemp("self", invokee);
		registertemp("actor", invoker);

		execute(code);
		popstack();
	}

	void doentscript(rpgent *invokee, rpgent *invoker, uint *code)
	{
		if(!invokee || !code) return;
		if(DEBUG_VSCRIPT)
			conoutf(CON_DEBUG, "DEBUG: conditions met, executing code with invokee %p and invoker %p", invokee, invoker);

		pushstack();

		registertemp("self", invokee);
		registertemp("actor", invoker);

		execute(code);
		popstack();
	}

	void domapscript(mapinfo *invokee, rpgent *invoker, uint *code)
	{
		if(!invokee || !code) return;
		if(DEBUG_VSCRIPT)
			conoutf(CON_DEBUG, "DEBUG: conditions met, executing code with invokee %p and invoker %p", invokee, invoker);

		pushstack();

		registertemp("self", invokee);
		registertemp("actor", invoker);

		execute(code);
		popstack();
	}

	#define getreference(var, name, cond, fail, fun) \
		if(!*var) \
		{ \
			conoutf(CON_ERROR, "ERROR: " #fun "; requires a reference to be specified"); \
			fail; return; \
		} \
		reference *name = searchstack(var); \
		if(!name || !name->ref || !(cond)) \
		{ \
			conoutf(CON_ERROR, "ERROR: " #fun "; invalid reference \"%s\" or of incompatible type", var); \
			fail; return; \
		}


	// Utility Functions

	ICOMMAND(worlduse, "", (),
		if(hover && hover->getent() && hover->ref != player1)
		{
			if(DEBUG_SCRIPT)
				conoutf(CON_DEBUG, "DEBUG: Player interacted with %p", hover);

			hover->getent()->getsignal("interact", false, player1);
		}
	)

	ICOMMAND(r_cansee, "ss", (const char *looker, const char *lookee),
		getreference(looker, ent, ent->getchar(), intret(0), r_cansee)
		getreference(lookee, obj, obj->getent(), intret(0), r_cansee)

		intret(ent->getchar()->cansee(obj->getent()));
	)

	ICOMMAND(r_preparemap, "sii", (const char *m, int *scr, int *f),
		if(*m)
		{
			if(DEBUG_SCRIPT)
				conoutf(CON_DEBUG, "DEBUG: Prepared mapinfo for %s with script %i and flags %i", m, *scr, *f);

			mapinfo *tmp = accessmap(m);
			tmp->script = *scr;
			tmp->flags = *f;
		}
	)

	ICOMMAND(r_reftype, "s", (const char *ref),
		reference *lookup = searchstack(ref);

		if(lookup && lookup->ref) intret(lookup->type);
		else intret(reference::T_INVALID);
	)

	ICOMMAND(r_registerref, "s", (const char *ref),
		if(!*ref) {conoutf(CON_ERROR, "ERROR; r_registerref; requires the reference to be named"); return;}
		searchstack(ref, true);
	)

	ICOMMAND(r_registertemp, "s", (const char *ref),
		if(!*ref) {conoutf(CON_ERROR, "ERROR; r_registertemp; requires the reference to be named"); return;}
		registertemp(ref, NULL);
	)

	ICOMMAND(r_setref, "ss", (const char *ref, const char *alias),
		if(!*ref) {conoutf(CON_ERROR, "ERROR; r_setref; requires reference to be named"); return;}

		reference *r = searchstack(ref, true);
		reference *a = *alias ? searchstack(alias) : NULL;
		if(a) { r->ref = a->ref; r->type = a->type; }
		else r->setnull();
	)

	ICOMMAND(r_swapref, "ss", (const char *f, const char *s),
		if(!*f || !*s) {conoutf(CON_ERROR, "ERROR: r_swapref; requires two valid references to swap"); return;}
		reference *a = searchstack(f);
		reference *b = searchstack(s);
		if(!a || !b) {conoutf(CON_ERROR, "ERROR: r_swapref; either %s or %s is an invalid reference", f, s); return;}
		swap(a->type, b->type);
		swap(a->ref, b->ref);
	)

	ICOMMAND(r_matchref, "ss", (const char *f, const char *s),
		if(!*f || !*s) {conoutf(CON_ERROR, "ERROR: r_matchref; requires two valid references to compare"); intret(0); return;}
		reference *a = searchstack(f);
		reference *b = searchstack(s);
		if(!a || !b) {conoutf(CON_ERROR, "ERROR: r_matchref; either %s or %s is an invalid reference", f, s); intret(0); return;}

		intret(a->ref == b->ref);
	)

	ICOMMAND(r_global_get, "i", (int *i),
		if(!variables.inrange(*i))
		{
			conoutf(CON_ERROR, "ERROR: no global variable of index %i, returning 0", *i);
			intret(0);
			return;
		}

		if(DEBUG_SCRIPT)
			conoutf(CON_DEBUG, "DEBUG: global variable %i requested, returning %i", *i, variables[*i]);

		intret(variables[*i])
	)

	ICOMMAND(r_global_set, "ii", (int *i, int *v),
		if(!variables.inrange(*i))
		{
			conoutf(CON_ERROR, "ERROR: no global variable of index %i, unable to replace", *i);
			return;
		}
		if(DEBUG_SCRIPT)
			conoutf(CON_DEBUG, "DEBUG: global variable %i set to %i from %i", *i, *v, variables[*i]);
		variables[*i] = *v;
	)

	// Script Commands

	void sendsignal(const char *sig, const char *send, const char *rec, int prop)
	{
		if(!sig) return;
		reference *sender = NULL, *receiver = NULL;
		if(*send) sender = searchstack(send);
		if(*rec) receiver = searchstack(rec);

		if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG, r_signal called with sig: %s - send: %s - rec: %s - prop: %i", sig, send, rec, prop);

		if(*rec && !receiver)
		{
			conoutf(CON_ERROR, "ERROR: receiver does not exist");
			return;
		}
		else if(!receiver)
		{
			if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: no receiver, sending signal to everything");
			enumerate(*game::mapdata, mapinfo, map,
				map.getsignal(sig, true, sender ? sender->getent() : NULL);
			)
			camera::getsignal(sig);
		}
		else if(receiver->getmap())
		{
			if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: receiver is map, sending signal");
			receiver->getmap()->getsignal(sig, prop, sender ? sender->getent() : NULL);
			return;
		}
		else  if(receiver->getent())
		{
			if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: receiver is ent, sending signal");
			receiver->getent()->getsignal(sig, prop, sender ? sender->getent() : NULL);
			return;
		}
		else if(receiver->getinv())
		{
			if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: receiver is item, sending signal");
			receiver->getinv()->getsignal(sig, prop, sender ? sender->getent() : NULL);
			return;
		}

	}
	COMMANDN(r_signal, sendsignal, "sssi");

	ICOMMAND(r_loop_maps, "se", (const char *ref, uint *body),
		if(!mapdata) return;
		if(!*ref) {conoutf(CON_ERROR, "ERROR; r_loop_maps; requires reference to alias maps to"); return;}

		pushstack();
		reference *cur = registertemp(ref, NULL);

		enumerate(*mapdata, mapinfo, map,
			cur->setref(&map);
			execute(body);
		)

		popstack();
	)

	ICOMMAND(r_loop_ents, "sse", (const char *map, const char *ref, uint *body),
		if(!*ref) {conoutf(CON_ERROR, "ERROR: r_loop_ents; requires map reference and a reference to alias ents to"); return;}
		getreference(map, mapref, mapref->getmap(), , r_loop_ents)

		pushstack();
		reference *cur = registertemp(ref, NULL);

		loopv(mapref->getmap()->objs)
		{
			cur->setref(mapref->getmap()->objs[i]);
			execute(body);
		}

		popstack();
	)

	ICOMMAND(r_loop_aeffects, "ssse", (const char *map, const char *vic, const char *ref, uint *body),
		if(!*ref) {conoutf(CON_ERROR, "ERROR: r_loop_aeffects; requires map reference and a reference to alias ents to"); return;}
		getreference(map, mapref, mapref->getmap(), , r_loop_aeffects)

		rpgent *victim = NULL;
		if(*vic)
		{
			if(DEBUG_VSCRIPT) conoutf(CON_DEBUG, "DEBUG: r_loop_aeffects, entity reference provided, may do bounds checking");
			reference *v = searchstack(vic, false);
			if(v) victim = v->getent();
		}

		pushstack();
		reference *cur = registertemp(ref, NULL);

		loopv(mapref->getmap()->aeffects)
		{
			areaeffect *ae = mapref->getmap()->aeffects[i];
			if(victim && victim->o.dist(ae->o) > ae->radius && victim->feetpos().dist(ae->o) > ae->radius) continue;

			cur->setref(ae);
			execute(body);
		}

		popstack();
	)

	ICOMMAND(r_loop_inv, "sse", (const char *ent, const char *ref, uint *body),
		if(!*ref) {conoutf(CON_ERROR, "ERROR: r_loop_inv, requires ent reference and reference to alias items to"); return;}
		getreference(ent, entref, entref->getchar() || entref->getcontainer(), , r_loop_inv)

		pushstack();
		reference *item = registertemp(ref, NULL);

		if(entref->getchar()) loopv(entref->getchar()->inventory)
		{
			if(!entref->getchar()->inventory[i]->quantity) continue;
			item->setref(entref->getchar()->inventory[i]);
			execute(body);
		}
		else loopv(entref->getcontainer()->inventory)
		{
			if(!entref->getcontainer()->inventory[i]->quantity) continue;
			item->setref(entref->getcontainer()->inventory[i]);
			execute(body);
		}

		popstack();
	)

	ICOMMAND(r_loop_equip, "sse", (const char *ent, const char *ref, uint *body),
		if(!*ref) {conoutf(CON_ERROR, "ERROR: r_loop_equip; requires ent reference and a reference name"); return;}
		getreference(ent, entref, entref->getchar(), intret(-1), r_loop_equip)

		pushstack();
		reference *equip = registertemp(ref, NULL);

		vector<equipment *> &equips = entref->getchar()->equipped;
		loopv(equips)
		{
			equip->setref(equips[i]);
			execute(body);
		}

		popstack();
	)

	ICOMMAND(r_loop_veffects, "sse", (const char *ent, const char *ref, uint *body),
		if(!*ref) {conoutf(CON_ERROR, "ERROR: r_loop_veffects; requires ent reference and a reference name"); return;}
		getreference(ent, entref, entref->getent(), intret(-1), r_loop_veffects)

		pushstack();
		reference *effect = registertemp(ref, NULL);

		vector<victimeffect *> &seffects = entref->getent()->seffects;
		loopv(seffects)
		{
			effect->setref(seffects[i]);
			execute(body);
		}

		popstack();
	)

	ICOMMAND(r_setref_inv, "ssi", (const char *entref, const char *ref, int *index),
		getreference(entref, ent, ent->getchar(), , r_setref_inv)
		if(!*ref) return;

		loopv(ent->getchar()->inventory)
		{
			if(ent->getchar()->inventory[i]->base == *index)
			{
				registerref(ref, ent->getchar()->inventory[i]);
				return;
			}
		}
	)

	// Highly generic stuff

	ICOMMAND(r_get_attr, "sii", (const char *ref, int *attr, int *u),
		if(*attr < 0 || *attr >= STAT_MAX) {conoutf(CON_ERROR, "ERROR: r_get_attr; attribute %i out of range", *attr); intret(0); return;}
		getreference(ref, obj, obj->getchar() || obj->getinv() || obj->getequip(), intret(0), r_get_attr)
		if(obj->getchar())
			intret(obj->getchar()->base.getattr(*attr));
		else
		{
			int i;
			if(obj->getinv()) i = obj->getinv()->base;
			else {i = obj->getequip()->base ; *u = obj->getequip()->use;}

			use_armour *arm = (use_armour *) items[i]->uses[*u];
			if(arm->type >= USE_CONSUME)
				intret(arm->reqs.attrs[*attr]);
			else
			{
				conoutf(CON_ERROR, "ERROR: r_get_attr; ref is associated with an item, but index %i or use %i is invalid", i, *u);
				intret(0);
			}
		}
	)

	ICOMMAND(r_get_skill, "sii", (const char *ref, int *skill, int *u),
		if(*skill < 0 || *skill >= SKILL_MAX) {conoutf(CON_ERROR, "ERROR: r_get_skill; skill %i out of range", *skill); intret(0); return;}
		getreference(ref, obj, obj->getchar() || obj->getinv() || obj->getequip(), intret(0), r_get_skill)
		if(obj->getchar())
			intret(obj->getchar()->base.getskill(*skill));
		else
		{
			int i;
			if(obj->getinv()) i = obj->getinv()->base;
			else {i = obj->getequip()->base ; *u = obj->getequip()->use;}

			if(items.inrange(i) && items[i]->uses.inrange(*u))
			{
				use_armour *arm = (use_armour *) items[i]->uses[*u];
				if(arm->type >= USE_CONSUME)
				{
					intret(arm->reqs.skills[*skill]);
					return;
				}
			}
			conoutf(CON_ERROR, "ERROR: r_get_skill; ref is associated with an item, but index %i or use %i is invalid", i, *u);
			intret(0);
		}
	)

	ICOMMAND(r_get_weight, "s", (const char *ref),
		getreference(ref, ent, ent->getchar() || ent->getcontainer() || ent->getinv() || ent->getequip(), floatret(0), r_get_weight)
		if(ent->getent())
			floatret(ent->getent()->getweight());
		else if(ent->getinv() && items.inrange(ent->getinv()->base)) //T_INV
			floatret(ent->getinv()->quantity * items[ent->getinv()->base]->weight);
		else if(ent->getequip() && items.inrange(ent->getequip()->base))
			floatret(items[ent->getequip()->base]->weight);
		else
		{
			floatret(0);
			conoutf(CON_ERROR, "ERROR: r_get_weight; base out of range?");
		}
	)

	ICOMMAND(r_get_name, "si", (const char *ref, int *u),
		getreference(ref, obj, obj->getent() || obj->getinv() || obj->getequip(), result(""), r_get_name)
		if(obj->getequip() || (obj->getinv() && *u >= 0))
		{
			int i;
			if(obj->getequip()) {i = obj->getequip()->base; *u = obj->getequip()->use;}
			else i = obj->getinv()->base;

			if(!items.inrange(i)) {result(""); return;}

			item_base *it = items[i];
			if(it->uses.inrange(*u))
				result(it->uses[*u]->name ? it->uses[*u]->name : "Dorificus");
			else
				result(it->name ? it->name : "Objectivus Maximus");
		}
		else
			result(obj->getent()->getname() ? obj->getent()->getname() : "");
	)

	ICOMMAND(r_get_description, "si", (const char *ref, int *u),
		getreference(ref, obj, /*obj->getent() ||*/obj->getinv() || obj->getequip(), result(""), r_get_description)
		/*if(obj->getinv() || obj->getequip())*/
		{
			int i;
			if(obj->getequip()) {i = obj->getequip()->base; *u = obj->getequip()->use;}
			else i = obj->getinv()->base;

			if(!items.inrange(i)) {result(""); return;}

			item_base *it = items[i];
			if(it->uses.inrange(*u))
				result(it->uses[*u]->description ? it->uses[*u]->description : "");
			else
				result(it->description ? it->description : "");
		}
		/*else
			result(ent->getent()->getname());*/
	)

	ICOMMAND(r_get_type, "s", (const char *ref),
		getreference(ref, ent, ent->getent(), intret(0), r_get_type)
		intret(ent->getent()->type());
	)

	// Item Commands

	ICOMMAND(r_get_base, "s", (const char *ref),
		getreference(ref, item, item->getinv() || item->getequip(), intret(-1), r_get_base)

		if(item->getinv()) intret(item->getinv()->base);
		else intret(item->getequip()->base);
	) //TODO save bases for everything and return here.

	ICOMMAND(r_get_use, "s", (const char *ref),
		getreference(ref, equip, equip->getequip(), intret(-1), r_get_use)
		intret(equip->getequip()->use);
	)

	ICOMMAND(r_mod_amount, "si", (const char *ref, int *q),
		getreference(ref, item, item->getinv(), , r_mod_amount)

		item->getinv()->quantity = max(0, item->getinv()->quantity + *q);
	)

	// Entity Commands

	ICOMMAND(r_get_state, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(CS_DEAD), r_get_state)

		intret(ent->getent()->state);
	)

	ICOMMAND(r_get_pos, "s", (const char *ref),
		getreference(ref, ent, ent->getent(), result("0 0 0"), r_get_pos)
		static string s; formatstring(s)("%f %f %f", ent->getent()->o.x, ent->getent()->o.y, ent->getent()->o.z);
		result(s);
	)

	ICOMMAND(r_get_faction, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(-1), r_get_faction)

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: faction requested of %p, returning %i", ent->getchar(), ent->getchar()->faction);
		intret(ent->getchar()->faction)
	)

	ICOMMAND(r_add_effect, "sifi", (const char *ref, int *st, float *mul, int *flags),
		if(!statuses.inrange(*st)) return;
		getreference(ref, victim, victim->getent(), , r_add_effect)

		inflict inf(*st, ATTACK_NONE, 1); //resistances aren't applied
		victim->getent()->seffects.add(new victimeffect(NULL, &inf, *flags, *mul));
	)

	ICOMMAND(r_kill, "ss", (const char *ref, const char *killer), //I was framed!
		getreference(ref, victim, victim->getchar(), , r_kill)
		reference *nasty = *killer ? searchstack(killer) : NULL;

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: r_kill; killing \"%s\" with \"%s\" as the killer", ref, killer);
		victim->getent()->die(nasty ? nasty->getent() : NULL);
	)

	ICOMMAND(r_resurrect, "s", (const char *ref),
		getreference(ref, target, target->getchar(), , r_resurrect)

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: r_resurrect; trying to revive \"%s\"", ref);
		target->getchar()->revive(false);
	)

	ICOMMAND(r_pickup, "ss", (const char *finder, const char *keepsake), //finders keepers :P
		getreference(finder, actor, actor->getchar(), , r_pickup)
		getreference(keepsake, item, item->getitem(), , r_pickup)

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: r_pickup; reference \"%s\" trying to pickup \"%s\"", finder, keepsake);
		actor->getchar()->pickup(item->getitem());
		if(!item->getitem()->quantity)
		{
			if(DEBUG_SCRIPT) conoutf("DEBUG: r_pickup; keepsake has no more items left, queueing for obit");
			 obits.add(item->getent());
		}
	)

	ICOMMAND(r_additem, "sii", (const char *ref, int *i, int *q),
		getreference(ref, ent, ent->getchar() || ent->getcontainer(), , r_additem)

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: added %i instances of item %i to reference \"%s\"", *q, *i, ref);
		ent->getent()->modinv(*i, max(0, *q));
	)

	ICOMMAND(r_get_amount, "si", (const char *ref, int *base),
		getreference(ref, ent, ent->getchar() || ent->getcontainer() || ent->getinv(), intret(0), r_get_amount)

		if(ent->getinv())
			intret(ent->getinv()->quantity);
		else
			intret(ent->getent()->getitemcount(*base));
	)

	ICOMMAND(r_drop, "sii", (const char *ref, int *i, int *q),
		getreference(ref, ent, ent->getchar() || ent->getcontainer(), intret(0), r_drop)

		int dropped = ent->getent()->modinv(*i, min(0, - *q), true);
		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: dropping %i instances of %i from refernece \"%s\"; %i dropped", *q, *i, ref, dropped);
		intret(dropped);
	)

	ICOMMAND(r_remove, "sii", (const char *ref, int *i, int *q),
		getreference(ref, ent, ent->getchar() || ent->getcontainer(), intret(0), r_remove)

		int dropped = ent->getent()->modinv(*i, min(0, - *q), false);
		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: removing %i instances of %i from reference \"%s\"; %i removed", *q, *i, ref, dropped);
		intret(dropped);
	)

	ICOMMAND(r_equip, "sii", (const char *ref, int *i, int *u),
		getreference(ref, ent, ent->getchar(), , r_equip)

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: attempting to equip item %i with use %i to reference \"%s\"", *i, *u, ref);
		ent->getent()->equip(*i, *u);
	)

	ICOMMAND(r_dequip, "sii", (const char *ref, int *i, int *s),
		getreference(ref, ent, ent->getchar(), , r_dequip);

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: attempting to dequip item %i on slots %i from reference \"%s\"", *i, *s, ref);
		ent->getent()->dequip(*i, *s);
	)

	ICOMMAND(r_trigger, "s", (const char *ref),
		getreference(ref, obj, obj->gettrigger(), , r_trigger)

		obj->gettrigger()->lasttrigger = lastmillis;
		obj->gettrigger()->flags ^= rpgtrigger::F_TRIGGERED;

		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: triggered reference \"%s\", triggered flag is now %i", ref, obj->gettrigger()->flags & rpgtrigger::F_TRIGGERED);
	)

	ICOMMAND(r_destroy, "s", (const char *victim),
		getreference(victim, obit, obit->getent(), , r_destroy)
		if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: queueing reference %s for destruction", victim);

		obits.add(obit->getent());
	)

	// Entity - stats

	ICOMMAND(r_get_mana, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_mana)
		intret(ent->getchar()->mana);
	)

	ICOMMAND(r_get_maxmana, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_maxmana)
		intret(ent->getchar()->base.getmaxmp());
	)

	ICOMMAND(r_get_manaregen, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_manaregen)
		floatret(ent->getchar()->base.getmpregen())
	)

	ICOMMAND(r_get_health, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_health)
		intret(ent->getchar()->health);
	)

	ICOMMAND(r_get_maxhealth, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_maxhealth)
		intret(ent->getchar()->base.getmaxhp());
	)

	ICOMMAND(r_get_healthregen, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_healthregen)
		floatret(ent->getchar()->base.gethpregen())
	)

	ICOMMAND(r_get_resist, "si", (const char *ref, int *elem),
		if(*elem < 0 || *elem >= ATTACK_MAX) {conoutf(CON_ERROR, "ERROR: r_get_resist; element %i out of range", *elem); intret(0); return;}
		getreference(ref, ent, ent->getchar(), intret(0), r_get_resist)
		intret(ent->getchar()->base.getresistance(*elem));
	)

	ICOMMAND(r_get_thresh, "si", (const char *ref, int *elem),
		if(*elem < 0 || *elem >= ATTACK_MAX) {conoutf(CON_ERROR, "ERROR: r_get_thres; element %i out of range", *elem); intret(0); return;}
		getreference(ref, ent, ent->getchar(), intret(0), r_get_thresh)
		intret(ent->getchar()->base.getthreshold(*elem));
	)

	ICOMMAND(r_get_points, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_points)
		intret(ent->getchar()->base.points);
	)

	ICOMMAND(r_get_level, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_level)
		intret(ent->getchar()->base.level);
	)

	ICOMMAND(r_get_exp, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_exp)
		intret(ent->getchar()->base.experience);
	)

	ICOMMAND(r_get_neededexp, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_neededexp)
		intret(stats::neededexp(ent->getchar()->base.level));
	)

	ICOMMAND(r_get_maxweight, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_maxweight)
		intret(ent->getchar()->base.getmaxcarry());
	)

	ICOMMAND(r_get_maxspeed, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_maxspeed)
		float spd; float jmp;
		ent->getchar()->base.setspeeds(spd, jmp);
		floatret(spd);
	)

	ICOMMAND(r_get_jumpvel, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_jumpvel)
		float spd; float jmp;
		ent->getchar()->base.setspeeds(spd, jmp);
		floatret(jmp);
	)

	ICOMMAND(r_invest_attr, "si", (const char *ref, int *attr),
		if(*attr < 0 || *attr >= STAT_MAX) return;
		getreference(ref, ent, ent->getchar(), , r_invest_attr)
		if(ent->getchar()->base.points <= 0 || ent->getchar()->base.baseattrs[*attr] >= 100) return;

		ent->getchar()->base.points--;
		ent->getchar()->base.baseattrs[*attr]++;
	)

	ICOMMAND(r_invest_skill, "si", (const char *ref, int *skill),
		if(*skill < 0 || *skill >= SKILL_MAX) return;
		getreference(ref, ent, ent->getchar(), , r_invest_skill)
		if(ent->getchar()->base.points <= 0 || ent->getchar()->base.baseskills[*skill] >= 100) return;

		ent->getchar()->base.points--;
		ent->getchar()->base.baseskills[*skill]++;
	)

	ICOMMAND(r_givexp, "si", (const char *ref, int *n),
		getreference(ref, ent, ent->getchar(), , r_givexp)
		ent->getchar()->givexp(*n);
	)

	ICOMMAND(r_get_lastaction, "s", (const char *ref),
		getreference(ref, ent, ent->getchar() || ent->gettrigger(), intret(lastmillis), r_get_lastaction)

		if(ent->getchar()) intret(ent->getchar()->lastaction);
		else if(ent->gettrigger()) intret(ent->gettrigger()->lasttrigger);
	)

	ICOMMAND(r_get_charge, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_charge)
		intret(ent->getchar()->charge);
	)

	ICOMMAND(r_get_primary, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_primary)
		intret(ent->getchar()->primary);
	)

	ICOMMAND(r_get_secondary, "s", (const char *ref),
		getreference(ref, ent, ent->getchar(), intret(0), r_get_secondary)
		intret(ent->getchar()->secondary);
	)

	ICOMMAND(r_check_recipe, "ii", (int *ind, int *num),
		if(!game::recipes.inrange(*ind))
		{
			conoutf(CON_ERROR, "ERROR: r_check_recipe; recipe %i out of range", *ind);
			intret(0); return;
		}
		intret(game::recipes[*ind]->checkreqs(game::player1, max(1, *num)));
	)

	ICOMMAND(r_use_recipe, "ii", (int *ind, int *num),
		if(!game::recipes.inrange(*ind))
		{
			conoutf(CON_ERROR, "ERROR: r_use_recipe; recipe %i out of range", *ind);
			return;
		}
		game::recipes[*ind]->craft(game::player1, max(1, *num));
	)

	ICOMMAND(r_learn_recipe, "i", (int *ind),
		if(!game::recipes.inrange(*ind))
		{
			conoutf(CON_ERROR, "ERROR: r_learn_recipe; recipe %i out of range", *ind);
			return;
		}
		game::recipes[*ind]->flags |= recipe::KNOWN;
	)

	//dialogue
	//see also r_script_say in rpgconfig.cpp
	ICOMMAND(r_chat, "si", (const char *s, int *pos),
		getreference(s, speaker, speaker->getent(), , r_chat)
		if((speaker != talker || *pos == -1) && talker->ref) talker->setnull();

		if(scripts.inrange(speaker->getent()->getscript()))
		{
			talker->setref(speaker->getent());
			script *scr = scripts[talker->getent()->getscript()];
			scr->pos = clamp(scr->dialogue.length() - 1, -2, *pos);
			if(scr->pos == -1) talker->setnull();
			else if(pos >= 0)
			{
				scr->dialogue[scr->pos]->close();
				scr->dialogue[scr->pos]->open();
				if(!scr->dialogue[scr->pos]->dests.length())
				{
					conoutf("ERROR: dialogue node %i of script %i has no destinations; closing", scr->pos, talker->getent()->getscript());
					scr->pos = -1;
				}
			}
		}
		else
			conoutf(CON_ERROR, "ERROR: invalid script %i, can't initialise dialogue", talker->getent()->getscript());
	)

	ICOMMAND(r_response, "sis", (char *t, int *d, char *s),
		if(!talker->getent()) { conoutf(CON_ERROR, "ERROR: r_response; no conversation in progress?"); return; }

		script *scr = scripts[talker->getent()->getscript()];
		if(scr->dialogue.inrange(scr->pos))
		{
			if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: added response for scr->dialogue[%i]: \"%s\" %i \"%s\"", scr->pos, t, *d, s);
			scr->dialogue[scr->pos]->dests.add(new response(t, *d, s));
		}
		else
			conoutf(CON_ERROR, "ERROR: currently in a non-existant dialogue - note pos must be >= 0");
	)

	// World Commands

	//ICOMMAND(r_resetmap, "s", (const char *ref), - resets contents status

	ICOMMAND(r_num_ents, "s", (const char *ref),
		getreference(ref, map, map->getmap(), intret(0), r_num_ents)

		intret(map->getmap()->objs.length());
	)

	ICOMMAND(r_on_map, "ss", (const char *map, const char *ent),
		getreference(map, mapref, mapref->getmap(), intret(0), r_on_map)
		getreference(ent, entref, entref->getent(), intret(0), r_on_map)

		intret(mapref->getmap()->objs.find(entref->getent()) >= 0);
	)

	#define SPAWNCOMMAND(type, con) \
		ICOMMAND(r_spawn_ ## type, "siiii", (const char *ref, int *attra, int *attrb, int *attrc, int *attrd), \
			getreference(ref, map, map->getmap(), , r_spawn_ ## type); \
			 \
			action_spawn *spawn = new action_spawn con; \
			if(map->getmap() == curmap) \
			{ \
				if(DEBUG_SCRIPT) conoutf("DEBUG: conditions met, spawning (" #type ") %i %i %i %i", *attra, *attrb, *attrc, *attrd); \
				spawn->exec(); \
				delete spawn; \
			} \
			else \
			{ \
				if(DEBUG_SCRIPT) conoutf("DEBUG: conditions met but on another map queueing (" #type ") %i %i %i %i", *attra, *attrb, *attrc, *attrd); \
				map->getmap()->loadactions.add(spawn); \
			} \
		)
	//tag, index, number, quantity
	SPAWNCOMMAND(item, (*attra, ENT_ITEM, *attrb, *attrc, max(1, *attrd)))
	//tag, index, number
	SPAWNCOMMAND(creature, (*attra, ENT_CHAR, *attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(obstacle, (*attra, ENT_OBSTACLE, *attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(container, (*attra, ENT_CONTAINER, *attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(platform, (*attra, ENT_PLATFORM, *attrb, *attrc, 1))
	//tag, index, number
	SPAWNCOMMAND(trigger, (*attra, ENT_TRIGGER, *attrb, *attrc, 1))

	#undef SPAWNCOMMAND

	ICOMMAND(r_teleport, "sis", (const char *ref, int *d, const char *m),
		getreference(ref, ent, ent->getent(), , r_teleport)

		mapinfo *destmap = *m ? accessmap(m) : curmap;

		if(ent->getent() == player1 && curmap != destmap)
		{
			if(DEBUG_SCRIPT) conoutf(CON_DEBUG, "DEBUG: teleporting player to map %s", m);
			destmap->loadactions.add(new action_teleport(ent->getent(), *d));
			load_world(m);
			return;
		}
		if(ent->getent() != player1)
		{
			if(DEBUG_SCRIPT) conoutf("DEBUG: moving creature to destination map");
			removereferences(ent->ref, false);
			destmap->objs.add(ent->getent());

			if(destmap != curmap)
			{
				if(DEBUG_SCRIPT) conoutf("DEBUG: different map, queueing teleport on map %s", m);
				destmap->loadactions.add(new action_teleport(ent->getent(), *d));
				return;
			}
		}

		if(DEBUG_SCRIPT) conoutf("DEBUG: teleporting creature to destination %d", *d);
		entities::teleport(ent->getent(), *d);
	)

	//commands for status effects

	ICOMMAND(r_get_status_group, "s", (const char *ref),
		getreference(ref, eff, eff->getaeffect() || eff->getveffect(), intret(-1), r_get_status_group)
		if(eff->getaeffect()) intret(eff->getaeffect()->group);
		else intret(eff->getveffect()->group);
	)

	ICOMMAND(r_get_status_owner, "ss", (const char *ref, const char *set),
		if(!*set) return;
		getreference(ref, eff, eff->getaeffect() || eff->getveffect(), , r_get_status_owner)
		reference *owner = searchstack(set, true);
		if(eff->getaeffect()) owner->setref(eff->getaeffect()->owner);
		else owner->setref(eff->getveffect()->owner);
	)

	ICOMMAND(r_get_status_num, "s", (const char *ref),
		getreference(ref, eff, eff->getaeffect() || eff->getveffect(), intret(0), r_get_status_num)
		if(eff->getaeffect()) intret(eff->getaeffect()->effects.length());
		else intret(eff->getveffect()->effects.length());
	)

	ICOMMAND(r_get_status, "si", (const char *ref, int *idx),
		getreference(ref, eff, eff->getaeffect() || eff->getveffect(), result("-1 0 0 0 0"), r_get_status_num)

		vector<status *> *seffects;
		if(eff->getaeffect()) seffects = &eff->getaeffect()->effects;
		else seffects = &eff->getveffect()->effects;

		if(!seffects->inrange(*idx)) return;

		status *s = (*seffects)[*idx];

		//should we return mode specific details? ala rpgconfig.cpp::r_status_get_effect
		defformatstring(str)("%i %i %i %i %f", s->type, s->strength, s->duration, s->remain, s->variance);
		result(str);
	)
	
	ICOMMAND(r_get_status_duration, "si", (const char *ref, int *idx),
		getreference(ref, eff, eff->getaeffect() || eff->getveffect(), intret(0), r_get_status_duration)
		vector<status *> &effects = eff->getaeffect() ? eff->getaeffect()->effects : eff->getveffect()->effects;
		int val = 0;
		
		if(*idx >= 0)
		{
			if(!effects.inrange(*idx))
			{
				val = 0;
				conoutf(CON_ERROR, "ERROR: r_get_status_duration, invalid index");
			}
			else 
				val = effects[*idx]->duration;
		}
		else loopv(effects)
		{
			val = max(val, effects[i]->duration);
		}
		
		if(DEBUG_VSCRIPT) conoutf("DEBUG: r_get_status_duration; returning %i for \"%s\" %i", val, ref, *idx);
		intret(val);
	)
		 
	ICOMMAND(r_get_status_remain, "si", (const char *ref, int *idx),
		getreference(ref, eff, eff->getaeffect() || eff->getveffect(), intret(0), r_get_status_remain)
		vector<status *> &effects = eff->getaeffect() ? eff->getaeffect()->effects : eff->getveffect()->effects;
		int val = 0;
		
		if(*idx >= 0)
		{
			if(!effects.inrange(*idx))
			{
				val = 0;
				conoutf(CON_ERROR, "ERROR: r_get_status_remain, invalid index");
			}
			else 
				val = effects[*idx]->remain;
		}
		else loopv(effects)
		{
			val = max(val, effects[i]->remain);
		}
		
		if(DEBUG_VSCRIPT) conoutf("DEBUG: r_get_status_remain; returning %i for \"%s\" %i", val, ref, *idx);
		intret(val);
	)

	ICOMMAND(r_sound, "ss", (const char *p, const char *r),
		if(*r)
		{
			getreference(r, ent, ent->getent(), , r_sound)
			playsoundname(p, &ent->getent()->o);
		}
		else
			playsoundname(p);
	)
}
