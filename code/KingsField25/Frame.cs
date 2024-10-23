
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Runtime.InteropServices;

namespace SomEx //COM interfaces
{
	//This has to be run as Admin to export the assembly
	//so it can be consumed by C++ Visual Studio projects
	//It's roughly equivalent to "Register for COM interop"
	//C:\Windows\Microsoft.NET\Framework\v4.0.30319\RegAsm.exe /tlb /codebase "C:\Users\Michael\Projects\KingsField25\bin\Debug\KingsField25.dll"

	[Guid("038c7e1f-b7c5-4b3a-afed-057991bd347f")]
    [ComVisible(true)]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IGame
    {
		int sound(string filename);

        float PlaySound(int sound, float x=0, float y=0, float z=0, int pitch=0, int volume=0);
    }

    [Guid("96f5c793-d564-41a7-bc4a-cf264718a0ce")]
    [ComVisible(true)]
    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IFrame
    {
        UInt32 clock { get; set; } //milliseconds

        UInt16[] counters { get; set;/*don't work*/ }
     
        int map { get; set; }
		int sky { get; set; }
        float x { get; set; }
        float y { get; set; }
        float z { get; set; }
        float v { get; set; }

        SomEx.IObj obj(int i);

		void set_counter(int i, UInt16 c);
    }

	[Guid("bcfde24d-b748-31a3-9784-f1ca722d96cb")]
    [ComVisible(true)]
    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IObj
    {
        int profile { get; set; }

		float spawn { get; set; }

        float x { get; set; }
        float y { get; set; }
        float z { get; set; }
        float u { get; set; }
        float v { get; set; }
        float w { get; set; }
    }

	[Guid("ce209beb-4935-4c66-9932-dae1c6929d3f")]
    [ComVisible(true)]
    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IEngine
    {
		IGame game { get; set; }

        UInt16[] objflags { get; }

		void Change(SomEx.IFrame A, SomEx.IFrame B);
        void Commit(SomEx.IFrame A, SomEx.IFrame B);		
		
		//return -1 to ignore, or a caption
		//to display
		int Use(int Item);
    }
}
namespace Kf25
{
	[Guid("5e16aa1f-ad54-4fa9-81cc-69f184834dc8")]
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    [ProgId("Kf25.Engine")]
    public partial class Engine : SomEx.IEngine
	{	
		private SomEx.IGame _game = null;

		private UInt16[] _oflags;	
		
		public SomEx.IGame game
        {
            get { return _game; } set { _game = value; }
        }
        public UInt16[] objflags
        {
            get { return _oflags; }
        }
	}

    [Guid("1d9d2874-fd9b-4159-bbd0-5bd1a4c8bdf6")]
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    [ProgId("Kf25.Frame")]
    public partial class Frame : SomEx.IFrame
    {
        private UInt32 _clock = 0;

		private UInt16[] _c;
		
        private int _map = 0;

		private int _sky = 0;

        private float _x = 0, _y = 0, _z = 0;
        private float _v = 0;

        private List<Kf25.Obj> _obj;

        public UInt32 clock
        {
            get { return _clock; } set { _clock = value; }
        }				
        public UInt16[] counters
        {
            get { return _c; } set { _ = value; }
        }
        public int map
        {
            get { return _map; } set { _map = value; }
        }
		public int sky
        {
            get { return _sky; } set { _sky = value; }
        }
        public float x
        {
            get { return _x; } set { _x = value; }
        }
        public float y
        {
            get { return _y; } set { _y = value; }
        }
        public float z
        {
            get { return _z; } set { _z = value; }
        }
        public float v
        {
            get { return _v; } set { _v = value; }
        }

        public SomEx.IObj obj(int i) { return _obj[i]; }

		public void set_counter(int i, UInt16 c){ _c[i] = c; }		
    }
}
