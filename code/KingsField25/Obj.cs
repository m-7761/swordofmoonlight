using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Runtime.InteropServices;

namespace Kf25
{
    [Guid("19372e46-4c20-4692-82ac-0d25e0cb1c40")]
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    [ProgId("Kf25.Obj")]
    public class Obj : SomEx.IObj
    {
        private int _profile = 0;

		private float _spawn = 0;

        private float _x = 0, _y = 0, _z = 0;
        private float _u = 0, _v = 0, _w = 0;

        public int profile
        {
            get { return _profile; }
            set { _profile = value; }
        }
		public float spawn
        {
            get { return _spawn; }
            set { _spawn = value; }
        }
        public float x
        {
            get { return _x; }
            set { _x = value; }
        }
        public float y
        {
            get { return _y; }
            set { _y = value; }
        }
        public float z
        {
            get { return _z; }
            set { _z = value; }
        }
        public float u
        {
            get { return _u; }
            set { _u = value; }
        }
        public float v
        {
            get { return _v; }
            set { _v = value; }
        }
        public float w
        {
            get { return _w; }
            set { _w = value; }
        }
    }
}
