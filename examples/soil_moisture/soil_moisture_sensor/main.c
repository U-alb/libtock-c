#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <adc.h>
#include <timer.h>
#include <gpio.h>
#include <ipc.h>
#include <tock.h>

tock_timer_t timer;

struct sensor_client {
  int pid;
  uint8_t* buffer;
};

struct sensor_client clients[10];
int client_count = 0;

int reference_voltage;

uint32_t reading_count = 0;

static void ipc_callback(int pid, int len, int buf, __attribute__ ((unused)) void* ud) {
  uint8_t* buffer = (uint8_t*) buf;


  if (client_count == 10 || len < 4) return;

  clients[client_count].pid = pid;
  clients[client_count].buffer = buffer;
  client_count += 1;

}




static uint32_t take_measurement(int ref) {
  uint16_t samples[30];

  gpio_set(0);

  int err = adc_sample_buffer_sync(0, 25, samples, 30);
  if (err != RETURNCODE_SUCCESS) {
    printf("Error sampling ADC: %d\n", err);
    return -1;
  }

  // for (int i=0; i<30; i++) {
  //   printf("sample: %i\n", samples[i]);
  // }

  uint32_t total = 0;
  for (int i=0; i<30; i++) {
    total += samples[i];
  }

  uint32_t average = total / 30;
  uint32_t voltage_mv = (average * ref) / ((1<<16)-1);

  reading_count += 1;


  // vcc
// 2.546
// 1.350
//
// gpio
// 2.093
// 1.045
//
// -0.0954x + 199.71
// -(9542/10000)x + 1997

  // uint32_t soil = ((2480000 / voltage_mv) - 720) / 100;
  uint32_t soil = 1797 - ((8111 * voltage_mv) / 10000);

  printf("[Soil Moisture Sensor] Reading #%lu\n", reading_count);
  printf("  voltage %ld.%03ldV\n", voltage_mv / 1000, voltage_mv % 1000);
  printf("  soil: %lu.%lu%%\n\n", soil/10, soil%10);

  gpio_clear(0);

  return soil;
}

static void timer_upcall(__attribute__ ((unused)) int temp,
                        __attribute__ ((unused)) int unused,
                        __attribute__ ((unused)) int unused1,
                        __attribute__ ((unused))  void* ud) {
  uint32_t moisture_percent = take_measurement(reference_voltage);

  // Copy in to each IPC app's shared buffer.
  for (int i=0; i<client_count; i++) {
    uint32_t* moisture_buf = (uint32_t*) clients[i].buffer;
    moisture_buf[0] = moisture_percent;
    ipc_notify_client(clients[i].pid);
  }
}


int main(void) {
  int err;
  printf("[Soil Moisture] Sensor App\n");

  // Check if ADC driver exists.
  if (!adc_exists()) {
    printf("[Soil Moisture] No ADC driver!\n");
    return -1;
  }

 err = ipc_register_service_callback("soil_moisture_sensor", ipc_callback, NULL);
 if (err != RETURNCODE_SUCCESS) {
  printf("Could not register %i ?\n", err);
 }

  reference_voltage = adc_get_reference_voltage();
  if (reference_voltage > 0) {
    printf("ADC reference voltage %d.%03dV\n", reference_voltage / 1000, reference_voltage % 1000);
  } else {
    reference_voltage = 3300;
    printf("ADC no reference voltage, assuming 3.3V\n");
  }


  timer_every(5000,timer_upcall,NULL,&timer);

  while(1) yield();
}