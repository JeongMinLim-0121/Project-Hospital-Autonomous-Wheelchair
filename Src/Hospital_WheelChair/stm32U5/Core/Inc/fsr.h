#ifndef FSR_H
#define FSR_H

#include "main.h"
#include <stdint.h>

typedef enum {
    FSR_SEAT_EMPTY = 0,
    FSR_SEAT_OCCUPIED = 1
} fsr_seat_state_t;

typedef struct {
    // ADC
    ADC_HandleTypeDef *hadc;

    // Raw / filtered
    uint16_t raw;
    float    filtered;     // EMA result (0~4095 scale)

    // Seat state
    fsr_seat_state_t seat;

    // Thresholds (hysteresis)
    uint16_t th_on;        // empty -> occupied
    uint16_t th_off;       // occupied -> empty

    // EMA alpha (0~1). larger = follow faster
    float alpha;

} fsr_t;

// 초기화: hadc, 임계값, EMA alpha 지정
void FSR_Init(fsr_t *fsr, ADC_HandleTypeDef *hadc,
              uint16_t th_on, uint16_t th_off, float alpha);

// 한 번 업데이트: ADC 읽고 필터/판정까지
void FSR_Update(fsr_t *fsr);

// 상태 조회
static inline uint16_t FSR_GetRaw(const fsr_t *fsr) { return fsr->raw; }
static inline uint16_t FSR_GetFilteredU16(const fsr_t *fsr) { return (uint16_t)(fsr->filtered + 0.5f); }
static inline fsr_seat_state_t FSR_GetSeat(const fsr_t *fsr) { return fsr->seat; }

// 임계값 바꾸기
void FSR_SetThresholds(fsr_t *fsr, uint16_t th_on, uint16_t th_off);

#endif
