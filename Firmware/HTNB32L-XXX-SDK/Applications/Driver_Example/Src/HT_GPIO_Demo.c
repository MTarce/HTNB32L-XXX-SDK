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

#include "HT_GPIO_Demo.h"

static volatile uint8_t gpio_exti = 0;

/*!******************************************************************
 * \fn static void HT_GPIO_Callback(void)
 * \brief GPIO callback. Function that will be called after aa GPIO IRQn
 * be detected by the device.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_GPIO_Callback(void);

/*!******************************************************************
 * \fn static void HT_GPIO_InitButton(void)
 * \brief Initialize GPIO button.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_GPIO_InitButton(void);

/*!******************************************************************
 * \fn static void HT_GPIO_InitLed(void)
 * \brief Initialize GPIO LED.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_GPIO_InitLed(void);

static void HT_GPIO_Callback(void) {
  //Save current irq mask and diable whole port interrupts to get rid of interrupt overflow
  uint16_t portIrqMask = GPIO_SaveAndSetIRQMask(BUTTON_INSTANCE);

  if (HT_GPIO_GetInterruptFlags(BUTTON_INSTANCE) & (1 << BUTTON_PIN)) {
      gpio_exti ^= 1;
      HT_GPIO_ClearInterruptFlags(BUTTON_INSTANCE, 1 << BUTTON_PIN);
      delay_us(10000000);
  }

  HT_GPIO_RestoreIRQMask(BUTTON_INSTANCE, portIrqMask);
}

static void HT_GPIO_InitButton(void) {
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

static void HT_GPIO_InitLed(void) {
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

void HT_GPIO_App(void) {

  ht_printf("GPIO Example Start!\n");
  
  HT_GPIO_InitButton();
  HT_GPIO_InitLed();

  slpManNormalIOVoltSet(IOVOLT_3_30V);

  while(1) {
    int estado_push = HT_GPIO_PinRead(BUTTON_INSTANCE, BUTTON_PIN);

      if (estado_push ==1)
        HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_ON);
      else
        HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, LED_OFF);
      
  }
}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
