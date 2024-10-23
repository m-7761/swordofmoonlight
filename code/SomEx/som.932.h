
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
SOM_932_(Systemwide) = "�V�X�e���S��"; 
SOM_932_(Fitness) = "�t�B�b�g�l�X";
SOM_932_(Powers)[5] = {"�̗�","����","����"};
SOM_932_(Item) = "�A�C�e��";
SOM_932_(Item_Haul) = "�A�C�e��������";
SOM_932_(Gold_Haul) = "�S�[���h������";
SOM_932_(LVT) = "�����^�C�v"; //���x��
SOM_932_(LEVEL) = "�k�d�u�d�k";
SOM_932_(LVT_Next) = "�K�v�o���l (0-99%)";
SOM_932_(LVT_Max)[10] = {"�ő�g�o","�ő�l�o","�ő�̗�","�ő喂��","�ő呬��"};
SOM_932_(Player_Setup) = "��ڲ԰�ݒ� 1-2";
SOM_932_(Magic_Table) = "���@ð���";
SOM_932_(Field) = "�t�B�[���h";
SOM_932_(Map) = "���̃}�b�v";
SOM_932_(Status) = "�X�e�[�^�X";
SOM_932_(Normal) = "�ʏ�";
SOM_932_(System) = "�V�X�e������";
SOM_932_(Save) = "�Z�[�u";
SOM_932_(Dash) = "�_�b�V��";
SOM_932_(Play) = "�v���[ (Alt+Break)";
SOM_932_(Fall) = "�������� (���Z�b�g����܂�)";
SOM_932_(Fall_Height) = "�g��";
SOM_932_(Fall_Impact) = "�Ռ� (���[�g�����b)";
SOM_932_(Weapon) = "����";
SOM_932_(Head) = "��";
SOM_932_(Body) = "�Z";
SOM_932_(Hands) = "�U��";
SOM_932_(Feet) = "�";
SOM_932_(Shield) = "��";
SOM_932_(Accessory) = "�����i";
SOM_932_(Magic) = "���@";
SOM_932_(Spell) = "���@�ԍ� 0-31";
SOM_932_(attack) = "%s�̍U��";
SOM_932_(defense) = "%s�̖h��";
SOM_932_(attacks)[3] = {"�a","��","�h","��","�y","��","��","��"};
SOM_932_(Math)[20] = {"�ύX","���₷","���炷","�f�[�^���猸�Z","��Z","���Z","�f�[�^�����Z","�p�[�Z���g","�f�[�^�̃p�[�Z���g","��]","�f�[�^�̏�]"};
//som.hacks.cpp
SOM_932_(Super_Modes)[32] = {"�R�c�A���`�G�C���A�V���O","�S�{�X�[�p�[�T���v�����O","�X�[�p�[�T���v���ł��܂��� (%d)"};
SOM_932_(Stereo_Modes)[32] = {"�ŏ��X�[�p�[�T���v�����O","�ő�X�[�p�[�T���v�����O"};
SOM_932_(Button_Swap) = "�{�^���X���b�v %d-%d";
SOM_932_(Analog_Mode) = "�A�i���O���[�h %d-%c";
SOM_932_(Analog_Disabled) = "�A�i���O�����ɂ���"; 
SOM_932_(Zoom) = "%d���Y�[��";
SOM_932_(Mono) = "�X�e���I�͗��p�ł��܂��� (%d)";
SOM_932_(Stereo) = "�g�p����VR (Win+Y, %sF2)";
SOM_932_(IPD) = "%d ����̌���";
SOM_932_(Master_Volume) = "�}�X�^�[�{�����[�� %d";
SOM_932_(Start)[3][16] =
{
/*do_som*/ {"PUSH ANY KEY","NEW GAME","CONTINUE"},
/*do_kf*/  {"","",""},
/*do_kf2*/ {"�r�s�`�q�s","�͂��߂�","���[�h����"}, 
/*do_kf3*/ {"","NEW","CONTINUE"}, 
/*do_st*/  {"PUSH START","NEW","LOAD"},
};
SOM_932_(Equip_Broke) = "�@�킪��ꂽ";
//som.menus.cpp
SOM_932_(Take) = "%s�����܂����H";
SOM_932_(Press_any_button) = "�x���̃{�^��������";
SOM_932_(Now_press_another) = "���ʂ̃{�^��������";
SOM_932_(Button) = "�{�^��"; 
//SOM_932W_(Buttons) = L"���@�A�B�C�D�E�F�G�H�I�J�K�L�M�N�O�P�Q�R�S";
SOM_932_(Numerals)[3] = 
{"�O","�P","�Q","�R","�S","�T","�U","�V","�W","�X"};
SOM_932_(Date) = "%4d�N%2d��%2d��";
SOM_932_(States)[4] = 
{" ��"," �"," ��"," ��"," �d"};
//SOM_932W_(States)[3] = 
//{L" ��",L" �",L" ��",L" ��",L" �d"}; //Ex.output.cpp
SOM_932_(Shadow_Tower)[16] = 
{				//substitutes
"�e��Q�[�W�\��",	"���Q�[�W�\��",
"�����\��",		"�e���ԕ\��",
"�A�C�e���\��",	"�_���[�W�\��",
"���s����",		"���s�ړ�����"
};
SOM_932_(HighColor) = "�n�C�J���[";
SOM_932_(ColorModes)[16] = 
{"�n�C�J���[","�g�D���[�J���["}; 
SOM_932_(Graphics) = "�O���t�B�b�N�X";
SOM_932_(Anisotropy) = "�ٕ���";
SOM_932_(AnisoModes)[24] = 
{"4x (�����\)","8x (���i��)","16x (�ō��̕i��)"};
SOM_932_(ConcealDisplay) = "�B���\��";
SOM_932_(EnlargeDisplay) = "�g��\��";
//som.money.cpp
SOM_932_(Money) = "%d%s��ɓ��ꂽ";
//som.status.cpp
SOM_932_(PAUSED) = "�ꎞ��~";
//SOM_932W_(SmartJoy) = L"���Z����";
SOM_932_(Shadow_Tower_HP) = "�g�o %3d";
SOM_932_(Shadow_Tower_MP) = "�l�o %3d";
//som.state.cpp
SOM_932_(MS_Mincho) = "�l�r ����";
//SOM_932W_(MS_Mincho) = L"�l�r ����";
SOM_932_(Nothing) = "�����Ȃ�";
//SOM_MAIN.cpp
//SOM_932W_(All) = L"���ׂ�";
//workshop.cpp
SOM_932_(PrtsEdit_99) = "99�͍��ł�";
SOM_932_(PrtsEdit_filter) = "�߰� ���̨�� ̧��(*.prt)\0*.prt\0";
SOM_932_(EneEdit_attack123) = "%s�ڍU��%s"; //���ڍU���Q //�ԐڍU���Q
//SOM_MAP magic menu
//SOM_932_(unregistered) = "���o�^"; //mi-tou-roku //0x490030
SOM_932_(SOM_MAP_load_standby_map) = "�X�^���o�C�}�b�v�����[�h";
SOM_932_(SOM_MAP_blend)[16] = {"�t�H�O��������","�X�J�C��������"};
//som.menus.cpp
SOM_932_(Shield_katakana) = "�V�[���h";
SOM_932_(Assist_katakana) = "�A�V�X�g";
SOM_932_(Diagonal) = "�΂�"; //oblique
SOM_932_(Lateral) = "������"; 
SOM_932_(Powerful) = "����";
SOM_932_(Straight) = "�ːi";
SOM_932_(Defense) = "�h���";
SOM_932_(Attack) = "�U����";

#undef SOM_932_
#endif SOM_932_INCLUDED