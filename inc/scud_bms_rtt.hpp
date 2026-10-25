#pragma once
#include <rtdevice.h>
#include <rtthread.h>
#include <stdint.h>

struct ScudBmsSnapshot { rt_uint16_t voltage_mv; rt_int16_t current_ma; rt_int16_t temperature_c10; rt_uint8_t soc; rt_uint32_t protect_status; rt_uint8_t valid; };

class ScudBmsRtt {
public:
    ScudBmsRtt(const char *device_name, rt_uint32_t baudrate);
    rt_err_t start(void);
    rt_err_t poll_once(ScudBmsSnapshot *snapshot);
    const ScudBmsSnapshot &snapshot(void) const { return snapshot_; }
private:
    static rt_uint16_t crc16(const rt_uint8_t *, rt_size_t);
    rt_err_t query(rt_uint8_t, rt_uint8_t *, rt_size_t);
    rt_err_t read_frame(rt_uint8_t, rt_uint8_t *, rt_size_t);
    static void worker_entry(void *);
    void worker(void);
    const char *device_name_; rt_uint32_t baudrate_; rt_device_t serial_; rt_thread_t thread_; struct rt_mutex lock_; ScudBmsSnapshot snapshot_; volatile rt_bool_t running_;
};
