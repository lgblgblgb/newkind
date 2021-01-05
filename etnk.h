/* (C)2017 LGB (Gabor Lenart)
 * For "Eltie New Kind", my own SDL2-ported version, generic header file, also includes SDL.h
 * It also contains some "config" options, including the old config.h (what will remain from it, we will see)
 */

#ifndef _ETNK_ETNK_H
#define _ETNK_ETNK_H

#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#include <SDL.h>

typedef int32_t fixed;



/* --- configuration related, old config.h --- */

#define GFX_ALLEGRO
/* #define GFX_WIN32_GDI */
/* #define GFX_OPENGL */
/* #defime GFX_X_WINDOWS */
/* #define GFX_DIRECTX */

/* Set the screen resolution ... (if nothing is defined, assume 256x256) */
#define RES_800_600
/* #define RES_512_512 */
/* #define RES_640_480 */
/* #define RES_320_240 */

/* --- define from the mdw's branch, for "HACKING" --- */

#define HACKING

/* --- my own hacks ~ LGB --- */

/* try to work-around problem with crash after jumping the system, because of Sun having type ID of -2 */
/* probably this is WRONG, TimSC suggested other more rational fix! So this is commented out for now, waiting for total removal. */
// #define HACKING_SUN_IS_POSITIVE

/* --- system stuffs --- */

#define USE_REGPARM
#define OUR_WINDOW_TITLE	"Elite - The New Kind"

#ifdef __GNUC__
#define ETNK_LIKELY(__x__)	__builtin_expect(!!(__x__), 1)
#define ETNK_UNLIKELY(__x__)	__builtin_expect(!!(__x__), 0)
#define ETNK_INLINE		__attribute__ ((__always_inline__)) inline
#else
#define ETNK_LIKELY(__x__)	(__x__)
#define ETNK_UNLIKELY(__x__)	(__x__)
#define ETNK_INLINE		inline
#endif

#if defined(USE_REGPARM) && defined(__GNUC__) && !defined(__EMSCRIPTEN__)
#define ETNK_REGPARM(__n__)	__attribute__ ((__regparm__ (__n__)))
#else
#define ETNK_REGPARM(__n__)
#endif

#ifndef _WIN32
#	define O_BINARY		0
#	define DIRSEP_STR	"/"
#	define DIRSEP_CHR	'/'
#	define NL		"\n"
#	define PRINTF_LLD	"%lld"
#	define PRINTF_LLU	"%llu"
#	define MKDIR(__n)	mkdir((__n), 0775)
#else
#	define DIRSEP_STR	"\\"
#	define DIRSEP_CHR	'\\'
#	define NL		"\r\n"
#	define PRINTF_LLD	"%I64d"
#	define PRINTF_LLU	"%I64u"
#	define MKDIR(__n)	mkdir(__n)
#endif


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define ETNK_EXIT(n)	do { emscripten_cancel_main_loop(); emscripten_force_exit(n); exit(n); } while (0)
#define EMSCRIPTEN_SDL_BASE_DIR "/files/"
#define MSG_POPUP_WINDOW(sdlflag, title, msg, win) \
	do { if (1 || sdlflag == SDL_MESSAGEBOX_ERROR) { EM_ASM_INT({ window.alert(Pointer_stringify($0)); }, msg); } } while(0)
#else
#define MSG_POPUP_WINDOW(sdlflag, title, msg, win) SDL_ShowSimpleMessageBox(sdlflag, title, msg, win)
#include <stdlib.h>
#define	ETNK_EXIT(n)	exit(n)
#endif

#define _REPORT_WINDOW_(sdlflag, dosdl, str, ...) do { \
	char _buf_for_win_msg_[4096]; \
	snprintf(_buf_for_win_msg_, sizeof _buf_for_win_msg_, __VA_ARGS__); \
	fprintf(stderr, str ": %s" NL, _buf_for_win_msg_); \
	if (dosdl) { \
		MSG_POPUP_WINDOW(sdlflag, OUR_WINDOW_TITLE, _buf_for_win_msg_, sdl_win); \
		SDL_RaiseWindow(sdl_win); \
	} \
} while (0)

#define ERROR_WINDOW(...)	_REPORT_WINDOW_(SDL_MESSAGEBOX_ERROR, 1, "ERROR", __VA_ARGS__)

//extern SDL_Window	*sdl_win;


#endif
