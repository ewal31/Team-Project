#ifndef CONTROLLER_H_
#define CONTROLLER_H_

void init_button_gpio(void);
void controller_task(void *param);
void end_journey_request(int source);
char *get_current_entry(void);

#endif
