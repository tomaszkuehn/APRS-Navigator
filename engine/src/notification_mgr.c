#include "internal.h"

/* ========================================================================
 * NOTIFICATION MANAGER — Map internal events to abstract sound IDs
 *
 * Extracted from:
 *   - peripherial.c (setsound)
 *   - globals.c (soundtab)
 *
 * Sound levels are 0-9 (0 = off). The actual sound generation is
 * left to the client. The engine just emits SOUND events with the
 * configured level.
 * ======================================================================== */

void nm_set_sound(aprs_engine_t* e, int sound_id, uint8_t level) {
    if (!e) return;
    if (sound_id < 0 || sound_id >= MAX_SOUNDS) return;
    if (level > 9) level = 9;

    e->sounds[sound_id].level = level;
    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

void nm_get_sound(aprs_engine_t* e, int sound_id, uint8_t* level) {
    if (!e || !level) return;
    if (sound_id < 0 || sound_id >= MAX_SOUNDS) {
        *level = 0;
        return;
    }

    *level = e->sounds[sound_id].level;
}

/* Trigger a sound event. The client receives a SOUND event and
 * decides how to render it (beep, tone, vibration, etc.) */
void nm_trigger(aprs_engine_t* e, int sound_id) {
    if (!e) return;
    if (sound_id < 0 || sound_id >= MAX_SOUNDS) return;
    if (e->sounds[sound_id].level == 0) return;

    /* Publish sound event with level */
    uint8_t level = e->sounds[sound_id].level;
    eb_publish(e, APRS_EVT_SOUND, "sound.event", &level, sizeof(level));
}
