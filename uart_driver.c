#include "uart_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#define UART_DRIVER_DEBUG 0

static speed_t uart_get_speed(unsigned baudrate)
{
    switch (baudrate)
    {
    case 50:
        return B50;
    case 75:
        return B75;
    case 110:
        return B110;
    case 134:
        return B134;
    case 150:
        return B150;
    case 200:
        return B200;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 1800:
        return B1800;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
#ifdef B57600
    case 57600:
        return B57600;
#endif
#ifdef B115200
    case 115200:
        return B115200;
#endif
#ifdef B230400
    case 230400:
        return B230400;
#endif
#ifdef B460800
    case 460800:
        return B460800;
#endif
#ifdef B500000
    case 500000:
        return B500000;
#endif
#ifdef B576000
    case 576000:
        return B576000;
#endif
#ifdef B921600
    case 921600:
        return B921600;
#endif
#ifdef B1000000
    case 1000000:
        return B1000000;
#endif
#ifdef B1152000
    case 1152000:
        return B1152000;
#endif
#ifdef B1500000
    case 1500000:
        return B1500000;
#endif
#ifdef B2000000
    case 2000000:
        return B2000000;
#endif
#ifdef B2500000
    case 2500000:
        return B2500000;
#endif
#ifdef B3000000
    case 3000000:
        return B3000000;
#endif
#ifdef B3500000
    case 3500000:
        return B3500000;
#endif
#ifdef B4000000
    case 4000000:
        return B4000000;
#endif
    default:
        return (speed_t)baudrate;
    }
}

static int32_t uart_set_prop(uart_driver_t *_this_driver, unsigned _baudrate, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control)
{
    struct termios tty;
    speed_t speed;

    if (_this_driver == NULL || _this_driver->m_driver_des < 0)
    {
        return UART_DRIVER_RETURN_FAILED;
    }

    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(_this_driver->m_driver_des, &tty) != 0)
    {
        return UART_DRIVER_RETURN_FAILED;
    }

    if (_parity == true)
        tty.c_cflag |= PARENB;
    else
        tty.c_cflag &= (tcflag_t)(~PARENB);

    if (_stop_bit == true)
        tty.c_cflag |= CSTOPB;
    else
        tty.c_cflag &= (tcflag_t)(~CSTOPB);

    tty.c_cflag &= (tcflag_t)(~CSIZE);
    switch (_bit_per_byte)
    {
    case 5:
        tty.c_cflag |= CS5;
        break;
    case 6:
        tty.c_cflag |= CS6;
        break;
    case 7:
        tty.c_cflag |= CS7;
        break;
    case 8:
        tty.c_cflag |= CS8;
        break;
    default:
        tty.c_cflag |= CS8;
        break;
    }

    if (_flow_control == true)
        tty.c_cflag |= CRTSCTS;
    else
        tty.c_cflag &= (tcflag_t)(~CRTSCTS);

    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= (tcflag_t)(~ICANON);
    tty.c_lflag &= (tcflag_t)(~ECHO);
    tty.c_lflag &= (tcflag_t)(~ECHOE);
    tty.c_lflag &= (tcflag_t)(~ECHONL);
    tty.c_lflag &= (tcflag_t)(~ISIG);

    tty.c_iflag &= (tcflag_t)(~(IXON | IXOFF | IXANY));
    tty.c_iflag &= (tcflag_t)(~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL));

    tty.c_oflag &= (tcflag_t)(~OPOST);
    tty.c_oflag &= (tcflag_t)(~ONLCR);

    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    speed = uart_get_speed(_baudrate);
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    if (tcsetattr(_this_driver->m_driver_des, TCSANOW, &tty) != 0)
    {
        return UART_DRIVER_RETURN_FAILED;
    }

    _this_driver->m_baudrate     = _baudrate;
    _this_driver->m_parity       = _parity;
    _this_driver->m_stop_bit     = _stop_bit;
    _this_driver->m_bit_per_byte = _bit_per_byte;
    _this_driver->m_flow_control = _flow_control;

    return UART_DRIVER_RETURN_SUCCESS;
}

static int32_t uart_send(uart_driver_t *_this_driver, uint8_t *_buffer_to_send, int _length)
{
    ssize_t sent_count;

    if (_this_driver == NULL || _buffer_to_send == NULL || _length <= 0 || _this_driver->m_driver_des < 0)
    {
        return UART_DRIVER_RETURN_FAILED;
    }

    sent_count = write(_this_driver->m_driver_des, _buffer_to_send, (size_t)_length);

    if (sent_count != _length)
    {
        return UART_DRIVER_RETURN_FAILED;
    }

    return UART_DRIVER_RETURN_SUCCESS;
}

static int32_t uart_recv(uart_driver_t *_this_driver, uint8_t *_buffer_to_read, int _length)
{
    ssize_t received_count;

    if (_this_driver == NULL || _buffer_to_read == NULL || _length <= 0 || _this_driver->m_driver_des < 0)
    {
        return UART_DRIVER_RETURN_FAILED;
    }

    received_count = read(_this_driver->m_driver_des, _buffer_to_read, (size_t)_length);

#if UART_DRIVER_DEBUG
    printf("[UART Driver] Packet recved in hex: ");
    for (int i = 0; i < _length; i++)
    {
        printf("%02x ", _buffer_to_read[i]);
    }
    printf("\n");
#endif

    if (received_count != _length)
    {
        return UART_DRIVER_RETURN_FAILED;
    }

    return UART_DRIVER_RETURN_SUCCESS;
}

static void uart_reset(uart_driver_t *_this_driver)
{
    uint8_t data;
    ssize_t received_count;
    struct timespec start_time;
    struct timespec current_time;
    long elapsed_us;

    if (_this_driver == NULL || _this_driver->m_driver_des < 0)
    {
        return;
    }

    received_count = read(_this_driver->m_driver_des, &data, 1);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (received_count > 0)
    {
        received_count = read(_this_driver->m_driver_des, &data, 1);
        clock_gettime(CLOCK_MONOTONIC, &current_time);

        elapsed_us = (current_time.tv_sec - start_time.tv_sec) * 1000000L +
                     (current_time.tv_nsec - start_time.tv_nsec) / 1000L;

        if (elapsed_us > 3000000L)
        {
            break;
        }
    }
}

uart_driver_t *uart_driver_create(const char *_name, unsigned _baudrate, bool _parity, bool _stop_bit, int _bit_per_byte, bool _flow_control)
{
    uart_driver_t *p_driver;

    if (_name == NULL)
    {
        return NULL;
    }

    p_driver = (uart_driver_t *)malloc(sizeof(uart_driver_t));
    if (p_driver == NULL)
    {
        return NULL;
    }

    memset(p_driver, 0, sizeof(uart_driver_t));

    strncpy(p_driver->m_driver_name, _name, sizeof(p_driver->m_driver_name) - 1);
    p_driver->m_driver_name[sizeof(p_driver->m_driver_name) - 1] = '\0';

    p_driver->m_driver_des = open(p_driver->m_driver_name, O_RDWR);
    if (p_driver->m_driver_des < 0)
    {
        free(p_driver);
        return NULL;
    }

    p_driver->m_baudrate                 = _baudrate;
    p_driver->m_parity                   = _parity;
    p_driver->m_stop_bit                 = _stop_bit;
    p_driver->m_bit_per_byte             = _bit_per_byte;
    p_driver->m_flow_control             = _flow_control;

    p_driver->uart_driver_set_properties = uart_set_prop;
    p_driver->uart_driver_send           = uart_send;
    p_driver->uart_driver_recv           = uart_recv;
    p_driver->uart_driver_reset          = uart_reset;

    if (p_driver->uart_driver_set_properties(p_driver, _baudrate, _parity, _stop_bit, _bit_per_byte, _flow_control) != UART_DRIVER_RETURN_SUCCESS)
    {
        close(p_driver->m_driver_des);
        free(p_driver);
        return NULL;
    }

    return p_driver;
}

void uart_driver_destroy(uart_driver_t *_this_driver)
{
    if (_this_driver == NULL)
    {
        return;
    }

    if (_this_driver->m_driver_des >= 0)
    {
        close(_this_driver->m_driver_des);
        _this_driver->m_driver_des = -1;
    }

    free(_this_driver);
}