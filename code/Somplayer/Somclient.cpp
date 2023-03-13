							
#include "Somplayer.pch.h"

#include "Somclient.h"
#include "Somcontrol.h"
#include "Somtexture.h"
#include "Somgraphic.h"
#include "Somenviron.h"

using namespace Somscribe_h; 

//private
Somclient::Somclient(Somconsole *c)
{
	(Somconsole*&)console = c; //blacklist 

	c->source(c->title(),soft_reset); environ = 0; 
	
	ep = Moonlight; id = 0; //classic game client

	event = CreateThread(0,0,ep,this,0,&id);
}

//private
Somclient::~Somclient()
{
	assert(!console&&!environ);
}

//static
Somclient *Somclient::main(Somconsole *c)
{
	if(c) c->client->exit(); else return 0;
		
	return c->client = new Somclient(c);
}

int Somclient::exit()
{
	if(!this||!console) return 0;

	assert(console->client==this);
					   	
	//scheduled obsolete
	TerminateThread(event,0); 
	while(GetExitCodeThread(event,&id)==STILL_ACTIVE) Sleep(1);
	CloseHandle(event);
	
	Sominstant *cosmos = environ->cosmos();

	environ->salt(); environ = 0; delete cosmos;

	console->client = 0; console = 0; 
	
	//todo: plug memory leaks!!

	delete this; return 0; 
}

const Somenviron *Somclient::watch(const wchar_t *item, int n)
{					
	if(!environ) 
	{			
		extern size_t Somscribe_f(Sominstant*,int*,size_t,float*,size_t);
		extern size_t Somscribe_d(Sominstant*,int*,size_t,double*,size_t);

		environ = Somenviron::open(); environ->ascribe(Somscribe_f,Somscribe_d);

		environ->submit(new Som<cosmos>,&Somenviron::cosmos);
	}									 

	const Somgraphic *g = console->mdo(item); 
			
	if(item[SLOT]==UNDEFINED) 
	{
		Som<wizard> *wiz = new Som<wizard>;

		if(g->fill(console->slot(console->field,item,g->height),&wiz,1))
		{
			wiz->identity<4,4>(); //TODO: g->place(map start point)

			wiz->o.z = 5; //testing: take a step back
		}
		else assert(0);
	}
	
	Sominstant *i = g->instant(item[SLOT]);
	Som<wizard> *wiz = dynamic_cast<Som<wizard>*>(i);  		
	Som<cyclops> *eye = Somscribe::detail<Som<cyclops>*>(4,wiz);
	
	Somenviron *out = 0;

	if(eye) for(int i=0;i<eye->number;i++) 
	{
		if(eye[i].zzZ)
		{	
			if(out=environ->admit(eye+i))
			{
				out->submit(wiz,&Somenviron::viewer);

				eye[i].tripod = 1.5; assert(item[TYPE]==PLAYER);
				
				eye[i].orient(wiz); Somscribe::disturb(eye+i);
			}
		}
		else continue; assert(out);
		
		break;
	}

	return out;
}

void Somclient::blind(const Somenviron*)
{
	assert(0); //unimplemented
}

int Somclient::frame(const Somenviron *env)
{
	const Som<cosmos> *now = env->cosmos()->cast_support_magic(now);

	return now?now->frame:0;
}

bool Somclient::shape(const Somenviron *env, const Somtexture *target)
{
	const Sominstant *viz = env->viewer(); 
	const Sominstant *pov = env->point_of_view(); 

	Som<wizard> *wiz = viz->cast_support_magic(wiz);
	Som<cyclops> *eye = wiz->cast_support_magic(eye);

	if(eye) for(int i=0;i<eye->number;i++) //paranoia
	{
		if(pov!=eye+i||eye->zzZ) continue; //paranoia
		
		float aspect = float(target->width)/target->height;

		if(fabs(eye->aspect-aspect)>0.00003)
		{		
			eye->aspect = aspect; eye->orient(wiz); return true;
		}
		else return true;
	}

	return false;
}

bool Somclient::layer(int l, Somenviron *eye, const Somtexture *dst, int ops)
{
	if(l!=THE_MAIN_CONTENT) return false; //testing
			   	
	const Somgraphic *g = 0; //1 graphic (testing)

	for(int i=1,n=console->field[META][SIZE];i<n;i++) 
	{
		if(console->id(console->field[i],0,GRAPHIC))
		{
			if(g=console->mdo(console->field[i])) 
			{
				g->release(); break; //testing...
			}
		}
	}
	
	if(g&&!g->height) //memory leak
	{					
	  //todo: have already done this

		Sominstant **ls = 0; 

		if(g->list(&ls,1))
		{
			int groups = g->group(0);

			Som<wizard> *wiz = new Som<wizard>;

			Somscribe::disturb(wiz,1,groups-1);

			for(size_t i=1;i<g->width;i++)
			{
				ls[i] = &wiz->table[g->group(i)-1]; //grouping
			}

			ls[0] = wiz; wiz->identity<4,4>(); 
		}

		g->place(0,ls,1); //hack
	}

	Som<cyclops> *pov = 
	eye->point_of_view()->cast_support_magic(pov);

	if(!pov) return false;

	//prevent a partial read of the matrix
	Somthread_h::readlock rd = pov->blink;

	if(dst->apply_state(eye,dst->Z)) 
	{
	  //todo: loop thru graphics here

		dst->clear(); 
		dst->apply(g);		
		dst->apply_state(eye,dst->Z|dst->BLEND);
		dst->apply(g);		
		dst->apply_state(0);
	}
	else return false; //???

	return true; //bullshit 		
}

namespace Somclient_h
{
	struct posture //input accumulation
	{	
		float speed; bool speeding, recenter;

		bool moving, turning, looking, pulling;

		float move[3], turn[3], look[3], pull[1];

		inline operator bool() //omitting speeding
		{
			return moving||turning||looking||pulling||recenter;
		}

		Somconsole *console; Somcontrol *control; bool power;

		posture(Somconsole *c)
		{
			memset(this,0x00,sizeof(*this));
			
			console = c; speed = 1;
		}

		bool ready(Somcontrol *c)
		{
			control = c; return power = c->power(); 
		}
				
		void operator()(int id, float bt)
		{				
			if(id==ID_MOVE_SPEED)
			{				
				if(speeding=power) 
				speed = std::max(bt*3+1,1.0f); return;				
			}			

			if(bt<0) return; //wrong way

			//scheduled obsolete
			const float base_tolerance = 0.3;
			
		#define POW bt>base_tolerance

			switch(id)
			{
			case ID_POWER_ON: 				

				control->press(control->POWER,POW); 
				break;

			case ID_POWER_RTC:
				
				if(control->press(control->PAUSE,POW))
				{
					if(!console->pause()) console->unpause();
				}
				break;

			case ID_POWER_MICE: case ID_POWER_KEYS: 

				for(size_t i=0;i<console->ports;i++)
				{
					Somcontrol *port = console->control(i);

					if(port&&port->portholder==control->portholder)
					{
						if(!port->system()) continue; 

						if(port->action_count()) //assuming mouse
						{
							if(id==ID_POWER_MICE) port->press(port->POWER,POW);
						}
						else if(id==ID_POWER_KEYS) port->press(port->POWER,POW);
					}
					else if(!port) break;
				}			
				break;

			case ID_POWER_BELL: 
				
				if(control->press(control->BELL,POW))
				{
					if(power) MessageBeep(-1); //TODO: mute and visual					
				}
				break;
			}

			if(!power) return;

		#undef POW

			const float step = 0.02;
			const float rads = step*3.141592/6;

			if((bt-=base_tolerance)<=0) return; 

		#define MOVE(n,_) moving=(move[n]_=bt*step); break;
		#define TURN(n,_) turning=(turn[n]_=bt*rads); break;
		#define LOOK(n,_) looking=(look[n]_=bt*rads); break;
		#define PULL(n,_) pulling=(pull[n]_=bt*step); break;		

			switch(id)
			{
			//For the record these matchup
			//with SOM's coordinate system

			case ID_MOVE_LEFT: MOVE(0,-)
			case ID_MOVE_RIGHT: MOVE(0,+)
			case ID_MOVE_FORWARD: MOVE(2,-)
			case ID_MOVE_BACKWARD: MOVE(2,+)

			case ID_MOVE_UP: MOVE(1,+)
			case ID_MOVE_DOWN: MOVE(1,-)

			case ID_TURN_LEFT: TURN(1,-) //+
			case ID_TURN_RIGHT: TURN(1,+) //-

			case ID_ROLL_LEFT: TURN(2,-) //+
			case ID_ROLL_RIGHT: TURN(2,+) //-
			case ID_ROLL_FORWARD: TURN(0,+) //-
			case ID_ROLL_BACKWARD: TURN(0,-) //+

			case ID_LOOK_UP: LOOK(1,-) //+
			case ID_LOOK_DOWN: LOOK(1,+) //-
			case ID_LOOK_LEFT: LOOK(0,-) //+
			case ID_LOOK_RIGHT: LOOK(0,+) //-

			case ID_LOOK_FORWARD: 
				
				recenter = true; break;

			case ID_PULL_UP: PULL(0,-)
			case ID_PULL_BACK: PULL(0,+)
			}

		#undef MOVE
		#undef TURN
		#undef LOOK
		#undef PULL
		}
	};
};

//static 
DWORD WINAPI Somclient::Moonlight(LPVOID c)
{		
	//scheduled obsolete
	bool still_frame = true; 

	//todo: might need a stop watch

	Somclient *client = (Somclient*)c; 

	Somconsole *console = client->console;

	while(!client->event);
	SetThreadPriority(client->event,THREAD_PRIORITY_TIME_CRITICAL);

	if(1) //// a work in progress... ////

	//TODO: gracefully handle disconnects???
	for(Somplayer *p=console->party;1;p=**p)
	{
		Somclient_h::posture accum(console);

		for(int i=0;i<Somconsole::ports;i++)
		{
			if(!console->controls[i]) break;

			if(console->controls[i]->portholder==p->id)
			{
				Somcontrol *control = console->controls[i];

				Somcontrol_h::context *c = control->contexts;

				const float *a = control->actions();
				const float *b = control->buttons();
				
				//// TODO: buffered approach //////////

				size_t a_s = control->action_count_s();
				size_t b_s = control->button_count_s();
				
				//todo: synchronize with the loop's period
				if(accum.ready(control)) control->capture();

				for(size_t i=0;i<a_s;i++) 
				{
					if(c->actions[i]) accum(c->actions[i],a[i]);

					if(c->reactions[i]) accum(c->reactions[i],-a[i]);
				}	 
				for(size_t i=0;i<b_s;i++) 
				{
					if(c->buttons[i]) accum(c->buttons[i],b[i]);
				}
			}
		}

		if(accum&&p->character) 
		{
			//TODO: lock the player!?

			int slot = p->character[SLOT]; 

			const wchar_t *id = 
			console->id(p->character,p->controller,GRAPHIC);

			const Somgraphic *g = console->graphics[id?id[ITEM]:0];	

			Som<wizard> *wiz = g->instant<Som<wizard>*>(slot);
			Som<skeleton> *pos = client->detail<Som<skeleton>*>(wiz);

			//update the character's posture...

			typedef Somvector::vector<float,3> float3; if(!pos) continue;

			if(accum.moving) //stored as a point 
			{
				if(accum.speeding) //ID_MOVE_SPEED
				float3::map(accum.move).se_scale<3>(accum.speed); 

				pos->move.move<3>(float3::map(accum.move).rotate<3>(pos->turn));
			}
			if(accum.turning) //stored as a quaternion
			{
				if(accum.speeding) if(accum.speed>1) //hack: shape turn rate
				{
					float3::map(accum.turn).se_scale<3>((accum.speed-1)*0.5+1); 
				}
				else float3::map(accum.turn).se_scale<3>(1.0f-(1-accum.speed)*0.5); 

				//Reminder: it might be worth doing this in an order-independent way
				Sominstant_h::vector q; pos->turn.premultiply(q.quaternion(accum.turn)); 

				pos->turn.unit<4>(); //todo: every so often
			}
			if(accum.recenter)
			{
				pos->look.se_copy<3>(0); //ID_LOOK_FORWARD
			}
			else if(accum.looking) //stored as Euler angles
			{
				pos->look.move<3>(accum.look).limit<3>(3.141592/2);
			}
			
			pos->standup(); //rebuild matrix

			g->place(slot,&pos,1); //update avatar

			//update the character's cameras...

			Som<cyclops> *eye = wiz->cast_support_magic(eye); 

			if(eye) for(int i=0;i<eye->number;i++) 
			{
				if(!eye[i].zzZ) eye[i].orient(wiz,eye+i); 
			}

			still_frame = false;
		}

		if(**p==console->party)
		{
			if(!still_frame) 
			{
				still_frame = true;

				if(client->environ) //universal cosmos
				{
					Sominstant *universe = client->environ->cosmos();

					Som<cosmos> *now = universe->cast_support_magic(now);

					if(now) now->frame++;
				}				
			}

			Sleep(10);
		}
	}

	return 0; //TODO: SOMPLAYER exit codes
}