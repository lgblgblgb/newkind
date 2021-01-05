/*
 * Elite - The New Kind.
 *
 * Allegro version of the keyboard routines.
 *
 * The code in this file has not been derived from the original Elite code.
 * Written by C.J.Pinder 1999-2001.
 * email: <christian@newkind.co.uk>
 *
 */

/*
 * keyboard.h
 *
 * Code to handle keyboard input.
 */

#ifndef ETNK_KEYBOARD_H
#define ETNK_KEYBOARD_H


#define KEY_0 1
#define KEY_a 2
#define KEY_A 3
#define KEY_BACKSPACE 4
#define KEY_c 5
#define KEY_COMMA 6
#define KEY_d 7
#define KEY_DOWN 8
#define KEY_e 9
#define KEY_ESCAPE 10
#define KEY_f 11
#define KEY_F1 12
#define KEY_F10 13
#define KEY_F11 14
#define KEY_F12 15
#define KEY_F2 16
#define KEY_F3 17
#define KEY_F4 18
#define KEY_F5 19
#define KEY_F6 20
#define KEY_F7 21
#define KEY_F8 22
#define KEY_F9 23
#define KEY_h 24
#define KEY_i 25
#define KEY_j 26
#define KEY_LCTRL 27
#define KEY_LEFT 28
#define KEY_m 29
#define KEY_n 30
#define KEY_o 31
#define KEY_p 32
#define KEY_r 33
#define KEY_RCTRL 34
#define KEY_RETURN 35
#define KEY_RIGHT 36
#define KEY_s 37
#define KEY_SLASH 38
#define KEY_SPACE 39
#define KEY_STOP 40
#define KEY_t 41
#define KEY_TAB 42
#define KEY_u 43
#define KEY_UP 44
#define KEY_x 45
#define KEY_y 46
#define KEY_z 47
#define KEY_MAX 48

extern char key[KEY_MAX];
extern int  sdl_last_pressed_key;

extern int readkey ( void );
 
extern int kbd_F1_pressed;
extern int kbd_F2_pressed;
extern int kbd_F3_pressed;
extern int kbd_F4_pressed;
extern int kbd_F5_pressed;
extern int kbd_F6_pressed;
extern int kbd_F7_pressed;
extern int kbd_F8_pressed;
extern int kbd_F9_pressed;
extern int kbd_F10_pressed;
extern int kbd_F11_pressed;
extern int kbd_F12_pressed;
extern int kbd_y_pressed;
extern int kbd_n_pressed;
extern int kbd_zoom_pressed;
extern int kbd_fire_pressed;
extern int kbd_ecm_pressed;
extern int kbd_energy_bomb_pressed;
extern int kbd_hyperspace_pressed;
extern int kbd_ctrl_pressed;
extern int kbd_jump_pressed;
extern int kbd_escape_pressed;
extern int kbd_dock_pressed;
extern int kbd_d_pressed;
extern int kbd_origin_pressed;
extern int kbd_find_pressed;
extern int kbd_i_pressed;
extern int kbd_fire_missile_pressed;
extern int kbd_target_missile_pressed;
extern int kbd_unarm_missile_pressed;
extern int kbd_pause_pressed;
extern int kbd_resume_pressed;
extern int kbd_inc_speed_pressed;
extern int kbd_dec_speed_pressed;
extern int kbd_up_pressed;
extern int kbd_down_pressed;
extern int kbd_left_pressed;
extern int kbd_right_pressed;
extern int kbd_enter_pressed;
extern int kbd_backspace_pressed;
extern int kbd_space_pressed;
extern char old_key[];


int kbd_keyboard_startup (void);
int kbd_keyboard_shutdown (void);
void kbd_poll_keyboard (void);
int kbd_read_key (void);
void kbd_clear_key_buffer (void);

#endif
