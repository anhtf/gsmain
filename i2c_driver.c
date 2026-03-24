#include "i2c_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define I2C_DEFAULT_NUMBER_OF_ADDRESS_BYTE 1
#define I2C_DEFAULT_NUMBER_OF_PAGEBYTE 32
#define I2C_DEFAULT_DELAY 1
#define INT_ADDR_MAX_BYTES 4
#define PAGE_MAX_BYTES 4096

#define GET_I2C_DELAY(delay) ((delay) == 0 ? I2C_DEFAULT_DELAY : (delay))
#define GET_I2C_FLAGS(tenbit, flags) ((tenbit) ? ((flags) | I2C_M_TEN) : (flags))
#define GET_WRITE_SIZE(addr, remain, page_bytes) ((addr) + (remain) > (page_bytes) ? (page_bytes) - (addr) : (remain))

static int i2c_open_bus(const char *bus_name)
{
    int fd = open(bus_name, O_RDWR);
    if (fd == -1)
    {
        return -1;
    }
    return fd;
}

static void i2c_close_bus(int bus)
{
    if (bus >= 0)
    {
        close(bus);
    }
}

static int i2c_select_device(int bus, unsigned long dev_addr, unsigned long tenbit)
{
    if (ioctl(bus, I2C_TENBIT, tenbit))
    {
        return -1;
    }

    if (ioctl(bus, I2C_SLAVE_FORCE, dev_addr))
    {
        return -1;
    }

    return 0;
}

static void i2c_iaddr_convert(unsigned int iaddr, unsigned int len, unsigned char *addr)
{
    union
    {
        unsigned int iaddr_value;
        unsigned char caddr[INT_ADDR_MAX_BYTES];
    } convert;

    int i;
    int j;

    convert.iaddr_value = htonl(iaddr);

    i = (int)len - 1;
    j = INT_ADDR_MAX_BYTES - 1;

    while (i >= 0 && j >= 0)
    {
        addr[i--] = convert.caddr[j--];
    }
}

static void i2c_delay_ms(unsigned char msec)
{
    usleep((useconds_t)msec * 1000U);
}

static ssize_t i2c_file_read(const i2c_device_t *device, unsigned int iaddr, void *buf, size_t len)
{
    ssize_t cnt;
    unsigned char addr[INT_ADDR_MAX_BYTES];
    unsigned char delay;

    if (device == NULL || buf == NULL)
    {
        return -1;
    }

    if (i2c_select_device(device->bus, device->addr, device->tenbit) == -1)
    {
        return -1;
    }

    memset(addr, 0, sizeof(addr));
    i2c_iaddr_convert(iaddr, device->iaddr_bytes, addr);

    if (device->iaddr_bytes > 0)
    {
        if (write(device->bus, addr, device->iaddr_bytes) != (ssize_t)device->iaddr_bytes)
        {
            return -1;
        }
    }

    delay = GET_I2C_DELAY(device->delay);
    i2c_delay_ms(delay);

    cnt = read(device->bus, buf, len);
    if (cnt == -1)
    {
        return -1;
    }

    return cnt;
}

static ssize_t i2c_file_write(const i2c_device_t *device, unsigned int iaddr, const void *buf, size_t len)
{
    ssize_t remain;
    ssize_t ret;
    size_t cnt;
    size_t size;
    const unsigned char *buffer;
    unsigned char delay;
    unsigned char tmp_buf[PAGE_MAX_BYTES + INT_ADDR_MAX_BYTES];

    if (device == NULL || buf == NULL)
    {
        return -1;
    }

    if (device->page_bytes == 0 || device->iaddr_bytes > INT_ADDR_MAX_BYTES || device->page_bytes > PAGE_MAX_BYTES)
    {
        return -1;
    }

    if (i2c_select_device(device->bus, device->addr, device->tenbit) == -1)
    {
        return -1;
    }

    remain = (ssize_t)len;
    cnt = 0;
    buffer = (const unsigned char *)buf;
    delay = GET_I2C_DELAY(device->delay);

    while (remain > 0)
    {
        size = GET_WRITE_SIZE(iaddr % device->page_bytes, (size_t)remain, device->page_bytes);

        memset(tmp_buf, 0, sizeof(tmp_buf));
        i2c_iaddr_convert(iaddr, device->iaddr_bytes, tmp_buf);
        memcpy(tmp_buf + device->iaddr_bytes, buffer, size);

        ret = write(device->bus, tmp_buf, device->iaddr_bytes + size);
        if (ret == -1 || (size_t)ret != device->iaddr_bytes + size)
        {
            return -1;
        }

        i2c_delay_ms(delay);

        cnt += size;
        iaddr += (unsigned int)size;
        buffer += size;
        remain -= (ssize_t)size;
    }

    return (ssize_t)cnt;
}

static ssize_t i2c_ioctl_read_impl(const i2c_device_t *device, unsigned int iaddr, void *buf, size_t len)
{
    struct i2c_msg ioctl_msg[2];
    struct i2c_rdwr_ioctl_data ioctl_data;
    unsigned char addr[INT_ADDR_MAX_BYTES];
    unsigned short flags;

    if (device == NULL || buf == NULL)
    {
        return -1;
    }

    memset(addr, 0, sizeof(addr));
    memset(ioctl_msg, 0, sizeof(ioctl_msg));
    memset(&ioctl_data, 0, sizeof(ioctl_data));

    flags = GET_I2C_FLAGS(device->tenbit, device->flags);

    if (device->iaddr_bytes > 0)
    {
        i2c_iaddr_convert(iaddr, device->iaddr_bytes, addr);

        ioctl_msg[0].len = device->iaddr_bytes;
        ioctl_msg[0].addr = device->addr;
        ioctl_msg[0].buf = addr;
        ioctl_msg[0].flags = flags;

        ioctl_msg[1].len = (unsigned short)len;
        ioctl_msg[1].addr = device->addr;
        ioctl_msg[1].buf = (__u8 *)buf;
        ioctl_msg[1].flags = (unsigned short)(flags | I2C_M_RD);

        ioctl_data.nmsgs = 2;
        ioctl_data.msgs = ioctl_msg;
    }
    else
    {
        ioctl_msg[0].len = (unsigned short)len;
        ioctl_msg[0].addr = device->addr;
        ioctl_msg[0].buf = (__u8 *)buf;
        ioctl_msg[0].flags = (unsigned short)(flags | I2C_M_RD);

        ioctl_data.nmsgs = 1;
        ioctl_data.msgs = ioctl_msg;
    }

    if (ioctl(device->bus, I2C_RDWR, (unsigned long)&ioctl_data) == -1)
    {
        return -1;
    }

    return (ssize_t)len;
}

static ssize_t i2c_ioctl_write_impl(const i2c_device_t *device, unsigned int iaddr, const void *buf, size_t len)
{
    ssize_t remain;
    size_t size;
    size_t cnt;
    const unsigned char *buffer;
    unsigned char delay;
    unsigned short flags;
    struct i2c_msg ioctl_msg;
    struct i2c_rdwr_ioctl_data ioctl_data;
    unsigned char tmp_buf[PAGE_MAX_BYTES + INT_ADDR_MAX_BYTES];

    if (device == NULL || buf == NULL)
    {
        return -1;
    }

    if (device->page_bytes == 0 || device->iaddr_bytes > INT_ADDR_MAX_BYTES || device->page_bytes > PAGE_MAX_BYTES)
    {
        return -1;
    }

    remain = (ssize_t)len;
    size = 0;
    cnt = 0;
    buffer = (const unsigned char *)buf;
    delay = GET_I2C_DELAY(device->delay);
    flags = GET_I2C_FLAGS(device->tenbit, device->flags);

    while (remain > 0)
    {
        size = GET_WRITE_SIZE(iaddr % device->page_bytes, (size_t)remain, device->page_bytes);

        memset(tmp_buf, 0, sizeof(tmp_buf));
        i2c_iaddr_convert(iaddr, device->iaddr_bytes, tmp_buf);
        memcpy(tmp_buf + device->iaddr_bytes, buffer, size);

        memset(&ioctl_msg, 0, sizeof(ioctl_msg));
        memset(&ioctl_data, 0, sizeof(ioctl_data));

        ioctl_msg.len = (unsigned short)(device->iaddr_bytes + size);
        ioctl_msg.addr = device->addr;
        ioctl_msg.buf = tmp_buf;
        ioctl_msg.flags = flags;

        ioctl_data.nmsgs = 1;
        ioctl_data.msgs = &ioctl_msg;

        if (ioctl(device->bus, I2C_RDWR, (unsigned long)&ioctl_data) == -1)
        {
            return -1;
        }

        i2c_delay_ms(delay);

        cnt += size;
        iaddr += (unsigned int)size;
        buffer += size;
        remain -= (ssize_t)size;
    }

    return (ssize_t)cnt;
}

static void i2c_reset(i2c_driver_t *_this_driver)
{
    if (_this_driver == NULL)
    {
        return;
    }

    if (_this_driver->m_device.bus >= 0)
    {
        fsync(_this_driver->m_device.bus);
    }
}

static void i2c_set_config(i2c_driver_t *_this_driver, unsigned char _tenbit, unsigned char _delay, unsigned short _flags, unsigned int _page_bytes, unsigned int _iaddr_bytes)
{
    if (_this_driver == NULL)
    {
        return;
    }

    _this_driver->m_device.tenbit = _tenbit;
    _this_driver->m_device.delay = _delay;
    _this_driver->m_device.flags = _flags;
    _this_driver->m_device.page_bytes = (_page_bytes == 0U) ? I2C_DEFAULT_NUMBER_OF_PAGEBYTE : _page_bytes;
    _this_driver->m_device.iaddr_bytes = _iaddr_bytes;
}

static int32_t i2c_driver_open_impl(i2c_driver_t *_this_driver, const char *_bus_name)
{
    if (_this_driver == NULL || _bus_name == NULL)
    {
        return I2C_DRIVER_RETURN_FAILED;
    }

    if (_this_driver->m_device.bus >= 0)
    {
        i2c_close_bus(_this_driver->m_device.bus);
        _this_driver->m_device.bus = -1;
    }

    memset(_this_driver->m_bus_name, 0, sizeof(_this_driver->m_bus_name));
    strncpy(_this_driver->m_bus_name, _bus_name, sizeof(_this_driver->m_bus_name) - 1);

    _this_driver->m_device.bus = i2c_open_bus(_this_driver->m_bus_name);
    if (_this_driver->m_device.bus == -1)
    {
        return I2C_DRIVER_RETURN_FAILED;
    }

    _this_driver->m_device.tenbit = 0;
    _this_driver->m_device.delay = I2C_DEFAULT_DELAY;
    _this_driver->m_device.flags = 0;
    _this_driver->m_device.page_bytes = I2C_DEFAULT_NUMBER_OF_PAGEBYTE;
    _this_driver->m_device.iaddr_bytes = I2C_DEFAULT_NUMBER_OF_ADDRESS_BYTE;
    _this_driver->m_device.addr = 0;

    return I2C_DRIVER_RETURN_SUCCESS;
}

static int32_t i2c_driver_close_impl(i2c_driver_t *_this_driver)
{
    if (_this_driver == NULL)
    {
        return I2C_DRIVER_RETURN_FAILED;
    }

    if (_this_driver->m_device.bus >= 0)
    {
        i2c_close_bus(_this_driver->m_device.bus);
        _this_driver->m_device.bus = -1;
    }

    return I2C_DRIVER_RETURN_SUCCESS;
}

static int32_t i2c_driver_read_impl(i2c_driver_t *_this_driver, bool _using_ioctl, unsigned int _slave_address, unsigned int _register_offset, void *_buf, int _length)
{
    ssize_t ret;

    if (_this_driver == NULL || _buf == NULL || _length <= 0 || _this_driver->m_device.bus < 0)
    {
        return I2C_DRIVER_RETURN_FAILED;
    }

    _this_driver->m_device.addr = (unsigned short)(_slave_address & 0x3FFU);

    if (_using_ioctl == true)
    {
        ret = i2c_ioctl_read_impl(&_this_driver->m_device, _register_offset, _buf, (size_t)_length);
    }
    else
    {
        ret = i2c_file_read(&_this_driver->m_device, _register_offset, _buf, (size_t)_length);
    }

    if (ret == -1 || (size_t)ret != (size_t)_length)
    {
        return I2C_DRIVER_RETURN_FAILED;
    }

    return I2C_DRIVER_RETURN_SUCCESS;
}

static int32_t i2c_driver_write_impl(i2c_driver_t *_this_driver, bool _using_ioctl, unsigned int _slave_address, unsigned int _register_offset, const void *_buf, int _length)
{
    ssize_t ret;

    if (_this_driver == NULL || _buf == NULL || _length <= 0 || _this_driver->m_device.bus < 0)
    {
        return I2C_DRIVER_RETURN_FAILED;
    }

    _this_driver->m_device.addr = (unsigned short)(_slave_address & 0x3FFU);

    if (_using_ioctl == true)
    {
        ret = i2c_ioctl_write_impl(&_this_driver->m_device, _register_offset, _buf, (size_t)_length);
    }
    else
    {
        ret = i2c_file_write(&_this_driver->m_device, _register_offset, _buf, (size_t)_length);
    }

    if (ret == -1 || (size_t)ret != (size_t)_length)
    {
        return I2C_DRIVER_RETURN_FAILED;
    }

    return I2C_DRIVER_RETURN_SUCCESS;
}

i2c_driver_t *i2c_driver_create(const char *_bus_name)
{
    i2c_driver_t *driver;

    if (_bus_name == NULL)
    {
        return NULL;
    }

    driver = (i2c_driver_t *)malloc(sizeof(i2c_driver_t));
    if (driver == NULL)
    {
        return NULL;
    }

    memset(driver, 0, sizeof(i2c_driver_t));

    driver->m_device.bus = -1;
    driver->m_device.tenbit = 0;
    driver->m_device.delay = I2C_DEFAULT_DELAY;
    driver->m_device.flags = 0;
    driver->m_device.page_bytes = I2C_DEFAULT_NUMBER_OF_PAGEBYTE;
    driver->m_device.iaddr_bytes = I2C_DEFAULT_NUMBER_OF_ADDRESS_BYTE;
    driver->m_device.addr = 0;

    driver->i2c_driver_open = i2c_driver_open_impl;
    driver->i2c_driver_close = i2c_driver_close_impl;
    driver->i2c_driver_read = i2c_driver_read_impl;
    driver->i2c_driver_write = i2c_driver_write_impl;
    driver->i2c_driver_set_config = i2c_set_config;
    driver->i2c_driver_reset = i2c_reset;

    if (driver->i2c_driver_open(driver, _bus_name) != I2C_DRIVER_RETURN_SUCCESS)
    {
        free(driver);
        return NULL;
    }

    return driver;
}

void i2c_driver_destroy(i2c_driver_t *_this_driver)
{
    if (_this_driver == NULL)
    {
        return;
    }

    if (_this_driver->m_device.bus >= 0)
    {
        i2c_close_bus(_this_driver->m_device.bus);
        _this_driver->m_device.bus = -1;
    }

    free(_this_driver);
}