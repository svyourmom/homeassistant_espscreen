#pragma once
// Elecrow CrowPanel Advance 7" (ESP32-S3) I2C board controller: early GT911 reset and backlight brightness.
//
// Two PCB revisions exist in the wild:
// - V1.2-V1.5: STC8H1K28 MCU at 0x30. Single-byte command: 0 = max brightness .. 245 = dark (inverted), 250 = GT911
//   reset pulse.
// - V1.0-V1.1: PCA9557 / TCA9534 expander at 0x18. Reg 0x01 outputs: IO0/IO1 = backlight enable, IO2 = GT911 reset.
//
// Runs at setup_priority::IO (900.0f), after I2C (BUS = 1000.0f) and before GT911 (DATA = 600.0f), holding GPIO1 (INT)
// low across the reset pulse so the GT911 latches address 0x5D.

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_ESP32
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
namespace esphome {
// ESPHome 2026.9 defines loop_task_handle in esp32/core.cpp; ESPHome 2026.10+ switched to xTaskCreateStatic and
// dropped the symbol. A weak fallback lets both versions link without touching smart_display/loop_record.cpp.
__attribute__((weak)) TaskHandle_t loop_task_handle = nullptr;
}  // namespace esphome
#endif

namespace esphome::crowpanel_advance_io {

class CrowPanelAdvanceIO final : public Component, public i2c::I2CDevice {
 public:
  float get_setup_priority() const override { return setup_priority::IO; }

  void setup() override {
#ifdef USE_ESP32
    if (esphome::loop_task_handle == nullptr) {
      esphome::loop_task_handle = xTaskGetCurrentTaskHandle();
    }
#endif
    delay(50);

    // 1) Probe V1.2-V1.5 STC8H1K28 at 0x30 (this device's default address): 250 pulses GT911 reset, 0 sets full light.
    const uint8_t stc_rst = 250;
    if (this->write(&stc_rst, 1) == i2c::ERROR_OK) {
      this->has_stc_ = true;
      ESP_LOGI(TAG, "Detected STC8H1K28 @ 0x30 (Advance V1.2-V1.5)");
#ifdef USE_ESP32
      gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT);
      gpio_set_level(GPIO_NUM_1, 0);
#endif
      delay(120);
#ifdef USE_ESP32
      gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);
#endif
      delay(100);
      const uint8_t stc_on = 0;
      this->write(&stc_on, 1);
      this->last_stc_ = stc_on;
    }

    // 2) Probe V1.0-V1.1 PCA9557 / TCA9534 at 0x18: IO0/IO1 = backlight, IO2 = GT911 reset.
    if (this->bus_ != nullptr) {
      const uint8_t cfg[4][2] = {
          {0x02, 0x00},  // polarity inversion: normal
          {0x03, 0x00},  // direction: outputs
          {0x01, 0x03},  // backlight on (IO0/IO1=1), GT911 reset asserted low (IO2=0)
          {0x01, 0x07},  // release GT911 reset high (IO2=1), keep backlight on
      };
      if (this->bus_->write_readv(PCA_ADDR, cfg[1], 2, nullptr, 0) == i2c::ERROR_OK) {
        this->has_pca_ = true;
        ESP_LOGI(TAG, "Detected TCA9534/PCA9557 @ 0x18 (Advance V1.0-V1.1)");
        this->bus_->write_readv(PCA_ADDR, cfg[0], 2, nullptr, 0);
#ifdef USE_ESP32
        gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_NUM_1, 0);
#endif
        this->bus_->write_readv(PCA_ADDR, cfg[2], 2, nullptr, 0);
        delay(20);
        this->bus_->write_readv(PCA_ADDR, cfg[3], 2, nullptr, 0);
        delay(100);
#ifdef USE_ESP32
        gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);
#endif
        this->last_pca_ = 0x07;
      }
    }
  }

  void write_brightness(float state) {
    // STC8H1K28 (0x30): 0 = brightest, 245 = off. Skip duplicate writes so fades do not crowd the 20 ms GT911 poll.
    const uint8_t stc_val = (state <= 0.001f) ? 245 : static_cast<uint8_t>((1.0f - state) * 220.0f);
    if ((this->has_stc_ || !this->has_pca_) && stc_val != this->last_stc_) {
      this->last_stc_ = stc_val;
      this->write(&stc_val, 1);
    }
    // PCA9557 / TCA9534 (0x18): binary backlight on IO0/IO1 while keeping GT911 reset (IO2) high.
    const uint8_t pca_val = (state > 0.001f) ? 0x07 : 0x04;
    if ((this->has_pca_ || !this->has_stc_) && this->bus_ != nullptr && pca_val != this->last_pca_) {
      this->last_pca_ = pca_val;
      const uint8_t out[2] = {0x01, pca_val};
      this->bus_->write_readv(PCA_ADDR, out, 2, nullptr, 0);
    }
  }

 protected:
  static constexpr const char *TAG = "crowpanel_io";
  static constexpr uint8_t PCA_ADDR = 0x18;
  bool has_stc_{false};
  bool has_pca_{false};
  uint8_t last_stc_{0xFF};
  uint8_t last_pca_{0xFF};
};

}  // namespace esphome::crowpanel_advance_io
