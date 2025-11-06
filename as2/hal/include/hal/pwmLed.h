#pragma once

void PWM_init(void);
void PWM_setDutyCycle(double dutyPercent); // 0–100 %
void PWM_cleanup(void);

