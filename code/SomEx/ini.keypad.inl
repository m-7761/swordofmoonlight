

//#define TURBO(delay,key,dik) turbo_keys[0x##key] = delay;

TURBO(16,1,ESCAPE) //do_escape

//5 is too much for Master Volume (16 is too little)
//TURBO(5,0c,MINUS)
TURBO(10,0c,MINUS)	   //These are currently assigned
TURBO(10,0d,EQUALS)   //for scrolling thru memory as
TURBO(10,1a,LBRACKET) //part of the output overlay 
TURBO(10,1b,RBRACKET) //interface

TURBO(16,3d,F3) //zoom

TURBO(1,47,NUMPAD7)  //movement assignments
TURBO(1,48,NUMPAD8)
TURBO(1,49,NUMPAD9)
TURBO(1,4b,NUMPAD4)
TURBO(1,4c,NUMPAD5)
TURBO(1,4d,NUMPAD6)
TURBO(1,4f,NUMPAD1)
TURBO(1,50,NUMPAD2)
TURBO(1,51,NUMPAD3)

TURBO(1,c8,UP)	   //movement assignments
TURBO(1,cb,LEFT)
TURBO(1,cd,RIGHT)
TURBO(1,d0,DOWN)

//TURBO(1,??,)
//TURBO(1,??,LEFT)
//TURBO(1,??,RIGHT)
//TURBO(1,??,DOWN)

TURBO(1,38,LMENU)
TURBO(1,B8,RMENU)

#undef TURBO
