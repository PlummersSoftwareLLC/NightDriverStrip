/**
 * @file      LilyGo_AMOLED.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-05-29
 *
 */

#if AMOLED_S3

#include "amoled/LilyGo_AMOLED.h"
#include <esp_adc_cal.h>
#include <driver/gpio.h>

#define SEND_BUF_SIZE           (16384)
#define TFT_SPI_MODE            SPI_MODE0
#define DEFAULT_SPI_HANDLER    (SPI3_HOST)

LilyGo_AMOLED::LilyGo_AMOLED() : boards(NULL)
{
    pBuffer = NULL;
    _brightness = AMOLED_DEFAULT_BRIGHTNESS;
    // Prevent previously set hold
    switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0 :
    case ESP_SLEEP_WAKEUP_EXT1 :
    case ESP_SLEEP_WAKEUP_TIMER:
    case ESP_SLEEP_WAKEUP_ULP :
        gpio_hold_dis(GPIO_NUM_14);
        gpio_deep_sleep_hold_dis();
        break;
    default :
        break;
    }
}

LilyGo_AMOLED::~LilyGo_AMOLED()
{
    if (pBuffer) {
        free(pBuffer);
        pBuffer = NULL;
    }
}

const char *LilyGo_AMOLED::getName()
{
    if (boards == &BOARD_AMOLED_147) {
        return "1.47 inch";
    } else if (boards == &BOARD_AMOLED_191 ) {
        return "1.91 inch";
    } else if (boards == &BOARD_AMOLED_241) {
        return "2.41 inch";
    }
    return "Unkonw";
}

uint8_t LilyGo_AMOLED::getBoardID()
{
    if (boards == &BOARD_AMOLED_147) {
        return LILYGO_AMOLED_147;
    } else if (boards == &BOARD_AMOLED_191 ) {
        return LILYGO_AMOLED_191;
    } else if (boards == &BOARD_AMOLED_241) {
        return LILYGO_AMOLED_241;
    }
    return LILYGO_AMOLED_UNKOWN;
}

const BoardsConfigure_t *LilyGo_AMOLED::getBoarsdConfigure()
{
    return boards;
}

uint16_t  LilyGo_AMOLED::width()
{
    return _width;
}

uint16_t  LilyGo_AMOLED::height()
{
    return _height;
}

void inline LilyGo_AMOLED::setCS()
{
    digitalWrite(boards->display.cs, LOW);
}

void inline LilyGo_AMOLED::clrCS()
{
    digitalWrite(boards->display.cs, HIGH);
}

bool LilyGo_AMOLED::isPressed()
{
    if (boards == &BOARD_AMOLED_147) {
        return TouchDrvCHSC5816::isPressed();
    } else if (boards == &BOARD_AMOLED_191 || boards == &BOARD_AMOLED_241) {
        return TouchDrvCSTXXX::isPressed();
    }
    return false;
}

uint8_t LilyGo_AMOLED::getPoint(int16_t *x, int16_t *y, uint8_t get_point )
{
    uint8_t point = 0;
    if (boards == &BOARD_AMOLED_147) {
        point =  TouchDrvCHSC5816::getPoint(x, y);
    } else if (boards == &BOARD_AMOLED_191 || boards == &BOARD_AMOLED_241) {
        point =  TouchDrvCSTXXX::getPoint(x, y);
    }
    return point;
}

uint16_t LilyGo_AMOLED::getBattVoltage(void)
{
    if (boards) {
        if (boards->pmu) {
            if (boards->pmu) {
                if (boards == &BOARD_AMOLED_147) {
                    return XPowersAXP2101::getBattVoltage();
                } else  if (boards == &BOARD_AMOLED_241) {
                    return PowersSY6970::getBattVoltage();
                }
            }
        } else if (boards->adcPins != -1) {
            esp_adc_cal_characteristics_t adc_chars;
            esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
            uint32_t v1 = 0,  raw = 0;
            raw = analogRead(boards->adcPins);
            v1 = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2;
            return v1;
        }
    }
    return 0;
}

uint16_t LilyGo_AMOLED::getVbusVoltage(void)
{
    if (boards) {
        if (boards->pmu) {
            if (boards == &BOARD_AMOLED_147) {
                return XPowersAXP2101::getVbusVoltage();
            } else  if (boards == &BOARD_AMOLED_241) {
                return PowersSY6970::getVbusVoltage();
            }
        }
    }
    return 0;
}

bool LilyGo_AMOLED::isBatteryConnect(void)
{
    if (boards) {
        if (boards->pmu) {
            if (boards == &BOARD_AMOLED_147) {
                return XPowersAXP2101::isBatteryConnect();
            } else  if (boards == &BOARD_AMOLED_241) {
                return PowersSY6970::isBatteryConnect();
            }
        }
    }
    return false;
}

uint16_t LilyGo_AMOLED::getSystemVoltage(void)
{
    if (boards) {
        if (boards->pmu) {
            if (boards == &BOARD_AMOLED_147) {
                return XPowersAXP2101::getSystemVoltage();
            } else  if (boards == &BOARD_AMOLED_241) {
                return PowersSY6970::getSystemVoltage();
            }
        }
    }
    return 0;
}

bool LilyGo_AMOLED::isCharging(void)
{
    if (boards) {
        if (boards->pmu) {
            if (boards == &BOARD_AMOLED_147) {
                return XPowersAXP2101::isCharging();
            } else  if (boards == &BOARD_AMOLED_241) {
                return PowersSY6970::isCharging();
            }
        }
    }
    return false;
}

bool LilyGo_AMOLED::isVbusIn(void)
{
    if (boards) {
        if (boards->pmu) {
            if (boards == &BOARD_AMOLED_147) {
                return XPowersAXP2101::isVbusIn();
            } else  if (boards == &BOARD_AMOLED_241) {
                return PowersSY6970::isVbusIn();
            }
        }
    }
    return false;
}


uint32_t deviceScan(TwoWire *_port, Stream *stream)
{
    stream->println("Devices Scan start.");
    uint8_t err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        _port->beginTransmission(addr);
        err = _port->endTransmission();
        if (err == 0) {
            stream->print("I2C device found at address 0x");
            if (addr < 16)
                stream->print("0");
            stream->print(addr, HEX);
            stream->println(" !");
            nDevices++;
        } else if (err == 4) {
            stream->print("Unknow error at address 0x");
            if (addr < 16)
                stream->print("0");
            stream->println(addr, HEX);
        }
    }
    if (nDevices == 0)
        stream->println("No I2C devices found\n");
    else
        stream->println("Done\n");
    return nDevices;
}

bool LilyGo_AMOLED::initPMU()
{
    bool res = XPowersAXP2101::init(Wire, boards->pmu->sda, boards->pmu->scl, AXP2101_SLAVE_ADDRESS);
    if (!res) {
        return false;
    }

    clearPMU();

    setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

    // ALDO1 = AMOLED logic power & Sensor Power voltage
    setALDO1Voltage(1800);
    enableALDO1();

    // ALDO3 = Level conversion enable and AMOLED power supply
    setALDO3Voltage(3300);
    enableALDO3();

    // BLDO1 = AMOLED LOGIC POWER 1.8V
    setBLDO1Voltage(1800);
    enableBLDO1();

    // No use power channel
    disableDC2();
    disableDC3();
    disableDC4();
    disableDC5();
    disableCPUSLDO();

    // Enable PMU ADC
    enableBattDetection();
    enableVbusVoltageMeasure();
    enableBattVoltageMeasure();

    return res;
}

bool LilyGo_AMOLED::initBUS()
{
    assert(boards);
    log_i("=====CONFIGURE======");
    log_i("RST    > %d", boards->display.rst);
    log_i("CS     > %d", boards->display.cs);
    log_i("SCK    > %d", boards->display.sck);
    log_i("D0     > %d", boards->display.d0);
    log_i("D1     > %d", boards->display.d1);
    log_i("D2     > %d", boards->display.d2);
    log_i("D3     > %d", boards->display.d3);
    log_i("TE     > %d", boards->display.te);
    log_i("Freq   > %d", boards->display.freq);
    log_i("Power  > %d", boards->PMICEnPins);
    log_i("==================");

    _width = boards->display.width;
    _height = boards->display.height;

    pinMode(boards->display.rst, OUTPUT);
    pinMode(boards->display.cs, OUTPUT);

    if (boards->display.te != -1) {
        pinMode(boards->display.te, INPUT);
    }

    if (boards->PMICEnPins != -1) {
        pinMode(boards->PMICEnPins, OUTPUT);
        digitalWrite(boards->PMICEnPins, HIGH);
    }

    //reset display
    digitalWrite(boards->display.rst, HIGH);
    delay(200);
    digitalWrite(boards->display.rst, LOW);
    delay(300);
    digitalWrite(boards->display.rst, HIGH);
    delay(200);

    spi_bus_config_t buscfg = {
        .data0_io_num = boards->display.d0,
        .data1_io_num = boards->display.d1,
        .sclk_io_num = boards->display.sck,
        .data2_io_num = boards->display.d2,
        .data3_io_num = boards->display.d3,
        .data4_io_num = BOARD_NONE_PIN,
        .data5_io_num = BOARD_NONE_PIN,
        .data6_io_num = BOARD_NONE_PIN,
        .data7_io_num = BOARD_NONE_PIN,
        .max_transfer_sz = (SEND_BUF_SIZE * 16) + 8,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
    };

    spi_device_interface_config_t devcfg = {
        .command_bits = boards->display.cmdBit,
        .address_bits = boards->display.addBit,
        .mode = TFT_SPI_MODE,
        .clock_speed_hz = boards->display.freq,
        .spics_io_num = -1,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 17,
    };
    esp_err_t ret = spi_bus_initialize(DEFAULT_SPI_HANDLER, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        log_e("spi_bus_initialize fail!");
        return false;
    }
    ret = spi_bus_add_device(DEFAULT_SPI_HANDLER, &devcfg, &spi);
    if (ret != ESP_OK) {
        log_e("spi_bus_add_device fail!");
        return false;
    }
    // prevent initialization failure
    int retry = 2;
    while (retry--) {
        const lcd_cmd_t *t = boards->display.initSequence;
        for (uint32_t i = 0; i < boards->display.initSize; i++) {
            writeCommand(t[i].addr, (uint8_t *)t[i].param, t[i].len & 0x1F);
            if (t[i].len & 0x80) {
                delay(120);
            }
            if (t[i].len & 0x20) {
                delay(10);
            }
        }
    }
    return true;
}


bool LilyGo_AMOLED::begin()
{
    //Try find 1.47 inch i2c devices
    Wire.begin(1, 2);
    Wire.beginTransmission(AXP2101_SLAVE_ADDRESS);
    if (Wire.endTransmission() == 0) {
        return beginAMOLED_147();
    }

    log_e("Unable to detect 1.47-inch board model!");


    Wire.end();

    delay(10);

    // Try find 1.91 inch i2c devices
    Wire.begin(3, 2);
    Wire.beginTransmission(CSTXXX_SLAVE_ADDRESS);
    if (Wire.endTransmission() == 0) {
        return beginAMOLED_191(true);
    }
    log_e("Unable to detect 1.91-inch touch board model!");

    Wire.end();

    delay(10);

    // Try find 2.41 inch i2c devices
    Wire.begin(6, 7);
    Wire.beginTransmission(SY6970_SLAVE_ADDRESS);
    if (Wire.endTransmission() == 0) {
        return beginAMOLED_241();
    }
    log_e("Unable to detect 2.41-inch touch board model!");

    Wire.end();


    log_e("Begin 1.91-inch no touch board model");

    return beginAMOLED_191(false);
}


bool LilyGo_AMOLED::beginAutomatic()
{
    return begin();
}

bool LilyGo_AMOLED::beginAMOLED_191(bool touchFunc)
{
    boards = &BOARD_AMOLED_191;

    initBUS();

    if (touchFunc && boards->touch) {
        if (boards->touch->sda != -1 && boards->touch->scl != -1) {
            Wire.begin(boards->touch->sda, boards->touch->scl);
            deviceScan(&Wire, &Serial);

            // Try to find touch device
            Wire.beginTransmission(CST816_SLAVE_ADDRESS);
            if (Wire.endTransmission() == 0) {
                TouchDrvCSTXXX::setPins(boards->touch->rst, boards->touch->irq);
                /*
                bool res = TouchDrvCSTXXX::init(Wire, boards->touch->sda, boards->touch->scl, CST816_SLAVE_ADDRESS);
                if (!res) {
                    log_e("Failed to find CST816T - check your wiring!");
                    // return false;
                    _touchOnline = false;
                } else {
                    _touchOnline = true;
                }
                */
            }
        }
    } else {
        _touchOnline = false;
    }

    setRotation(0);

    return true;
}


bool LilyGo_AMOLED::beginAMOLED_241()
{
    boards = &BOARD_AMOLED_241;

    initBUS();

    if (boards->pmu) {
        Wire.begin(boards->pmu->sda, boards->pmu->scl);
        deviceScan(&Wire, &Serial);
        PowersSY6970::init(Wire, boards->pmu->sda, boards->pmu->scl, SY6970_SLAVE_ADDRESS);
        PowersSY6970::enableADCMeasure();
        PowersSY6970::disableOTG();
    }

    if (boards->touch) {
        // Try to find touch device
        Wire.beginTransmission(CST226SE_SLAVE_ADDRESS);
        if (Wire.endTransmission() == 0) {
            TouchDrvCSTXXX::setPins(boards->touch->rst, boards->touch->irq);
            /*
            bool res = TouchDrvCSTXXX::init(Wire, boards->touch->sda, boards->touch->scl, CST226SE_SLAVE_ADDRESS);
            if (!res) {
                log_e("Failed to find CST226SE - check your wiring!");
                // return false;
            } else {
                _touchOnline = true;
            }
            */
        }
    }

    if (boards->sd) {
        SPI.begin(boards->sd->sck, boards->sd->miso, boards->sd->mosi);
        // Set mount point to /fs
        if (!SD.begin(boards->sd->cs, SPI, 4000000U, "/fs")) {
            log_e("Failed to dected SDCard!");
        }
        if (SD.cardType() != CARD_NONE) {
            log_i("SD Card Size: %llu MB\n", SD.cardSize() / (1024 * 1024));
        }
    }

    setRotation(0);

    return true;
}


bool LilyGo_AMOLED::beginAMOLED_147()
{
    boards = &BOARD_AMOLED_147;

    if (!initPMU()) {
        log_e("Failed to find AXP2101 - check your wiring!");
        return false;
    }

    if (ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO) {
        deviceScan(&Wire, &Serial);
    }

    initBUS();


    if (boards->display.frameBufferSize) {
        if (psramFound()) {
            pBuffer = (uint16_t *)ps_malloc(boards->display.frameBufferSize);
        } else {
            pBuffer = (uint16_t *)malloc(boards->display.frameBufferSize);
        }
        assert(pBuffer);
    }

    TouchDrvCHSC5816::setPins(boards->touch->rst, boards->touch->irq);
    _touchOnline = TouchDrvCHSC5816::begin(Wire, CHSC5816_SLAVE_ADDRESS, boards->touch->sda, boards->touch->scl);
    if (!_touchOnline) {
        log_e("Failed to find CHSC5816 - check your wiring!");
        // return false;
    } else {
        TouchDrvCHSC5816::setMaxCoordinates(_width, _height);
        TouchDrvCHSC5816::setSwapXY(true);
        TouchDrvCHSC5816::setMirrorXY(false, true);
    }

    // Share I2C Bus
    bool res = SensorCM32181::begin(Wire, CM32181_SLAVE_ADDRESS, boards->sensor->sda, boards->sensor->scl);
    if (!res) {
        log_e("Failed to find CM32181 - check your wiring!");
        // return false;
    } else {
        /*
            Sensitivity mode selection
                SAMPLING_X1
                SAMPLING_X2
                SAMPLING_X1_8
                SAMPLING_X1_4
        */
        SensorCM32181::setSampling(SensorCM32181::SAMPLING_X2),
                      powerOn();
    }

    // Temperature detect
    beginCore();

    return true;
}

void LilyGo_AMOLED::writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t lenght)
{
    setCS();
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.flags = (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR);
    t.cmd = 0x02;
    t.addr = cmd;
    if (lenght != 0) {
        t.tx_buffer = pdat;
        t.length = 8 * lenght;
    } else {
        t.tx_buffer = NULL;
        t.length = 0;
    }
    spi_device_polling_transmit(spi, &t);
    clrCS();
}

void LilyGo_AMOLED::setBrightness(uint8_t level)
{
    _brightness = level;
    lcd_cmd_t t = {0x5100, {level}, 0x01};
    writeCommand(t.addr, t.param, t.len);
}

uint8_t LilyGo_AMOLED::getBrightness()
{
    return _brightness;
}

void LilyGo_AMOLED::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    lcd_cmd_t t[3] = {
        {
            0x2A00, {
                (uint8_t)((xs >> 8) & 0xFF),
                (uint8_t)(xs & 0xFF),
                (uint8_t)((xe >> 8) & 0xFF),
                (uint8_t)(xe & 0xFF)
            }, 0x04
        },
        {
            0x2B00, {
                (uint8_t)((ys >> 8) & 0xFF),
                (uint8_t)(ys & 0xFF),
                (uint8_t)((ye >> 8) & 0xFF),
                (uint8_t)(ye & 0xFF)
            }, 0x04
        },
        {
            0x2C00, {
                0x00
            }, 0x00
        },
    };

    for (uint32_t i = 0; i < 3; i++) {
        writeCommand(t[i].addr, t[i].param, t[i].len);
    }
}

// Push (aka write pixel) colours to the TFT (use setAddrWindow() first)
void LilyGo_AMOLED::pushColors(uint16_t *data, uint32_t len)
{
    bool first_send = true;
    uint16_t *p = data;
    assert(p);
    assert(spi);
    setCS();
    do {
        size_t chunk_size = len;
        spi_transaction_ext_t t = {0};
        memset(&t, 0, sizeof(t));
        if (first_send) {
            t.base.flags = SPI_TRANS_MODE_QIO;
            t.base.cmd = 0x32 ;
            t.base.addr = 0x002C00;
            first_send = 0;
        } else {
            t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;
            t.command_bits = 0;
            t.address_bits = 0;
            t.dummy_bits = 0;
        }
        if (chunk_size > SEND_BUF_SIZE) {
            chunk_size = SEND_BUF_SIZE;
        }
        t.base.tx_buffer = p;
        t.base.length = chunk_size * 16;
        spi_device_polling_transmit(spi, (spi_transaction_t *)&t);
        len -= chunk_size;
        p += chunk_size;
    } while (len > 0);
    clrCS();
}

void LilyGo_AMOLED::pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t hight, uint16_t *data)
{

    if (boards->display.frameBufferSize) {
        assert(pBuffer);
        uint16_t _x = this->height() - (y + hight);
        uint16_t _y = x;
        uint16_t _h = width;
        uint16_t _w = hight;
        uint16_t *p = data;
        uint32_t cum = 0;
        for (uint16_t j = 0; j < width; j++) {
            for (uint16_t i = 0; i < hight; i++) {
                pBuffer[cum] = ((uint16_t)p[width * (hight - i - 1) + j]);
                cum++;
            }
        }
        setAddrWindow(_x, _y, _x + _w - 1, _y + _h - 1);
        pushColors(pBuffer, width * hight);
    } else {
        setAddrWindow(x, y, x + width - 1, y + hight - 1);
        pushColors(data, width * hight);
    }
}


void LilyGo_AMOLED::beginCore()
{
    // https://docs.espressif.com/projects/esp-idf/zh_CN/v4.4.4/esp32s3/api-reference/peripherals/temp_sensor.html
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
#else
    // https://docs.espressif.com/projects/esp-idf/zh_CN/v5.0.1/esp32s3/api-reference/peripherals/temp_sensor.html
    static temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    temperature_sensor_install(&temp_sensor_config, &temp_sensor);
    temperature_sensor_enable(temp_sensor);
#endif
}


float LilyGo_AMOLED::readCoreTemp()
{
    float tsens_value;
    // https://docs.espressif.com/projects/esp-idf/zh_CN/v4.4.4/esp32s3/api-reference/peripherals/temp_sensor.html
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
    temp_sensor_read_celsius(&tsens_value);
#else
    // https://docs.espressif.com/projects/esp-idf/zh_CN/v5.0.1/esp32s3/api-reference/peripherals/temp_sensor.html
    temperature_sensor_get_celsius(temp_sensor, &tsens_value);
#endif
    return tsens_value;
}


void LilyGo_AMOLED::attachPMU(void(*cb)(void))
{
    if (boards) {
        if (boards->pmu && (boards == &BOARD_AMOLED_147)) {
            pinMode(BOARD_PMU_IRQ, INPUT_PULLUP);
            attachInterrupt(BOARD_PMU_IRQ, cb, FALLING);
        }
    }
}

uint64_t LilyGo_AMOLED::readPMU()
{
    if (boards) {
        if (boards->pmu && (boards == &BOARD_AMOLED_147)) {
            return XPowersAXP2101::getIrqStatus();
        }
    }
    return 0;
}

void LilyGo_AMOLED::clearPMU()
{
    if (boards) {
        if (boards->pmu && (boards == &BOARD_AMOLED_147)) {
            log_i("clearPMU");
            XPowersAXP2101::clearIrqStatus();
        }
    }
}

void LilyGo_AMOLED::enablePMUInterrupt(uint32_t params)
{
    if (boards) {
        if (boards->pmu && (boards == &BOARD_AMOLED_147)) {
            XPowersAXP2101::enableIRQ(params);
        }
    }
}
void LilyGo_AMOLED::diablePMUInterrupt(uint32_t params)
{
    if (boards) {
        if (boards->pmu && (boards == &BOARD_AMOLED_147)) {
            XPowersAXP2101::disableIRQ(params);
        }
    }
}


void LilyGo_AMOLED::sleep()
{
    assert(boards);

    //Wire amoled to sleep mode
    lcd_cmd_t t = {0x1000, {0x00}, 1}; //Sleep in
    writeCommand(t.addr, t.param, t.len);

    if (boards) {

        if (boards == &BOARD_AMOLED_241) {
            PowersSY6970::disableADCMeasure();
            PowersSY6970::disableOTG();

            // Disable amoled power
            digitalWrite(boards->PMICEnPins, LOW);
            TouchDrvCSTXXX::sleep();

        } else if (boards == &BOARD_AMOLED_147) {
            Serial.println("PMU Disbale AMOLED Power");

            // Turn off Sensor
            SensorCM32181::powerDown();

            // Turn off ADC data monitoring to save power
            disableTemperatureMeasure();
            disableBattDetection();
            disableVbusVoltageMeasure();
            disableBattVoltageMeasure();
            disableSystemVoltageMeasure();
            setChargingLedMode(XPOWERS_CHG_LED_OFF);

            // Disbale amoled power
            disableBLDO1();
            disableALDO3();

            // Don't turn off ALDO1
            // disableALDO1();

            // Keep touch reset to HIGH
            digitalWrite(boards->touch->rst, HIGH);
            gpio_hold_en((gpio_num_t )boards->touch->rst);
            gpio_deep_sleep_hold_en();
            // Enter sleep mode
            TouchDrvCHSC5816::sleep();

        } else {
            if (boards->PMICEnPins != -1) {
                // Disable amoled power
                digitalWrite(boards->PMICEnPins, LOW);
                TouchDrvCSTXXX::sleep();
            }
        }
    }
}

void LilyGo_AMOLED::wakeup()
{
    lcd_cmd_t t = {0x1100, {0x00}, 1};// Sleep Out
    writeCommand(t.addr, t.param, t.len);
}

bool LilyGo_AMOLED::hasTouch()
{
    if (boards && _touchOnline) {
        if (boards->touch) {
            return true;
        }
    }
    return false;
}

void LilyGo_AMOLED::setRotation(uint8_t rotation)
{
    uint8_t data = 0x00;
    rotation %= 4;
    _rotation = rotation;
    if (boards == &BOARD_AMOLED_191) {
        switch (_rotation) {
        case 1:
            data = RM67162_MADCTL_RGB;
            _height = boards->display.height;
            _width = boards->display.width;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(true);
                TouchDrvCSTXXX::setMirrorXY(true, false);
            }
            break;
        case 2:
            data = RM67162_MADCTL_MV | RM67162_MADCTL_MY | RM67162_MADCTL_RGB;
            _height = boards->display.width;
            _width = boards->display.height;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(false);
                TouchDrvCSTXXX::setMirrorXY(true, true);
            }
            break;
        case 3:
            data = RM67162_MADCTL_MX | RM67162_MADCTL_MY | RM67162_MADCTL_RGB;
            _height = boards->display.height;
            _width = boards->display.width;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(true);
                TouchDrvCSTXXX::setMirrorXY(false, true);
            }
            break;
        default:
            data = RM67162_MADCTL_MX | RM67162_MADCTL_MV | RM67162_MADCTL_RGB;
            _height = boards->display.width;
            _width = boards->display.height;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(false);
                TouchDrvCSTXXX::setMirrorXY(false, false);
            }
            break;
        }
        writeCommand(0x3600, &data, 1);
    } else if (boards == &BOARD_AMOLED_241) {
        switch (_rotation) {
        case 1:
            data = RM690B0_MADCTL_RGB;
            _height = boards->display.width;
            _width = boards->display.height;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(false);
                TouchDrvCSTXXX::setMirrorXY(false, false);
            }
            break;
        case 2:
            data = RM690B0_MADCTL_MV | RM690B0_MADCTL_MY | RM690B0_MADCTL_RGB;
            _height = boards->display.height;
            _width = boards->display.width;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(true);
                TouchDrvCSTXXX::setMirrorXY(true, false);
            }
            break;
        case 3:
            data = RM690B0_MADCTL_MX | RM690B0_MADCTL_MY | RM690B0_MADCTL_RGB;
            _height = boards->display.width;
            _width = boards->display.height;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(false);
                TouchDrvCSTXXX::setMirrorXY(true, true);
            }
            break;
        default:
            data = RM690B0_MADCTL_MX | RM690B0_MADCTL_MV | RM690B0_MADCTL_RGB;
            _height = boards->display.height;
            _width = boards->display.width;
            if (_touchOnline) {
                TouchDrvCSTXXX::setMaxCoordinates(_width, _height);
                TouchDrvCSTXXX::setSwapXY(true);
                TouchDrvCSTXXX::setMirrorXY(false, true);
            }
            break;
        }
        writeCommand(0x3600, &data, 1);
    } else {
        Serial.println("The screen you are currently using does not support screen rotation!!!");
    }
}

uint8_t LilyGo_AMOLED::getRotation()
{
    return (_rotation);
}

#endif