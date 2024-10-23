using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Kf25
{
    public partial class Map01 : Kf25.IMap
    {
        public void init(Kf25.Engine e, Kf25.Frame a)
        {
        }

        public void commit(Kf25.Frame a, Kf25.Frame b)
		{			
			float x = a.x/2, y = a.z/2, z = a.y;

			//BLUE SKY

			//sky[1] = if(neg(if(neg(50-x),70,79)-y,12-z,40-x,x-56,0),4,1)
			if(y>(x>50?70:79))
			{			
				if(z>12&&x>40&&x<56) a.sky = 1;
			}

		}

    }
}
