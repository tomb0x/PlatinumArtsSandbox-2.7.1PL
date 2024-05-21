#include "rpggame.h"

namespace game
{
	void g3d_gamemenus()
	{

	}

	VARP(projallowlight, 0, 1, 1);
	void adddynlights()
	{
		if(!curmap) return;

		if(projallowlight)
		{
			loopv(curmap->projs)
				curmap->projs[i]->dynlight();
		}
		loopv(curmap->objs)
		{
			vec4 &light = curmap->objs[i]->temp.light;
			int radius = light.w;
			vec col = vec(light.x, light.y, light.z);

			if(light.magnitude() > 4)
				adddynlight(curmap->objs[i]->abovehead(), radius, col);
		}
	}

	void quad(vec ur, vec ul, vec ll, vec lr)
	{
		glBegin(GL_TRIANGLE_FAN);
		glTexCoord2f(0, 0); glVertex3f(ur.x, ur.y, ur.z);
		glTexCoord2f(1, 0); glVertex3f(ul.x, ul.y, ul.z);
		glTexCoord2f(1, 1); glVertex3f(ll.x, ll.y, ll.z);
		glTexCoord2f(0, 1); glVertex3f(lr.x, lr.y, lr.z);
		glEnd();
	}

	VARP(editdrawgame, 0, 0, 1);

	void rendergame(bool mainpass)
	{
		startmodelbatches();

		if(editmode)
		{
			entities::renderentities();
			if(isthirdperson())
				player1->render(mainpass);
		}

		if(!editmode || editdrawgame)
		{
			loopv(curmap->objs)
			{
				if(camera::cutscene ? (curmap->objs[i] != camera::attached ||
					fabs(camera::camera.o.x - curmap->objs[i]->o.x) > curmap->objs[i]->radius ||
					fabs(camera::camera.o.y - curmap->objs[i]->o.y) > curmap->objs[i]->radius
				) : (curmap->objs[i] != player1 || isthirdperson()))
				{
					curmap->objs[i]->render(mainpass);

					if(DEBUG_ENT && mainpass)
					{
						defformatstring(ds)("%p", curmap->objs[i]);
						particle_textcopy(curmap->objs[i]->abovehead(), ds, PART_TEXT, 1, 0xFFFFFF, 4);
					}
				}
			}

			loopv(curmap->projs)
			{
				curmap->projs[i]->render(mainpass);

				if(DEBUG_PROJ && mainpass)
				{
					defformatstring(ds)("%p", curmap->projs[i]);
					vec pos = vec(0, 0, 6).add(curmap->projs[i]->o);
					particle_textcopy(pos, ds, PART_TEXT, 1, 0xFFFFFF, 4);
				}
			}

			if(mainpass)
			{
				loopv(curmap->aeffects)
					curmap->aeffects[i]->render();
			}
		}

		if(mainpass && DEBUG_AI)
		{
			ai::renderwaypoints();
		}

		endmodelbatches();
	}

	void renderavatar()
	{
		if(editmode) return;
		use_armour *left = NULL, *right = NULL;
		loopv(player1->equipped)
		{
			use_armour *cur = (use_armour *) items[player1->equipped[i]->base]->uses[player1->equipped[i]->use];
			if(cur->slots & SLOT_LHAND)
				left = cur;
			if(cur->slots & SLOT_RHAND)
				right = cur;
		}

		//TODO attach hands
		//TODO place models at correct positions
		//TODO animations
		//TODO tags for particle emissions
		//for now just get it working
		if(left == right)
		{
			if(!left || !left->hudmdl) return;

			rendermodel(NULL, left->hudmdl, ANIM_GUN_IDLE|ANIM_LOOP, player1->o, player1->yaw+90, player1->pitch, player1->roll, MDL_LIGHT|MDL_HUD, NULL, NULL, player1->lastaction, 500);
		}
		else
		{
			if(left && left->hudmdl)
			{
				rendermodel(NULL, left->hudmdl, ANIM_GUN_IDLE|ANIM_LOOP, player1->o, player1->yaw+90, player1->pitch, player1->roll, MDL_LIGHT|MDL_HUD, NULL, NULL, player1->lastaction, 500);
			}
			if(right && right->hudmdl)
			{
				rendermodel(NULL, left->hudmdl, ANIM_GUN_IDLE|ANIM_LOOP, player1->o, player1->yaw+90, player1->pitch, player1->roll, MDL_LIGHT|MDL_HUD, NULL, NULL, player1->lastaction, 500);
			}
		}
	}
}