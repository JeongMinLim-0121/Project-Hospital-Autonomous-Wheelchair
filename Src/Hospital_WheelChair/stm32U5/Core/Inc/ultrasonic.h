#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

#include "main.h"
#include <stdint.h>

extern volatile uint8_t Ultrasonic_Done;
// main에서 바로 읽을 수 있게 Distance를 외부로 노출(원본 코드 스타일 유지)
extern volatile float Ultrasonic_Distance_cm;

// 초기화: 사용할 타이머/채널/트리거 핀만 등록
void Ultrasonic_Init(TIM_HandleTypeDef *htim_delay_us,
                     TIM_HandleTypeDef *htim_ic,
                     uint32_t ic_channel,
                     GPIO_TypeDef *trig_port,
                     uint16_t trig_pin);

// 10us 트리거 발생 + 캡처 시작(인터럽트 enable 포함)
void Ultrasonic_TriggerAndStart(void);

// (필요하면) 측정 시작만 따로
void Ultrasonic_StartCaptureIT(void);

// (필요하면) 트리거만 따로
void Ultrasonic_Trigger10us(void);

// 너가 쓰던 HAL_TIM_IC_CaptureCallback 로직을 모듈로 이동한 함수
void Ultrasonic_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);


#endif
