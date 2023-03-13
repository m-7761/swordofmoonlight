
namespace SOM
{
#ifdef SOM_MENUS_INCLUDED
#ifndef SOM_ENUMS_INCLUDED_MENUS
#define SOM_ENUMS_INCLUDED_MENUS

#define SOM_MENUS_X //som.menus.inl

#define COUNTER(a,b) a##b
#define UNNAMED(a,b) COUNTER(a,b)

#define NAMESPACE(NAME)\
	namespace NAME\
	{\
		enum\
		{
#define NAMECLOSE\
		TOTAL_NODES\
		};\
	}

#define TEXT(X,f,e,...) e##_NODE,  
#define MENU(X,f,e,...) e##_NODE, 
#define STOP(X,f,e,...) e##_NODE, 
#define FLOW(X,c,...) UNNAMED(c,__LINE__/*__COUNTER__*/)_NODE,

#include "som.menus.inl"

#define NAMESPACE(NAME)\
	namespace NAME\
	{\
		enum\
		{
#define NAMECLOSE\
		TOTAL_ELEMENTS\
		};\
	}

#define TEXT(X,f,e,...) 
#define MENU(X,f,e,...) e##_ELEMENT, 
#define STOP(X,f,e,...) e##_ELEMENT, 
#define FLOW(X,c,...) UNNAMED(c,__LINE__/*__COUNTER__*/)_ELEMENT,

#include "som.menus.inl"

#define COUNTER(a,b) a##b
#define UNNAMED(a,b) COUNTER(a,b)

#define NAMESPACE(NAME)\
	namespace NAME\
	{\
		enum\
		{
#define NAMECLOSE\
		TOTAL_TEXT\
		};\
	}

#define TEXT(X,f,e,...) e##_TEXT,  
#define MENU(X,f,e,...) e##_TEXT, 
#define STOP(X,f,e,...) e##_TEXT, 
#define FLOW(X,c,...) UNNAMED(c,__LINE__/*__COUNTER__*/)_TEXT,

#include "som.menus.inl"

#undef COUNTER
#undef UNNAMED

#undef TEXT
#undef MENU
#undef STOP
#undef FLOW
#undef NAMECLOSE
#undef NAMESPACE

#undef SOM_MENUS_X
		 
///////////////////////////////////
///// formerly of som.menus.h /////
///////////////////////////////////

#define SOM_MENUS_X //som.menus.inl

	enum
	{
	PSEUDO_MENUS = 0,

#define MENU(...)
#define STOP(...)
#define TEXT(...)
#define FLOW(...)

#define NAMESPACE(NAME)	NAME##_NS,
#define NAMECLOSE

#include "som.menus.inl"

	TOTAL_MENUS
	};

#undef NAMECLOSE
#define NAMESPACE(NAME)\
	namespace NAME\
	{\
		enum{ NS=SOM::NAME##_NS };

#define MENU(X,f,e,x,...) extern SOM::Menu::Elem x;
#define STOP(X,f,e,x,...) extern SOM::Menu::Elem x;
#define TEXT(X,f,e,x,...) extern SOM::Menu::Text x;
#define FLOW(...)				 

#include "som.menus.inl"
		
#undef MENU
#undef STOP
#undef TEXT
#undef FLOW
#undef NAMECLOSE
#undef NAMESPACE

#undef SOM_MENUS_X 

///////////////////////////////////

#endif //SOM_ENUMS_INCLUDED_MENUS
#endif //SOM_MENUS_INCLUDED

}//SOM