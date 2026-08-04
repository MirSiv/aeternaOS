#ifndef SERIAL_H
#define SERIAL_H

void init_serial(void);
void klog(const char *str);
void klog_hex(uint64_t value);
void klog_dec(int64_t value);

#endif