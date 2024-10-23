using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Kf25
{
	public partial class Map02 : Kf25.IMap
	{
		int[] steps = //(46)
		{
			//castle steps 1 (17)
			92,41,53,56,57,58,59,60,61,62,63,64,
			78,79,80,91,108,
			//castle steps 2 (22)
			65,66,67,72,73,75,93,94,95,96,97,264,
			262,263,218,105,103,102,101,100,99,98,
			//castle steps 3 (4)
			115,118,119,111,
			//corridor steps (3)
			42,43,124
		};

		Kf25.Bridge[] bridges = 
		{
			new Kf25.Bridge(0,17),
			new Kf25.Bridge(17,22),
			new Kf25.Bridge(39,4),
			new Kf25.Bridge(43,3)
		};
		
		public void init(Kf25.Engine e, Kf25.Frame a)
		{
			for(int i=steps.Length;i-->0;)
			{
				int o = steps[i];

				e.objflags[o] = 1;

				a.obj(o).spawn = 0;
			}
		}

		public void commit(Kf25.Frame a, Kf25.Frame b)
		{
			float x = a.x/2, y = a.z/2, z = a.y;

			//BLUE SKY

			if(z>(x<12?16:19))
			{			
				if(x<=46&&y<=(x<12?46:40))
				{
					a.sky = 1;				
				}
			}

			//BRIDGES

			//show any active wind pillar bridges
			for(int i=4;i-->0;)
			if(bridges[i].build(a.clock,b.clock))
			{
				int s = bridges[i].start;
				int n = bridges[i].steps.Length;
				for(int j=0;j<n;j++)
				{
					int o = steps[s+j];

					if(bridges[i].steps[j]!=0)
					{
						double t = bridges[i].steps[j]/4000.0f; //DUPLICATE

						t = Math.Cos(t*Math.PI*2)/2+0.5;

						a.obj(o).spawn = 1-(float)(t*t*t);
					}
					else a.obj(o).spawn = 0;
				}
			}
			bridges[0].summon(x-32,y-65,a.clock,+1);
			bridges[1].summon(x-34,y-85,a.clock,+1);
			bridges[1].summon(x-53,y-85,a.clock,-1);
			bridges[2].summon(x-57,y-83,a.clock,+1);
			bridges[2].summon(x-62,y-83,a.clock,-1);
			bridges[3].summon(x-66,y-36,a.clock,+1);
			bridges[3].summon(x-70,y-36,a.clock,-1);
		}
	}
}
