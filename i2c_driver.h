#ifndef __I2C_DRIVER_H__
#define __I2C_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

typedef enum
{
    I2C_DRIVER_RETURN_FAILED = -1,
    I2C_DRIVER_RETURN_SUCCESS = 0
} I2C_DRIVER_RETURN_E;

typedef struct _i2c_device
{
    int bus;
    unsigned short addr;
    unsigned char tenbit;
    unsigned char delay;
    unsigned short flags;
    unsigned int page_bytes;
    unsigned int iaddr_bytes;
} i2c_device_t;

typedef struct _i2c_driver i2c_driver_t;

struct _i2c_driver
{
    char m_bus_name[128];
    i2c_device_t m_device;

    int32_t (*i2c_driver_open)(i2c_driver_t *_this_driver, const char *_bus_name);
    int32_t (*i2c_driver_close)(i2c_driver_t *_this_driver);
    int32_t (*i2c_driver_read)(i2c_driver_t *_this_driver, bool _using_ioctl, unsigned int _slave_address, unsigned int _register_offset, void *_buf, int _length);
    int32_t (*i2c_driver_write)(i2c_driver_t *_this_driver, bool _using_ioctl, unsigned int _slave_address, unsigned int _register_offset, const void *_buf, int _length);
    void (*i2c_driver_set_config)(i2c_driver_t *_this_driver, unsigned char _tenbit, unsigned char _delay, unsigned short _flags, unsigned int _page_bytes, unsigned int _iaddr_bytes);
    void (*i2c_driver_reset)(i2c_driver_t *_this_driver);
};

i2c_driver_t *i2c_driver_create(const char *_bus_name);
void i2c_driver_destroy(i2c_driver_t *_this_driver);

#endif