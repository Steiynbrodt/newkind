/*
 * Elite - The New Kind.
 *
 * Allegro 5 keyboard backend.
 *
 * Original Allegro 4 code by C.J.Pinder 1999-2001.
 * Ported to Allegro 5.
 */

#include <stdlib.h>
#include <string.h>

#include <allegro5/allegro.h>
#include "allegro5/keyboard.h"

#include "keyboard.h"     /* keep your header as-is */

/* --- Allegro 4 compatibility globals (keep game logic unchanged) --- */

int kbd_F1_pressed;
int kbd_F2_pressed;
int kbd_F3_pressed;
int kbd_F4_pressed;
int kbd_F5_pressed;
int kbd_F6_pressed;
int kbd_F7_pressed;
int kbd_F8_pressed;
int kbd_F9_pressed;
int kbd_F10_pressed;
int kbd_F11_pressed;
int kbd_F12_pressed;

int kbd_y_pressed;
int kbd_n_pressed;

int kbd_fire_pressed;
int kbd_ecm_pressed;
int kbd_energy_bomb_pressed;
int kbd_hyperspace_pressed;
int kbd_ctrl_pressed;
int kbd_jump_pressed;
int kbd_escape_pressed;

int kbd_dock_pressed;
int kbd_d_pressed;
int kbd_origin_pressed;
int kbd_find_pressed;

int kbd_fire_missile_pressed;
int kbd_target_missile_pressed;
int kbd_unarm_missile_pressed;

int kbd_pause_pressed;
int kbd_resume_pressed;

int kbd_inc_speed_pressed;
int kbd_dec_speed_pressed;

int kbd_up_pressed;
int kbd_down_pressed;
int kbd_left_pressed;
int kbd_right_pressed;

int kbd_enter_pressed;
int kbd_backspace_pressed;
int kbd_space_pressed;

/* Allegro 5 key state buffer */
static ALLEGRO_KEYBOARD_STATE kbd_state;

/* For readkey()-style buffered input */
static ALLEGRO_EVENT_QUEUE *kbd_queue = NULL;


/* ---------------------------------------------------------------
 * Startup
 * --------------------------------------------------------------- */
int kbd_keyboard_startup(void)
{
    if (!al_is_keyboard_installed())
        al_install_keyboard();

    if (!kbd_queue)
        kbd_queue = al_create_event_queue();

    al_register_event_source(kbd_queue, al_get_keyboard_event_source());

    return 0;
}


/* ---------------------------------------------------------------
 * Shutdown
 * --------------------------------------------------------------- */
int kbd_keyboard_shutdown(void)
{
    if (kbd_queue)
        al_destroy_event_queue(kbd_queue);

    kbd_queue = NULL;
    return 0;
}


/* ---------------------------------------------------------------
 * Poll keyboard state (Allegro 4 compatibility)
 * --------------------------------------------------------------- */
void kbd_poll_keyboard(void)
{
    al_get_keyboard_state(&kbd_state);

    /* Function keys */
    kbd_F1_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F1);
    kbd_F2_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F2);
    kbd_F3_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F3);
    kbd_F4_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F4);
    kbd_F5_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F5);
    kbd_F6_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F6);
    kbd_F7_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F7);
    kbd_F8_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F8);
    kbd_F9_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_F9);
    kbd_F10_pressed = al_key_down(&kbd_state, ALLEGRO_KEY_F10);
    kbd_F11_pressed = al_key_down(&kbd_state, ALLEGRO_KEY_F11);
    kbd_F12_pressed = al_key_down(&kbd_state, ALLEGRO_KEY_F12);

    /* Yes/No */
    kbd_y_pressed = al_key_down(&kbd_state, ALLEGRO_KEY_Y);
    kbd_n_pressed = al_key_down(&kbd_state, ALLEGRO_KEY_N);

    /* Gameplay */
    kbd_fire_pressed          = al_key_down(&kbd_state, ALLEGRO_KEY_A);
    kbd_ecm_pressed           = al_key_down(&kbd_state, ALLEGRO_KEY_E);
    kbd_energy_bomb_pressed   = al_key_down(&kbd_state, ALLEGRO_KEY_TAB);
    kbd_hyperspace_pressed    = al_key_down(&kbd_state, ALLEGRO_KEY_H);
    kbd_ctrl_pressed          = al_key_down(&kbd_state, ALLEGRO_KEY_LCTRL) ||
                                al_key_down(&kbd_state, ALLEGRO_KEY_RCTRL);
    kbd_jump_pressed          = al_key_down(&kbd_state, ALLEGRO_KEY_J);
    kbd_escape_pressed        = al_key_down(&kbd_state, ALLEGRO_KEY_ESCAPE);

    kbd_dock_pressed          = al_key_down(&kbd_state, ALLEGRO_KEY_C);
    kbd_d_pressed             = al_key_down(&kbd_state, ALLEGRO_KEY_D);
    kbd_origin_pressed        = al_key_down(&kbd_state, ALLEGRO_KEY_O);
    kbd_find_pressed          = al_key_down(&kbd_state, ALLEGRO_KEY_F);

    kbd_fire_missile_pressed  = al_key_down(&kbd_state, ALLEGRO_KEY_M);
    kbd_target_missile_pressed= al_key_down(&kbd_state, ALLEGRO_KEY_T);
    kbd_unarm_missile_pressed = al_key_down(&kbd_state, ALLEGRO_KEY_U);

    kbd_pause_pressed         = al_key_down(&kbd_state, ALLEGRO_KEY_P);
    kbd_resume_pressed        = al_key_down(&kbd_state, ALLEGRO_KEY_R);

    kbd_inc_speed_pressed     = al_key_down(&kbd_state, ALLEGRO_KEY_SPACE);
    kbd_dec_speed_pressed     = al_key_down(&kbd_state, ALLEGRO_KEY_SLASH);

    kbd_up_pressed            = al_key_down(&kbd_state, ALLEGRO_KEY_S)    ||
                                al_key_down(&kbd_state, ALLEGRO_KEY_UP);
    kbd_down_pressed          = al_key_down(&kbd_state, ALLEGRO_KEY_X)    ||
                                al_key_down(&kbd_state, ALLEGRO_KEY_DOWN);
    kbd_left_pressed          = al_key_down(&kbd_state, ALLEGRO_KEY_COMMA)||
                                al_key_down(&kbd_state, ALLEGRO_KEY_LEFT);
    kbd_right_pressed         = al_key_down(&kbd_state, ALLEGRO_KEY_FULLSTOP) ||
                                al_key_down(&kbd_state, ALLEGRO_KEY_RIGHT);

    kbd_enter_pressed         = al_key_down(&kbd_state, ALLEGRO_KEY_ENTER);
    kbd_backspace_pressed     = al_key_down(&kbd_state, ALLEGRO_KEY_BACKSPACE);
    kbd_space_pressed         = al_key_down(&kbd_state, ALLEGRO_KEY_SPACE);

    /* Clear buffered events */
    ALLEGRO_EVENT ev;
    while (al_get_next_event(kbd_queue, &ev)) {
        /* discard events (equivalent to Allegro 4 readkey()) */
    }
}


/* ---------------------------------------------------------------
 * Allegro 4 readkey() style function
 * --------------------------------------------------------------- */
int kbd_read_key(void)
{
    ALLEGRO_EVENT ev;

    kbd_enter_pressed = 0;
    kbd_backspace_pressed = 0;

    /* Wait for event */
    while (true)
    {
        al_wait_for_event(kbd_queue, &ev);

        if (ev.type == ALLEGRO_EVENT_KEY_CHAR)
        {
            int unicode = ev.keyboard.unichar;

            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER)
            {
                kbd_enter_pressed = 1;
                return 0;
            }

            if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE)
            {
                kbd_backspace_pressed = 1;
                return 0;
            }

            return unicode;  /* return normal characters */
        }
    }
}


/* ---------------------------------------------------------------
 * Clear the key buffer
 * --------------------------------------------------------------- */
void kbd_clear_key_buffer(void)
{
    ALLEGRO_EVENT ev;
    while (al_get_next_event(kbd_queue, &ev)) {
        /* discard all key events */
    }
}
