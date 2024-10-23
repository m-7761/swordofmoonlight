
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Runtime.InteropServices;

namespace Kf25
{
	[ComVisible(false)]
    public interface IMap
    {
        void init(Kf25.Engine e, Kf25.Frame a);
        void commit(Kf25.Frame a, Kf25.Frame b);
    }

	/*see Frame.cs
	[Guid("1d9d2874-fd9b-4159-bbd0-5bd1a4c8bdf6")]
	[ComVisible(true)]
	[ClassInterface(ClassInterfaceType.None)]
	[ProgId("Kf25.Frame")]*/
	public partial class Frame
    {
        public Frame()
        {
            _c = new UInt16[1024];

            _obj = new List<Kf25.Obj>(512);

            for(int i=512;i-->0;) _obj.Add(new Kf25.Obj());
        }        
	}

	/*see Frame.cs
    [Guid("5e16aa1f-ad54-4fa9-81cc-69f184834dc8")]
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    [ProgId("Kf25.Engine")]*/
    public partial class Engine
	{
		private List<Kf25.IMap> _maps;

		public Engine()
		{
			_oflags = new UInt16[512];

		    _maps = new List<Kf25.IMap>(8);
            _maps.Add(new Kf25.Map00());
            _maps.Add(new Kf25.Map01());
            _maps.Add(new Kf25.Map02());
            _maps.Add(new Kf25.Map03());
            _maps.Add(new Kf25.Map04());
            _maps.Add(new Kf25.Map05());
            _maps.Add(new Kf25.Map06());
            _maps.Add(new Kf25.Map07());            
		}

		public int Use(int Item)
		{
			return -1; //UNIMPLEMENTED
		}

		public void Change(SomEx.IFrame A, SomEx.IFrame B)
        {
			for(int i=512;i-->0;) _oflags[i] = 0;

            if(A.map<=7) _maps[A.map].init(this,(Kf25.Frame)A);
        }
		public void Commit(SomEx.IFrame A, SomEx.IFrame B)
        {
            if(A.map<=7)
			{
                _maps[A.map].commit((Kf25.Frame)A,(Kf25.Frame)B);                
            }
        }
	}
}