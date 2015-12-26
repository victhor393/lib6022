/*  lib6022 - a library for controlling the Hantek 6022BE USB oscilloscope
    Copyright (C) 2015 Victhor Foster <victhor.foster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */


#include <libusb-1.0/libusb.h>
#include <stdio.h>

/** 
    Sample rates: 0x0A: 100 Ks/s
                  0x14: 200 Ks/s
                  0x32: 500 Ks/s
                  0x01: 1 MS/s
                  0x04: 4 MS/s
                  0x08: 8 MS/s
                  0x10: 16 MS/s
                  0x18: 24 MS/s
                  0x1E: 30 MS/s
                  0x30: 48 MS/s

    Voltage ranges: 0x01: +/- 5 V
                    0x02: +/- 2.5 V 
                    0x05: +/- 1 V
                    0x0a: +/- 500 mV **/ 

static void async_callback(struct libusb_transfer *xfr)
{
    int err;
    
    if(xfr->status != LIBUSB_TRANSFER_COMPLETED)
    {
        fprintf(stderr, "Transfer error: %d\n", xfr->status);
    }
    err = libusb_submit_transfer(xfr);
    if(err != 0)
    {
        fprintf(stderr, "Error submitting transfer: %d\n", err);
    }
}



int scope_set_channels(libusb_device_handle *handle, unsigned char nch)
{
/* Set number of channels enabled by scope. Only supported by custom firmware.
   handle = libusb device handle
   nch = number of channels to enable. 0x01 = enable channel 1 only; 0x02 = enable both channels */

    int err;

    if(nch != 1 && nch != 2)
    {
        fprintf(stderr, "Invalid number of channels!\n");
        return -127;
    }
    else
    {
        err = libusb_control_transfer(handle, 0x40, 0xe4, 0, 0, &nch, sizeof(nch), 0);
        return err;
    }
}

int scope_set_sample_rate(libusb_device_handle *handle, unsigned char r)
{
/* Set sample rate.
   handle = libusb device handle
   r = sample rate */

    int i;
    int err;
    const unsigned char val_rate[] = {0x01, 0x04, 0x08, 0x10, 0x18, 0x1e, 0x30, 0x0a, 0x14, 0x32};
    
    for(i = 0; i <= 9; i++)
    {
        if(r == val_rate[i])
        {
            break;
        }
    }
    if(i == 10)
    {
        fprintf(stderr, "Invalid sample rate!\n");
        return -127;
    }
    err = libusb_control_transfer(handle, 0x40, 0xe2, 0, 0, &r, sizeof(r), 0);
    return err;
}

int scope_set_voltage_range(libusb_device_handle *handle, unsigned char v, int sch)
{
/* Set voltage range for channel
   handle = libusb device handle
   v = voltage range
   sch = channel to change voltage range, 1 or 2 */

    int i;
    int err;
    uint8_t ch;
    const unsigned char val_range[] = {0x01, 0x02, 0x05, 0x0a};

    for(i = 0; i <= 3; i++)
    {
        if(v == val_range[i])
        {
            break;
        }
    }
    if(i == 4)
    {
        fprintf(stderr, "Invalid voltage range!\n");
        return -127;
    }

    if(sch == 1)
    {
        ch = 0xe0;
    }
    if(sch == 2)
    {
        ch = 0xe1;
    }
    if(sch != 1 && sch != 2)
    {
        fprintf(stderr, "Invalid channel for voltage range!\n");
    }

    err = libusb_control_transfer(handle, 0x40, ch, 0, 0, &v, sizeof(v), 0);
    return err;
}

int scope_start(libusb_device_handle *handle)
{
/* Clears FIFO and starts sampling. Does not actually trigger the scope, this must be done by software, if required. Custom firmware requires a small delay after calling this function to avoid a hang.
   handle = libusb device handle */

    int err;
    unsigned char data = 0x01;
    err = libusb_control_transfer(handle, 0x40, 0xe3, 0, 0, &data, sizeof(data), 0);
    return err;
}

int scope_stop(libusb_device_handle *handle)
{
/* Stops sampling. Only supported by custom firmware. 
   handle = libusb device handle */

    int err;
    unsigned char data = 0x00;
    err = libusb_control_transfer(handle, 0x40, 0xe3, 0, 0, &data, sizeof(data), 0);
    return err;
}

int scope_read_async(libusb_device_handle *handle, unsigned char *buf, int length, int num_xfrs)
{
/* Sets up asynchronous bulk transfer from scope.
   handle = libusb device handle
   buf = buffer to store transfer data
   length = size of buffer
   num_xfrs = number of asynchronous transfers to queue */

    int i;
    int err = 0;
    struct libusb_transfer *xfr;
    
    for(i = 0; i < num_xfrs; i++)
    {
        xfr = libusb_alloc_transfer(0);
        libusb_fill_bulk_transfer(xfr, handle, 0x86, buf, length, async_callback, NULL, 0);
        err = libusb_submit_transfer(xfr);
    }
    return err;
}

int scope_read_sync(libusb_device_handle *handle, unsigned char *buf, int length, int* transferred)
{
/* Perform synchronous bulk transfer from scope.
   handle = libusb device handle
   buf = buffer to store transfer data
   length = size of buffer
   transferred = number of bytes transferred. does not accept NULL pointer */

    int err;
    
    err = libusb_bulk_transfer(handle, 0x86, buf, length, transferred, 0);

    return err;
}
 
int scope_async_wait(void)
{
/* Handle asynchronous transfer events. Blocking function, only returns on error. */

    int err;

    while(1)
    {
        err = libusb_handle_events(NULL);
        if(err != 0)
        {
            fprintf(stderr, "libusb transfer event error: %d\n", err);

            if (err != LIBUSB_ERROR_BUSY && err != LIBUSB_ERROR_TIMEOUT && err != LIBUSB_ERROR_OVERFLOW && err != LIBUSB_ERROR_INTERRUPTED) 
            {
                break; /* Exit only on fatal errors */
            }
        }
    }

    return err;
}
