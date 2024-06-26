
#ifndef SOM_FILES_INCLUDED
#define SOM_FILES_INCLUDED

namespace SOM
{
	namespace PARAM
	{
		extern struct Sys
		{
			const struct Dat
			{				
				enum{ counter_s=1024 };

				char counters[counter_s][31];

				enum{ magics_s=32 };

				unsigned char magics[magics_s];	
			
				enum{ messages_s=12 };

				//3 are tacked onto the end
				char messages[12][41];

				//turn is in radians
				float walk, dash, turn;

				static bool open();

			}*dat;

			Sys():dat(){}

		}Sys;

		extern struct Item
		{
			const struct Prm
			{	
				enum{ records_s=256 };

				char records[records_s][31];
				short profiles[records_s];

				static bool open(), wrote;

			}*prm;

			const struct Pr2 //PRO
			{	
				/*UNUSED/ABANDONED (2021)
				//som_map_codecbnproc has some code
				//that suggests the idea here was to
				//filter some comboboxes in SOM_MAP

				enum use:unsigned char
				{
					unused=0,
					supply=1,
					weapon=2,
					attire=3,
					shield=4,
					effect=5,

				}uses[Prm::records_s];

				struct fit //
				{					
					unsigned char head:1;
					unsigned char body:1;
					unsigned char hand:1;
					unsigned char feet:1;	

				}fits[Prm::records_s];
				*/
				static bool open(){ return false; }
				
				static bool wrote;

			}*pr2;
			
			const struct Arm //2021
			{
				enum{ records_s=1024 };

				struct record //88
				{
					//this is a my/arm profile with
					//the relevant data converted to
					//plain binary packed into the 3d
					//model field (first 31 bytes)

					//NOTE: the model/name fields are
					//swapped in this data structure
					//to simplify the alignment
					WORD mid,mvs[4];
					BYTE _reserved[21];
					char description[31];

					BYTE equip,my;
					float _center;
					DWORD _1:10,_2:10,_3:10,_0b:2;
					BYTE _0a,_,_rem1[6];
					float _scale;
					CHAR _nz,_rem2[3];

				}records[records_s];

				struct file //2023
				{
					wchar_t name[31],data;

				}files[records_s];

				//NOTE: for tools this reads my/arm
				//for games it reads PARAM/ITEM.ARM
				static bool open(),_game(),_tool();

				static void clear(); //SOM_PRM

			}*arm;

			Item():prm(),pr2(),arm(){}

		}Item;

		extern struct Magic
		{
			const struct Prm
			{	
				enum{ records_s=256 };

				char records[records_s][31];
								
				static bool open(), wrote;

			}*prm;

			const struct Pr2 //PRO
			{
				static bool wrote;

			}*pr2;

			Magic():prm(),pr2(){}

		}Magic;

		extern struct Enemy
		{
			const struct Prm
			{			
				enum{ records_s=1024 };

				char records[records_s][31];
								
				static bool open(), wrote;

			}*prm;

			const struct Pr2 //PRO
			{
				static bool wrote;
			
			}*pr2;

			Enemy():prm(),pr2(){}

		}Enemy;

		extern struct NPC
		{
			const struct Prm
			{
				enum{ records_s=1024 };

				char records[records_s][31];
								
				static bool open(), wrote;

			}*prm;

			const struct Pr2 //PRO
			{
				static bool wrote;

			}*pr2;

			NPC():prm(),pr2(){}
		
		}NPC;

		extern struct Obj
		{
			const struct Prm
			{
				static bool wrote;

			}*prm;

			const struct Pr2 //PRO
			{
				static bool wrote;

			}*pr2;

			Obj():prm(),pr2(){}
		
		}Obj;

		//this creates a thread that monitors the
		//pr2/pro/prm file write times and sets the
		//wrote members of the above structs on write
		extern void kickoff_write_monitoring_thread(HWND);
		extern void trigger_write_monitor();
		extern void (*onWrite)();		
	}

	namespace DATA
	{
		extern struct Map
		{
			const struct Evt
			{	
				char header[4];

				enum{ records_s=1024 };

				char records[records_s][252];

				//-1: opens sys.ezt below
				static bool open(int=-1);

				bool error;
				Evt(){ error = true; }

			}*evt;

			//NEW: open ezt with ezt->open()
			static struct Sys{ const Evt *ezt; }sys;

			Map():evt(){}

		}Map[64];

		extern struct Sfx
		{
			const struct Dat
			{
				enum{ records_s=1024 };

				BYTE records[records_s][48];

				static bool open();
				static void clear();

				bool error;
				Dat(){ error = true; }

			}*dat;

			Sfx():dat(){}

		}Sfx;
	}
}

#endif //SOM_FILES_INCLUDED