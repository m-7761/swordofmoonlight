	
/*ATTENTION*/
//
// This file contains UTF-16-LE text, care must be taken 
// when saving it so it retains its Unicode width & BOM!
// (note: using UTF-8 encoding. VS only supports w/ sig)

#ifndef SOM_932W_INCLUDED
#define SOM_932W_INCLUDED

//NOTE: see som.932.h notes before adding to this header
#define SOM_932W_(x) static const wchar_t som_932w_##x[]

//som.menus.cpp
SOM_932W_(Buttons) = L"◯①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳";
SOM_932W_(States)[3] = 
{L" 毒",L" 痺",L" 闇",L" 呪",L" 重"}; //Ex.output.cpp
//som.status.cpp
SOM_932W_(SmartJoy) = L"△〇※□";
//som.state.cpp
SOM_932W_(MS_Mincho) = L"ＭＳ 明朝";
//SOM_MAIN.cpp
SOM_932W_(MS_Gothic) = L"ＭＳ ゴシック";
SOM_932W_(All) = L"すべて";
SOM_932W_(New) = L"新規";
SOM_932W_(NoContext)[16] = 
{L"",L"コメント",L"メッセージ",L"ディスプレイ"};
SOM_932W_(Message)[16] =
{L"本文",L"文脈",L"置換本文",L"木"};
namespace som_932w_EXML
{static const wchar_t 
li[]=L"項目",l[]=L"話題",lh[]=L"見出し",alt[]=L"要約",
c[]=L"創造者",tc[]=L"創造された",tm[]=L"修正された",cc[]=L"原作者",
hl[]=L"色相",il[]=L"アイコン";
}
SOM_932W_(Insert) = L"挟み込む";
SOM_932W_(Duplicate) = L"[重複]";
SOM_932W_(LocalTime) = L"現地時間";

SOM_932W_(tile_view_patch)[5] = {L"外部配置",L"レイヤ"}; //ライン

#undef SOM_932W_
#endif SOM_932_INCLUDED