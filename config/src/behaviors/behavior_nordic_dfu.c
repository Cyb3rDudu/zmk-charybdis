/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * &nordic_dfu — enter the Nordic DFU bootloader from the running app.
 *
 * Writes 0xB1 (BOOTLOADER_DFU_START) into the nRF52 GPREGRET register
 * and performs a warm reset. GPREGRET survives software resets, and the
 * Nordic Open DFU Bootloader (Holyiot dongles, PCA10059) checks it in
 * nrf_dfu_enter_check():
 *
 *     if (nrf_power_gpregret_get() == BOOTLOADER_DFU_START) return true;
 *
 * The bootloader clears GPREGRET itself after the check, so no cleanup
 * is needed on this side. The Adafruit nRF52 UF2 bootloader (nice!nano)
 * honours the same value, so the behavior also replaces the
 * double-tap-reset dance on the keyboard halves.
 *
 * Locality is CENTRAL: the behavior executes on the split central
 * (the dongle), no matter which half the key that triggers it sits on.
 */

#define DT_DRV_COMPAT zmk_behavior_nordic_dfu

#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

#include <nrfx.h>
#include <hal/nrf_power.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define NORDIC_BOOTLOADER_DFU_START 0xB1

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    LOG_INF("Rebooting into DFU bootloader (GPREGRET=0x%02x)",
            NORDIC_BOOTLOADER_DFU_START);

    nrf_power_gpregret_set(NRF_POWER, 0, NORDIC_BOOTLOADER_DFU_START);
    sys_reboot(SYS_REBOOT_WARM);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_nordic_dfu_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define NRF_DFU_INST(n)                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_nordic_dfu_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NRF_DFU_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
