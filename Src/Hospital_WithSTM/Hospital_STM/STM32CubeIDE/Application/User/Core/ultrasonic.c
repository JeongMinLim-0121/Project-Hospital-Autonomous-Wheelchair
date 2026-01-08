#include "ultrasonic.h"

// ====== 너가 main.c에 두던 전역변수들 (그대로 이동) ======
static uint32_t IC_Val1 = 0;
static uint32_t IC_Val2 = 0;
static uint32_t Difference = 0;
static uint8_t Is_First_Captured = 0;

volatile float Ultrasonic_Distance_cm = 0.0f;
volatile uint8_t Ultrasonic_Done = 0;

// ====== 너 main에서 쓰던 자원 포인터들 ======
static TIM_HandleTypeDef *g_htim_delay = NULL;   // TIM2
static TIM_HandleTypeDef *g_htim_ic    = NULL;   // TIM4
static uint32_t           g_ic_channel = 0;      // TIM_CHANNEL_2

static GPIO_TypeDef      *g_trig_port  = NULL;
static uint16_t           g_trig_pin   = 0;




// ====== 너 main.c에 있던 delay_us 그대로 이동 ======
static void delay_us(uint16_t us)
{
    //__HAL_TIM_SET_COUNTER(g_htim_delay, 0);
    //HAL_TIM_Base_Start(g_htim_delay);
    //while (__HAL_TIM_GET_COUNTER(g_htim_delay) < us);
    //HAL_TIM_Base_Stop(g_htim_delay);
	HAL_Delay(1);
}

void Ultrasonic_Init(TIM_HandleTypeDef *htim_delay_us,
                     TIM_HandleTypeDef *htim_ic,
                     uint32_t ic_channel,
                     GPIO_TypeDef *trig_port,
                     uint16_t trig_pin)
{
    g_htim_delay  = htim_delay_us;   // TIM2
    g_htim_ic     = htim_ic;         // TIM4
    g_ic_channel  = ic_channel;      // TIM_CHANNEL_2
    g_trig_port   = trig_port;
    g_trig_pin    = trig_pin;

    // 상태 초기화
    IC_Val1 = 0;
    IC_Val2 = 0;
    Difference = 0;
    Is_First_Captured = 0;
    Ultrasonic_Distance_cm = 0.0f;
    Ultrasonic_Done = 0;

    // 안전 상태 초기화



    // 캡처 폴라리티 초기화: Rising
    __HAL_TIM_SET_CAPTUREPOLARITY(g_htim_ic, g_ic_channel, TIM_INPUTCHANNELPOLARITY_RISING);
}

void Ultrasonic_Trigger10us(void)
{
    HAL_GPIO_WritePin(g_trig_port, g_trig_pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(g_trig_port, g_trig_pin, GPIO_PIN_RESET);

}

void Ultrasonic_StartCaptureIT(void)
{
    // 너 main에서 하던 순서 그대로:
    Ultrasonic_Done = 0;
    Is_First_Captured = 0;  // ✅ 첫 캡처 상태 리셋
    __HAL_TIM_SET_COUNTER(g_htim_ic, 0);  // ✅ 타이머 카운터 리셋
    __HAL_TIM_SET_CAPTUREPOLARITY(g_htim_ic, g_ic_channel, TIM_INPUTCHANNELPOLARITY_RISING); // ✅ 상승엣지부터
    // 너는 CC2 고정이었음 (TIM_CHANNEL_2)
    __HAL_TIM_ENABLE_IT(g_htim_ic, TIM_IT_CC2);
    HAL_TIM_IC_Start_IT(g_htim_ic, g_ic_channel);
}

void Ultrasonic_TriggerAndStart(void)
{
	Ultrasonic_StartCaptureIT();
    Ultrasonic_Trigger10us();
}

// ====== 너가 main.c에 있던 HAL_TIM_IC_CaptureCallback 내용 그대로 ======
void Ultrasonic_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    // 너 코드: TIM4만 처리
    if (htim->Instance == TIM4)
    {
        if (Is_First_Captured == 0) // 상승 엣지 (에코 시작)
        {
            IC_Val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
            Is_First_Captured = 1;

            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
        }
        else if (Is_First_Captured == 1) // 하강 엣지 (에코 끝)
        {
            IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

            if (IC_Val2 > IC_Val1) Difference = IC_Val2 - IC_Val1;
            else Difference = (65535 - IC_Val1) + IC_Val2;

            Ultrasonic_Distance_cm = (float)Difference / 58.0f;

            Is_First_Captured = 0;

            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
            Ultrasonic_Done = 1;
            __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC2);
        }
    }
}





