#ifndef DATA_PROCESSIGN_H_
#define DATA_PROCESSIGN_H_

int get_current_speed(void);
int get_current_accel(void);
int get_current_power(void);
int get_current_distance(void);
int get_current_wheel_size(void);
int get_radio_status(void);
void set_current_wheel_size(int size);
int get_current_weight(void);
void set_current_weight(int weigth);
void processing_task(void *params);
void data_processor_end_recording(void);
void data_processor_start_recording(void);

#endif
