#ifndef __UART_DRIVER_H__
#define __UART_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum
{
    UART_DRIVER_RETURN_FAILED = -1,
    UART_DRIVER_RETURN_SUCCESS = 0
} UART_DRIVER_RETURN_E;

typedef struct _uart_driver uart_driver_t;

struct _uart_driver
{
    char     m_driver_name[128];
    int      m_driver_des;
    unsigned m_baudrate;
    bool     m_parity;
    bool     m_stop_bit;
    int      m_bit_per_byte;
    bool     m_flow_control;

    int32_t (*uart_driver_set_properties)(uart_driver_t *_this_driver, unsigned _baudrate, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control);
    int32_t (*uart_driver_send)          (uart_driver_t *_this_driver, uint8_t *_buffer_to_send, int _length);
    int32_t (*uart_driver_recv)          (uart_driver_t *_this_driver, uint8_t *_buffer_to_read, int _length);
    void    (*uart_driver_reset)         (uart_driver_t *_this_driver);
};

uart_driver_t *uart_driver_create(const char *_name, unsigned _baudrate, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control);
void uart_driver_destroy(uart_driver_t *_this_driver);

#endif