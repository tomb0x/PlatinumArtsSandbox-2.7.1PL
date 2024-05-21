#include "rpggame.h"

victimeffect::victimeffect(rpgent *o, inflict *inf, int chargeflags, float mul) : owner(o)
{
	group = inf->status;
	elem = game::statuses[group]->friendly ? ATTACK_NONE : inf->element;
	mul *= inf->mul;

	statusgroup *sg = game::statuses[inf->status];
	loopv(sg->effects)
	{
		status *st = effects.add(sg->effects[i]->dup());
		st->remain = st->duration;

		if(chargeflags & CHARGE_MAG) st->strength *= mul;
		if(chargeflags & CHARGE_DURATION) st->remain *= mul;
	}
}

bool victimeffect::update(rpgent *victim)
{
	int resist = 0;
	int thresh = 0;
	if(victim->type() == ENT_CHAR)
	{
		rpgchar *d = (rpgchar *) victim;
		resist = d->base.getresistance(elem);
		thresh = d->base.getthreshold(elem);
	}

	loopvrev(effects)
	{
		effects[i]->update(victim, owner, resist, thresh);

		if(effects[i]->duration >= 0 && (effects[i]->remain -= curtime) <= 0)
		{
			delete effects.remove(i);
		}
	}

	return effects.length();
}

bool areaeffect::update()
{
	loopvj(game::curmap->objs)
	{
		//checking the head and the feet should be sufficiently accurate for gaming purposes
		rpgent *victim = game::curmap->objs[j];
		if(victim->o.dist(o) <= radius || victim->feetpos().dist(o) <= radius)
		{
			int resist = 0;
			int thresh = 0;
			if(victim->type() == ENT_CHAR)
			{
				rpgchar *d = (rpgchar *) victim;
				resist = d->base.getresistance(elem);
				thresh = d->base.getthreshold(elem);
			}

			loopvrev(effects)
				effects[i]->update(victim, owner, resist, thresh);

		}
	}

	loopvrev(effects)
	{
		if(effects[i]->duration >= 0 && (effects[i]->remain -= curtime) <= 0)
		{
			delete effects.remove(i);
		}
	}

	return effects.length();
}

void areaeffect::render()
{
	if(!game::effects.inrange(fx)) return;
	effect *e = game::effects[fx];

	switch(e->particle)
	{
		default:
			if((lastmillis - lastemit) >= emitmillis)
			{
				e->drawsphere(o, radius, e->size, effect::DEATH_PROLONG, lastmillis - lastemit);
				lastemit = lastmillis;
			}
			break;

		case PART_EXPLOSION:
		case PART_EXPLOSION_BLUE:
			e->drawsphere(o, radius, e->size, effect::DEATH_PROLONG, 0);
			break;
	}
}

void status_generic::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	//time based multiplier, dwindles on last 20% of duration
	int strength = this->strength < 0 ? min(0, this->strength + thresh) : max(0, this->strength - thresh);
	float mult = (mul + extra * variance) * (100 - resist) / 100.f;
	if(duration > 0)
		mult *= clamp<float>(1, 0, 5.f * remain/duration);

	switch(victim->type())
	{
		case ENT_CHAR:
		{
			rpgchar *ent = (rpgchar *) victim;
			//instant applicable types
			switch(type)
			{
				case STATUS_HEALTH:
					ent->health += strength * mult * (duration != 0 ? curtime / 1000.f : 1);
					if(strength < 0 && ent->health < 0) ent->die(owner);
					return;

				case STATUS_MANA:
					ent->mana += strength * mult * (duration != 0 ? curtime / 1000.f : 1);
					ent->mana = max<float>(ent->mana, 0);
					return;

				case STATUS_DOOM:
					//either instant death, or to cater to duration == -1 a gradual percentage wise drain of health
					if(duration == 0 || (ent->health -= ent->base.getmaxhp() * strength * mult * curtime / 100000.f) < 0)
						ent->die(owner);
					return;
			}
			if(duration == 0) break;

			//the rest
			switch(type)
			{
				case STATUS_MOVE:
					ent->base.deltamovespeed += strength * mult;
					ent->base.deltajumpvel += strength * mult;
					return;

				case STATUS_CRIT:
					ent->base.deltacrit += strength * mult;
					return;

				case STATUS_HREGEN:
					ent->base.deltahregen += strength * mult;
					return;

				case STATUS_MREGEN:
					ent->base.deltamregen += strength * mult;
					return;

				case STATUS_STRENGTH:
				case STATUS_ENDURANCE:
				case STATUS_AGILITY:
				case STATUS_CHARISMA:
				case STATUS_WISDOM:
				case STATUS_INTELLIGENCE:
				case STATUS_LUCK:
					ent->base.deltaattrs[type - STATUS_STRENGTH] += strength * mult;
					return;

				case STATUS_ARMOUR:
				case STATUS_DIPLOMANCY:
				case STATUS_MAGIC:
				case STATUS_MARKSMAN:
				case STATUS_MELEE:
				case STATUS_STEALTH:
				case STATUS_CRAFT:
					ent->base.deltaskills[type - STATUS_ARMOUR] += strength * mult;
					return;

				case STATUS_FIRE_T:
				case STATUS_WATER_T:
				case STATUS_AIR_T:
				case STATUS_EARTH_T:
				case STATUS_MAGIC_T:
				case STATUS_SLASH_T:
				case STATUS_BLUNT_T:
				case STATUS_PIERCE_T:
					ent->base.deltathresh[type - STATUS_FIRE_T] += strength * mult;
					return;

				case STATUS_FIRE_R:
				case STATUS_WATER_R:
				case STATUS_AIR_R:
				case STATUS_EARTH_R:
				case STATUS_MAGIC_R:
				case STATUS_SLASH_R:
				case STATUS_BLUNT_R:
				case STATUS_PIERCE_R:
					ent->base.deltaresist[type - STATUS_FIRE_R] += strength * mult;
					return;

				case STATUS_STUN:
					///WRITE ME, immobilise; don't allow movement or attacks with somatic components
				case STATUS_SILENCE:
					///WRITE ME, prevent attacks with a vocal component, special speech path for talking to NPCs too?
					return;
			}
		}
		case ENT_CONTAINER:
		{
			rpgcontainer *ent = (rpgcontainer *) victim;
			switch(type)
			{
				case STATUS_LOCK:
					if(ent->lock == 101) return; //special case, no unlocking

					if(-strength * mult > ent->lock) ent->lock = 0; //unlock!
					else if(strength * mult > ent->lock) ent->lock = min<int>(100, strength * mult); //lock!

					return;
				case STATUS_MAGELOCK: //magelock
					if(!duration != 0)
						ent->magelock += strength * mult;
					return;
			}
		}
	}

	//generic types, note that unhandled types are a) specialise or b) not handled by that entiy
	switch(type)
	{
		case STATUS_DISPEL:
			///WRITE ME; select a random status effect every update and weaken it
			return;

		case STATUS_REFLECT:
			///WRITE ME; what do?
			return;

		case STATUS_INVIS:
			victim->temp.alpha -= strength * mult / 100.f;
			victim->temp.alpha = clamp<float>(1, 0, victim->temp.alpha);
			break;
	}
}

void status_polymorph::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	if(duration == 0) return;

	victim->temp.mdl = mdl;
}

void status_light::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	if(duration == 0) return;

	int strength = this->strength < 0 ? min(0, this->strength + thresh) : max(0, this->strength - thresh);
	float mult = (mul + extra * variance) * (100 - resist) / 100.f;
	if(duration > 0)
		mult *= clamp<float>(1, 0, 5.f * remain/duration);

	victim->temp.light.add(vec4(colour, strength * mult));
}

VAR(eff_strength, 1, -1, -1);
VAR(eff_remain, 1, -1, -1);
VAR(eff_mult, 1, -1, -1);
VAR(eff_duration, 1, -1, -1);

void status_signal::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	eff_strength = strength < 0 ? min(0, strength + thresh) : max(0, strength - thresh);
	eff_remain = remain;
	eff_duration = duration;
	eff_mult = (mul + extra * variance) * (100 - resist) / 100.f;

	victim->getsignal(signal, true, owner);
}

void status_script::update(rpgent *victim, rpgent *owner, int resist, int thresh, float mul, float extra)
{
	if(!script) return;

	if(!code) code = compilecode(script);
	eff_strength = strength < 0 ? min(0, strength + thresh) : max(0, strength - thresh);
	eff_remain = remain;
	eff_duration = duration;
	eff_mult = (mul + extra * variance) * (100 - resist) / 100.f;

	rpgscript::doentscript(victim, owner, code);
}