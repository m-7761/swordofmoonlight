
//TODO: convert this file/workflow over to UTF-8 strings

/*ATTENTION*/
//
// This file contains Shift_JIS text, care must be taken 
// when saving it so it retains its ANSI (932) codepage!

//notes: * means match anything (and maybe nothing also)
//       < and > are the chosen delimiters because these 
//       characters should always mean the same thing in 
//       ASCII and Shift_JIS. This notation is useful to 
//       unambiguously qualify certain menus by their ID
//       0 means SOM uses TextOutA and 1 means DrawTextA

//NEW: <, >, and $ are being used to adjust the built-in
//	   English so that abbreviations can be avoided. But
//     $ (center mid screen") is also applied to all text
//     so that do_fix_widescreen_font_height can be used
//     (reminder: < and > will mess up HTML like markup)

#ifdef SOM_MENUS_X //som.menus.h/cpp
#undef SOM_MENUS_X

#define SOM_MAIN_MENU NAMESPACE(MAIN)	
#include "som.menus.inl"
#undef SOM_MAIN_MENU
	#define SOM_ITEM_MENU NAMESPACE(ITEM)	
	#include "som.menus.inl"
	#undef SOM_ITEM_MENU
	#define SOM_CAST_MENU NAMESPACE(CAST)
	#include "som.menus.inl"
	#undef SOM_CAST_MENU
	#define SOM_EQUIP_MENU NAMESPACE(EQUIP)
	#include "som.menus.inl"
	#undef SOM_EQUIP_MENU
		#define SOM_WEAPON_MENU NAMESPACE(WEAPON)
		#include "som.menus.inl"
		#undef SOM_WEAPON_MENU
		#define SOM_HEAD_MENU NAMESPACE(HEAD)
		#include "som.menus.inl"
		#undef SOM_HEAD_MENU
		#define SOM_BODY_MENU NAMESPACE(BODY)
		#include "som.menus.inl"
		#undef SOM_BODY_MENU
		#define SOM_HANDS_MENU NAMESPACE(HANDS)
		#include "som.menus.inl"
		#undef SOM_HANDS_MENU
		#define SOM_FEET_MENU NAMESPACE(FEET)
		#include "som.menus.inl"
		#undef SOM_FEET_MENU
		#define SOM_SHIELD_MENU NAMESPACE(SHIELD)
		#include "som.menus.inl"
		#undef SOM_SHIELD_MENU
		#define SOM_ACCESSORY_MENU NAMESPACE(ACC)
		#include "som.menus.inl"
		#undef SOM_ACCESSORY_MENU
		#define SOM_MAGIC_MENU NAMESPACE(MAGIC)
		#include "som.menus.inl"
		#undef SOM_MAGIC_MENU
	#define SOM_STATS_MENU NAMESPACE(STATS)
	#include "som.menus.inl"
	#undef SOM_STATS_MENU
	#define SOM_STORE_MENU NAMESPACE(STORE)
	#include "som.menus.inl"
	#undef SOM_STORE_MENU
	#define SOM_SYSTEM_MENU NAMESPACE(SYS)
	#include "som.menus.inl"
	#undef SOM_SYSTEM_MENU
		#define SOM_LOAD_MENU NAMESPACE(LOAD)
		#include "som.menus.inl"
		#undef SOM_LOAD_MENU
		#define SOM_SAVE_MENU NAMESPACE(SAVE)
		#include "som.menus.inl"
		#undef SOM_SAVE_MENU
		#define SOM_OPTION_MENU NAMESPACE(OPTION)
		#include "som.menus.inl"
		#undef SOM_OPTION_MENU
		#define SOM_CONFIG_MENU NAMESPACE(CONFIG)
		#include "som.menus.inl"
		#undef SOM_CONFIG_MENU
		#define SOM_QUIT_MENU NAMESPACE(QUIT)
		#include "som.menus.inl"
		#undef SOM_QUIT_MENU
		
#define SOM_TAKE_MENU NAMESPACE(TAKE)
#include "som.menus.inl"
#undef SOM_TAKE_MENU

#define SOM_INFO_MENU NAMESPACE(INFO)
#include "som.menus.inl"
#undef SOM_INFO_MENU

#define SOM_SHOP_MENU NAMESPACE(SHOP)
#include "som.menus.inl"
#undef SOM_SHOP_MENU

#define SOM_SELL_MENU NAMESPACE(SELL)
#include "som.menus.inl"
#undef SOM_SELL_MENU

#define SOM_ASCII_MENU NAMESPACE(ASCII)
#include "som.menus.inl"
#undef SOM_ASCII_MENU

#define SOM_MENUS_X
#else //...

//notes: Essentially MENU are links to menus or at least exit
//       the current menu. STOP are up/down keyboard stops that
//       do not link to new menus. TEXT is just displayed text.
//       FLOW is different control codes... 
//
//case: A case signals a branch in a pattern. Each case block
//      is a different branch which will be followed if the 
//      text of the first MENU/STOP/TEXT is matched. A case
//      block can end either with 'cont' signaling control is
//      back to normal. Or by 'none', which also signals non- 
//      match is acceptable and control should continue anyway.
//cond: cond is short for conditional continue and can end an 
//      'item' block. If after the 'item' block a match can be
//      made against the subsequent text the item block will
//		exit. 'case' FLOW elements will be evaluated in turn
//      however the 'none' FLOW element will be ignored for
//      purposes of evaluating the conditional clause.
//      See 'item' and 'cont' for more details.
//cont: cont is short for continue and ends an 'item' block.
//      See 'item' for details, and 'cond' also.
//done: done like 'none' ends a 'case' block. done requires one
//      of the cases be matched (none does not.")
//      See 'case' for details.
//item: This describes an element in a repeating list such as 
//      an inventory selection menu. item blocks end with the 
//      'cont' FLOW code which signals where the item pattern 
//      repeats. See 'cond' (conditional continue") also.
//none: none ends a case block while also signaling a default
//      non-case is an acceptable match and control should 
//      continue regardless. None by itself can also signal a
//      matching menu could not be found upon request.
//trap: A 'trap' signals that keyboard control no longer has
//      access to the previous menu, and is trapped inside a
//      new sub-menu overlayed atop the original typically by  
//      a fullscreen black tint.

#undef X
#define _(X) X

#ifdef SOM_MAIN_MENU
SOM_MAIN_MENU
#define X MAIN
	MENU(_(X),0,MAIN,Main,"���C�����j���[","Menu")
	MENU(_(X),0,ITEM,Item,"�A�C�e���g�p","Use Item")
	MENU(_(X),0,CAST,Cast,"�⏕���@�r��","Work Magic")
	MENU(_(X),0,EQUIP,Equip,"�����ύX","Equip")
	MENU(_(X),0,STATS,Stats,"�ڍ׃X�e�[�^�X�Q��","View Summary")
	MENU(_(X),0,STORE,Store,"�A�C�e������","Arrange Inventory")
	MENU(_(X),0,SYS,Sys,"�V�X�e��","System")	
#endif

#ifdef SOM_ITEM_MENU
SOM_ITEM_MENU
#define X ITEM
	MENU(_(X),0,ITEM,Item,"�A�C�e���g�p\4�A�C�e���g�p","Use Item")	
	FLOW(_(X),item,item selection...)
	MENU(_(X),0,MENU,Menu,"*","*")
	TEXT(_(X),1,HAVE,Have,"x%3d","x%3d")
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"%s���g�p���܂����H","Use %s?") 	
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_CAST_MENU
SOM_CAST_MENU
#define X CAST
	MENU(_(X),0,CAST,Cast,"�⏕���@�r��\4�⏕���@�r��","Work Magic")
	FLOW(_(X),item,spell selection...)	
	MENU(_(X),0,MENU,Menu,"*","*")
	FLOW(_(X),case,hack: cost/magic description)
	TEXT(_(X),1,COST,Cost,"���@\4%3d","%3d")
	FLOW(_(X),none,hack: see case^)
	FLOW(_(X),cond,conditional continue)
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"%s���r�����܂����H","Use %s?") 	
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_EQUIP_MENU
SOM_EQUIP_MENU
#define X EQUIP			   
	MENU(_(X),0,EQUIP,Equip,"�����ύX\4�����ύX","Equip")	
	MENU(_(X),0,WEAPON,Weapon,"����","Weapon")	
	MENU(_(X),0,HEAD,Head,"��","Head")	
	MENU(_(X),0,BODY,Body,"�Z","Body")	
	MENU(_(X),0,HANDS,Hands,"�U��","Hands")	
	MENU(_(X),0,FEET,Feet,"�","Feet")	
	MENU(_(X),0,SHIELD,Shield,"��","Shield")	
	MENU(_(X),0,ACC,Acc,"�����i","Accessory")
	MENU(_(X),0,MAGIC,Magic,"���@","Magic")
	TEXT(_(X),0,EDGE,Edge,"����\4�a","<<Edge")
	TEXT(_(X),0,AREA,Area,"����\4��","<Area")
	TEXT(_(X),0,POINT,Point,"����\4�h","<<Point")
	TEXT(_(X),0,FIRE,Fire,"����\4��","<<Fire")
	TEXT(_(X),0,EARTH,Earth,"����\4�y","<Earth")	
	TEXT(_(X),0,WIND,Wind,"����\4��","<<Wind")
	TEXT(_(X),0,WATER,Water,"����\4��","<Water")
	TEXT(_(X),0,HOLY,Holy,"����\4��","<<Holy")	
	FLOW(_(X),case,nothing/something equipped)
	TEXT(_(X),0,NONE,None,"�����Ȃ�            ","Nothing")
	FLOW(_(X),case,in case som_game_nothing is full up)
	TEXT(_(X),0,NONE2,None2,"�����Ȃ�","Nothing")
	FLOW(_(X),case,something...)
	TEXT(_(X),0,NAME,Name,"����\4%s","%s") //"%-20.20s"
	FLOW(_(X),done,one or the other)
	FLOW(_(X),case,attack or defense)
	TEXT(_(X),0,ATTACK,Attack,"����\4�U����","")
	FLOW(_(X),case,...)
	TEXT(_(X),0,DEFEND,Defense,"����\4�h���","")
	FLOW(_(X),none,magic displays neither)
	FLOW(_(X),case,length of a weapon)
	TEXT(_(X),0,RADIUS,Radius,"����\4����","<<Radius")
	TEXT(_(X),1,_RADIUS,_Radius,"����\4%3.1fcm","<<<<%4.0fcm")
	TEXT(_(X),0,WEIGHT,Weight,"����\4�d��","<<Weight\nAttack")
	TEXT(_(X),1,_WEIGHT,_Weight,"����\4%2.1fKg","<<<<%2.1fkg")
	FLOW(_(X),none,only weapons have length)
	FLOW(_(X),case,weight of equipment)
	TEXT(_(X),0,WEIGHT2,Weight2,"����\4�d��","<<Weight\nDefense")
	TEXT(_(X),1,_WEIGHT2,_Weight2,"����\4%2.1fKg","<<<<%2.1fkg")
	FLOW(_(X),none,magic does not have weight)
	FLOW(_(X),case,not used by Nothing)
	TEXT(_(X),1,_EDGE,_Edge,"�����a\4%3d","%3d")				
	TEXT(_(X),1,_AREA,_Area,"������\4%3d",">%3d")
	TEXT(_(X),1,_POINT,_Point,"�����h\4%3d","%3d")
	TEXT(_(X),1,_FIRE,_Fire,"������\4%3d","%3d")
	TEXT(_(X),1,_EARTH,_Earth,"�����y\4%3d",">%3d")	
	TEXT(_(X),1,_WIND,_Wind,"������\4%3d","%3d")
	TEXT(_(X),1,_WATER,_Water,"������\4%3d",">%3d")
	TEXT(_(X),1,_HOLY,_Holy,"������\4%3d","%3d")		
	FLOW(_(X),none,continued from Nothing)
	TEXT(_(X),0,WT,Wt,"�����d��","<Equipped")
	TEXT(_(X),1,_WT,_Wt,"%4.1fKg",">%3.1fkg") 
	TEXT(_(X),0,CAP,Cap,"�����\�d��","<Capacity")
	TEXT(_(X),1,_CAP,_Cap,"%4.1fKg",">%3.1fkg")
#endif

#ifdef SOM_WEAPON_MENU
SOM_WEAPON_MENU
#define X WEAPON
	MENU(_(X),0,WEAPON,Weapon,"�����ύX�F����","Weapon")
#endif
#ifdef SOM_HEAD_MENU
SOM_HEAD_MENU
#define X HEAD
	MENU(_(X),0,HEAD,Head,"�����ύX�F��","Head")	
#endif
#ifdef SOM_BODY_MENU
SOM_BODY_MENU
#define X BODY
	MENU(_(X),0,BODY,Body,"�����ύX�F�Z","Body")	
#endif
#ifdef SOM_HANDS_MENU
SOM_HANDS_MENU
#define X HANDS
	MENU(_(X),0,HANDS,Hands,"�����ύX�F�U��","Hands")	
#endif
#ifdef SOM_FEET_MENU
SOM_FEET_MENU
#define X FEET
	MENU(_(X),0,FEET,Feet,"�����ύX�F�","Feet")	
#endif
#ifdef SOM_SHIELD_MENU
SOM_SHIELD_MENU
#define X SHIELD
	MENU(_(X),0,SHIELD,Shield,"�����ύX�F��","Shield")	
#endif
#ifdef SOM_ACCESSORY_MENU
SOM_ACCESSORY_MENU
#define X ACC
	MENU(_(X),0,ACC,Acc,"�����ύX�F�����i","Accessory")	
#endif
#ifdef SOM_MAGIC_MENU
SOM_MAGIC_MENU
#define X MAGIC
	MENU(_(X),0,MAGIC,Magic,"�����ύX�F���@","Magic")	
#endif

#if defined(SOM_WEAPON_MENU)\
||	defined(SOM_HEAD_MENU)\
||	defined(SOM_BODY_MENU)\
||	defined(SOM_HANDS_MENU)\
||	defined(SOM_FEET_MENU)\
||	defined(SOM_SHIELD_MENU)\
||	defined(SOM_ACCESSORY_MENU)\
||	defined(SOM_MAGIC_MENU)
	FLOW(_(X),item,item/magic selection...)
	MENU(_(X),0,MENU,Menu,"*","*") 
#ifdef SOM_MAGIC_MENU	
	FLOW(_(X),case,hack: cost/magic description)
	TEXT(_(X),1,COST,Cost,"���@\4%3d","%3d")
	FLOW(_(X),none,hack: see case^)
#endif	
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),case,attack/defense/none)
#ifdef SOM_MAGIC_MENU	
	TEXT(_(X),0,ATTACK,Attack,"�U����",">Attack")
#else
#ifdef SOM_WEAPON_MENU	
	TEXT(_(X),0,ATTACK,Attack,"�U����",">Attack")
	TEXT(_(X),0,RADIUS,Radius,"����","<Radius")
	TEXT(_(X),1,_RADIUS,_Radius,"%3.1fcm","%4.0fcm") 
#else
	TEXT(_(X),0,DEFEND,Defense,"�h���",">Defense")
#endif
	TEXT(_(X),0,WEIGHT,Weight,"�d��","<Weight")
	TEXT(_(X),1,_WEIGHT,_Weight,"%2.1fKg",">%2.1fkg") 
#endif
	TEXT(_(X),0,EDGE,Edge,"�a","<<<Edge")
	TEXT(_(X),1,_EDGE,_Edge,"�a\4%3d","<%3d")
	TEXT(_(X),0,AREA,Area,"��","<Area")
	TEXT(_(X),1,_AREA,_Area,"��\4%3d",">%3d")
	TEXT(_(X),0,POINT,Point,"�h","<<<Point")
	TEXT(_(X),1,_POINT,_Point,"�h\4%3d","<%3d")
	TEXT(_(X),0,FIRE,Fire,"��","<<<Fire")
	TEXT(_(X),1,_FIRE,_Fire,"��\4%3d","<%3d")
	TEXT(_(X),0,EARTH,Earth,"�y","<Earth")	
	TEXT(_(X),1,_EARTH,_Earth,"�y\4%3d",">%3d")	
	TEXT(_(X),0,WIND,Wind,"��","<<<Wind")
	TEXT(_(X),1,_WIND,_Wind,"��\4%3d","<%3d")
	TEXT(_(X),0,WATER,Water,"��","<Water")
	TEXT(_(X),1,_WATER,_Water,"��\4%3d",">%3d")
	TEXT(_(X),0,HOLY,Holy,"��","<<<Holy")
	TEXT(_(X),1,_HOLY,_Holy,"��\4%3d","<%3d")			  			  	
	FLOW(_(X),none,...)
	FLOW(_(X),trap,confirmation dialog...)
	#if !defined(SOM_MAGIC_MENU)
 	TEXT(_(X),0,OK,Ok,"%s�𑕔����܂����H","Use %s?")	
	#else
	TEXT(_(X),0,OK,Ok,"���@\4%s�𑕔����܂����H","Use %s?")	
	#endif
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_STATS_MENU
SOM_STATS_MENU
#define X STATS
	MENU(_(X),0,STATS,Stats,"�ڍ׃X�e�[�^�X","Summary") 
#endif

#ifdef SOM_STORE_MENU
SOM_STORE_MENU
#define X STORE
	MENU(_(X),0,STORE,Store,"�A�C�e������\4�A�C�e������","Inventory")	
	FLOW(_(X),item,stored items...)
  //hack: MENU really does not capture the behavior here...	
	MENU(_(X),0,MENU,Menu,"*","*") //"%-16.16s"
	TEXT(_(X),1,HAVE,Have,"x%3d","x%3d")
	FLOW(_(X),cond,conditional continue)	
  ////%s has been added to these////////	
	FLOW(_(X),case,put away)
	TEXT(_(X),0,HIDE,Hide,"%s��Еt���܂����H","Pack %s?")
	FLOW(_(X),case,take out)
	TEXT(_(X),0,SHOW,Show,"%s�����o���܂����H","Unpack %s?")	
	FLOW(_(X),done,one or the other)
  //////////////////////////////////////
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")		
#endif
	
#ifdef SOM_SYSTEM_MENU
SOM_SYSTEM_MENU
#define X SYS
	MENU(_(X),0,SYS,Sys,"�V�X�e��\4�V�X�e��","System")
	MENU(_(X),0,LOAD,Load,"�f�[�^���[�h","Load Game")
	MENU(_(X),0,SAVE,Save,"�f�[�^�Z�[�u","Save Game")
	MENU(_(X),0,OPTION,Option,"�I�v�V�����ݒ�","Options")
	MENU(_(X),0,CONFIG,Config,"�p�b�h�R���t�B�O","Controls")
	MENU(_(X),0,QUIT,Quit,"�Q�[���I��","Leave Game")
#endif

#ifdef SOM_LOAD_MENU
SOM_LOAD_MENU
#define X LOAD
	MENU(_(X),0,LOAD,Load,"�f�[�^���[�h\4�f�[�^���[�h","Load Game")
	FLOW(_(X),item,saved game selection...)
	MENU(_(X),0,DATA,Data,"�f�[�^%02d","SAVE %02d")	
	TEXT(_(X),0,DATE,Date,"���t","")
	TEXT(_(X),1,_DATE,_Date,"%4d�N%2d��%2d��","%2$02d/%3$02d/%1$4d")
	TEXT(_(X),0,EXP,Exp,"�f�[�^\4�o���l","EP")	
	TEXT(_(X),1,_EXP,_Exp,"�o���l\4%6d","%6d")
	TEXT(_(X),0,TIME,Time,"������","Time")	
	TEXT(_(X),1,_TIME,_Time,"%4dh%02dm%02ds","%4d:%02d:%02d")
	FLOW(_(X),cont,continue for each saved game)
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"���[�h���܂����H","Load?")	
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_SAVE_MENU
SOM_SAVE_MENU
#define X SAVE
	MENU(_(X),0,SAVE,Save,"�f�[�^�Z�[�u\4�f�[�^�Z�[�u","Save Game")
	FLOW(_(X),item,saved game slot selection...)
	FLOW(_(X),case,data or no data)
	MENU(_(X),0,FREE,Free,"�f�[�^%02d�@�@�@�@�f�[�^�Ȃ�","SAVE %02d        No Data")
	FLOW(_(X),case,data exists...) 
	MENU(_(X),0,DATA,Data,"�f�[�^%02d","SAVE %02d")
	TEXT(_(X),0,DATE,Date,"���t","")
	TEXT(_(X),1,_DATE,_Date,"%4d�N%2d��%2d��","%2$02d/%3$02d/%1$4d")
	TEXT(_(X),0,EXP,Exp,"�f�[�^\4�o���l","EP")	
	TEXT(_(X),1,_EXP,_Exp,"�o���l\4%6d","%6d")
	TEXT(_(X),0,TIME,Time,"������","Time")	
	TEXT(_(X),1,_TIME,_Time,"%4dh%02dm%02ds","%4d:%02d:%02d")
	FLOW(_(X),cont,continue for each saved game)
	FLOW(_(X),trap,confirmation dialog...)
	FLOW(_(X),case,overwrite/save)
	TEXT(_(X),0,OW,Ow,"�㏑�����܂����H","Overwrite?")
	FLOW(_(X),case,new save...)
	TEXT(_(X),0,OK,Ok,"�Z�[�u���܂����H","Save?")	
	FLOW(_(X),done,...)
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_OPTION_MENU
SOM_OPTION_MENU
#define X OPTION
	MENU(_(X),0,OPTION,Option,"�I�v�V�����ݒ�\4�I�v�V�����ݒ�","Options")
	STOP(_(X),0,DEV,Dev,"�f�o�C�X","") //Device
	STOP(_(X),0,RES,Res,"�𑜓x","Resolution")
	STOP(_(X),0,COLOR,Color,"�F��","Colors")
	STOP(_(X),0,FILTER,Filter,"�e�N�X�`�����[�h","Texture Mode")
	STOP(_(X),0,BRIGHT,Bright,"���邳����","Brightness")
	STOP(_(X),0,PAD,Pad,"�p�b�h�I��","Game Controller")
	STOP(_(X),0,BGM,Bgm,"�a�f�l�{�����[��","Music Volume")
	STOP(_(X),0,SOUND,Sound,"���ʉ��{�����[��","Sound Volume")
	STOP(_(X),0,HUD,Hud,"�e��Q�[�W�\��","Stamina Display")
	STOP(_(X),0,NAV,Nav,"�����\��","Compass Display")
	//2022: I think this Japanese is okay for this?
//	STOP(_(X),0,INV,Inv,"�A�C�e���\��","Inventory Icons")
STOP(_(X),0,INV,Inv,"�A�C�e���\��","Examine Display")
	STOP(_(X),0,BOB,Bob,"���s����","Stepping Motion")
	TEXT(_(X),1,_DEV,_Dev,"*WS*","*")
	TEXT(_(X),1,_RES,_Res,"%4d x %4d","%4d x %4d")
	TEXT(_(X),1,_COLOR,_Color,"%2d bpp","%2d bpp")
	TEXT(_(X),1,_FILTER,_Filter,"*WS*","*")
	FLOW(_(X),case,joypad if present)
	TEXT(_(X),1,_PAD,_Pad,"�p�b�h�I��\4(%d)","(%d)")
	//Reminder: "PadID: " is a hack used to identify SOM_CONFIG_MENU
	TEXT(_(X),0,PADID,PadID,"�p�b�h�I��\4PadID: %s","%s") //%-42.42s
	FLOW(_(X),case,joypad if present)
	TEXT(_(X),1,PADXX,PadXX,"�p�b�h�Ȃ�","None Found")
	FLOW(_(X),done,...)
	TEXT(_(X),1,_BGM,_Bgm,"�a�f�l�{�����[��\4%d","%d")
	TEXT(_(X),1,_SOUND,_Sound,"���ʉ��{�����[��\4%d","%d")
	TEXT(_(X),1,_BRIGHT,_Bright,"���邳����\4%d","%d")
    TEXT(_(X),0,_HUD,_Hud,"�e��Q�[�W�\��\4*WS*","*")
    TEXT(_(X),0,_NAV,_Nav,"�����\��\4*WS*","*")
    TEXT(_(X),0,_INV,_Inv,"�A�C�e���\��\4*WS*","*")
    TEXT(_(X),0,_BOB,_Bob,"���s����\4*WS*","*")
    FLOW(_(X),trap,confirmation dialog...)
    FLOW(_(X),case,...)
	TEXT(_(X),0,OK,Ok,"���̐ݒ�ł�낵���ł����H","Use these settings?")	
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
	FLOW(_(X),case,messages...)
	TEXT(_(X),0,DISPLAYERR,DisplayERR,"�w�肳�ꂽ�𑜓x�ł͕\���ł��܂���ł���","Could not display at selected resolution")
	FLOW(_(X),case,...)
	TEXT(_(X),0,GAMMAERR,GammaERR,"�K���}�ݒ�s��","Could not adjust brightness")
#endif

#if defined(SOM_MAIN_MENU)||defined(SOM_SYSTEM_MENU)||\
    defined(SOM_STATS_MENU)
	TEXT(_(X),0,LEVEL,Level,"���x��","Level")
	TEXT(_(X),0,CLASS,Class,"�N���X","Class")
	TEXT(_(X),0,HP,Hp,"�g�o","HP")
	TEXT(_(X),0,MP,Mp,"�l�o","MP")
	TEXT(_(X),0,POW,Pow,"�̗�","Strength")
	TEXT(_(X),0,MAG,Mag,"����","Magic")	   
	TEXT(_(X),0,GOLD,Gold,"�S�[���h","Gold")
	TEXT(_(X),0,EXP,Exp,"�o���l","EP")
	TEXT(_(X),0,MODE,Mode,"���","") 
	TEXT(_(X),0,TIME,Time,"�o�ߎ���","Time")	
#endif
	
#ifdef SOM_STATS_MENU
	TEXT(_(X),0,EST,Est,"�]���U����","Attack")   
	TEXT(_(X),0,EST2,Est2,"�]���h���","Defense\nApproach")
	TEXT(_(X),0,ATTACK,Attack,"�ڍ�\4�U����","")
	TEXT(_(X),0,DEFENSE,Defense,"�ڍ�\4�h���","")
	TEXT(_(X),0,EDGE,Edge,"�ڍ�\4�a","Edge")
	TEXT(_(X),0,AREA,Area,"�ڍ�\4��","Area")
	TEXT(_(X),0,POINT,Point,"�ڍ�\4�h","Point")
	TEXT(_(X),0,FIRE,Fire,"�ڍ�\4��","Fire")
	TEXT(_(X),0,EARTH,Earth,"�ڍ�\4�y","Earth")	
	TEXT(_(X),0,WIND,Wind,"�ڍ�\4��","Wind")
	TEXT(_(X),0,WATER,Water,"�ڍ�\4��","Water")	
	TEXT(_(X),0,HOLY,Holy,"�ڍ�\4��","Holy")	
#endif

#if defined(SOM_MAIN_MENU)||defined(SOM_SYSTEM_MENU)||\
    defined(SOM_STATS_MENU)
	TEXT(_(X),1,_LEVEL,_Level,"%2d","%2d")
	TEXT(_(X),1,_CLASS,_Class,"�N���X\4%s","%s") //%15.15s
	TEXT(_(X),1,_HP,_Hp,"�g�o\4%3d�^%3d","%3d/%3d")
	TEXT(_(X),1,_MP,_Mp,"�l�o\4%3d�^%3d","%3d/%3d")
	TEXT(_(X),1,_STR,_Str,"�̗�\4%3d","%3d")
	TEXT(_(X),1,_MAG,_Mag,"����\4%3d","%3d")
	TEXT(_(X),1,_GOLD,_Gold,"�S�[���h\4%5d","%5d")
	TEXT(_(X),1,_EXP,_Exp,"�o���l\4%6d","%6d")
	TEXT(_(X),1,_MODE,_Mode,"*WS*","*")
	TEXT(_(X),1,_TIME,_Time,"%4dh%02dm%02ds","%4d:%02d:%02d")
#endif

#ifdef SOM_STATS_MENU
	TEXT(_(X),1,_EDGE,_Edge,"�a�X\4%3d",">%3d")
	TEXT(_(X),1,_EDGE2,_Edge2,"�a�X\4%4d",">%4d")
	TEXT(_(X),1,_AREA,_Area,"���X\4%3d",">%3d")
	TEXT(_(X),1,_AREA2,_Area2,"���X\4%4d",">%4d")
	TEXT(_(X),1,_POINT,_Point,"�h�X\4%3d",">%3d")
	TEXT(_(X),1,_POINT2,_Point2,"�h�X\4%4d",">%4d")
	TEXT(_(X),1,_FIRE,_Fire,"�΁X\4%3d",">%3d")
	TEXT(_(X),1,_FIRE2,_Fire2,"�΁X\4%4d",">%4d")
	TEXT(_(X),1,_EARTH,_Earth,"�y�X\4%3d",">%3d")	
	TEXT(_(X),1,_EARTH2,_Earth2,"�y�X\4%4d",">%4d")
	TEXT(_(X),1,_WIND,_Wind,"���X\4%3d",">%3d")
	TEXT(_(X),1,_WIND2,_Wind2,"���X\4%4d",">%4d")
	TEXT(_(X),1,_WATER,_Water,"���X\4%3d",">%3d")	
	TEXT(_(X),1,_WATER2,_Water2,"���X\4%4d",">%4d")	
	TEXT(_(X),1,_HOLY,_Holy,"���X\4%3d",">%3d")	
	TEXT(_(X),1,_HOLY2,_Holy2,"���X\4%4d",">%4d")	
	TEXT(_(X),1,_EST,_Est,"�]���U����\4%5.1f","%5.1f")
	TEXT(_(X),1,_EST2,_Est2,"�]���h���\4%5.1f","%5.1f")
#endif

#ifdef SOM_SYSTEM_MENU	
	FLOW(_(X),trap,messages...)		
	TEXT(_(X),0,LOADERR,LoadERR,"�Z�[�u�f�[�^������܂���","There is no saved game")
#endif

#ifdef SOM_CONFIG_MENU			
SOM_CONFIG_MENU
#define X CONFIG
	//Reminder: "PadID: " is a hack used to identify this menu
	TEXT(_(X),0,PAD,PadID,"�p�b�h�R���t�B�O\4PadID: %s","%s")	//%-42.42s
	TEXT(_(X),1,_PAD,_PadID,"�p�b�h�R���t�B�O\4(%d)","(%d)")
	MENU(_(X),0,CONFIG,Config,"�p�b�h�R���t�B�O\4�p�b�h�R���t�B�O","Controller")	
	STOP(_(X),0,CFG1,Cfg1,"�{�^���P","Button 1")
	STOP(_(X),0,CFG2,Cfg2,"�{�^���Q","Button 2") 
	STOP(_(X),0,CFG3,Cfg3,"�{�^���R","Button 3") 
	STOP(_(X),0,CFG4,Cfg4,"�{�^���S","Button 4") 
	STOP(_(X),0,CFG5,Cfg5,"�{�^���T","Button 5") 
	STOP(_(X),0,CFG6,Cfg6,"�{�^���U","Button 6") 
	STOP(_(X),0,CFG7,Cfg7,"�{�^���V","Button 7") 
	STOP(_(X),0,CFG8,Cfg8,"�{�^���W","Button 8") 
	TEXT(_(X),0,_CFG1,_Cfg1,"�p�b�h�R���t�B�O\4*WS*","*")
	TEXT(_(X),0,_CFG2,_Cfg2,"�p�b�h�R���t�B�O\4*WS*","*")
	TEXT(_(X),0,_CFG3,_Cfg3,"�p�b�h�R���t�B�O\4*WS*","*")
	TEXT(_(X),0,_CFG4,_Cfg4,"�p�b�h�R���t�B�O\4*WS*","*")
	TEXT(_(X),0,_CFG5,_Cfg5,"�p�b�h�R���t�B�O\4*WS*","*")
	TEXT(_(X),0,_CFG6,_Cfg6,"�p�b�h�R���t�B�O\4*WS*","*")
	TEXT(_(X),0,_CFG7,_Cfg7,"�p�b�h�R���t�B�O\4*WS*","*")
	TEXT(_(X),0,_CFG8,_Cfg8,"�p�b�h�R���t�B�O\4*WS*","*")
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"���̐ݒ�ł�낵���ł����H","Use these settings?")	
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_QUIT_MENU
SOM_QUIT_MENU
#define X QUIT
	MENU(_(X),0,QUIT,Quit,"�Q�[�����I�����܂����H","Quit?")
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_TAKE_MENU
SOM_TAKE_MENU
#define X TAKE
	MENU(_(X),0,TAKE,Take,"%s�����܂����H","Take %s?")
	MENU(_(X),0,YES,Yes,"�͂�","Yes")
	MENU(_(X),0,NO,No,"������","No")
#endif

#ifdef SOM_SHOP_MENU
SOM_SHOP_MENU
#define X SHOP
	MENU(_(X),0,SHOP,Shop,"�w��","Buy")
	FLOW(_(X),case,sell sub menu...)
	MENU(_(X),0,SELL,Sell,"���p","Sell")
	FLOW(_(X),none)
	TEXT(_(X),0,GOLD,Gold,"�������z",">>>>Gold")
	TEXT(_(X),1,_GOLD,_Gold,"%5d%s","%5d%s")
	FLOW(_(X),item,inventory...)
	MENU(_(X),0,MENU,Menu,"*","*")
	FLOW(_(X),case,absent for 65535)
	TEXT(_(X),1,HAVE,Have,"x%5d","x%5d")
	FLOW(_(X),none,moving on)
	TEXT(_(X),1,COST,Cost,"%5d%s","%5d%s")
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),case,attack/defense/etc...)
	TEXT(_(X),0,ATTACK,Attack,"�U����",">Attack")			 
	TEXT(_(X),0,RADIUS,Radius,"����","<Radius")
	TEXT(_(X),1,_RADIUS,_Radius,"%3.1fcm","%4.0fcm") 
	FLOW(_(X),case,...)
	TEXT(_(X),0,DEFEND,Defense,"�h���",">Defense")
	FLOW(_(X),none,...)
	FLOW(_(X),case,weight/etc...)
	TEXT(_(X),0,WEIGHT,Weight,"�d��","<Weight")
	TEXT(_(X),1,_WEIGHT,_Weight,"%2.1fKg",">%2.1fkg") 
	TEXT(_(X),0,EDGE,Edge,"�a","<<<Edge")
	TEXT(_(X),1,_EDGE,_Edge,"�a\4%3d","<%3d")
	TEXT(_(X),0,AREA,Area,"��","<Area")
	TEXT(_(X),1,_AREA,_Area,"��\4%3d",">%3d")
	TEXT(_(X),0,POINT,Point,"�h","<<<Point")
	TEXT(_(X),1,_POINT,_Point,"�h\4%3d","<%3d")
	TEXT(_(X),0,FIRE,Fire,"��","<<<Fire")
	TEXT(_(X),1,_FIRE,_Fire,"��\4%3d","<%3d")
	TEXT(_(X),0,EARTH,Earth,"�y","<Earth")	
	TEXT(_(X),1,_EARTH,_Earth,"�y\4%3d",">%3d")	
	TEXT(_(X),0,WIND,Wind,"��","<<<Wind")
	TEXT(_(X),1,_WIND,_Wind,"��\4%3d","<%3d")
	TEXT(_(X),0,WATER,Water,"��","<Water")
	TEXT(_(X),1,_WATER,_Water,"��\4%3d",">%3d")
	TEXT(_(X),0,HOLY,Holy,"��","<<<Holy")
	TEXT(_(X),1,_HOLY,_Holy,"��\4%3d","<%3d")			  			  	
	FLOW(_(X),none,...)
	FLOW(_(X),trap,transaction dialog...)
	MENU(_(X),0,YES,Yes,"�w��\4�w��","Buy")
	TEXT(_(X),1,AMOUNT,Amount,"x%3d","x%3d")
	TEXT(_(X),0,NO,No,"��߂�","Cancel")
	TEXT(_(X),0,GOLD2,Gold2,"�������z",">>>>Gold")
	TEXT(_(X),1,_GOLD2,_Gold2,"%5d%s","%5d%s")	
	TEXT(_(X),0,BULK,Bulk,"���v���z",">>>>Total")
	TEXT(_(X),1,_BULK,_Bulk,"%5d%s","%5d%s")				
#endif

#ifdef SOM_SELL_MENU
SOM_SELL_MENU
#define X SELL
	MENU(_(X),0,SELL,Sell,"���p","Sell")
	TEXT(_(X),0,GOLD,Gold,"�������z",">>>>Gold")
	TEXT(_(X),1,_GOLD,_Gold,"%5d%s","%5d%s")
	FLOW(_(X),item,inventory...)
	MENU(_(X),0,MENU,Menu,"*","*")
	TEXT(_(X),1,HAVE,Have,"x%3d","x%3d")
	TEXT(_(X),1,COST,Cost,"%5d%s","%5d%s")
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),trap,transaction dialog...)
	MENU(_(X),0,YES,Yes,"���p\4���p","Sell")
	TEXT(_(X),1,AMOUNT,Amount,"x%3d","x%3d")
	TEXT(_(X),0,NO,No,"���p\4��߂�","Cancel")
	TEXT(_(X),0,GOLD2,Gold2,"�������z",">>>>Gold")
	TEXT(_(X),1,_GOLD2,_Gold2,"%5d%s","%5d%s")	
	TEXT(_(X),0,BULK,Bulk,"���v���z",">>>>Total")
	TEXT(_(X),1,_BULK,_Bulk,"%5d%s","%5d%s")	
#endif

#ifdef SOM_INFO_MENU
SOM_INFO_MENU
#define X INFO
	MENU(_(X),0,INFO,Info,"�Ӓ�","Show")
	FLOW(_(X),case,sell sub menu...)
	MENU(_(X),0,SELL,Sell,"�Ӓ�\4���p","Sell")
	FLOW(_(X),none)
	TEXT(_(X),0,GOLD,Gold,"�Ӓ�\4�������z",">>>>Gold")
	TEXT(_(X),1,_GOLD,_Gold,"�Ӓ�\4%5d%s","%5d%s")
	FLOW(_(X),item,inventory...)
	MENU(_(X),0,MENU,Menu,"*","*")
	FLOW(_(X),case,no cost--2017)
	TEXT(_(X),1,FREE,Free,"�Ӓ�\4    0%s","0%s") //2017
	FLOW(_(X),case,will cost you--2017)
	TEXT(_(X),1,COST,Cost,"�Ӓ�\4%5d%s","%5d%s")
	FLOW(_(X),done,one or the other--2017)
	FLOW(_(X),cond,conditional continue)
	FLOW(_(X),trap,transaction dialog...)
	MENU(_(X),0,YES,Yes,"�Ӓ�\4�Ӓ�","Show")
	TEXT(_(X),0,NO,No,"�Ӓ�\4��߂�","Cancel")
	TEXT(_(X),0,GOLD2,Gold2,"�Ӓ�\4�������z",">>>>Gold")
	TEXT(_(X),1,_GOLD2,_Gold2,"�Ӓ�\4%5d%s","%5d%s")	
	TEXT(_(X),0,BULK,Bulk,"�Ӓ�\4���v���z",">>>>Total")
	TEXT(_(X),1,_BULK,_Bulk,"�Ӓ�\4%5d%s","%5d%s")		
#endif

#ifdef SOM_ASCII_MENU
SOM_ASCII_MENU
#define X ASCII	
	//junk that appears on screen from time to time
	MENU(_(X),0,ASCII,Ascii," !\"#$%&,()*+,-./","")
	TEXT(_(X),0,ASCIIb,AsciiB,"0123456789:;<=>?","")
	TEXT(_(X),0,ASCIIc,AsciiC,"@ABCDEFGHIJKLMNO","")
	TEXT(_(X),0,ASCIId,AsciiD,"PQRSTUVWXYZ[\\]^_","")
	TEXT(_(X),0,ASCIIe,AsciiE,",abcdefghijklmno","")
	TEXT(_(X),0,ASCIIf,AsciiF,"pqrstuvwxyz{|}~","")
#endif

#ifdef NAMECLOSE
NAMECLOSE
#elif defined NAMESPACE
}
#endif

#undef _
#undef X

#endif