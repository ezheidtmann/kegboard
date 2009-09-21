#include <inttypes.h>
#include <avr/io.h>

#include "kegboard.h"
#include "ds1820.h"
#include "OneWire.h"

#define REFRESH_MS    5000

static uint8_t null_addr[] = {0,0,0,0,0,0,0,0};

DS1820Sensor::DS1820Sensor() {
  m_bus = 0;
  m_converting = false;
  m_next_conversion_clock = 0;
  m_conversion_start_clock = 0;
  m_temp = 0;
  m_temp_is_valid = false;
  m_initialized = false;
}

void DS1820Sensor::Initialize(OneWire* bus, uint8_t* addr)
{
  m_bus = bus;
  for (int i=0; i<8;i++) {
    m_addr[i] = addr[i];
  }
  m_initialized = true;
}

bool DS1820Sensor::Initialized() {
  return m_initialized;
}

void DS1820Sensor::Update(unsigned long clock)
{
  if (clock < m_conversion_start_clock) { // overflow of clock
    m_conversion_start_clock = 0;
  }

  unsigned long delta = clock - m_conversion_start_clock;

  if (!m_converting && clock >= m_next_conversion_clock) {
    // we're not converting, and it is time to start
    m_converting = true;
    m_conversion_start_clock = clock;
    StartConversion();
  } else if (m_converting && clock >= (m_conversion_start_clock + 1000)) {
    // we're converting and it is time to fetch
    m_converting = false;

    m_next_conversion_clock = clock + REFRESH_MS;
    if (FetchConversion()) {
      m_temp_is_valid = true;
    } else {
      m_temp_is_valid = false;
    }
  } else {
    // we're either in the middle of a conversion, or it is too soon to start
    // the next cycle.
  }

}

bool DS1820Sensor::ResetAndSelect()
{
  if (!m_bus->reset()) {
    return false;
  }
  m_bus->select(m_addr);
  return true;
}

bool DS1820Sensor::StartConversion()
{
  if (!ResetAndSelect())
    return false;
  m_bus->write(0x44, 1);
  return true;
}

bool DS1820Sensor::FetchConversion()
{
  if (!ResetAndSelect()) {
    return false;
  }

  uint8_t data[9];
  m_bus->write(0xBE); // read scratchpad

  for (int i = 0; i < 9; i++) {
    data[i] = m_bus->read();
  }

  if (OneWire::crc8(data, 8) != data[8]) {
    // bad CRC, drop reading.
    return false;
  }

  m_temp = ((data[1] << 8) | data[0] );
  return true;
}

long DS1820Sensor::GetTemp(void)
{
  long scaled_temp = INVALID_TEMPERATURE_VALUE;
  if (!m_temp_is_valid)
    return scaled_temp;

  // The DS18B20 reports temperature as a 16 bit value.  The least significant
  // 12 bits are the temperature value, in increments of 1/16th deg C.  The most
  // significant bits carry the sign extension.
  //
  // This method returns the temperature in 1/10^6 deg C, so the value is scaled
  // up by (10^3/16) = 62500.
  //
  // TODO(mikey): support ds18s20 ?
  scaled_temp = (m_temp & 0x0fff) * 62500;

  // Check sign extension
  bool negative = (m_temp & 0xf000) ? true : false;

  if (negative)
    scaled_temp = -1 * scaled_temp;

  return scaled_temp;
}

bool DS1820Sensor::Busy() {
  return m_converting;
}

int DS1820Sensor::CompareId(uint8_t* other) {
  for (int i = 0; i < 8; i++) {
    if (m_addr[i] == other[i]) {
      continue;
    } else {
      return (m_addr[i] < other[i]) ? -1 : 1;
    }
  }
  return 0;
}

void DS1820Sensor::PrintTemp(void)
{
  long temp = GetTemp() / 1000000;
  Serial.print(temp);
}