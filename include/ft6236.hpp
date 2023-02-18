// Derived from Adafruit's FT6236 lib
// https://github.com/DustinWatts/FT6236
// Original license below:
/*
This is a library for the FT6236 touchscreen controller by FocalTech.
The FT6236 and FT6236u work the same way.
A lot of this library is originally written by Limor Fried/Ladyada.
Because Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!
@section author Author
Written by Limor Fried/Ladyada for Adafruit Industries.
@section license License
MIT license, all text above must be included in any redistribution
*/
#pragma once
#include <Arduino.h>
#include <Wire.h>
namespace arduino {
template <uint16_t Width, uint16_t Height, uint8_t Threshhold = 128, uint8_t Address = 0x38>
class ft6236 final {
    constexpr static const uint8_t TOUCH_REG_XL = 0x04;
    constexpr static const uint8_t TOUCH_REG_XH = 0x03;
    constexpr static const uint8_t TOUCH_REG_YL = 0x06;
    constexpr static const uint8_t TOUCH_REG_YH = 0x05;
    constexpr static const uint8_t TOUCH_REG_NUMTOUCHES = 0x2;
    constexpr static const uint8_t TOUCH_REG_THRESHHOLD = 0x80;
    constexpr static const uint8_t TOUCH_REG_VENDID = 0xA8;
    constexpr static const uint8_t TOUCH_REG_CHIPID = 0xA3;
    constexpr static const uint8_t FT6236_VENDID = 0x11;
    constexpr static const uint8_t FT6206_CHIPID = 0x6;
    constexpr static const uint8_t FT6236_CHIPID = 0x36;
    constexpr static const uint8_t FT6236U_CHIPID = 0x64;

    TwoWire& m_i2c;
    uint8_t m_rotation;
    bool m_initialized;
    size_t m_touches;
    uint16_t m_touches_x[2], m_touches_y[2], m_touches_id[2];

    ft6236(const ft6236& rhs) = delete;
    ft6236& operator=(const ft6236& rhs) = delete;
    void do_move(ft6236& rhs) {
        m_i2c = rhs.m_i2c;
        m_rotation = rhs.m_rotation;
        m_initialized = rhs.m_initialized;
        m_touches = rhs.m_touches;
        memcpy(m_touches_x,rhs.m_touches_x,sizeof(m_touches_x));
        memcpy(m_touches_y,rhs.m_touches_y,sizeof(m_touches_y));
        memcpy(m_touches_id,rhs.m_touches_id,sizeof(m_touches_id));
    }
    int reg(int r) const {
        int result = 0;
        m_i2c.beginTransmission(address);
        m_i2c.write(r);
        m_i2c.endTransmission();
        m_i2c.requestFrom((uint8_t)address, (uint8_t)1);
        if (m_i2c.available()) {
            result = m_i2c.read();
        }
        return result;
    }
    void reg(int r, int value) {
        m_i2c.beginTransmission(address);
        m_i2c.write((uint8_t)r);
        m_i2c.write((uint8_t)value);
        m_i2c.endTransmission();
    }
    void read_all() {
        uint8_t i2cdat[16];
        m_i2c.beginTransmission(address);
        m_i2c.write((uint8_t)0);
        m_i2c.endTransmission();

        m_i2c.requestFrom((uint8_t)address, (uint8_t)16);
        for (uint8_t i = 0; i < 16; i++)
            i2cdat[i] = m_i2c.read();

        m_touches = i2cdat[0x02];
        if (m_touches > 2) {
            m_touches = 0;
        }

        for (uint8_t i = 0; i < 2; i++) {
            m_touches_x[i] = i2cdat[0x03 + i * 6] & 0x0F;
            m_touches_x[i] <<= 8;
            m_touches_x[i] |= i2cdat[0x04 + i * 6];
            m_touches_y[i] = i2cdat[0x05 + i * 6] & 0x0F;
            m_touches_y[i] <<= 8;
            m_touches_y[i] |= i2cdat[0x06 + i * 6];
            m_touches_id[i] = i2cdat[0x05 + i * 6] >> 4;
        }
    }
    bool read_point(size_t n,uint16_t* out_x, uint16_t* out_y) const {
        if(m_touches==0 || n<0 || n>=m_touches) {
            if (out_x != nullptr) {
                *out_x = 0;
            }
            if (out_y != nullptr) {
                *out_y = 0;
            }
            return false;
        }
        uint16_t x = m_touches_x[n];
        uint16_t y = m_touches_y[n];
        if (x >= native_width) {
            x = native_width - 1;
        }
        if (y >= native_height) {
            y = native_height - 1;
        }
        translate(x, y);
        if (out_x != nullptr) {
            *out_x = x;
        }
        if (out_y != nullptr) {
            *out_y = y;
        }
        return true;
    }
    void translate(uint16_t& x, uint16_t& y) const {
        uint16_t tmp;
        switch (m_rotation & 3) {
            case 1:
                tmp = x;
                x = y;
                y = native_width - tmp - 1;
                break;
            case 2:
                x = native_width - x - 1;
                y = native_height - y - 1;
                break;
            case 3:
                tmp = x;
                x = native_height - y - 1;
                y = tmp;
            default:
                break;
        }
    }

   public:
    constexpr static const uint16_t native_width = Width;
    constexpr static const uint16_t native_height = Height;
    constexpr static const uint8_t threshhold = Threshhold;
    constexpr static const uint8_t address = Address;
    ft6236(ft6236&& rhs) {
        do_move(rhs);
    }
    ft6236& operator=(ft6236&& rhs) {
        do_move(rhs);
        return *this;
    }
    ft6236(TwoWire& i2c = Wire) : m_i2c(i2c), m_rotation(0), m_touches(0) {
    }

    bool initialized() const {
        return m_initialized;
    }
    bool initialize() {
        if (!m_initialized) {
            m_i2c.begin();
            reg(TOUCH_REG_THRESHHOLD, threshhold);

            // Check if our chip has the correct Vendor ID
            if (reg(TOUCH_REG_VENDID) != FT6236_VENDID) {
                return false;
            }
            // Check if our chip has the correct Chip ID.
            uint8_t id = reg(TOUCH_REG_CHIPID);
            if ((id != FT6236_CHIPID) && (id != FT6236U_CHIPID) &&
                (id != FT6206_CHIPID)) {
                return false;
            }
            m_touches = 0;
            m_initialized = true;
        }
        return m_initialized;
    }
    uint8_t rotation() const {
        return m_rotation;
    }
    void rotation(uint8_t value) {
        m_rotation = value & 3;
    }
    uint16_t width() const {
        return m_rotation & 1 ? native_height : native_width;
    }
    uint16_t height() const {
        return m_rotation & 1 ? native_width : native_height;
    }
    size_t touches() const {
        uint8_t result = reg(TOUCH_REG_NUMTOUCHES);
        if (result > 2) {
            result = 0;
        }
        return result;
    }
    bool xy(uint16_t* out_x, uint16_t* out_y) const {
        return read_point(0,out_x,out_y);
    }
    bool xy2(uint16_t* out_x, uint16_t* out_y) const {
        return read_point(1,out_x,out_y);
    }
    bool update() {
        if(!initialize()) {
            return false;
        }
        read_all();
        return true;
    }
};
}  // namespace arduino