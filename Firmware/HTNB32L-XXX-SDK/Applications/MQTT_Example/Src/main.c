/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#include "slpman_qcx212.h"
#include "pad_qcx212.h"
#include "HT_gpio_qcx212.h"
#include "ic_qcx212.h"
#include "HT_ic_qcx212.h"

/*===============================
Definindo Fila
=================================*/
#include "queue.h"

// GPIO10 - BUTTON
#define BUTTON_INSTANCE 0               /**</ Button pin instance. */
#define BUTTON_PIN 10                   /**</ Button pin number. */
#define BUTTON_PAD_ID 25                /**</ Button Pad ID. */
#define BUTTON_PAD_ALT_FUNC PAD_MuxAlt0 /**</ Button pin alternate function. */

// GPIO3 - LED
#define LED_INSTANCE 0               /**</ LED pin instance. */
#define LED_GPIO_PIN 3               /**</ LED pin number. */
#define LED_PAD_ID 14                /**</ LED Pad ID. */
#define LED_PAD_ALT_FUNC PAD_MuxAlt0 /**</ LED pin alternate function. */

#define LED_ON 1  /**</ LED on. */
#define LED_OFF 0 /**</ LED off. */

/*===============================================
PROTOTIPOS DAS FUNÇÕESS
================================================*/
static void HT_GPIO_Callback(void);
static void HT_GPIO_InitLed(void);
static void HT_GPIO_InitButton(void);

static volatile uint8_t gpio_exti = 0;

static void HT_GPIO_Callback(void)
{
  // Save current irq mask and diable whole port interrupts to get rid of interrupt overflow
  uint16_t portIrqMask = GPIO_SaveAndSetIRQMask(BUTTON_INSTANCE);

  if (HT_GPIO_GetInterruptFlags(BUTTON_INSTANCE) & (1 << BUTTON_PIN))
  {
    gpio_exti ^= 1;
    HT_GPIO_ClearInterruptFlags(BUTTON_INSTANCE, 1 << BUTTON_PIN);
    delay_us(10000000);
  }

  HT_GPIO_RestoreIRQMask(BUTTON_INSTANCE, portIrqMask);
}

// PARA O BOTÃO
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

  // Enable IRQ
  HT_XIC_SetVector(PXIC_Gpio_IRQn, HT_GPIO_Callback);
  HT_XIC_EnableIRQ(PXIC_Gpio_IRQn);
}

// PARA O LED

static void HT_GPIO_InitLed(void)
{
  GPIO_InitType GPIO_InitStruct = {0};

  GPIO_InitStruct.af = PAD_MuxAlt0;
  GPIO_InitStruct.pad_id = LED_PAD_ID;
  GPIO_InitStruct.gpio_pin = LED_GPIO_PIN;
  GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
  GPIO_InitStruct.init_output = 0;
  GPIO_InitStruct.pull = PAD_AutoPull;
  GPIO_InitStruct.instance = LED_INSTANCE;
  GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;

  HT_GPIO_Init(&GPIO_InitStruct);
}

static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE |
                              ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;

/*
void Task2(void *pvParameters)
{
  while (1)
  {
    printf("Controlando led...\n");
    if (estado_push == 1)
      HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_ON);
    else
      HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_OFF);

    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay de 1000ms
  }
}
*/

/*==============================================
Incluindo FILA
================================================*/
QueueHandle_t xfila;

void vTaskLeituraBtn(void *pv)
{
  bool estado_push;
  while (1)
  {
    estado_push = HT_GPIO_PinRead(BUTTON_INSTANCE, BUTTON_PIN);
    xQueueSend(xfila, &estado_push, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void vTaskestadoLed(void *pv)
{
  bool recebido;
  while (1)
  {
    xQueueReceive(xfila, &recebido, portMAX_DELAY);
    if (recebido == 1)
      HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_ON);
    else
      HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_OFF);
  }
}

void main_entry(void)
{

  HT_GPIO_InitButton();
  HT_GPIO_InitLed();

  slpManNormalIOVoltSet(IOVOLT_3_30V);

  HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
  printf("Exemplo FreeRTOS\n");

  xfila = xQueueCreate(10, sizeof(bool));
  if (xfila == NULL)
  {
    printf("Erro ao criar a fila\n");
    while (1)
      ;
  }

  xTaskCreate(vTaskLeituraBtn, "btn", 128, NULL, 2, NULL);
  xTaskCreate(vTaskestadoLed, "led", 128, NULL, 1, NULL);

  vTaskStartScheduler();

  printf("Nao deve chegar aqui.\n");

  while (1)
    ;
}

/******** HT Micron Semicondutores S.A **END OF FILE*/