#include "CST328.h"
#include "pico/stdlib.h"



typedef enum
{
    CST328_1ST_TOUCH_ID = 0xd000,
    CST328_1ST_TOUCH_XH,
    CST328_1ST_TOUCH_YH,
    CST328_1ST_TOUCH_XLYL,
    CST328_1ST_TOUCH_PRESSURE,

    CST328_TOUCH_FLAG_AND_NUM,

    CST328_2ND_TOUCH_ID = 0xd007,
    CST328_2ND_TOUCH_XH,
    CST328_2ND_TOUCH_YH,
    CST328_2ND_TOUCH_XLYL,
    CST328_2ND_TOUCH_PRESSURE,

    CST328_3RD_TOUCH_ID,
    CST328_3RD_TOUCH_XH,
    CST328_3RD_TOUCH_YH,
    CST328_3RD_TOUCH_XLYL,
    CST328_3RD_TOUCH_PRESSURE,

    CST328_4TH_TOUCH_ID,
    CST328_4TH_TOUCH_XH,
    CST328_4TH_TOUCH_YH,
    CST328_4TH_TOUCH_XLYL,
    CST328_4TH_TOUCH_PRESSURE,

    CST328_5TH_TOUCH_ID,
    CST328_5TH_TOUCH_XH,
    CST328_5TH_TOUCH_YH,
    CST328_5TH_TOUCH_XLYL,
    CST328_5TH_TOUCH_PRESSURE,

    CST328_ENUM_MODE_DEBUG_INFO = 0xd101,
    CST328_SYSTEM_RESET,
    CST328_REDO_CALIBRATION = 0xd104,
    CST328_DEEP_SLEEP,
    CST328_ENUM_MODE_DEBUG_POINTS = 0xd108,
    CST328_ENUM_MODE_NORMAL,
    CST328_ENUM_MODE_DEBUG_RAWDATA,
    CST328_ENUM_MODE_DEBUG_WRITE,
    CST328_ENUM_MODE_DEBUG_CALIBRATION,
    CST328_ENUM_MODE_DEBUG_DIFF,
    CST328_ENUM_MODE_FACTORY = 0xd119,

    CST328_IC_INFO = 0xd1f4,
    CST328_IC_RES = 0xd1f8,
    CST328_IC_CHECK_CODE_AND_BOOT_TIMER = 0xd1fc,
    CST328_IC_TYPE_AND_PROJECT_ID = 0xd200,
    CST328_IC_VERSION = 0xd204,
} cst328_reg_t;


// Set by ISR when data read. Note that only one static flag so need to 
// revist if more than one touch panel (need global dispatcher look up table)
bool CST328::read_data_done = false;

void CST328::read_byte(uint16_t reg_addr, uint8_t *data, size_t len)
{
    uint8_t write_buffer[2];
    write_buffer[0] = (uint8_t)(reg_addr >> 8);
    write_buffer[1] = (uint8_t)(reg_addr);
    _pi2c->write(CST328_DEVICE_ADDR, write_buffer, 2, true);
    _pi2c->read(CST328_DEVICE_ADDR, data, len, false);
}

void CST328::write_byte(uint16_t reg_addr, uint8_t *data, size_t len)
{
    uint8_t write_buffer[len + 2];
    write_buffer[0] = (uint8_t)(reg_addr >> 8);
    write_buffer[1] = (uint8_t)(reg_addr);
    memcpy(write_buffer + 2, data, len);
    _pi2c->write(CST328_DEVICE_ADDR, write_buffer, len+2, false);
}

void CST328::reset(void)
{
    gpio_put(_rst_pin, 0);
    sleep_ms(10);
    gpio_put(_rst_pin, 1);
    sleep_ms(50);
}

void CST328::read(void)
{
    uint8_t buf[28];
    uint8_t points = 0;
    uint8_t num = 0;
    
    if (!read_data_done)
        return;
    read_data_done = false;
    
    read_byte(CST328_TOUCH_FLAG_AND_NUM, &buf[0], 1);
    if ((buf[0] & 0x0F) != 0x00)
    {
        points = buf[0] & 0x0F;
        read_byte(CST328_1ST_TOUCH_ID, &buf[1], 27);

        if ((buf[1] & 0x0f) != 0x06)
        {
            _data.points = 0;
            return;
        }

        if (points > CST328_LCD_TOUCH_MAX_POINTS)
            points = CST328_LCD_TOUCH_MAX_POINTS;
        _data.points = (uint8_t)points;

        for (int i = 0; i < points; i++)
        {
            if (i > 0)
                num = 2;
            _data.coords[i].x = (uint16_t)(((uint16_t)buf[(i * 5) + 2 + num] << 4) + ((buf[(i * 5) + 4 + num] & 0xF0) >> 4));
            _data.coords[i].y = (uint16_t)(((uint16_t)buf[(i * 5) + 3 + num] << 4) + (buf[(i * 5) + 4 + num] & 0x0F));
            _data.coords[i].pressure = ((uint16_t)buf[(i * 5) + 5 + num]);
        }
    }
}

bool CST328::get_touch_data(CST328::Data* pdata)
{
    memcpy(pdata, &_data, sizeof(CST328::Data));
    pdata->points = _data.points;
    _data.points = 0;

    switch (_rotation)
    {
    case 1:
        for (int i = 0; i < pdata->points; i++)
        {
            pdata->coords[i].x = _data.coords[i].y;
            pdata->coords[i].y = _height - 1 - _data.coords[i].x;
        }
        break;
    case 2:
        for (int i = 0; i < pdata->points; i++)
        {
            pdata->coords[i].x = _width - 1 - _data.coords[i].x;
            pdata->coords[i].y = _height - 1 - _data.coords[i].y;
        }
        break;

    case 3:
        for (int i = 0; i < pdata->points; i++)
        {
            pdata->coords[i].x = _width - _data.coords[i].y;
            pdata->coords[i].y = _data.coords[i].x;
        }
        break;
    default:
        break;
    }

    // printf("x:%d, y:%d rotation:%d\r\n", cst328_data->coords[0].x, cst328_data->coords[0].y, _rotation);
    // printf("g_cst328_info ->width:%d ->height:%d  rotation:%d \r\n", _width, _height, _rotation);
    return (pdata->points != 0);
}

void CST328::set_rotation(uint16_t rotation)
{
    uint16_t swap;
    _rotation = rotation;
    if (rotation == 1 || rotation == 3)
    {
        if (_width < _height)
        {
            swap = _width;
            _width = _height;
            _height = swap;
        }
    }
    else
    {
        if (_width > _height)
        {
            swap = _width;
            _width = _height;
            _height = swap;
        }
    }
}

// External interrupt service routine
void gpio_isr_handler(uint gpio, uint32_t events)
{
    if (gpio == BSP_CST328_INT_PIN)
    {
        // This is where you can handle key press events.
        // printf("Button pressed!\n");
        // bsp_cst328_read();
        CST328::read_data_done = true;
    }
}

CST328::CST328(I2C* pi2c, uint8_t rst_pin, uint8_t int_pin, uint16_t rotation,  uint16_t width,  uint16_t height)
: _pi2c(pi2c)
, _rst_pin(rst_pin)
, _int_pin(int_pin)
,_rotation(rotation)
, _width(width)
, _height(height){

    uint8_t buf[24] = {0};
    uint16_t check_code;
    gpio_init(_rst_pin);
    gpio_set_dir(_rst_pin, GPIO_OUT);

    reset();
    
    set_rotation(rotation);

    // View touch screen information mode
    write_byte(CST328_ENUM_MODE_DEBUG_INFO, NULL, 0);

    read_byte(CST328_IC_INFO, buf, 24);
    printf("D1F4:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[0], buf[1], buf[2], buf[3]);
    printf("D1F8:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[4], buf[5], buf[6], buf[7]);
    printf("D1FC:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[8], buf[9], buf[10], buf[11]);
    printf("D200:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[12], buf[13], buf[14], buf[15]);
    printf("D204:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[16], buf[17], buf[18], buf[19]);
    printf("D208:0x%02x,0x%02x,0x%02x,0x%02x\r\n", buf[20], buf[21], buf[22], buf[23]);
    check_code = (((uint16_t)buf[11] << 8) | buf[10]);

    // Enter normal reporting mode
    write_byte(CST328_ENUM_MODE_NORMAL, NULL, 0);
    if (check_code == 0xcaca)
        printf("Find CST328!\r\n");
    else
        printf("CST328 not found!\r\n");

    gpio_init(_int_pin);
    gpio_set_dir(_int_pin, GPIO_IN);
    gpio_pull_up(_int_pin);
    gpio_set_irq_enabled_with_callback(_int_pin, GPIO_IRQ_EDGE_FALL, true, gpio_isr_handler);

}

