
#ifndef EX_MOVIE_INCLUDED
#define EX_MOVIE_INCLUDED

namespace EX
{
	struct Movie //name subject to change
	{
		//intended to be a floating point rect

		float x1, y1, x2, y2; //left top right bottom 
	};

	void playing_movie(const wchar_t *filename);

	void stop_playing_movie();

	bool is_playing_movie();
}

#endif //EX_MOVIE_INCLUDED
