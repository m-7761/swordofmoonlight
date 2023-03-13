/*
**  bmp2png --- conversion from (Windows or OS/2 style) BMP to PNG
**  png2bmp --- conversion from PNG to (Windows style) BMP
**
**  Copyright (C) 1999-2005 MIYASAKA Masaru
**
**  For conditions of distribution and use,
**  see copyright notice in common.h.
*/

#include "common.h"

#if defined(__DJGPP__)		/* DJGPP V.2 */
#include <crt0.h>
int _crt0_startup_flags = _CRT0_FLAG_DISALLOW_RESPONSE_FILES;
unsigned short _djstat_flags =	/* for stat() */
	_STAT_INODE | _STAT_EXEC_EXT | _STAT_EXEC_MAGIC | _STAT_DIRSIZE |
	_STAT_ROOT_TIME;
#endif

#if defined(__BORLANDC__)	/* Borland C++ */
#include <wildargs.h>
typedef void _RTLENTRY (* _RTLENTRY _argv_expand_fnc)(char *, _PFN_ADDARG);
typedef void _RTLENTRY (* _RTLENTRY _wargv_expand_fnc)(wchar_t *, _PFN_ADDARG);
_argv_expand_fnc  _argv_expand_ptr  = _expand_wild;		/* expand wild cards */
_wargv_expand_fnc _wargv_expand_ptr = _wexpand_wild;
#endif


/* -----------------------------------------------------------------------
**		screen management
*/

#define LINE_LEN	79
#define STATUS_LEN	22
#define PROGBAR_MAX	(LINE_LEN - STATUS_LEN - 1)

static char status_msg[128];
static int  progbar_scale = 0;
static int  progbar_len   = 0;
static int  progbar_pos   = -1;

int quietmode = 0;		/* -Q option */
int errorlog  = 0;		/* -L option */


static void print_status(void)
{
	fprintf(stderr, "\r%-*.*s ", STATUS_LEN, STATUS_LEN, status_msg);
	fflush(stderr);
	progbar_pos = 0;
}

static void put_dots(int dotchar, int num)
{
	int i;

	if (num > PROGBAR_MAX) num = PROGBAR_MAX;
	if (progbar_pos == -1) print_status();

	for (i = progbar_pos; i < num; i++)
		fputc(dotchar, stderr);

	if (progbar_pos < num) {
		progbar_pos = num;
		fflush(stderr);
	}
}

static void print_scale(void)
{
	if (progbar_pos != 0) print_status();
	put_dots('.', progbar_len);
	print_status();
	progbar_scale = 1;
}

static void init_progress_bar(int max)
{
	if (quietmode) return;

	progbar_len = max;
	print_scale();
}

static void update_progress_bar(int num)
{
	if (quietmode) return;

	if (!progbar_scale) print_scale();
	put_dots('o', num);
}

static void clear_line(void)
{
	if (quietmode) return;

	fprintf(stderr, "\r%*c\r", LINE_LEN, ' ');
	progbar_scale = 0;
	progbar_pos   = -1;
}

void xxprintf(const char *fmt, ...)
{
	va_list ap;
	FILE *f;

	va_start(ap, fmt);

	clear_line();
	vfprintf(stderr, fmt, ap);
	fflush(stderr);

	if (errorlog && (f = fopen(errlogfile, "a")) != NULL) {
		vfprintf(f, fmt, ap);
		fclose(f);
	}

	va_end(ap);
}

void set_status(const char *fmt, ...)
{
	va_list ap;

	if (quietmode) return;

	va_start(ap, fmt);
	vsprintf(status_msg, fmt, ap);
	va_end(ap);

	print_status();
}

void feed_line(void)
{
	if (quietmode) return;

	fputc('\n', stderr);
	fflush(stderr);
	progbar_scale = 0;
	progbar_pos   = -1;
}


/* -----------------------------------------------------------------------
**		libpng progress meter
*/

/*
 * -------------------------------------------------------------
 *
 *  PNG ¤Î¥¤¥ó¥¿¡¼¥E¹·Á¼° "Adam7" ¤Î²èÁEÑ¥é¥á¡¼¥¿¡§
 *
 *  ¥Ñ¥¹   Éý  ¹â¤µ  ³«»ÏÅÀ ÌÌÀÑÈE¿Ê¹Ô¾õ¶·
 *  pass width height origin  area progress
 *    0   1/8   1/8   (0,0)   1/64  1/64 (  1.6%)
 *    1   1/8   1/8   (4,0)   1/64  1/32 (  3.1%)
 *    2   1/4   1/8   (0,4)   1/32  1/16 (  6.3%)
 *    3   1/4   1/4   (2,0)   1/16  1/8  ( 12.5%)
 *    4   1/2   1/4   (0,2)   1/8   1/4  ( 25.0%)
 *    5   1/2   1/2   (1,0)   1/4   1/2  ( 50.0%)
 *    6    1    1/2   (0,1)   1/2   1/1  (100.0%)
 *
 *  Adam7 ¤Î»þ¤Î¿Ê¹Ô¾õ¶·»»½ÐË¡¡§
 *
 *  (width / 8) * 1 ¤Î¥Ô¥¯¥»¥E°¥E¼¥×¤ò£±¥Ö¥úÁÃ¥¯¤È¹Í¤¨¡¢
 *  ¤³¤Î¥Ö¥úÁÃ¥¯¤òÄÌ»»¤Ç¤¤¤¯¤Ä½ÐÎÏ¤·¤¿¤«¤Ç¿Ê¹Ô¾õ¶·¤ò»»½Ð¤¹¤E£
 *  Îã¤¨¤Ð pass 0 ¤Î»þ¤Ï¡¢²£Éý¤¬¸µ¤Î²èÁEÎ 1/8 ¤Ê¤Î¤Ç¡¢
 *  ¥³¡¼¥EÐ¥Ã¥¯´Ø¿ô¤¬¸Æ¤Ð¤EE°EÔ½ÐÎÏ¤¹¤E¤´¤È¤Ë£±¥Ö¥úÁÃ¥¯¤E *  ½ÐÎÏ¤·¤¿¤³¤È¤Ë¤Ê¤ê¡¢pass 4 ¤Î»þ¤Ï(Æ±ÍÍ¤Ë¹Í¤¨¤Æ)£´¥Ö¥úÁÃ¥¯¤E *  ½ÐÎÏ¤·¤¿¤³¤È¤Ë¤Ê¤E£
 *  ¤³¤Î·×»»ÊýË¡¤Ë¤è¤EÈ¡¢ÆÃÄê¤Î¥Ñ¥¹¤¬Â¸ºß¤·¤Ê¤¯¤Ê¤Eè¤¦¤Ê
 *  ¶ËÃ¼¤Ë¾®¤µ¤¤²èÁEÇ¤Ê¤¤¸Â¤E²¼¤Î maxcount_adam7() ¤ò»²¾È)¡¢
 *  ²èÁE´ÂÎ¤ÎÁúÁÖ¥úÁÃ¥¯¿ô¤Ï (height * 8) ¤ËÅù¤·¤¯¤Ê¤E£
 *
 *  ¼ÂºÝ¤Ë¤³¤ÎÊý¼°¤Ç¿Ê¹Ô¾õ¶·¤òÉ½¼¨¤·¤Æ¤ß¤EÈ¡¢Á°È¾ÉôÊ¬(pass0-5)
 *  ¤è¤ê¤â¸åÈ¾ÉôÊ¬(pass6)¤¬Â®¤¯¿Ê¹Ô¤¹¤Eè¤¦¤Ë¸«¤¨¤E£¤³¤EÏ¡¢
 *  Adam7 ¤ÎÆÃÄ§¤È¤·¤Æ¥Ô¥¯¥»¥E¬½Ä²£ÁÐÊý¸þ¤Ë´Ö°ú¤«¤EÆÊ¬²E *  ¤µ¤EÆ¤ª¤ê¡¢ÆÃ¤Ë²£Êý¸þ¤Ë´Ö°ú¤«¤EÆ¤¤¤Epass0-5 (Á°È¾ÉôÊ¬)
 *  ¤Ç¤Ï²èÁEÎºÆ¹½À®¤Ë»þ´Ö¤¬¤«¤«¤Ã¤Æ¤¤¤Eâ¤Î¤È»×¤EEE£
 *
 * -------------------------------------------------------------
 */

static png_uint_32 counter;
static png_uint_32 maxcount;
static int         barlen;


static png_uint_32
 maxcount_adam7(png_uint_32 width, png_uint_32 height)
{
	png_uint_32 c = 0;

	if (    1    ) c += ((height - 0 + 7) / 8) * 1;		/* Pass 0 */
	if (width > 4) c += ((height - 0 + 7) / 8) * 1;		/* Pass 1 */
	if (    1    ) c += ((height - 4 + 7) / 8) * 2;		/* Pass 2 */
	if (width > 2) c += ((height - 0 + 3) / 4) * 2;		/* Pass 3 */
	if (    1    ) c += ((height - 2 + 3) / 4) * 4;		/* Pass 4 */
	if (width > 1) c += ((height - 0 + 1) / 2) * 4;		/* Pass 5 */
	if (    1    ) c += ((height - 1 + 1) / 2) * 8;		/* Pass 6 */

	return c;
}


/*
**		initialize the progress meter
*/
void init_progress_meter(png_structp png_ptr, png_uint_32 width,
                         png_uint_32 height)
{
	enum { W = 1024, H = 768 };

	if (png_set_interlace_handling(png_ptr) == 7) {
		maxcount = maxcount_adam7(width, height);	/* interlaced image */
	} else {
		maxcount = height;							/* non-interlaced image */
	}
	if (height > ((png_uint_32)W * H) / width) {
		barlen = PROGBAR_MAX;
	} else {
		barlen = (PROGBAR_MAX * width * height + (W * H - 1)) / (W * H);
	}
	counter = 0;
	init_progress_bar(barlen);
}


/*
**		row callback function for progress meter
*/
void row_callback(png_structp png_ptr, png_uint_32 row, int pass)
{
/*	static const png_byte step[] = { 1, 1, 2, 2, 4, 4, 8 }; */

	if (row == 0) pass--;
	/*
	 *	libpng's bug ?? : In the case of interlaced image,
	 *	  this function is called with row=0 and pass=current_pass+1
	 *	  when the row should be equal to height and the pass should
	 *	  be equal to current_pass.
	 */

	counter += (1 << (pass >> 1));	/* step[pass]; */
	update_progress_bar(barlen * counter / maxcount);
}


/* -----------------------------------------------------------------------
**		libpng error handling
*/

/*
**		fatal error handling function
*/
void png_my_error(png_structp png_ptr, png_const_charp message)
{
	xxprintf("ERROR(libpng): %s - %s\n", message,
	             (char *)png_get_error_ptr(png_ptr));
	longjmp(png_jmpbuf(png_ptr), 1);
}


/*
**		non-fatal error handling function
*/
void png_my_warning(png_structp png_ptr, png_const_charp message)
{
	xxprintf("WARNING(libpng): %s - %s\n", message,
	             (char *)png_get_error_ptr(png_ptr));
}


/* -----------------------------------------------------------------------
**		image buffer management
*/

/*
**		allocate image buffer
*/
BOOL imgbuf_alloc(IMAGE *img)
{
	BYTE *bp, **rp;
	LONG n;

	if (img->palnum > 0) {
		img->palette = malloc((size_t)img->palnum * sizeof(PALETTE));
		if (img->palette == NULL) { imgbuf_init(img); return FALSE; }
	} else {
		img->palette = NULL;
	}
	img->rowbytes = ((DWORD)img->width * img->pixdepth + 31) / 32 * 4;
	img->imgbytes = img->rowbytes * img->height;
	img->rowptr   = malloc((size_t)img->height * sizeof(BYTE *));
	img->bmpbits  = malloc((size_t)img->imgbytes);

	if (img->rowptr == NULL || img->bmpbits == NULL) {
		imgbuf_free(img); imgbuf_init(img); return FALSE;
	}

	n  = img->height;
	rp = img->rowptr;
	bp = img->bmpbits;

	if (img->topdown) {
		while (--n >= 0) {
			*(rp++) = bp;
			bp += img->rowbytes;
		/*	((DWORD *)bp)[-1] = 0;	*/
		}
	} else {	/* bottom-up */
		bp += img->imgbytes;
		while (--n >= 0) {
			/* fill zeros to padding bytes (for write_bmp()) */
			((DWORD *)bp)[-1] = 0;
			bp -= img->rowbytes;
			*(rp++) = bp;
		}
	}

	return TRUE;
}


/*
**		free image buffer allocated by imgbuf_alloc()
*/
void imgbuf_free(IMAGE *img)
{
	free(img->palette);
	free(img->rowptr);
	free(img->bmpbits);
}


/*
**		init image buffer to empty
*/
void imgbuf_init(IMAGE *img)
{
	img->palette = NULL;
	img->rowptr  = NULL;
	img->bmpbits = NULL;
}


/* -----------------------------------------------------------------------
**		¥³¥Þ¥ó¥É¥é¥¤¥ó°ú¿ô¤Î½èÍý
*/

#define isoption(p)	(IsOptChar((p)[0]) && (p)[1]!='\0')

/*
**	¥³¥Þ¥ó¥É¥é¥¤¥ó°ú¿ô¤òÆÉ¤E*/
int parsearg(int *opt, char **arg, int argc, char **argv, char *aopts)
{
	static int   agi = 1;
	static char *agp = NULL;
	char *p;
	int c, i;

	if (agp != NULL && *agp == '\0') {
		agp = NULL;
		agi++;
	}
	if (agi >= argc) return 0;		/* end */

	if (p = argv[agi], agp == NULL && !isoption(p)) {
		/* non-option element */
		c = 0;
		agi++;
	} else {
		if (agp == NULL) agp = p + 1;
		if (c = (*agp & 0xFF), strchr(aopts, c) != NULL) {
			/* option with an argument */
			if (p = agp + 1, *p != '\0') {
				/*NULL*/;
			} else if (i = agi + 1, p = argv[i], i < argc && !isoption(p)) {
				agi = i;
			} else {
				p = NULL;
			}
			agp = NULL;
			agi++;
		} else {
			/* option without an argument */
			p = NULL;
			agp++;
		}
	}
	*opt = c;
	*arg = p;

	return 1;
}


/*
**	´Ä¶­ÊÑ¿ô¤Ç»ØÄê¤µ¤EÆ¤¤¤Eª¥×¥·¥ç¥ó¤Eargc, argv ¤ËÊ»¹ç¤¹¤E*/
char **envargv(int *argcp, char ***argvp, const char *envn)
{
	int argc, nagc, envc, i;
	char **argv, **nagv, *envs, *ep;

	ep = getenv(envn);
	if (ep == NULL || ep[0] == '\0') return NULL;

	envs = malloc(strlen(ep) + 1);
	if (envs == NULL) return NULL;
	strcpy(envs, ep);

	envc = tokenize(envs, envs);
	if (envc == 0) { free(envs); return NULL; }

	argc = *argcp;
	argv = *argvp;
	nagv = malloc((argc + envc + 1) * sizeof(char *));
	if (nagv == NULL) { free(envs); return NULL; }

	nagc = 1;
	nagv[0] = argv[0];

	for (i = 0; i < envc; i++) {
		nagv[nagc++] = envs;
		while (*(envs++) != '\0') ;
	}
	for (i = 1; i < argc; i++) {
		nagv[nagc++] = argv[i];
	}
	nagv[nagc] = NULL;

	*argcp = nagc;
	*argvp = nagv;

	return argv;
}


/*
**	Ê¸»úÎó¤ò¶õÇòÊ¸»E¥¹¥Ú¡¼¥¹/¿åÊ¿¥¿¥Ö/²þ¹Ô)¤Î½ê¤Ç¶èÀÚ¤E¥¯¥ª¡¼¥È½èÍýÉÕ¤­)
**	¶èÀÚ¤é¤E¿ÉôÊ¬Ê¸»úÎó¤Î¿ô¤òÊÖ¤¹
*/
int tokenize(char *buf, const char *str)
{
	enum { STR = 0x01, QUOTE = 0x02 };
	int flag = 0;
	int num = 0;
	char c;
	int i;

	while ((c = *str++) != '\0') {
		if (!(flag & QUOTE) &&
		    (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
			if (flag & STR) {
				flag &= ~STR;
				*buf++ = '\0';
			}
		} else {
			if (!(flag & STR)) {
				flag |= STR;
				num++;
			}
			switch (c) {
			case '\\':
				/*
				 *	Escaping of `"' is the same as
				 *	command-line parsing of MS-VC++.
				 *
				 *	ex.) "     ->      quote
				 *	     \"    ->      "
				 *	     \\"   -> \  + quote
				 *	     \\\"  -> \  + "
				 *	     \\\\" -> \\ + quote
				 *	     \\\\\ -> \\\\\
				 */
				for (i = 1; (c = *str) == '\\'; str++, i++) ;
				if (c == '"') {
					while ((i -= 2) >= 0)
						*buf++ = '\\';
					if (i == -1) {
						*buf++ = '"';
						str++;
					}
				} else {
					while ((--i) >= 0)
						*buf++ = '\\';
				}
				break;

			case '"':
				flag ^= QUOTE;
				break;

			default:
				*buf++ = c;
			}
		}
	}
	if (flag & STR) *buf = '\0';

	return num;
}


/* -----------------------------------------------------------------------
**		¥Õ¥¡¥¤¥EË´Ø¤¹¤E¨ÍÑ½èÍý
*/

/*
**	Ê£¿ô³¬ÁØ¤Î¥Ç¥£¥E¯¥È¥ê¤ò°EÙ¤ËºûÜ®¤¹¤E*/
int makedir(const char *path)
{
	char dir[FILENAME_MAX];
	struct stat sbuf;
	char *p, c;
	int r;

	delslash(strcpy(dir, path));
	if (stat(dir, &sbuf) == 0) {
		if ((sbuf.st_mode & S_IFMT) == S_IFDIR) return 0;
		/* errno = EEXIST; */
		return -1;
	}
	p = path_skiproot(dir);
	do {
		p = path_nextslash(p);
		c = *p;  *p = '\0';
		r = MKDIR(dir, 0777);
		*p++ = c;
	} while (c != '\0');

	return r;
}


/*
**	´ûÂ¸¤ÎÆ±Ì¾¥Õ¥¡¥¤¥Eò¥Ð¥Ã¥¯¥¢¥Ã¥×(¥EÍ¡¼¥E¤¹¤E*/
int renbak(const char *path)
{
	char bak[FILENAME_MAX];
	struct stat sbuf;
	char *sfx;
	int i;

	strcpy(bak, path);
	if (stat(bak, &sbuf) != 0) return 0;

#ifdef MSDOS
	sfx = suffix(bak);
#else
	sfx = bak + strlen(bak);
#endif
	strcpy(sfx, ".bak");
	i = 0;
	while (1) {
		if (stat(bak, &sbuf) != 0 && rename(path, bak) == 0) return 0;
		if (i >= 1000) break;
		sprintf(sfx, ".%03d", i++);
	}
	return -1;
}


/*
**	¥Õ¥¡¥¤¥EÎ¥¿¥¤¥à¥¹¥¿¥ó¥×¤ò¥³¥Ô¡¼¤¹¤E*/
int cpyftime(const char *srcf, const char *dstf)
{
	struct stat sbuf;
	struct utimbuf ubuf;

	if (stat(srcf, &sbuf) != 0) return -1;

	ubuf.actime  = sbuf.st_atime;
	ubuf.modtime = sbuf.st_mtime;

	return utime(dstf, &ubuf);
}


/*
**	¥Ð¥¤¥Ê¥Eâ¡¼¥É¤ÎÉ¸½àÆþ½ÐÎÏ¥¹¥È¥ê¡¼¥à¤ò¼èÆÀ¤¹¤E*/
FILE *binary_stdio(int fd)
{
	FILE *fp;

	if (fd != 0 && fd != 1) return NULL;

#ifdef BINSTDIO_FDOPEN
	fp = fdopen(fd, (fd==0)? "rb":"wb");
#else
#ifdef BINSTDIO_SETMODE
	setmode(fd, O_BINARY);
#endif
	fp = (fd == 0) ? stdin : stdout;
#endif
	return fp;
}


/* -----------------------------------------------------------------------
**   path functions
*/

/*
**	Return a pointer that points the suffix of the PATH
**	ex.) c:\dosuty\log\test.exe -> .exe
**	ex.) c:\dosuty\log\test.tar.gz -> .gz
*/
char *suffix(const char *path)
{
	char c, *p, *q, *r;

	for (r = q = p = basname(path); (c = *p) != '\0'; p++)
		if (c == '.') q = p;
	if (q == r) q = p;			/* dotfile with no suffix */

	return q;
}


/*
**	Return a pointer that points the basename of the PATH
**	ex.) c:\dos\format.exe -> format.exe
*/
char *basname(const char *path)
{
	const char *p, *q;

	for (p = path_skiproot(path);
	     *(q = path_nextslash(p)) != '\0'; p = q + 1) ;

	return (char *)p;
}


/*
**	Append a path-delimiter to the PATH. If the PATH is a string
**	like "c:\", "\", "c:", "", do nothing.
**	ex.) c:\dos -> c:\dos\
*/
char *addslash(char *path)
{
	char *p, *q;

	for (p = path_skiproot(path);
	     *(q = path_nextslash(p)) != '\0'; p = q + 1) ;
	/*
	 *	s = path_skiproot( path );
	 *	if( q==s && q==p ) - s is a mull string.
	 *	if( q!=s && q==p ) - s is followed by a path delimiter.
	 *	if( q!=s && q!=p ) - s is not followed by a path delimiter.
	 */
	if (q != p) {
		*q++ = PATHDELIM;
		*q   = '\0';
	}

	return path;
}


/*
**	Remove a path-delimiter at the end of the PATH. If the PATH is
**	a string like "c:\", "\", "c:", "", append a dot.
**	ex.) c:\dos\ -> c:\dos
**	     c:\ -> c:\.
*/
char *delslash(char *path)
{
	char *p, *q, *s;

	for (p = s = path_skiproot(path);
	     *(q = path_nextslash(p)) != '\0'; p = q + 1) ;
	/*
	 *	if( q==s && q==p ) - s is a mull string.
	 *	if( q!=s && q==p ) - s is followed by a path delimiter.
	 *	if( q!=s && q!=p ) - s is not followed by a path delimiter.
	 */
	if (q == s) {
		*q++ = '.';
		*q   = '\0';
	} else if (q == p) {
		*--q = '\0';
	}

	return path;
}


char *path_skiproot(const char *path)
{
#ifdef DRIVESUFFIX
	if (isalpha((unsigned char)path[0])
	    && path[1] == DRIVESUFFIX) path += 2;
#endif
	if (IsPathDelim(path[0])) path++;
	return (char *)path;
}


char *path_nextslash(const char *path)
{
	char c;

	for (; (c = *path) != '\0'; path++) {
		if (IsDBCSLead((unsigned char)c)) {
			if (*(++path) == '\0') break;
			continue;
		}
		if (IsPathDelim(c)) break;
	}
	return (char *)path;
}

#ifdef WIN32_LFN

/*
**	return TRUE if the PATH is a dos-style filename.
*/
int is_dos_filename(const char *path)
{
	unsigned char c;
	char *b, *p;

	for (b = p = basname(path); (c = *p) != '\0' && c != '.'; p++)
		if (islower(c)) return 0;
	if ((p - b) == 0 || (p - b) > 8) return 0;
	if (c == '.') {
		for (b = ++p; (c = *p) != '\0'; p++)
			if (islower(c) || c == '.') return 0;
		if ((p - b) == 0 || (p - b) > 3) return 0;
	}
	return 1;
}

#endif /* WIN32_LFN */
