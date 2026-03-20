#include "uart_driver.h"

#include <fcntl.h>   // Contains file controls like O_RDWR
#include <errno.h>   // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()
#include <sys/mman.h>
#include <unistd.h>
#include <memory.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include "stdlib.h"

#define UART_DRIVER_DEBUG 1

const char DEVICE_FILE_DATA_COLLECTION_BOX_CTRL[12] = "/dev/ttyUL1";

static int32_t uart_set_prop(struct _uart_driver *_this_driver, unsigned _baudrate, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control)
{
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(_this_driver->m_driver_des, &tty) != 0)
    {
        printf("[UART Driver] Failed to get attributes\r\n");
        return -1;
    }

    if (_parity)
    {
        tty.c_cflag |= PARENB;
    }
    else
    {
        tty.c_cflag &= ~PARENB;
    }

    if (_stop_bit)
    {
        tty.c_cflag |= CSTOPB;
    }
    else
    {
        tty.c_cflag &= ~CSTOPB;
    }

    tty.c_cflag &= ~CSIZE;
    switch (_bit_per_byte)
    {
    case 5:
    {
        tty.c_cflag |= CS5;
        break;
    }
    case 6:
    {
        tty.c_cflag |= CS6;
        break;
    }
    case 7:
    {
        tty.c_cflag |= CS7;
        break;
    }
    case 8:
    {
        tty.c_cflag |= CS8;
        break;
    }
    default:
    {
        tty.c_cflag |= CS8;
        break;
    }
    }

    if (_flow_control == true)
    {
        tty.c_cflag |= CRTSCTS;
    }
    else
    {
        tty.c_cflag &= ~CRTSCTS;
    }

    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~ICANON; // Disabling Canonical Mode
    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_lflag &= ~ECHOE;  // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    cfsetospeed(&tty, (speed_t)_baudrate);
    cfsetispeed(&tty, (speed_t)_baudrate);

    if (tcsetattr(_this_driver->m_driver_des, TCSANOW, &tty) != 0)
    {
        printf("[UART Driver] Failed to set attributes\n");
        return UART_DRIVER_RETURN_FAILED;
    }
    else
    {
        printf("[UART Driver] Success set attributes\n");
        return UART_DRIVER_RETURN_SUCCESS;
    }
}

static bool uart_send(struct _uart_driver *_this_driver, uint8_t *_buffer_to_send, int _length)
{
    ssize_t size_return = write(_this_driver->m_driver_des, _buffer_to_send, _length);
    if (size_return != _length)
    {
        printf("[UART Driver] Packet size sent not match\n");
        return UART_DRIVER_RETURN_FAILED;
    }
    else
    {
        printf("[UART Driver] Packet sent success\n");
        return UART_DRIVER_RETURN_SUCCESS;
    }
}

static bool uart_recv(struct _uart_driver *_this_driver, uint8_t *_buffer_to_read, int _length)
{
    ssize_t size_recv = read(_this_driver->m_driver_des, _buffer_to_read, _length);

#ifdef UART_DRIVER_DEBUG
    printf("[UART Driver] Packet recved in hex: ");
    for (int i = 0; i < _length; i++)
    {
        printf("%02x", _buffer_to_read[i]);
    }
    printf("\r\n");
#endif
    if (size_recv != _length)
    {
        printf("[UART Driver] Packet size recv not match\n");
        return UART_DRIVER_RETURN_FAILED;
    }
    else
    {
        return UART_DRIVER_RETURN_SUCCESS;
    }
}

uart_driver_t *uart_driver_create(unsigned _baudrate, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control)
{
    static uart_driver_t current_driver;
    uart_driver_t *p_driver = (uart_driver_t *)malloc(sizeof(uart_driver_t));
    p_driver = &current_driver;

    memset(p_driver, 0, sizeof(p_driver));
    memcpy(p_driver->m_driver_name, DEVICE_FILE_DATA_COLLECTION_BOX_CTRL, 12);

    p_driver->m_driver_des   = open(p_driver->m_driver_name, O_RDWR);
    p_driver->m_baudrate     = _baudrate;
    p_driver->m_bit_per_byte = _bit_per_byte;
    p_driver->m_flow_control = _flow_control;
    p_driver->m_stop_bit     = _stop_bit;
    p_driver->m_parity       = _parity;

    p_driver->uart_driver_set_properties = uart_set_prop;
    p_driver->uart_driver_send = uart_send;
    p_driver->uart_driver_recv = uart_recv;

    p_driver->uart_driver_set_properties(p_driver, p_driver->m_baudrate, p_driver->m_parity, p_driver->m_stop_bit, p_driver->m_bit_per_byte, p_driver->m_flow_control);

    return p_driver;
}
