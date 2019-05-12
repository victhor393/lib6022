#include <libusb-1.0/libusb.h>
#ifndef _lib6022_h
#define _lib6022_h
#ifdef __cplusplus
    extern "C"
    {
#endif

typedef struct scopedev scope_dev;
typedef void(*callback_function)(unsigned char *buf, int len, void *ctx);
extern int scope_init(scope_dev **scope, uint8_t dev_idx);
extern int scope_set_channels(scope_dev *scope, uint8_t nch);
extern int scope_set_sample_rate(scope_dev *scope, uint8_t r);
extern int scope_set_voltage_range(scope_dev *scope, uint8_t v, int sch);
extern int scope_start(scope_dev *scope);
extern int scope_stop(scope_dev *scope);
extern int scope_read_async(scope_dev *scope, int length, int num_xfrs, callback_function cb, void *ctx);
extern int scope_read_sync(scope_dev *scope, unsigned char *buf, int length, int* transferred);
extern int scope_async_wait(scope_dev *scope);
extern int scope_async_wait_completed(scope_dev *scope);
extern void test_function(void);

#ifdef __cplusplus
    }
#endif

#endif
