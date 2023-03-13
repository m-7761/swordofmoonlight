
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
	MENU(_(X),0,MAIN,Main,"メインメニュー","Menu")
	MENU(_(X),0,ITEM,Item,"アイテム使用","Use Item")
	MENU(_(X),0,CAST,Cast,"補助魔法詠唱","Work Magic")
	MENU(_(X),0,EQUIP,Equip,"装備変更","Equip")
	MENU(_(X),0,STATS,Stats,"詳細ステータス参照","View Summary")
	MENU(_(X),0,STORE,Store,"アイテム整理","Arrange Inventory")
	MENU(_(X),0,SYS,Sys,"システム","System")	
#endif

#ifdef SOM_ITEM_MENU
SOM_ITEM_MENU
#define X ITEM
	MENU(_(X),0,ITEM,Item,"アイテム使用\4アイテム使用","Use Item")	
	FLOW(_(X),item,item selection...)
	MENU(_(X),0,MENU,Menu,"*","*")
	TEXT(_(X),1,HAVE,Have,"x%3d","x%3d")
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"%sを使用しますか？","Use %s?") 	
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_CAST_MENU
SOM_CAST_MENU
#define X CAST
	MENU(_(X),0,CAST,Cast,"補助魔法詠唱\4補助魔法詠唱","Work Magic")
	FLOW(_(X),item,spell selection...)	
	MENU(_(X),0,MENU,Menu,"*","*")
	FLOW(_(X),case,hack: cost/magic description)
	TEXT(_(X),1,COST,Cost,"魔法\4%3d","%3d")
	FLOW(_(X),none,hack: see case^)
	FLOW(_(X),cond,conditional continue)
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"%sを詠唱しますか？","Use %s?") 	
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_EQUIP_MENU
SOM_EQUIP_MENU
#define X EQUIP			   
	MENU(_(X),0,EQUIP,Equip,"装備変更\4装備変更","Equip")	
	MENU(_(X),0,WEAPON,Weapon,"武器","Weapon")	
	MENU(_(X),0,HEAD,Head,"兜","Head")	
	MENU(_(X),0,BODY,Body,"鎧","Body")	
	MENU(_(X),0,HANDS,Hands,"篭手","Hands")	
	MENU(_(X),0,FEET,Feet,"具足","Feet")	
	MENU(_(X),0,SHIELD,Shield,"盾","Shield")	
	MENU(_(X),0,ACC,Acc,"装飾品","Accessory")
	MENU(_(X),0,MAGIC,Magic,"魔法","Magic")
	TEXT(_(X),0,EDGE,Edge,"装備\4斬","<<Edge")
	TEXT(_(X),0,AREA,Area,"装備\4殴","<Area")
	TEXT(_(X),0,POINT,Point,"装備\4刺","<<Point")
	TEXT(_(X),0,FIRE,Fire,"装備\4火","<<Fire")
	TEXT(_(X),0,EARTH,Earth,"装備\4土","<Earth")	
	TEXT(_(X),0,WIND,Wind,"装備\4風","<<Wind")
	TEXT(_(X),0,WATER,Water,"装備\4水","<Water")
	TEXT(_(X),0,HOLY,Holy,"装備\4聖","<<Holy")	
	FLOW(_(X),case,nothing/something equipped)
	TEXT(_(X),0,NONE,None,"装備なし            ","Nothing")
	FLOW(_(X),case,in case som_game_nothing is full up)
	TEXT(_(X),0,NONE2,None2,"装備なし","Nothing")
	FLOW(_(X),case,something...)
	TEXT(_(X),0,NAME,Name,"装備\4%s","%s") //"%-20.20s"
	FLOW(_(X),done,one or the other)
	FLOW(_(X),case,attack or defense)
	TEXT(_(X),0,ATTACK,Attack,"装備\4攻撃力","")
	FLOW(_(X),case,...)
	TEXT(_(X),0,DEFEND,Defense,"装備\4防御力","")
	FLOW(_(X),none,magic displays neither)
	FLOW(_(X),case,length of a weapon)
	TEXT(_(X),0,RADIUS,Radius,"装備\4長さ","<<Radius")
	TEXT(_(X),1,_RADIUS,_Radius,"装備\4%3.1fcm","<<<<%4.0fcm")
	TEXT(_(X),0,WEIGHT,Weight,"装備\4重量","<<Weight\nAttack")
	TEXT(_(X),1,_WEIGHT,_Weight,"装備\4%2.1fKg","<<<<%2.1fkg")
	FLOW(_(X),none,only weapons have length)
	FLOW(_(X),case,weight of equipment)
	TEXT(_(X),0,WEIGHT2,Weight2,"装備\4重量","<<Weight\nDefense")
	TEXT(_(X),1,_WEIGHT2,_Weight2,"装備\4%2.1fKg","<<<<%2.1fkg")
	FLOW(_(X),none,magic does not have weight)
	FLOW(_(X),case,not used by Nothing)
	TEXT(_(X),1,_EDGE,_Edge,"装備斬\4%3d","%3d")				
	TEXT(_(X),1,_AREA,_Area,"装備殴\4%3d",">%3d")
	TEXT(_(X),1,_POINT,_Point,"装備刺\4%3d","%3d")
	TEXT(_(X),1,_FIRE,_Fire,"装備火\4%3d","%3d")
	TEXT(_(X),1,_EARTH,_Earth,"装備土\4%3d",">%3d")	
	TEXT(_(X),1,_WIND,_Wind,"装備風\4%3d","%3d")
	TEXT(_(X),1,_WATER,_Water,"装備水\4%3d",">%3d")
	TEXT(_(X),1,_HOLY,_Holy,"装備聖\4%3d","%3d")		
	FLOW(_(X),none,continued from Nothing)
	TEXT(_(X),0,WT,Wt,"装備重量","<Equipped")
	TEXT(_(X),1,_WT,_Wt,"%4.1fKg",">%3.1fkg") 
	TEXT(_(X),0,CAP,Cap,"装備可能重量","<Capacity")
	TEXT(_(X),1,_CAP,_Cap,"%4.1fKg",">%3.1fkg")
#endif

#ifdef SOM_WEAPON_MENU
SOM_WEAPON_MENU
#define X WEAPON
	MENU(_(X),0,WEAPON,Weapon,"装備変更：武器","Weapon")
#endif
#ifdef SOM_HEAD_MENU
SOM_HEAD_MENU
#define X HEAD
	MENU(_(X),0,HEAD,Head,"装備変更：兜","Head")	
#endif
#ifdef SOM_BODY_MENU
SOM_BODY_MENU
#define X BODY
	MENU(_(X),0,BODY,Body,"装備変更：鎧","Body")	
#endif
#ifdef SOM_HANDS_MENU
SOM_HANDS_MENU
#define X HANDS
	MENU(_(X),0,HANDS,Hands,"装備変更：篭手","Hands")	
#endif
#ifdef SOM_FEET_MENU
SOM_FEET_MENU
#define X FEET
	MENU(_(X),0,FEET,Feet,"装備変更：具足","Feet")	
#endif
#ifdef SOM_SHIELD_MENU
SOM_SHIELD_MENU
#define X SHIELD
	MENU(_(X),0,SHIELD,Shield,"装備変更：盾","Shield")	
#endif
#ifdef SOM_ACCESSORY_MENU
SOM_ACCESSORY_MENU
#define X ACC
	MENU(_(X),0,ACC,Acc,"装備変更：装飾品","Accessory")	
#endif
#ifdef SOM_MAGIC_MENU
SOM_MAGIC_MENU
#define X MAGIC
	MENU(_(X),0,MAGIC,Magic,"装備変更：魔法","Magic")	
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
	TEXT(_(X),1,COST,Cost,"魔法\4%3d","%3d")
	FLOW(_(X),none,hack: see case^)
#endif	
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),case,attack/defense/none)
#ifdef SOM_MAGIC_MENU	
	TEXT(_(X),0,ATTACK,Attack,"攻撃力",">Attack")
#else
#ifdef SOM_WEAPON_MENU	
	TEXT(_(X),0,ATTACK,Attack,"攻撃力",">Attack")
	TEXT(_(X),0,RADIUS,Radius,"長さ","<Radius")
	TEXT(_(X),1,_RADIUS,_Radius,"%3.1fcm","%4.0fcm") 
#else
	TEXT(_(X),0,DEFEND,Defense,"防御力",">Defense")
#endif
	TEXT(_(X),0,WEIGHT,Weight,"重量","<Weight")
	TEXT(_(X),1,_WEIGHT,_Weight,"%2.1fKg",">%2.1fkg") 
#endif
	TEXT(_(X),0,EDGE,Edge,"斬","<<<Edge")
	TEXT(_(X),1,_EDGE,_Edge,"斬\4%3d","<%3d")
	TEXT(_(X),0,AREA,Area,"殴","<Area")
	TEXT(_(X),1,_AREA,_Area,"殴\4%3d",">%3d")
	TEXT(_(X),0,POINT,Point,"刺","<<<Point")
	TEXT(_(X),1,_POINT,_Point,"刺\4%3d","<%3d")
	TEXT(_(X),0,FIRE,Fire,"火","<<<Fire")
	TEXT(_(X),1,_FIRE,_Fire,"火\4%3d","<%3d")
	TEXT(_(X),0,EARTH,Earth,"土","<Earth")	
	TEXT(_(X),1,_EARTH,_Earth,"土\4%3d",">%3d")	
	TEXT(_(X),0,WIND,Wind,"風","<<<Wind")
	TEXT(_(X),1,_WIND,_Wind,"風\4%3d","<%3d")
	TEXT(_(X),0,WATER,Water,"水","<Water")
	TEXT(_(X),1,_WATER,_Water,"水\4%3d",">%3d")
	TEXT(_(X),0,HOLY,Holy,"聖","<<<Holy")
	TEXT(_(X),1,_HOLY,_Holy,"聖\4%3d","<%3d")			  			  	
	FLOW(_(X),none,...)
	FLOW(_(X),trap,confirmation dialog...)
	#if !defined(SOM_MAGIC_MENU)
 	TEXT(_(X),0,OK,Ok,"%sを装備しますか？","Use %s?")	
	#else
	TEXT(_(X),0,OK,Ok,"魔法\4%sを装備しますか？","Use %s?")	
	#endif
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_STATS_MENU
SOM_STATS_MENU
#define X STATS
	MENU(_(X),0,STATS,Stats,"詳細ステータス","Summary") 
#endif

#ifdef SOM_STORE_MENU
SOM_STORE_MENU
#define X STORE
	MENU(_(X),0,STORE,Store,"アイテム整理\4アイテム整理","Inventory")	
	FLOW(_(X),item,stored items...)
  //hack: MENU really does not capture the behavior here...	
	MENU(_(X),0,MENU,Menu,"*","*") //"%-16.16s"
	TEXT(_(X),1,HAVE,Have,"x%3d","x%3d")
	FLOW(_(X),cond,conditional continue)	
  ////%s has been added to these////////	
	FLOW(_(X),case,put away)
	TEXT(_(X),0,HIDE,Hide,"%sを片付けますか？","Pack %s?")
	FLOW(_(X),case,take out)
	TEXT(_(X),0,SHOW,Show,"%sを取り出しますか？","Unpack %s?")	
	FLOW(_(X),done,one or the other)
  //////////////////////////////////////
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")		
#endif
	
#ifdef SOM_SYSTEM_MENU
SOM_SYSTEM_MENU
#define X SYS
	MENU(_(X),0,SYS,Sys,"システム\4システム","System")
	MENU(_(X),0,LOAD,Load,"データロード","Load Game")
	MENU(_(X),0,SAVE,Save,"データセーブ","Save Game")
	MENU(_(X),0,OPTION,Option,"オプション設定","Options")
	MENU(_(X),0,CONFIG,Config,"パッドコンフィグ","Controls")
	MENU(_(X),0,QUIT,Quit,"ゲーム終了","Leave Game")
#endif

#ifdef SOM_LOAD_MENU
SOM_LOAD_MENU
#define X LOAD
	MENU(_(X),0,LOAD,Load,"データロード\4データロード","Load Game")
	FLOW(_(X),item,saved game selection...)
	MENU(_(X),0,DATA,Data,"データ%02d","SAVE %02d")	
	TEXT(_(X),0,DATE,Date,"日付","")
	TEXT(_(X),1,_DATE,_Date,"%4d年%2d月%2d日","%2$02d/%3$02d/%1$4d")
	TEXT(_(X),0,EXP,Exp,"データ\4経験値","EP")	
	TEXT(_(X),1,_EXP,_Exp,"経験値\4%6d","%6d")
	TEXT(_(X),0,TIME,Time,"総時間","Time")	
	TEXT(_(X),1,_TIME,_Time,"%4dh%02dm%02ds","%4d:%02d:%02d")
	FLOW(_(X),cont,continue for each saved game)
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"ロードしますか？","Load?")	
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_SAVE_MENU
SOM_SAVE_MENU
#define X SAVE
	MENU(_(X),0,SAVE,Save,"データセーブ\4データセーブ","Save Game")
	FLOW(_(X),item,saved game slot selection...)
	FLOW(_(X),case,data or no data)
	MENU(_(X),0,FREE,Free,"データ%02d　　　　データなし","SAVE %02d        No Data")
	FLOW(_(X),case,data exists...) 
	MENU(_(X),0,DATA,Data,"データ%02d","SAVE %02d")
	TEXT(_(X),0,DATE,Date,"日付","")
	TEXT(_(X),1,_DATE,_Date,"%4d年%2d月%2d日","%2$02d/%3$02d/%1$4d")
	TEXT(_(X),0,EXP,Exp,"データ\4経験値","EP")	
	TEXT(_(X),1,_EXP,_Exp,"経験値\4%6d","%6d")
	TEXT(_(X),0,TIME,Time,"総時間","Time")	
	TEXT(_(X),1,_TIME,_Time,"%4dh%02dm%02ds","%4d:%02d:%02d")
	FLOW(_(X),cont,continue for each saved game)
	FLOW(_(X),trap,confirmation dialog...)
	FLOW(_(X),case,overwrite/save)
	TEXT(_(X),0,OW,Ow,"上書きしますか？","Overwrite?")
	FLOW(_(X),case,new save...)
	TEXT(_(X),0,OK,Ok,"セーブしますか？","Save?")	
	FLOW(_(X),done,...)
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_OPTION_MENU
SOM_OPTION_MENU
#define X OPTION
	MENU(_(X),0,OPTION,Option,"オプション設定\4オプション設定","Options")
	STOP(_(X),0,DEV,Dev,"デバイス","") //Device
	STOP(_(X),0,RES,Res,"解像度","Resolution")
	STOP(_(X),0,COLOR,Color,"色数","Colors")
	STOP(_(X),0,FILTER,Filter,"テクスチャモード","Texture Mode")
	STOP(_(X),0,BRIGHT,Bright,"明るさ調整","Brightness")
	STOP(_(X),0,PAD,Pad,"パッド選択","Game Controller")
	STOP(_(X),0,BGM,Bgm,"ＢＧＭボリューム","Music Volume")
	STOP(_(X),0,SOUND,Sound,"効果音ボリューム","Sound Volume")
	STOP(_(X),0,HUD,Hud,"各種ゲージ表示","Stamina Display")
	STOP(_(X),0,NAV,Nav,"方向表示","Compass Display")
	//2022: I think this Japanese is okay for this?
//	STOP(_(X),0,INV,Inv,"アイテム表示","Inventory Icons")
STOP(_(X),0,INV,Inv,"アイテム表示","Examine Display")
	STOP(_(X),0,BOB,Bob,"歩行効果","Stepping Motion")
	TEXT(_(X),1,_DEV,_Dev,"*WS*","*")
	TEXT(_(X),1,_RES,_Res,"%4d x %4d","%4d x %4d")
	TEXT(_(X),1,_COLOR,_Color,"%2d bpp","%2d bpp")
	TEXT(_(X),1,_FILTER,_Filter,"*WS*","*")
	FLOW(_(X),case,joypad if present)
	TEXT(_(X),1,_PAD,_Pad,"パッド選択\4(%d)","(%d)")
	//Reminder: "PadID: " is a hack used to identify SOM_CONFIG_MENU
	TEXT(_(X),0,PADID,PadID,"パッド選択\4PadID: %s","%s") //%-42.42s
	FLOW(_(X),case,joypad if present)
	TEXT(_(X),1,PADXX,PadXX,"パッドなし","None Found")
	FLOW(_(X),done,...)
	TEXT(_(X),1,_BGM,_Bgm,"ＢＧＭボリューム\4%d","%d")
	TEXT(_(X),1,_SOUND,_Sound,"効果音ボリューム\4%d","%d")
	TEXT(_(X),1,_BRIGHT,_Bright,"明るさ調整\4%d","%d")
    TEXT(_(X),0,_HUD,_Hud,"各種ゲージ表示\4*WS*","*")
    TEXT(_(X),0,_NAV,_Nav,"方向表示\4*WS*","*")
    TEXT(_(X),0,_INV,_Inv,"アイテム表示\4*WS*","*")
    TEXT(_(X),0,_BOB,_Bob,"歩行効果\4*WS*","*")
    FLOW(_(X),trap,confirmation dialog...)
    FLOW(_(X),case,...)
	TEXT(_(X),0,OK,Ok,"この設定でよろしいですか？","Use these settings?")	
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
	FLOW(_(X),case,messages...)
	TEXT(_(X),0,DISPLAYERR,DisplayERR,"指定された解像度では表示できませんでした","Could not display at selected resolution")
	FLOW(_(X),case,...)
	TEXT(_(X),0,GAMMAERR,GammaERR,"ガンマ設定不可","Could not adjust brightness")
#endif

#if defined(SOM_MAIN_MENU)||defined(SOM_SYSTEM_MENU)||\
    defined(SOM_STATS_MENU)
	TEXT(_(X),0,LEVEL,Level,"レベル","Level")
	TEXT(_(X),0,CLASS,Class,"クラス","Class")
	TEXT(_(X),0,HP,Hp,"ＨＰ","HP")
	TEXT(_(X),0,MP,Mp,"ＭＰ","MP")
	TEXT(_(X),0,POW,Pow,"体力","Strength")
	TEXT(_(X),0,MAG,Mag,"魔力","Magic")	   
	TEXT(_(X),0,GOLD,Gold,"ゴールド","Gold")
	TEXT(_(X),0,EXP,Exp,"経験値","EP")
	TEXT(_(X),0,MODE,Mode,"状態","") 
	TEXT(_(X),0,TIME,Time,"経過時間","Time")	
#endif
	
#ifdef SOM_STATS_MENU
	TEXT(_(X),0,EST,Est,"評価攻撃力","Attack")   
	TEXT(_(X),0,EST2,Est2,"評価防御力","Defense\nApproach")
	TEXT(_(X),0,ATTACK,Attack,"詳細\4攻撃力","")
	TEXT(_(X),0,DEFENSE,Defense,"詳細\4防御力","")
	TEXT(_(X),0,EDGE,Edge,"詳細\4斬","Edge")
	TEXT(_(X),0,AREA,Area,"詳細\4殴","Area")
	TEXT(_(X),0,POINT,Point,"詳細\4刺","Point")
	TEXT(_(X),0,FIRE,Fire,"詳細\4火","Fire")
	TEXT(_(X),0,EARTH,Earth,"詳細\4土","Earth")	
	TEXT(_(X),0,WIND,Wind,"詳細\4風","Wind")
	TEXT(_(X),0,WATER,Water,"詳細\4水","Water")	
	TEXT(_(X),0,HOLY,Holy,"詳細\4聖","Holy")	
#endif

#if defined(SOM_MAIN_MENU)||defined(SOM_SYSTEM_MENU)||\
    defined(SOM_STATS_MENU)
	TEXT(_(X),1,_LEVEL,_Level,"%2d","%2d")
	TEXT(_(X),1,_CLASS,_Class,"クラス\4%s","%s") //%15.15s
	TEXT(_(X),1,_HP,_Hp,"ＨＰ\4%3d／%3d","%3d/%3d")
	TEXT(_(X),1,_MP,_Mp,"ＭＰ\4%3d／%3d","%3d/%3d")
	TEXT(_(X),1,_STR,_Str,"体力\4%3d","%3d")
	TEXT(_(X),1,_MAG,_Mag,"魔力\4%3d","%3d")
	TEXT(_(X),1,_GOLD,_Gold,"ゴールド\4%5d","%5d")
	TEXT(_(X),1,_EXP,_Exp,"経験値\4%6d","%6d")
	TEXT(_(X),1,_MODE,_Mode,"*WS*","*")
	TEXT(_(X),1,_TIME,_Time,"%4dh%02dm%02ds","%4d:%02d:%02d")
#endif

#ifdef SOM_STATS_MENU
	TEXT(_(X),1,_EDGE,_Edge,"斬々\4%3d",">%3d")
	TEXT(_(X),1,_EDGE2,_Edge2,"斬々\4%4d",">%4d")
	TEXT(_(X),1,_AREA,_Area,"殴々\4%3d",">%3d")
	TEXT(_(X),1,_AREA2,_Area2,"殴々\4%4d",">%4d")
	TEXT(_(X),1,_POINT,_Point,"刺々\4%3d",">%3d")
	TEXT(_(X),1,_POINT2,_Point2,"刺々\4%4d",">%4d")
	TEXT(_(X),1,_FIRE,_Fire,"火々\4%3d",">%3d")
	TEXT(_(X),1,_FIRE2,_Fire2,"火々\4%4d",">%4d")
	TEXT(_(X),1,_EARTH,_Earth,"土々\4%3d",">%3d")	
	TEXT(_(X),1,_EARTH2,_Earth2,"土々\4%4d",">%4d")
	TEXT(_(X),1,_WIND,_Wind,"風々\4%3d",">%3d")
	TEXT(_(X),1,_WIND2,_Wind2,"風々\4%4d",">%4d")
	TEXT(_(X),1,_WATER,_Water,"水々\4%3d",">%3d")	
	TEXT(_(X),1,_WATER2,_Water2,"水々\4%4d",">%4d")	
	TEXT(_(X),1,_HOLY,_Holy,"聖々\4%3d",">%3d")	
	TEXT(_(X),1,_HOLY2,_Holy2,"聖々\4%4d",">%4d")	
	TEXT(_(X),1,_EST,_Est,"評価攻撃力\4%5.1f","%5.1f")
	TEXT(_(X),1,_EST2,_Est2,"評価防御力\4%5.1f","%5.1f")
#endif

#ifdef SOM_SYSTEM_MENU	
	FLOW(_(X),trap,messages...)		
	TEXT(_(X),0,LOADERR,LoadERR,"セーブデータがありません","There is no saved game")
#endif

#ifdef SOM_CONFIG_MENU			
SOM_CONFIG_MENU
#define X CONFIG
	//Reminder: "PadID: " is a hack used to identify this menu
	TEXT(_(X),0,PAD,PadID,"パッドコンフィグ\4PadID: %s","%s")	//%-42.42s
	TEXT(_(X),1,_PAD,_PadID,"パッドコンフィグ\4(%d)","(%d)")
	MENU(_(X),0,CONFIG,Config,"パッドコンフィグ\4パッドコンフィグ","Controller")	
	STOP(_(X),0,CFG1,Cfg1,"ボタン１","Button 1")
	STOP(_(X),0,CFG2,Cfg2,"ボタン２","Button 2") 
	STOP(_(X),0,CFG3,Cfg3,"ボタン３","Button 3") 
	STOP(_(X),0,CFG4,Cfg4,"ボタン４","Button 4") 
	STOP(_(X),0,CFG5,Cfg5,"ボタン５","Button 5") 
	STOP(_(X),0,CFG6,Cfg6,"ボタン６","Button 6") 
	STOP(_(X),0,CFG7,Cfg7,"ボタン７","Button 7") 
	STOP(_(X),0,CFG8,Cfg8,"ボタン８","Button 8") 
	TEXT(_(X),0,_CFG1,_Cfg1,"パッドコンフィグ\4*WS*","*")
	TEXT(_(X),0,_CFG2,_Cfg2,"パッドコンフィグ\4*WS*","*")
	TEXT(_(X),0,_CFG3,_Cfg3,"パッドコンフィグ\4*WS*","*")
	TEXT(_(X),0,_CFG4,_Cfg4,"パッドコンフィグ\4*WS*","*")
	TEXT(_(X),0,_CFG5,_Cfg5,"パッドコンフィグ\4*WS*","*")
	TEXT(_(X),0,_CFG6,_Cfg6,"パッドコンフィグ\4*WS*","*")
	TEXT(_(X),0,_CFG7,_Cfg7,"パッドコンフィグ\4*WS*","*")
	TEXT(_(X),0,_CFG8,_Cfg8,"パッドコンフィグ\4*WS*","*")
	FLOW(_(X),trap,confirmation dialog...)
	TEXT(_(X),0,OK,Ok,"この設定でよろしいですか？","Use these settings?")	
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_QUIT_MENU
SOM_QUIT_MENU
#define X QUIT
	MENU(_(X),0,QUIT,Quit,"ゲームを終了しますか？","Quit?")
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_TAKE_MENU
SOM_TAKE_MENU
#define X TAKE
	MENU(_(X),0,TAKE,Take,"%sを取りますか？","Take %s?")
	MENU(_(X),0,YES,Yes,"はい","Yes")
	MENU(_(X),0,NO,No,"いいえ","No")
#endif

#ifdef SOM_SHOP_MENU
SOM_SHOP_MENU
#define X SHOP
	MENU(_(X),0,SHOP,Shop,"購入","Buy")
	FLOW(_(X),case,sell sub menu...)
	MENU(_(X),0,SELL,Sell,"売却","Sell")
	FLOW(_(X),none)
	TEXT(_(X),0,GOLD,Gold,"所持金額",">>>>Gold")
	TEXT(_(X),1,_GOLD,_Gold,"%5d%s","%5d%s")
	FLOW(_(X),item,inventory...)
	MENU(_(X),0,MENU,Menu,"*","*")
	FLOW(_(X),case,absent for 65535)
	TEXT(_(X),1,HAVE,Have,"x%5d","x%5d")
	FLOW(_(X),none,moving on)
	TEXT(_(X),1,COST,Cost,"%5d%s","%5d%s")
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),case,attack/defense/etc...)
	TEXT(_(X),0,ATTACK,Attack,"攻撃力",">Attack")			 
	TEXT(_(X),0,RADIUS,Radius,"長さ","<Radius")
	TEXT(_(X),1,_RADIUS,_Radius,"%3.1fcm","%4.0fcm") 
	FLOW(_(X),case,...)
	TEXT(_(X),0,DEFEND,Defense,"防御力",">Defense")
	FLOW(_(X),none,...)
	FLOW(_(X),case,weight/etc...)
	TEXT(_(X),0,WEIGHT,Weight,"重量","<Weight")
	TEXT(_(X),1,_WEIGHT,_Weight,"%2.1fKg",">%2.1fkg") 
	TEXT(_(X),0,EDGE,Edge,"斬","<<<Edge")
	TEXT(_(X),1,_EDGE,_Edge,"斬\4%3d","<%3d")
	TEXT(_(X),0,AREA,Area,"殴","<Area")
	TEXT(_(X),1,_AREA,_Area,"殴\4%3d",">%3d")
	TEXT(_(X),0,POINT,Point,"刺","<<<Point")
	TEXT(_(X),1,_POINT,_Point,"刺\4%3d","<%3d")
	TEXT(_(X),0,FIRE,Fire,"火","<<<Fire")
	TEXT(_(X),1,_FIRE,_Fire,"火\4%3d","<%3d")
	TEXT(_(X),0,EARTH,Earth,"土","<Earth")	
	TEXT(_(X),1,_EARTH,_Earth,"土\4%3d",">%3d")	
	TEXT(_(X),0,WIND,Wind,"風","<<<Wind")
	TEXT(_(X),1,_WIND,_Wind,"風\4%3d","<%3d")
	TEXT(_(X),0,WATER,Water,"水","<Water")
	TEXT(_(X),1,_WATER,_Water,"水\4%3d",">%3d")
	TEXT(_(X),0,HOLY,Holy,"聖","<<<Holy")
	TEXT(_(X),1,_HOLY,_Holy,"聖\4%3d","<%3d")			  			  	
	FLOW(_(X),none,...)
	FLOW(_(X),trap,transaction dialog...)
	MENU(_(X),0,YES,Yes,"購入\4購入","Buy")
	TEXT(_(X),1,AMOUNT,Amount,"x%3d","x%3d")
	TEXT(_(X),0,NO,No,"やめる","Cancel")
	TEXT(_(X),0,GOLD2,Gold2,"所持金額",">>>>Gold")
	TEXT(_(X),1,_GOLD2,_Gold2,"%5d%s","%5d%s")	
	TEXT(_(X),0,BULK,Bulk,"合計金額",">>>>Total")
	TEXT(_(X),1,_BULK,_Bulk,"%5d%s","%5d%s")				
#endif

#ifdef SOM_SELL_MENU
SOM_SELL_MENU
#define X SELL
	MENU(_(X),0,SELL,Sell,"売却","Sell")
	TEXT(_(X),0,GOLD,Gold,"所持金額",">>>>Gold")
	TEXT(_(X),1,_GOLD,_Gold,"%5d%s","%5d%s")
	FLOW(_(X),item,inventory...)
	MENU(_(X),0,MENU,Menu,"*","*")
	TEXT(_(X),1,HAVE,Have,"x%3d","x%3d")
	TEXT(_(X),1,COST,Cost,"%5d%s","%5d%s")
	FLOW(_(X),cond,conditional continue...)
	FLOW(_(X),trap,transaction dialog...)
	MENU(_(X),0,YES,Yes,"売却\4売却","Sell")
	TEXT(_(X),1,AMOUNT,Amount,"x%3d","x%3d")
	TEXT(_(X),0,NO,No,"売却\4やめる","Cancel")
	TEXT(_(X),0,GOLD2,Gold2,"所持金額",">>>>Gold")
	TEXT(_(X),1,_GOLD2,_Gold2,"%5d%s","%5d%s")	
	TEXT(_(X),0,BULK,Bulk,"合計金額",">>>>Total")
	TEXT(_(X),1,_BULK,_Bulk,"%5d%s","%5d%s")	
#endif

#ifdef SOM_INFO_MENU
SOM_INFO_MENU
#define X INFO
	MENU(_(X),0,INFO,Info,"鑑定","Show")
	FLOW(_(X),case,sell sub menu...)
	MENU(_(X),0,SELL,Sell,"鑑定\4売却","Sell")
	FLOW(_(X),none)
	TEXT(_(X),0,GOLD,Gold,"鑑定\4所持金額",">>>>Gold")
	TEXT(_(X),1,_GOLD,_Gold,"鑑定\4%5d%s","%5d%s")
	FLOW(_(X),item,inventory...)
	MENU(_(X),0,MENU,Menu,"*","*")
	FLOW(_(X),case,no cost--2017)
	TEXT(_(X),1,FREE,Free,"鑑定\4    0%s","0%s") //2017
	FLOW(_(X),case,will cost you--2017)
	TEXT(_(X),1,COST,Cost,"鑑定\4%5d%s","%5d%s")
	FLOW(_(X),done,one or the other--2017)
	FLOW(_(X),cond,conditional continue)
	FLOW(_(X),trap,transaction dialog...)
	MENU(_(X),0,YES,Yes,"鑑定\4鑑定","Show")
	TEXT(_(X),0,NO,No,"鑑定\4やめる","Cancel")
	TEXT(_(X),0,GOLD2,Gold2,"鑑定\4所持金額",">>>>Gold")
	TEXT(_(X),1,_GOLD2,_Gold2,"鑑定\4%5d%s","%5d%s")	
	TEXT(_(X),0,BULK,Bulk,"鑑定\4合計金額",">>>>Total")
	TEXT(_(X),1,_BULK,_Bulk,"鑑定\4%5d%s","%5d%s")		
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