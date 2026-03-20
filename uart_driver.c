#include "uart_driver.h"

#include <fcntl.h>    // Contains file controls like O_RDWR
#include <errno.h>    // Error integer and strerror() function
#include <termios.h>  // Contains POSIX terminal control definitions
#include <unistd.h>   // write(), read(), close()

#include "stdlib.h"


const char DEVICE_FILE_DATA_COLLECTION_BOX_CTRL[12] = "/dev/ttyUL1";

uart_driver_t* uart_driver_create(unsigned _speed, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control)
{
    static uart_driver_t current_driver;

    uart_driver_t *p_driver = (uart_driver_t *)malloc(sizeof(uart_driver_t));
    p_driver = &current_driver;
    memset(p_driver, 0, sizeof(p_driver));
    memcpy(p_driver->m_driver_name, DEVICE_FILE_DATA_COLLECTION_BOX_CTRL, 12);

    struct termios tty;
    memset(&tty, 0, sizeof(tty));


}
