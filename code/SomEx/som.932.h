
/*ATTENTION*/
//
// This file contains Shift_JIS text, care must be taken 
// when saving it so it retains its ANSI (932) codepage!

#ifndef SOM_932_INCLUDED
#define SOM_932_INCLUDED

//TODO? #pragma code_page(932) can be used in an RC file

//Shift-JIS string literals go here unless they are part
//of another block definition file such as som.menus.inl
//NOTE: wchar_t literals are kept som.932w.h so that the
//UTF16+ format is compiled correctly on non-932 systems

#define SOM_932_(x) static const char som_932_##x[]

//som.tool.cpp
SOM_932_(Systemwide) = "システム全体"; 
SOM_932_(Fitness) = "フィットネス";
SOM_932_(Powers)[5] = {"体力","魔力","速力"};
SOM_932_(Item) = "アイテム";
SOM_932_(Item_Haul) = "アイテム所持数";
SOM_932_(Gold_Haul) = "ゴールド所持数";
SOM_932_(LVT) = "成長タイプ"; //レベル
SOM_932_(LEVEL) = "ＬＥＶＥＬ";
SOM_932_(LVT_Next) = "必要経験値 (0-99%)";
SOM_932_(LVT_Max)[10] = {"最大ＨＰ","最大ＭＰ","最大体力","最大魔力","最大速力"};
SOM_932_(Player_Setup) = "ﾌﾟﾚｲﾔｰ設定 1-2";
SOM_932_(Magic_Table) = "魔法ﾃｰﾌﾞﾙ";
SOM_932_(Field) = "フィールド";
SOM_932_(Map) = "このマップ";
SOM_932_(Status) = "ステータス";
SOM_932_(Normal) = "通常";
SOM_932_(System) = "システム条件";
SOM_932_(Save) = "セーブ";
SOM_932_(Dash) = "ダッシュ";
SOM_932_(Play) = "プレー (Alt+Break)";
SOM_932_(Fall) = "落下着陸 (リセットされます)";
SOM_932_(Fall_Height) = "身長";
SOM_932_(Fall_Impact) = "衝撃 (メートル毎秒)";
SOM_932_(Weapon) = "武器";
SOM_932_(Head) = "兜";
SOM_932_(Body) = "鎧";
SOM_932_(Hands) = "篭手";
SOM_932_(Feet) = "具足";
SOM_932_(Shield) = "盾";
SOM_932_(Accessory) = "装飾品";
SOM_932_(Magic) = "魔法";
SOM_932_(Spell) = "魔法番号 0-31";
SOM_932_(attack) = "%sの攻撃";
SOM_932_(defense) = "%sの防御";
SOM_932_(attacks)[3] = {"斬","殴","刺","火","土","風","水","聖"};
SOM_932_(Math)[20] = {"変更","増やす","減らす","データから減算","乗算","除算","データを除算","パーセント","データのパーセント","剰余","データの剰余"};
//som.hacks.cpp
SOM_932_(Super_Modes)[32] = {"３Ｄアンチエイリアシング","４倍スーパーサンプリング","スーパーサンプルできません (%d)"};
SOM_932_(Stereo_Modes)[32] = {"最小スーパーサンプリング","最大スーパーサンプリング"};
SOM_932_(Button_Swap) = "ボタンスワップ %d-%d";
SOM_932_(Analog_Mode) = "アナログモード %d-%c";
SOM_932_(Analog_Disabled) = "アナログ無効にする"; 
SOM_932_(Zoom) = "%d°ズーム";
SOM_932_(Mono) = "ステレオは利用できません (%d)";
SOM_932_(Stereo) = "使用中のVR (Win+Y, %sF2)";
SOM_932_(IPD) = "%d 両眼の隙間";
SOM_932_(Master_Volume) = "マスターボリューム %d";
SOM_932_(Start)[3][16] =
{
/*do_som*/ {"PUSH ANY KEY","NEW GAME","CONTINUE"},
/*do_kf*/  {"","",""},
/*do_kf2*/ {"ＳＴＡＲＴ","はじめる","ロードする"}, 
/*do_kf3*/ {"","NEW","CONTINUE"}, 
/*do_st*/  {"PUSH START","NEW","LOAD"},
};
SOM_932_(Equip_Broke) = "機器が壊れた";
//som.menus.cpp
SOM_932_(Take) = "%sを取りますか？";
SOM_932_(Press_any_button) = "孰かのボタンを押す";
SOM_932_(Now_press_another) = "今別のボタンを押す";
SOM_932_(Button) = "ボタン"; 
//SOM_932W_(Buttons) = L"◯①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳";
SOM_932_(Numerals)[3] = 
{"０","１","２","３","４","５","６","７","８","９"};
SOM_932_(Date) = "%4d年%2d月%2d日";
SOM_932_(States)[4] = 
{" 毒"," 痺"," 闇"," 呪"," 重"};
//SOM_932W_(States)[3] = 
//{L" 毒",L" 痺",L" 闇",L" 呪",L" 重"}; //Ex.output.cpp
SOM_932_(Shadow_Tower)[16] = 
{				//substitutes
"各種ゲージ表示",	"健ゲージ表示",
"方向表示",		"各種状態表示",
"アイテム表示",	"ダメージ表示",
"歩行効果",		"歩行移動効果"
};
SOM_932_(HighColor) = "ハイカラー";
SOM_932_(ColorModes)[16] = 
{"ハイカラー","トゥルーカラー"}; 
SOM_932_(Graphics) = "グラフィックス";
SOM_932_(Anisotropy) = "異方性";
SOM_932_(AnisoModes)[24] = 
{"4x (高性能)","8x (高品質)","16x (最高の品質)"};
SOM_932_(ConcealDisplay) = "隠さ表示";
SOM_932_(EnlargeDisplay) = "拡大表示";
//som.money.cpp
SOM_932_(Money) = "%d%s手に入れた";
//som.status.cpp
SOM_932_(PAUSED) = "一時停止";
//SOM_932W_(SmartJoy) = L"△〇※□";
SOM_932_(Shadow_Tower_HP) = "ＨＰ %3d";
SOM_932_(Shadow_Tower_MP) = "ＭＰ %3d";
//som.state.cpp
SOM_932_(MS_Mincho) = "ＭＳ 明朝";
//SOM_932W_(MS_Mincho) = L"ＭＳ 明朝";
SOM_932_(Nothing) = "装備なし";
//SOM_MAIN.cpp
//SOM_932W_(All) = L"すべて";
//workshop.cpp
SOM_932_(PrtsEdit_99) = "99は黒です";
SOM_932_(PrtsEdit_filter) = "ﾊﾟｰﾂ ﾌﾟﾛﾌｨｰﾙ ﾌｧｲﾙ(*.prt)\0*.prt\0";
SOM_932_(EneEdit_attack123) = "%s接攻撃%s"; //直接攻撃２ //間接攻撃２
//SOM_MAP magic menu
//SOM_932_(unregistered) = "未登録"; //mi-tou-roku //0x490030
SOM_932_(SOM_MAP_load_standby_map) = "スタンバイマップをロード";
SOM_932_(SOM_MAP_blend)[16] = {"フォグを混ぜる","スカイを混ぜる"};
//som.menus.cpp
SOM_932_(Shield_katakana) = "シールド";
SOM_932_(Assist_katakana) = "アシスト";
SOM_932_(Diagonal) = "斜め"; //oblique
SOM_932_(Lateral) = "横方向"; 
SOM_932_(Powerful) = "強力";
SOM_932_(Straight) = "突進";
SOM_932_(Defense) = "防御力";
SOM_932_(Attack) = "攻撃力";

#undef SOM_932_
#endif SOM_932_INCLUDED