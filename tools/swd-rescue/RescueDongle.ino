/* v10: brownout-loop breaker.
 * Hammers SWD continuously to catch the stable LDO window of the
 * boot loop, halts the core, erases the first app page (kills the
 * DCDC-crashing app), then flashes the LDO-fixed firmware.
 */
#include "Adafruit_DAP.h"
#include "dongle_fixed.h"

#define SWDIO 11
#define SWCLK 12
#define SWRST -1

#define APP_BASE 0x1000UL
#define PAGE_SIZE 4096UL
#define NRF_NVMC_BASE 0x4001E000UL
#define NRF_NVMC_READY (NRF_NVMC_BASE + 0x400)
#define NRF_NVMC_CONFIG (NRF_NVMC_BASE + 0x504)
#define NRF_NVMC_ERASEPAGE (NRF_NVMC_BASE + 0x508)

Adafruit_DAP_nRF5x dap;
bool done = false;
bool loopkilled = false;
uint32_t hammer = 0;
const char *last_status = "boot";

void error(const char *t) { }

bool flashReady() {
  for (uint32_t i = 0; i < 200000; i++) if (dap.dap_read_word(NRF_NVMC_READY) & 1UL) return true;
  return false;
}

void erasePage(uint32_t addr) {
  if (!flashReady()) return;
  dap.dap_write_word(NRF_NVMC_CONFIG, 2);
  dap.dap_write_word(NRF_NVMC_ERASEPAGE, addr);
  flashReady();
  dap.dap_write_word(NRF_NVMC_CONFIG, 0);
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 4000) delay(10);
  delay(200);
  Serial.println("\n=== v10 brownout-loop breaker ===");
}

void loop() {
  if (done) { Serial.print("[status] "); Serial.println(last_status); delay(2000); return; }

  hammer++;
  dap.begin(SWCLK, SWDIO, SWRST, &error);

  if (!dap.targetConnect()) {
    if ((hammer % 100) == 0) { Serial.print("[hammer "); Serial.print(hammer); Serial.println("] no sync"); }
    delay(2);
    return;
  }
  Serial.print("SYNC after "); Serial.print(hammer); Serial.println(" hammers!");

  uint32_t id;
  if (!dap.select(&id)) { last_status = "sync but no select"; delay(5); return; }

  if (!loopkilled) {
    Serial.println("halted - killing app page 0x1000 NOW");
    erasePage(APP_BASE);
    loopkilled = true;
    Serial.println("boot loop should be dead - chip stable now");
  }

  uint32_t pages = (firmware_len + PAGE_SIZE - 1) / PAGE_SIZE;
  Serial.println("full erase...");
  for (uint32_t p = 0; p < pages; p++) erasePage(APP_BASE + p * PAGE_SIZE);
  Serial.println("programming...");
  if (!dap.programFlash(APP_BASE, firmware, firmware_len, true)) {
    last_status = "FAIL:verify - retrying";
    return;
  }
  dap.deselect();
  last_status = "SUCCESS-LDO-FW-FLASHED!";
  done = true;
  Serial.println("=== SUCCESS - unplug everything, dongle into USB ===");
}
