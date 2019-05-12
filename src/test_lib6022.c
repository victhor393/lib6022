/* Quick and dirty example code for lib6022. Currently it has no support for initializing and closing the device, so for now this has to be done manually. No threading, so this uses the synchronous interface.
   Device settings are hardcoded! 
   Output consists of raw 8-bit samples. 
   2 channel sample files are in interleaved format: [ch1][ch2][ch1][...] */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "lib6022.h"

#define SAMPLE_RATE 0x01
#define N_CHANNELS 0x01
#define VOLTAGE_RANGE 0x0a
#define FILE_SIZE 8388608

static struct libusb_device_handle *devh = NULL;

int main(int argc, char *argv[])
{
    if(argc <= 1)
    {
        fprintf(stderr, "No file name specified, exiting.\n");
        fprintf(stderr, "Usage: test_lib6022 file\n");
        fprintf(stderr, "file: path to output file\n");
        return 0;
    }

    if(argc > 2)
    {
        fprintf(stderr, "Too many arguments, exiting.\n");
        fprintf(stderr, "Usage: test_lib6022 file\n");
        fprintf(stderr, "file: path to output file\n");
        return 0;
    }

    FILE *f;
    f = fopen(argv[1], "wb");
    if(f == NULL)
    {
        fprintf(stderr, "Failed to open output file.\n");
        return -1;
    }

    int err;
    int transf;
    int bufsize = FILE_SIZE;
    unsigned char *readbuf;
    readbuf = malloc(bufsize);
    if(readbuf == NULL)
    {
        fprintf(stderr, "Failed to allocate memory.\n");
    }

    err = libusb_init(NULL);
    if(err != 0)
    {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(err));
        return -1;
    }

    devh = libusb_open_device_with_vid_pid(NULL, 0x04b5, 0x6022); // Should probably read the device list instead...
    if(devh == NULL)
    {
        fprintf(stderr, "Error initializing oscilloscope. Maybe firmware isn't loaded, or permissions are incorrect?\n");
        return -1;
    }

    err = libusb_claim_interface(devh, 0); // Claim the interface before using it to avoid libusb complaining
    if(err != 0)
    {
        fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(err));
        return -1;
    }

    err = libusb_set_interface_alt_setting(devh, 0, 0); // Select bulk interface
    if(err != 0)
    {
        fprintf(stderr, "Error setting bulk interface: %s\n", libusb_error_name(err));
        return -1;
    }
    
    err = scope_set_channels(devh, N_CHANNELS); // Select single channel mode
    if(err < 0)
    {
        fprintf(stderr, "Error during channel selection: %d\n", err);
        return -1;
    }

    err = scope_set_sample_rate(devh, SAMPLE_RATE); // Set sample rate
    if(err < 0)
    {
        fprintf(stderr, "Error during set sample rate: %d\n", err);
        return -1;
    }

    err = scope_set_voltage_range(devh, VOLTAGE_RANGE, 1); // Set voltage gain on channel 1
    if(err < 0)
    {
        fprintf(stderr, "Error during set voltage range: %d\n", err);
        return -1;
    }

    err = scope_start(devh); // Start sampling
    if(err < 0)
    {
        fprintf(stderr, "Error starting sampling: %s\n", libusb_error_name(err));
        return -1;
    }

    sleep(3); // Wait for a little bit to avoid hanging
    err = scope_read_sync(devh, readbuf, bufsize, &transf); // Perform bulk transfer
    if(err != 0)
    {
        fprintf(stderr, "Error during bulk transfer: %d\n", err);
        return -1;
    }
    else
    {
        fprintf(stderr, "%d bytes transferred\n", transf);
    }
    fwrite(readbuf, sizeof(unsigned char), bufsize, f);

    err = scope_stop(devh); // Stop sampling
    if(err < 0)
    {
        fprintf(stderr, "Error stopping sampling: %s\n", libusb_error_name(err));
        return -1;
    }
    
    libusb_close(devh);
    libusb_exit(NULL);
    fclose(f);
    free(readbuf);
    return 0;
}
