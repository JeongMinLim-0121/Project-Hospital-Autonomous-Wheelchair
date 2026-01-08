#include "fsr.h"

static uint16_t fsr_adc_read(ADC_HandleTypeDef *hadc)
{
    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 10);
    uint16_t v = (uint16_t)HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);
    return v;
}

void FSR_Init(fsr_t *fsr, ADC_HandleTypeDef *hadc,
              uint16_t th_on, uint16_t th_off, float alpha)
{
    fsr->hadc = hadc;

    fsr->raw = 0;
    fsr->filtered = 0.0f;

    fsr->seat = FSR_SEAT_EMPTY;

    fsr->th_on = th_on;
    fsr->th_off = th_off;

    // alpha 범위 보호
    if (alpha < 0.01f) alpha = 0.01f;
    if (alpha > 1.0f)  alpha = 1.0f;
    fsr->alpha = alpha;
}

void FSR_SetThresholds(fsr_t *fsr, uint16_t th_on, uint16_t th_off)
{
    fsr->th_on = th_on;
    fsr->th_off = th_off;
}

void FSR_Update(fsr_t *fsr)
{
    // 1) read raw
    fsr->raw = fsr_adc_read(fsr->hadc);

    // 2) EMA filter
    if (fsr->filtered <= 0.01f) {
        // 초기 1회는 raw로 바로 세팅
        fsr->filtered = (float)fsr->raw;
    } else {
        fsr->filtered = (1.0f - fsr->alpha) * fsr->filtered + fsr->alpha * (float)fsr->raw;
    }

    // 3) seat 판단 (히스테리시스)
    uint16_t f = (uint16_t)(fsr->filtered + 0.5f);

    if (fsr->seat == FSR_SEAT_EMPTY) {
        if (f >= fsr->th_on)
            fsr->seat = FSR_SEAT_OCCUPIED;
    } else { // OCCUPIED
        if (f <= fsr->th_off)
            fsr->seat = FSR_SEAT_EMPTY;
    }
}
