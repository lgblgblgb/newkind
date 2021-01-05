/*
 * Elite - The New Kind.
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder.
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 *
 */

/*
 * file.h
 */

#ifndef ETNK_FILE_H
#define ETNK_FILE_H

extern const char *pref_path;

extern void datafile_select ( const char *fn, const Uint8 **data_p, int *data_size );

extern void write_config_file (void);
extern void read_config_file (void);
extern int  save_commander_file (const char *path);
extern int  load_commander_file (const char *path);

#endif
