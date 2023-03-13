
#include <string>
#include <fstream>
#include <iostream>
#include <assert.h>

//FindFirstFile
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>

bool yes_no(char *q, char def)
{
	assert(def=='Y'||def=='N');
		
	std::cout << "\n\n";
	std::cout << q << " (Y/N) [" << def <<"]:"; 
	
	char ok[] = {0,0}; std::cin.getline(ok,2); 	

	if(*ok>='a'&&*ok<='z') *ok-=32;

	return !*ok||*ok==def;
}

//max file name length
const int file_ext = 20;
const int safe_path = MAX_PATH-file_ext-1;

int main(int argc, const char* argv[])
{		
	const char slash[] = "\\";
	
	std::string dir = ".", cat = "set"; 

	retry: char out[MAX_PATH] = "";
	
	std::cout << "\n";
	std::cout << "This program is intended to handle ANSI i/o.";

	if(argc==1)	
	{
		std::cout << "\n\n";
		std::cout << "Directory is ? [" << dir << "]:";	
		
		std::cin.getline(out,safe_path);

		if(*out) dir = out;	*out = '\0'; //paranoia?
	}
	else dir = argv[1]; 	

	std::cout << "\n\n";
	std::cout << "Sets are ? [" << cat << "]: ";

	std::cin.getline(out,MAX_PATH-dir.size()-1-4);

	if(*out) cat = out; *out = '\0'; //paranoia?

	int out_s = 
	_snprintf(out,MAX_PATH,"%s%s%s.set",dir.c_str(),slash,cat.c_str());
	out[MAX_PATH-1] = '\0';

	if(out_s>8&&!strcmp(out+out_s-8," set.set"))
	{
		strcpy(out+out_s-8,".set");
	}

	int out_slash = -1; if(out_s<0) return 1;
		
	for(int i=0;i<out_s;i++) 
	if(out[i]=='/'||out[i]=='\\') out[out_slash=i] = *slash;

	char *file = out+out_slash+1;

	std::cout << "\n\n";
	std::cout << "\Output is " << out << std::endl;

	std::ofstream set;

	if(yes_no("Is this ok?",'Y'))
	{
		set.open(out);

		if(!set.is_open())
		{		
			if(yes_no("Unable to create file. Retry?",'N')) return 0;

			argc = 1; goto retry;
		}
	}
	else
	{
		if(!yes_no("Preview instead?",'Y'))
		{
			if(yes_no("Retry?",'N')) return 0;

			argc = 1; goto retry;
		}
		else set.open("NUL");
	}

	set << cat << "\n\n";

	std::cout << cat << '\n' << std::endl;

	char description[31];

	int types[] = {'prf','prt'};
	char *globs[] = {"*.prf","*.prt",""};

	for(int i=0;*globs[i];i++)
	{
		strcpy(file,globs[i]);

		WIN32_FIND_DATAA found;
		HANDLE glob = FindFirstFileA(out,&found);

		if(glob==INVALID_HANDLE_VALUE) continue;

		do //FindNextFile
		{
			retry2:
			strncpy(file,found.cFileName,file_ext);
			file[file_ext] = '\0';			

			FILE *f = fopen(out,"rb");

			if(!f) //todo: no comment?
			{
				std::cout << "\n\n";
				std::cout << "Unable to open " << found.cFileName << "for comment";
			
				if(!yes_no("Retry",'N')) goto retry2;

				set << '+' << found.cFileName << '\n';
			}
			else //todo: interactive mode?
			{
				if(types[i]=='prt') 
				fseek(f,100,SEEK_SET);
				fread(description,30,1,f);
				fclose(f);

				description[30] = '\0';				

				set << ';' << description << '\n';
				set << '+' << found.cFileName << '\n';				

				std::cout << ';' << description << std::endl;
			}

			std::cout << '+' << found.cFileName << std::endl;

		}while(FindNextFileA(glob,&found));
	}

	if(!yes_no("Retry",'N'))
	{
		argc = 1; goto retry; //courtesy
	}

	return 0;
}