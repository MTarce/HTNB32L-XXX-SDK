/*=========================================
                PROJETO FINAL
===========================================*/

/*=========================================
1 - Leitura de luminosidade: LDR conectado a uma entrada analógica(ADC).

2 - LED 1: Acende quando o valor da luminosidade lida pelo LDR ficar
abaixo de um limite definido pelo aluno.

3 - LED 2: Deve piscar constantemente. A frequência do piscar é definida
dinamicamente via UART.

4 - Botão: Deve ser realizada a contagem de tempo que o botão
permanece pressionado. Esse tempo deve ser armazenado em uma fila
para acionar o LED 3.

5 - LED 3: Deve permanecer aceso pela mesma quantidade de tempo que
que o botão foi pressionado, em seguida, deve permanecer apagado
por pelo menos 500ms antes de acionar novamente.
===========================================*/

#include "hal_adc.h"
#include "HT_adc_qcx212.h"
#include "adc_qcx212.h"
#include "stdint.h"

#include "main.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <string.h>
#include "timer_qcx212.h"

#include "slpman_qcx212.h"
#include "pad_qcx212.h"
#include "HT_gpio_qcx212.h"
#include "ic_qcx212.h"
#include "HT_ic_qcx212.h"

#include <stdio.h>
#include "string.h"
#include "HT_bsp.h"
#include "htnb32lxxx_hal_usart.h"

#include <stdlib.h>

// GPIO10 - BUTTON3
#define BUTTON_INSTANCE 0               /**</ Button pin instance. */
#define BUTTON_PIN 10                   /**</ Button pin number. */
#define BUTTON_PAD_ID 25                /**</ Button Pad ID. */
#define BUTTON_PAD_ALT_FUNC PAD_MuxAlt0 /**</ Button pin alternate function. */

// GPIO1 - LED_01
#define LED_INSTANCE_01 0            /**</ LED pin instance. */
#define LED_GPIO_PIN_01 3            /**</ LED pin number. */
#define LED_PAD_ID_01 14             /**</ LED Pad ID. */
#define LED_PAD_ALT_FUNC PAD_MuxAlt0 /**</ LED pin alternate function. */

#define LED_ON_01 1  /**</ LED on. */
#define LED_OFF_01 0 /**</ LED off. */

// GPIO4 - LED_02
#define LED_INSTANCE_02 0            /**</ LED pin instance. */
#define LED_GPIO_PIN_02 4            /**</ LED pin number. */
#define LED_PAD_ID_02 15             /**</ LED Pad ID. */
#define LED_PAD_ALT_FUNC PAD_MuxAlt0 /**</ LED pin alternate function. */

#define LED_ON_02 1
#define LED_OFF_02 0

// GPIO5 - LED_03
#define LED_INSTANCE_03 0            /**</ LED pin instance. */
#define LED_GPIO_PIN_03 5            /**</ LED pin number. */
#define LED_PAD_ID_03 16             /**</ LED Pad ID. */
#define LED_PAD_ALT_FUNC PAD_MuxAlt0 /**</ LED pin alternate function. */

#define LED_ON_03 1  /**</ LED on. */
#define LED_OFF_03 0 /**</ LED off. */

volatile uint32_t timmer = 0;
QueueHandle_t filaTempo;

#define USART_BUFFER_SIZE 10

static uint32_t freqBlink;

volatile uint8_t timerInterrupt1 = 0;

volatile uint8_t rx_callback = 0;
volatile uint8_t tx_callback = 0;

extern uint8_t *usart_tx_buffer;
extern uint8_t *usart_rx_buffer;
extern uint32_t usart_tx_buffer_size;
extern uint32_t usart_rx_buffer_size;

static volatile uint32_t callback = 0;
static volatile uint32_t user_adc_channel = 0;
static volatile uint32_t vbat_result = 0;
static volatile uint32_t thermal_result = 0;

static adc_config_t adcConfig;

static uint8_t rx_buffer[USART_BUFFER_SIZE] = {0};
uint8_t controlByte[1] = {0};

#define DEMO_ADC_CHANNEL ADC_ChannelAio2 /**</ ADC channel. */

extern USART_HandleTypeDef huart1;

uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE |
                       ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

/*===================================
Protótipos_TEMPO_BTN_LED_03
=====================================*/
static void HT_Timer_Init(int seconds);
static void HT_Timer0_ISR(void);
static void HT_GPIO_InitButton(void);
static void HT_GPIO_InitLed_03(void);
/*===================================
Protótipos UART_LED
=====================================*/
void leituraFreq(void *pv);
void ledpisca(void *pv);

void vTimeBtn(void *pvParameters);
void vTimeLed(void *pvParameters);

/*===================================
Protótipos LED_LDR
=====================================*/

static void HT_ADC_ConversionCallback(uint32_t result);

static uint32_t HT_ADC_GetVoltageValue(uint32_t ad_value);

static void HT_ADC_Init(uint8_t channel);

void VledLDR(void *pvParameters);

static void HT_ADC_ConversionCallback(uint32_t result)
{
    callback |= DEMO_ADC_CHANNEL;
    user_adc_channel = result;
}

static uint32_t HT_ADC_GetVoltageValue(uint32_t ad_value)
{
    uint32_t value;
    value = HAL_ADC_CalibrateRawCode(ad_value);
    return (uint32_t)(value * 16 / 3);
}

// passa 01 - inicializa o periferico
static void HT_ADC_Init(uint8_t channel)
{
    ADC_GetDefaultConfig(&adcConfig);

    adcConfig.channelConfig.aioResDiv = ADC_AioResDivRatio3Over16;

    ADC_ChannelInit(channel, ADC_UserAPP, &adcConfig, HT_ADC_ConversionCallback);
}

/*===================================
TASK DE LEITURA PARA UART_LED02
=====================================*/
void leituraFreq(void *pvParameters)
{
    while (1)
    {
        printf("Freq led 1 >>\n");
        uint32_t idx = 0;
        memset(rx_buffer, 0, sizeof(rx_buffer));

        while (1)
        {
            HAL_USART_ReceivePolling(&huart1, controlByte, 1);

            if (controlByte[0] == '\n')
            {
                break;
            }

            if (idx < USART_BUFFER_SIZE - 1)
            { // Evita ultrapassar os limites do buffer
                rx_buffer[idx++] = controlByte[0];
            }
        }
        rx_buffer[idx] = '\0';
        printf("Valor de Freq ->: %s\n", (char *)rx_buffer);

        int freq = atoi((char *)rx_buffer);
        printf("Valor convertido: %d\n", freq);
        freqBlink = freq;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*===================================
TASK DE PISCA PARA UART_LED02
=====================================*/

void ledpisca(void *pvParameters)
{
    bool flagLed = true;

    // Aguardar freqBlink ser definido
    while (freqBlink == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (1)
    {
        HT_GPIO_WritePin(LED_GPIO_PIN_02, LED_INSTANCE_02, flagLed);
        printf("Piscando...\n");
        flagLed = !flagLed;
        vTaskDelay(pdMS_TO_TICKS(freqBlink));
    }
}

static void HT_Timer0_ISR(void)
{
    timerInterrupt1 = 1;
    timmer++;
}

static void HT_Timer_Init(int seconds)
{

    timer_config_t timerConfig;

    // Config TIMER clock, source from 32.768KHz and divide by 1
    CLOCK_SetClockSrc(GPR_TIMER0FuncClk, GPR_TIMER0ClkSel_32K);
    CLOCK_SetClockDiv(GPR_TIMER0FuncClk, 1);

    TIMER_DriverInit();

    // Config timer period as 1s, match0 value is 32768 = 0x8000
    TIMER_GetDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_ReloadOnMatch0;
    timerConfig.match0 = 0x20 * seconds;

    TIMER_Init(0, &timerConfig);

    // Note interrupt flag won't assert in TIMER_InterruptPulse mode
    TIMER_InterruptConfig(0, TIMER_Match0Select, TIMER_InterruptPulse);
    TIMER_InterruptConfig(0, TIMER_Match1Select, TIMER_InterruptDisabled);
    TIMER_InterruptConfig(0, TIMER_Match2Select, TIMER_InterruptDisabled);

    // Enable IRQ
    XIC_SetVector(PXIC_Timer0_IRQn, HT_Timer0_ISR);
    XIC_EnableIRQ(PXIC_Timer0_IRQn);
}

static void HT_GPIO_InitButton(void)
{
    GPIO_InitType GPIO_InitStruct = {0};

    GPIO_InitStruct.af = PAD_MuxAlt0;
    GPIO_InitStruct.pad_id = BUTTON_PAD_ID;
    GPIO_InitStruct.gpio_pin = BUTTON_PIN;
    GPIO_InitStruct.pin_direction = GPIO_DirectionInput;
    GPIO_InitStruct.pull = PAD_InternalPullUp;
    GPIO_InitStruct.instance = BUTTON_INSTANCE;
    GPIO_InitStruct.exti = GPIO_EXTI_ENABLE;
    // GPIO_InitStruct.interrupt_config = GPIO_InterruptFallingEdge;

    HT_GPIO_Init(&GPIO_InitStruct);
}

/*===================================
INICIANDO PARAMETROS DO LED1
=====================================*/

static void HT_GPIO_InitLed_01(void)
{
    GPIO_InitType GPIO_InitStruct = {0};

    GPIO_InitStruct.af = PAD_MuxAlt0;
    GPIO_InitStruct.pad_id = LED_PAD_ID_01;
    GPIO_InitStruct.gpio_pin = LED_GPIO_PIN_01;
    GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
    GPIO_InitStruct.init_output = 0;
    GPIO_InitStruct.pull = PAD_AutoPull;
    GPIO_InitStruct.instance = LED_INSTANCE_01;
    GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_InitStruct);
}

/*===================================
INICIANDO PARAMETROS DO LED2
=====================================*/

static void HT_GPIO_InitLed_02(void)
{
    GPIO_InitType GPIO_InitStruct = {0};

    GPIO_InitStruct.af = PAD_MuxAlt0;
    GPIO_InitStruct.pad_id = LED_PAD_ID_02;
    GPIO_InitStruct.gpio_pin = LED_GPIO_PIN_02;
    GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
    GPIO_InitStruct.init_output = 0;
    GPIO_InitStruct.pull = PAD_AutoPull;
    GPIO_InitStruct.instance = LED_INSTANCE_02;
    GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_InitStruct);
}

/*===================================
INICIANDO PARAMETROS DO LED3
=====================================*/

static void HT_GPIO_InitLed_03(void)
{
    GPIO_InitType GPIO_InitStruct = {0};

    GPIO_InitStruct.af = PAD_MuxAlt0;
    GPIO_InitStruct.pad_id = LED_PAD_ID_03;
    GPIO_InitStruct.gpio_pin = LED_GPIO_PIN_03;
    GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
    GPIO_InitStruct.init_output = 0;
    GPIO_InitStruct.pull = PAD_AutoPull;
    GPIO_InitStruct.instance = LED_INSTANCE_03;
    GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_InitStruct);
}

void vTimeBtn(void *pvParameters)
{
    bool estadoAnterior = 1; // Botão solto (puxado para cima por pull-up)
    bool estadoAtual;
    uint32_t tempoPressionado = 0;

    while (1)
    {
        estadoAtual = HT_GPIO_PinRead(BUTTON_INSTANCE, BUTTON_PIN);
        if (estadoAnterior == 1 && estadoAtual == 0)
        {
            TIMER_Start(0); // Inicia o timer 0

            // printf("tempoinicio %lu ms\n", tempoInicio);
        }

        if (estadoAnterior == 0 && estadoAtual == 1)
        {
            TIMER_Stop(0); // Para o timer

            // printf("tempoFim %lu | %lu ms\n", tempoFim,timmer);

            // Cálculo do tempo pressionado (em ms)
            if (timmer > 0)
            {
                // 1 tick = 1 / 32768 s → ms = ticks * 1000 / 32768
                tempoPressionado = timmer;

                // Envia para a fila
                xQueueSend(filaTempo, &tempoPressionado, portMAX_DELAY);
                printf("Botão pressionado por %lu ms\n", tempoPressionado);
                timmer = 0;
            }
        }
        estadoAnterior = estadoAtual;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vTimeLed(void *pvParameters)
{
    uint32_t ledTime = 0;
    while (1)
    {
        if (xQueueReceive(filaTempo, &ledTime, portMAX_DELAY))
        {
            printf("Acionando LED3 por %d segundos\n", ledTime);
            HT_GPIO_WritePin(LED_GPIO_PIN_03, LED_INSTANCE_03, LED_OFF_03);
            vTaskDelay(pdMS_TO_TICKS(ledTime));
            HT_GPIO_WritePin(LED_GPIO_PIN_03, LED_INSTANCE_03, LED_ON_03);
            vTaskDelay(pdMS_TO_TICKS(ledTime));
        }
    }
}

void VledLDR(void *pvParameters)
{
    while (1)
    {
        callback = 0;

        HT_ADC_StartConversion(DEMO_ADC_CHANNEL, ADC_UserAPP);

        while (callback != (DEMO_ADC_CHANNEL));

        uint32_t source = HT_ADC_GetVoltageValue(user_adc_channel);
        printf("ADC Value: %umv\n", source);

        if (source <= 450)
        {
            HT_GPIO_WritePin(LED_GPIO_PIN_01, LED_INSTANCE_01, LED_OFF_01);
        }
        else
        {
            HT_GPIO_WritePin(LED_GPIO_PIN_01, LED_INSTANCE_01, LED_ON_01);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void main_entry(void)
{

    BSP_CommonInit();
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    HT_Timer_Init(1); // 1s
    HT_GPIO_InitButton();

    HT_GPIO_InitLed_01();
    HT_GPIO_InitLed_02();
    HT_GPIO_InitLed_03();

    HT_ADC_Init(DEMO_ADC_CHANNEL);

    slpManNormalIOVoltSet(IOVOLT_3_30V);

    filaTempo = xQueueCreate(10, sizeof(uint32_t));

    if (filaTempo == NULL)
    {
        printf("Erro ao criar fila\n");
        while (1)
            ;
    }
    // PARA TEMPO SEGURANDO BTN
    xTaskCreate(vTimeBtn, "TempoBtn", 128, NULL, 2, NULL);
    xTaskCreate(vTimeLed, "TempoLED3", 128, NULL, 1, NULL);

    // PARA UART E LED
    xTaskCreate(leituraFreq, "Leitura", 128, NULL, 1, NULL);
    xTaskCreate(ledpisca, "Pisca", 128, NULL, 3, NULL);

     xTaskCreate(VledLDR, "LDR", 128, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1);
}
