#include "includes.h"
#include "input.h"
#include <string.h>

static input_backend_t *inp_backend = 0;

/* ── Key injection queue (circular buffer) ── */
#define IQ_SIZE 64
static int iq_buf[IQ_SIZE];
static int iq_head = 0;
static int iq_tail = 0;

/* ── Key remap table (identity by default) ── */
#define KM_SIZE 256
static int keymap[KM_SIZE];

void input_register_backend(input_backend_t *backend)
{
    int i;
    inp_backend = backend;
    for (i = 0; i < KM_SIZE; i++) keymap[i] = i;
}

void input_inject_key(int keycode)
{
    int next = (iq_head + 1) % IQ_SIZE;
    if (next == iq_tail) return;  /* queue full — drop */
    iq_buf[iq_head] = keycode;
    iq_head = next;
}

void input_inject_sequence(const char *keys)
{
    int i, kc;
    for (i = 0; keys[i]; i++) {
        switch (keys[i]) {
        case 'U': kc = KEY_UP;    break;
        case 'D': kc = KEY_DOWN;  break;
        case 'L': kc = KEY_LEFT;  break;
        case 'R': kc = KEY_RIGHT; break;
        case 'E': kc = KEY_ENTER; break;
        case 'M': kc = KEY_MENU;  break;
        case 'C': kc = KEY_CONFIG;break;
        case '1': kc = KEY_1;     break;
        case '2': kc = KEY_2;     break;
        case '3': kc = KEY_3;     break;
        case '4': kc = KEY_4;     break;
        case '5': kc = KEY_5;     break;
        case '6': kc = KEY_6;     break;
        default:  continue;  /* skip unknown chars */
        }
        input_inject_key(kc);
    }
}

void input_set_keymap(int from_key, int to_key)
{
    if (from_key >= 0 && from_key < KM_SIZE)
        keymap[from_key] = to_key;
}

void input_flush_queue(void)
{
    iq_head = iq_tail = 0;
}

int input_read_key(void)
{
    int raw;

    /* Drain injection queue first */
    if (iq_tail != iq_head) {
        raw = iq_buf[iq_tail];
        iq_tail = (iq_tail + 1) % IQ_SIZE;
        return (raw >= 0 && raw < KM_SIZE) ? keymap[raw] : raw;
    }

    /* Fall back to hardware backend */
    if (inp_backend && inp_backend->read)
        raw = inp_backend->read();
    else
        raw = 0;

    return (raw > 0 && raw < KM_SIZE) ? keymap[raw] : raw;
}
