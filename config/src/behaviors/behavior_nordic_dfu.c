/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * &nordic_dfu — enter the DFU bootloader from the running app.
 *
 * Writes a magic value into the nRF52 GPREGRET register and performs a
 * warm reset. GPREGRET survives software resets, so the bootloader reads
 * it on the next boot and stays in DFU mode. The magic value differs per
 * bootloader:
 *
 * - Nordic Open DFU Bootloader (Holyiot dongles, PCA10059):
 *     0xB1 (BOOTLOADER_DFU_START), checked in nrf_dfu_enter_check():
 *     `if (nrf_power_gpregret_get() == BOOTLOADER_DFU_START) return true;`
 *     The bootloader clears GPREGRET itself after the check.
 *
 * - Adafruit nRF52 UF2 bootloader (nice!nano):
 *     0x57 (DFU_MAGIC_UF2_RESET) shows the NICENANO UF2 drive.
 *     0xB1 would enter BLE OTA mode instead (no drive!), which is why
 *     the value is chosen per shield.
 *
 * Locality is CENTRAL: the behavior executes on the split central
 * (the dongle in dongle mode, the right half in bt mode), no matter
 * which half the key that triggers it sits on.
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

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_ENABLED(CONFIG_SHIELD_CHARYBDIS_DONGLE)
/*
 * Nordic Open DFU Bootloader (Holyiot dongles): this bootloader family
 * IGNORES GPREGRET. Its DFU entry is the pca10059 button check: P0.19
 * must read LOW at boot (community-proven by SlimeNRF, which drives
 * GPIO19 low and reboots). GPIO config survives the warm reset, so we
 * drive the pin low and reboot - the bootloader then stays in DFU mode
 * and enumerates as 1915:521f for nrfutil.
 */
#define NORDIC_DFU_VIA_PIN19
#else
/* Adafruit nRF52 UF2 bootloader (nice!nano): NICENANO UF2 drive.
 * 0xB1 (DFU_MAGIC_OTA_APPJUM) would enter BLE OTA mode without a drive. */
#define NORDIC_BOOTLOADER_DFU_MAGIC 0x57
#endif

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

#ifdef NORDIC_DFU_VIA_PIN19
    LOG_INF("Rebooting into DFU bootloader (P0.19 low)");

    /* Configure P0.19 as output driving low; survives the reset below. */
    NRF_P0->PIN_CNF[19] = 0x00000003UL; /* DIR=output, INPUT=disconnected */
    NRF_P0->DIRSET = (1UL << 19);
    NRF_P0->OUTCLR = (1UL << 19);
#else
    LOG_INF("Rebooting into DFU bootloader (GPREGRET=0x%02x)",
            NORDIC_BOOTLOADER_DFU_MAGIC);

    nrf_power_gpregret_set(NRF_POWER, 0, NORDIC_BOOTLOADER_DFU_MAGIC);
#endif
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
