#ifndef __UART_DRIVER_H__
#define __UART_DRIVER_H__

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum 
{
    UART_DRIVER_RETURN_FAILED = -1,
    UART_DRIVER_RETURN_SUCCESS 
} UART_DRIVER_RETURN_E;
typedef struct _uart_driver
{
    char m_driver_name[12];
    int m_driver_des;
    unsigned m_baudrate;
    bool m_parity;
    bool m_stop_bit;
    int  m_bit_per_byte;
    bool m_flow_control;

    int32_t uart_driver_set_properties(_uart_driver *_this_driver, unsigned _speed, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control);
    bool    uart_driver_send          (_uart_driver *_this_driver, uint8_t * _buffer_to_send, int _length);
    bool    uart_driver_recv          (_uart_driver *_this_driver, uint8_t * _buffer_to_send, int _length);
    void    uart_driver_reset         (_uart_driver *_this_driver);
} uart_driver_t;

uart_driver_t* uart_driver_create(unsigned _speed, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control);
void uart_driver_destroy         (_uart_driver *_this_driver);

#endif // __UART_DRIVER_H__