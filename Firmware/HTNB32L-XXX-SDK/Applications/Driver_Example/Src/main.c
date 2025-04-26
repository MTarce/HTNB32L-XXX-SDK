
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

static HT_Peripheral peripheral_select = GPIO_EX;

char buff_size[513];

extern USART_HandleTypeDef huart1;

void main_entry(void) {
    uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | 
                                ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

    BSP_CommonInit();
    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
 
    switch(peripheral_select) {
        case SPI_EX:
            HT_SPI_App();
            break;
        case I2C_EX:
            HT_I2C_App();
            break;
        case GPIO_EX:
            HT_GPIO_App();
            break;
        case AON_EX:
            HT_AON_App();
            break;
        case ADC_EX:
            HT_ADC_App();
            break;
        case WDT_EX:
            HT_WDT_App();
            break;
        case USART_EX:
            HT_USART_App();
            break;
        default:
            break;
    }

    while(1);

}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
