using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Kf25
{
    public partial class Map00 : Kf25.IMap
    {
		SomEx.IGame game;
		
		List<Kf25.Sound> tides;
		Kf25.Sound water_fall;
		Kf25.Sound small_fountain;

		void SoundScape(Kf25.Frame a)
		{
			float x = a.x/2, y = a.z/2, z = a.y;

			tides[0].x = Math.Min(45*2,x*2);
			tides[0].z = y*2;
			for(int i=tides.Count();i-->0;)
			tides[i].Loop(game,1,a.clock);
			water_fall.Loop(game,0.8f,a.clock);
			small_fountain.Loop(game,0.8f,a.clock);
		}

		public void init(Kf25.Engine e, Kf25.Frame a)
        {
			game = e.game;
			
			tides = new List<Sound>(6);
			int s = game.sound("0921.wav");
			tides.Add(new Sound(s,49.0f,0.0f,11.5f));
			tides.Add(new Sound(s,50,39,11.5f));
			tides.Add(new Sound(s,56,16,11.5f));
			tides.Add(new Sound(s,52,66,11.5f));
			tides.Add(new Sound(s,52,77,11.5f));
			tides.Add(new Sound(s,62,79,11.5f));
			water_fall = new Sound(game.sound("0923.wav"),76.0f,79.5f,12.75f);
			small_fountain = new Sound(game.sound("0930.wav"),61.5f,60.5f,12.5f);
        }

        public void commit(Kf25.Frame a, Kf25.Frame b)
		{
			SoundScape(a);
		}
    }
}
