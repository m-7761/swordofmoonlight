using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Kf25
{
	class Sound //Emitter/Source
	{
		private int snd;

		public float x,y,z;

		private UInt32 time;

		public Sound(int sound, float xx, float yy, float zz)
		{
			snd = sound; x = xx*2; y = zz; z = yy*2; time = 0;
		}

		public float Play(SomEx.IGame g)
		{
			return g.PlaySound(snd,x,y,z); //pitch? //volume?
		}

		public void Loop(SomEx.IGame g, float repeat, UInt32 ctime)
		{
			if(time<ctime)
			{
				time = ctime+(UInt32)(1000*repeat*Play(g));
			}
		}		
	}
}
