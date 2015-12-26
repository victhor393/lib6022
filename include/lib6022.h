#ifndef _lib6022_h
#define _lib6022_h

extern int scope_set_channels(libusb_device_handle *handle, unsigned char nch);
extern int scope_set_sample_rate(libusb_device_handle *handle, unsigned char r);
extern int scope_set_voltage_range(libusb_device_handle *handle, unsigned char v, int sch);
extern int scope_start(libusb_device_handle *handle);
extern int scope_stop(libusb_device_handle *handle);
extern int scope_read_async(libusb_device_handle *handle, unsigned char *buf, int length, int num_xfrs);
extern int scope_read_sync(libusb_device_handle *handle, unsigned char *buf, int length, int* transferred);
extern int scope_async_wait(void);

#endif
