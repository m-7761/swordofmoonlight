
#ifndef EX_MOVIE_INCLUDED
#define EX_MOVIE_INCLUDED

namespace EX
{
	void playing_movie(const wchar_t *filename, float volume=1.0f);

	void stop_playing_movie();

	bool is_playing_movie();
	void is_playing_movie_WM_SIZE(long,long); //Ex.window.cpp
}

#endif //EX_MOVIE_INCLUDED
