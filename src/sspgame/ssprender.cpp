#include "sspgame.h"

namespace game
{
	void rendergame(bool mainpass)
	{
		startmodelbatches();

		loopvrev(sspobjs)
			sspobjs[i]->render();

		loopvrev(entities::items)
			entities::items[i]->render();

		entities::renderentities();

		renderprojs();
		endmodelbatches();

		if(mainpass && debug)
		{
			loopv(sspobjs)
			{
				sspent *o = sspobjs[i];
				defformatstring(ds)("%p", o);
				particle_textcopy(o->abovehead(), ds, PART_TEXT, 5, 0xFFFFFF, 4);
			}
			loopv(entities::items)
			{
				sspitem *o = entities::items[i];
				defformatstring(ds)("%p", o);
				particle_textcopy(vec(o->o).add(vec(0, 0, 4)), ds, PART_TEXT, 5, 0xFFFFFF, 4);
			}
		}
	}
}
