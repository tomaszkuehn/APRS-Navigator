#include "internal.h"

/* ========================================================================
 * TRX CONFIG — TRX/TNC tuning parameters
 *
 * Extracted from:
 *   - config.c (config_trx)
 *   - globals.c (update_aprs_config)
 *
 * These parameters control the TNC (Terminal Node Controller) timing
 * for AX.25 frame transmission:
 *
 *   tx_delay  — PTT-to-data delay in 10ms units (0-15)
 *   tx_header — TX header/preamble time in 10ms units (0-15)
 *   bit_delay — bit period timing in 200us units (0-15)
 *   rx_tune   — receiver tuning capacitance (0-63)
 *   squelch   — squelch threshold (0-15)
 * ======================================================================== */

void trx_set_params(aprs_engine_t* e, uint8_t tx_delay, uint8_t tx_header,
                    uint8_t bit_delay, uint8_t rx_tune, uint8_t squelch) {
    if (!e) return;

    e->trx.tx_delay = tx_delay;
    e->trx.tx_header = tx_header;
    e->trx.bit_delay = bit_delay;
    e->trx.rx_tune = rx_tune;
    e->trx.squelch = squelch;

    eb_publish(e, APRS_EVT_CONFIG_CHANGED, "config.changed", NULL, 0);
}

void trx_get_params(aprs_engine_t* e, uint8_t* tx_delay, uint8_t* tx_header,
                    uint8_t* bit_delay, uint8_t* rx_tune, uint8_t* squelch) {
    if (!e) return;

    if (tx_delay)  *tx_delay  = e->trx.tx_delay;
    if (tx_header) *tx_header = e->trx.tx_header;
    if (bit_delay) *bit_delay = e->trx.bit_delay;
    if (rx_tune)   *rx_tune   = e->trx.rx_tune;
    if (squelch)   *squelch   = e->trx.squelch;
}
