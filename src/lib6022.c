/*  lib6022 - a library for controlling the Hantek 6022BE USB oscilloscope
    Copyright (C) 2015-2019 Victhor Foster <victhor.foster@gmail.com>

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
#include <stdlib.h>
#include "../include/lib6022.h"

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

struct scopedev
{
    libusb_context *ctx;
    struct libusb_device_handle *devh;
    callback_function cb;
    void *cb_ctx;
    int buf_len;
    int num_xfrs;
    unsigned char **xfr_buf;
    uint8_t sample_rate;
    uint8_t ch1_gain;
    uint8_t ch2_gain;
    uint8_t ch_count;
    struct libusb_transfer **xfr;
};

void test_function(void)
{
    fprintf(stderr, "this is a test\n");
}

void alloc_async_buffers(scope_dev *scope)
{
    int i;
    
    if(scope->xfr == NULL)
    {
        scope->xfr = malloc(scope->num_xfrs * sizeof(struct libusb_transfer*));
        fprintf(stderr, "num_xfrs: %d\n", scope->num_xfrs);
        for(i = 0; i < scope->num_xfrs; i++)
        {
            scope->xfr[i] = libusb_alloc_transfer(0);
        }
    }
    else fprintf(stderr, "xfr is not NULL\n");
    if(scope->xfr_buf == NULL)
    {
        scope->xfr_buf = malloc(scope->num_xfrs * sizeof(unsigned char*));
        fprintf(stderr, "buffer num_xfrs: %d\n", scope->num_xfrs);
        for(i = 0; i < scope->num_xfrs; i++)
        {
            scope->xfr_buf[i] = malloc(scope->buf_len);
        }
    }
    else fprintf(stderr, "xfr_buf is not NULL\n");
}

static void async_callback(struct libusb_transfer *xfr)
{
    scope_dev *sd = (scope_dev*)xfr->user_data;
    if(xfr->status == LIBUSB_TRANSFER_COMPLETED)
    {
        sd->cb(xfr->buffer, xfr->actual_length, sd->cb_ctx);
        libusb_submit_transfer(xfr);
    }
    else
    {
        fprintf(stderr, "Transfer error: %d\n", xfr->status);
        libusb_cancel_transfer(xfr);
    }
    //fprintf(stderr, "%d of %d bytes transferred\n", xfr->actual_length, xfr->length);
    /*err = libusb_submit_transfer(xfr);
    if(err != 0)
    {
        fprintf(stderr, "Error submitting transfer: %d\n", err);
    }*/

}

int scope_init(scope_dev **scope, uint8_t dev_idx/*libusb_context *ctx*/)
{
    libusb_device **list = NULL;
    int i = 0;
    ssize_t count;
    libusb_device *device;
    struct libusb_device_descriptor desc = {0};
    scope_dev *sd = NULL;

    sd = malloc(sizeof(scope_dev));
    libusb_init(&sd->ctx);
    count = libusb_get_device_list(sd->ctx, &list);
    
    for(size_t idx = 0; idx < count; idx++)
    {
        device = list[idx];
        libusb_get_device_descriptor(device, &desc);
        if(desc.idVendor == 0x04b5 && desc.idProduct == 0x6022)
        {
            fprintf(stderr, "Device found\n");
            if(i == dev_idx) break;
            else i++;
        }
    }

    libusb_open(device, &sd->devh);
    libusb_free_device_list(list, 1);
    device = libusb_get_device(sd->devh);
    libusb_get_device_descriptor(device, &desc);
    fprintf(stderr, "Vendor:Device = %04x:%04x\n", desc.idVendor, desc.idProduct);
    libusb_detach_kernel_driver(sd->devh, 0);
    libusb_claim_interface(sd->devh, 0);
    libusb_set_interface_alt_setting(sd->devh, 0, 0);
    
    *scope = sd;
    return 0;
}

int scope_set_channels(scope_dev *scope, uint8_t nch)
{
/* Set number of channels enabled by scope-> Only supported by custom firmware.
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
        scope->ch_count = nch;
        err = libusb_control_transfer(scope->devh, 0x40, 0xe4, 0, 0, &nch, sizeof(nch), 5);
        return err;
    }
}

int scope_set_sample_rate(scope_dev *scope, uint8_t r)
{
/* Set sample rate.
   handle = libusb device handle
   r = sample rate */

    int i;
    int err;
    const uint8_t val_rate[] = {1, 4, 8, 16, 24, 30, 48, 10, 20, 50};
    
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
    err = libusb_control_transfer(scope->devh, 0x40, 0xe2, 0, 0, &r, sizeof(r), 0);
    scope->sample_rate = r;
    return err;
}

int scope_set_voltage_range(scope_dev *scope, uint8_t v, int sch)
{
/* Set voltage range for channel
   handle = libusb device handle
   v = voltage range
   sch = channel to change voltage range, 1 or 2 */

    int i;
    int err;
    uint8_t ch;
    /*struct libusb_transfer *xfr;
    unsigned char *buf;
    buf = malloc(9*sizeof(unsigned char));*/
    const uint8_t val_range[] = {1, 2, 5, 10};

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
        scope->ch1_gain = v;
        ch = 0xe0;
    }
    if(sch == 2)
    {
        scope->ch2_gain = v;
        ch = 0xe1;
    }
    if(sch != 1 && sch != 2)
    {
        fprintf(stderr, "Invalid channel for voltage range!\n");
    }

    /*if(xfr != NULL)
    {
        libusb_free_transfer(xfr);
    }
    xfr = libusb_alloc_transfer(0);
    libusb_fill_control_setup(buf, 0x40, ch, 0, 0, 1);
    buf[8] = v;
    fprintf(stderr, "buf[8] = %d\n", buf[8]);
    libusb_fill_control_transfer(xfr, scope_dev.devh, buf, async_callback, NULL, 0);
    err = libusb_submit_transfer(xfr);
    fprintf(stderr, "transfer submitted\n");*/
    err = libusb_control_transfer(scope->devh, 0x40, ch, 0, 0, &v, sizeof(v), 0);
    return err;
}

int scope_start(scope_dev *scope)
{
/* Clears FIFO and starts sampling. Does not actually trigger the scope, this must be done by software, if required. Custom firmware requires a small delay after calling this function to avoid a hang.
   handle = libusb device handle */

    int err;
    unsigned char data = 0x01;
    err = libusb_control_transfer(scope->devh, 0x40, 0xe3, 0, 0, &data, sizeof(data), 0);
    return err;
}

int scope_stop(scope_dev *scope)
{
/* Stops sampling. Only supported by custom firmware. 
   handle = libusb device handle */

    int err;
    unsigned char data = 0x00;
    err = libusb_control_transfer(scope->devh, 0x40, 0xe3, 0, 0, &data, sizeof(data), 0);
    return err;
}

int scope_read_async(scope_dev *scope, int length, int num_xfrs, callback_function cb, void *ctx)
{
/* Sets up asynchronous bulk transfer from scope->
   handle = libusb device handle
   buf = buffer to store transfer data
   length = size of buffer
   num_xfrs = number of asynchronous transfers to queue */

    int i;
    int err = 0;
//    struct libusb_transfer *xfr;
    scope->cb = cb;
    scope->cb_ctx = ctx;
    scope->num_xfrs = num_xfrs;
    scope->buf_len = length;
    scope->xfr = NULL;
    scope->xfr_buf = NULL;
    
    alloc_async_buffers(scope);
    
    for(i = 0; i < scope->num_xfrs; i++)
    {
        //fprintf(stderr, "fill bulk transfer\n");
        libusb_fill_bulk_transfer(scope->xfr[i], scope->devh, 0x86, scope->xfr_buf[i], scope->buf_len, async_callback, (void *)scope, 0);
        err = libusb_submit_transfer(scope->xfr[i]);
    }
    return err;
}

int scope_read_sync(scope_dev *scope, unsigned char *buf, int length, int* transferred)
{
/*Perform synchronous bulk transfer from scope->
   handle = libusb device handle
   buf = buffer to store transfer data
   length = size of buffer
   transferred = number of bytes transferred. does not accept NULL pointer*/

    int err;
    
    err = libusb_bulk_transfer(scope->devh, 0x86, buf, length, transferred, 0);

    return err;
}
 
int scope_async_wait(scope_dev *scope)
{
/* Handle asynchronous transfer events. Blocking function, only returns on error. */

    int err;

    while(1)
    {
        err = libusb_handle_events(scope->ctx);
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

int scope_async_wait_completed(scope_dev *scope)
{
/* Handle asynchronous transfer events. Blocks until transfer event is completed. */

    int err;

    err = libusb_handle_events_completed(scope->ctx, NULL);
    /*if(err != 0)
    {
        fprintf(stderr, "libusb transfer event error: %d\n", err);
    }*/


    return err;
}
