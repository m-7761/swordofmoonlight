using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Kf25
{
	class Bridge //Wind Bridge
	{
		public UInt32 time = 0;

		public bool back = false;

		public int start = 0; 

		public UInt32[] steps;

		public Bridge(int st, int sz)
		{
			start = st; steps = new UInt32[sz];			
		}

		public bool build(UInt32 a, UInt32 b)
		{
			if(0==time) return false;

			UInt32 step = (UInt32)((a-time)/750.0f);

			if(step<steps.Length)
			{
				if(back) step = (UInt32)steps.Length-1-step;

				steps[step] = 4000; //DUPLICATE
			}

			UInt32 d = a-b, z = 0;

			for(int i=steps.Length;i-->0;)
			{
				steps[i]-=Math.Min(steps[i],d);

				if(0==steps[i]) z++;
			}

			if(z==steps.Length) time = 0;

			return true;
		}

		public void summon(float dx, float dy, UInt32 ctime, int dir)
		{
			if(Math.Abs(dx)<=0.5&&Math.Abs(dy)<=0.5)
			{
				if(time==0) //REMOVE ME
				{
					time = ctime; back = dir<0; 
				}
			}

		}
	}
}
