#pragma once
#include <Arduino.h>
#include <Wire.h>
namespace arduino {
    template<uint16_t Width, uint16_t Height, uint8_t Address = 0x38>
    class ft6236 final {
            constexpr static const uint8_t TOUCH_REG_XL  = 0x04;
            constexpr static const uint8_t TOUCH_REG_XH = 0x03;
            constexpr static const uint8_t TOUCH_REG_YL = 0x06;
            constexpr static const uint8_t TOUCH_REG_YH = 0x05;

            TwoWire& m_i2c;
            uint8_t m_rotation;
            ft6236(const ft6236& rhs)=delete;
            ft6236& operator=(const ft6236& rhs)=delete;
            void do_move(ft6236& rhs) {

            }
            int reg(int value) const {
                int result = 0;
                m_i2c.beginTransmission(address);
                m_i2c.write(value);
                m_i2c.endTransmission();
                m_i2c.requestFrom((uint8_t)address, (uint8_t)1);
                if (m_i2c.available()) {
                    result = m_i2c.read();
                }
                return result;
            }
            void translate(uint16_t& x,uint16_t& y) const {
                uint16_t tmp;
                switch(m_rotation & 3) {
                    case 1:
                        tmp = x;
                        x = y;
                        y = native_width - tmp - 1;
                        break;
                    case 2: // TODO: test
                        x = native_width - x - 1;
                        y = native_height - y - 1;
                        break;
                    case 3: // TODO: test
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
            constexpr static const uint8_t address = Address;
            ft6236(ft6236&& rhs) {
                do_move(rhs);
            }
            ft6236& operator=(ft6236&& rhs) {
                do_move(rhs);
                return *this;
            }
            ft6236(TwoWire& i2c = Wire) : m_i2c(i2c) {
                m_rotation = 0;
            }
            bool initialize() {
                m_i2c.begin();
                return true;
            }
            uint8_t rotation() const {
                return m_rotation;
            }
            void rotation(uint8_t value) {
                m_rotation = value & 3;
            }
            uint16_t width() const {
                return m_rotation&1?native_height:native_width;
            }
            uint16_t height() const {
                return m_rotation&1?native_width:native_height;
            }
            bool xy(uint16_t* out_x,uint16_t* out_y) const {
                int XL = 0;
                int XH = 0;
                int YL = 0;
                int YH = 0;

                XH = reg(TOUCH_REG_XH);
                if(XH >> 6 == 1)
                {
                    if(out_x!=nullptr) {
                        *out_x = 0;
                    }
                    if(out_y!=nullptr) {
                        *out_y = 0;
                    }
                    return false;
                }
                XL = reg(TOUCH_REG_XL);
                uint16_t x = ((XH & 0x0F) << 8) | XL;
                YH = reg(TOUCH_REG_YH);
                YL = reg(TOUCH_REG_YL);
                uint16_t y =  ((YH & 0x0F) << 8) | YL;
                if(x>=native_width) {
                    x=native_width-1;
                }
                if(y>=native_height) {
                    y=native_height-1;
                }
                translate(x,y);
                if(out_x!=nullptr) {
                    *out_x = x;
                }
                if(out_y!=nullptr) {
                    *out_y = y;
                }
                return true;
            }
    };
}