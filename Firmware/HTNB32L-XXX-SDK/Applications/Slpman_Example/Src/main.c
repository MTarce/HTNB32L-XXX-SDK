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


/* Function prototypes  ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn static void HT_BeforeHibCallback(void *pdata, slpManLpState state)
 * \brief Function that executes before the Hibernate state.
 *
 * \param[in]  void *pdata                  pointer to the data parameter.
 * \param[in]  slpManLpState state          slpman state.
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_BeforeHibCallback(void *pdata, slpManLpState state);

/*!******************************************************************
 * \fn static void HT_AfterHibCallback(void *pdata, slpManLpState state)
 * \brief Function that executes after the Hibernate state.
 * It is called in case the Hibernate vote fails.
 *
 * \param[in]  void *pdata                  pointer to the data parameter.
 * \param[in]  slpManLpState state          slpman state.
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_AfterHibCallback(void *pdata, slpManLpState state);

/*!******************************************************************
 * \fn static void HT_BeforeSlp1Callback(void *pdata, slpManLpState state)
 * \brief Function that executes before the Sleep1 state.
 *
 * \param[in]  void *pdata                  pointer to the data parameter.
 * \param[in]  slpManLpState state          slpman state.
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_BeforeSlp1Callback(void *pdata, slpManLpState state);

/*!******************************************************************
 * \fn static void HT_AfterSlp1Callback(void *pdata, slpManLpState state)
 * \brief Function that executes just after the Sleep 1 state. This functions
 * is called even if the Sleep 1 vote succeed and its objective is to restore 
 * the state before sleep.
 *
 * \param[in]  void *pdata                  pointer to the data parameter.
 * \param[in]  slpManLpState state          slpman state.
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_AfterSlp1Callback(void *pdata, slpManLpState state);

/*!******************************************************************
 * \fn static void HT_BeforeSlp2Callback(void *pdata, slpManLpState state)
 * \brief Function that executes before the Sleep2 state.
 *
 * \param[in]  void *pdata                  pointer to the data parameter.
 * \param[in]  slpManLpState state          slpman state.
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_BeforeSlp2Callback(void *pdata, slpManLpState state);

/*!******************************************************************
 * \fn static void HT_AfterSlp2Callback(void *pdata, slpManLpState state)
 * \brief Function that executes after the Sleep2 state.
 * It is called in case the Sleep2 vote fails.
 *
 * \param[in]  void *pdata                  pointer to the data parameter.
 * \param[in]  slpManLpState state          slpman state.
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_AfterSlp2Callback(void *pdata, slpManLpState state);

/*!******************************************************************
 * \fn static void HT_FsWrite(void)
 * \brief FS write function. Writes a buffer in the file system in order to
 * keep a variable value during deep sleep states.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_FsWrite(void);

/*!******************************************************************
 * \fn static void HT_FsRead(void)
 * \brief Reads the content stored in the file system just after waking up
 * from a deep sleep state.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none
 *******************************************************************/
static void HT_FsRead(void);

/* ---------------------------------------------------------------------------------------*/

__attribute__((__section__(".testData"))) uint32_t abc;

static StaticTask_t usrapp_task;
static uint8_t usrapp_task_stack[USRAPP_TASK_STACK_SIZE];
static FSM_State state;

uint8_t slpmanSlpHandler = 0xFF;
uint8_t deepslpTimerID = 0;
uint32_t fileContent = 0;

static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | 
                                ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;

static void HT_BeforeHibCallback(void *pdata, slpManLpState state) {
    uint32_t *p_param = (uint32_t *)pdata;
    abc = 1110;

    printf("Before Hibernate = %X\n",*p_param);

#if USE_AON_PIN == 1
    printf("\nAON pin latch enable\n");
    slpManAONIOLatchEn(AonIOLatch_Enable);
#endif

}

static void HT_AfterHibCallback(void *pdata, slpManLpState state) {
    printf("Try Hibernate Failed, Interrupt Pending. Only sleep failed this function will excute\n");
}

static void HT_BeforeSlp1Callback(void *pdata, slpManLpState state) {
    printf("Before Sleep1\n");

#if USE_AON_PIN == 1
	slpManAONIOLatchEn(AonIOLatch_Enable);
#endif
}

static void HT_AfterSlp1Callback(void *pdata, slpManLpState state) {
    printf("After Sleep1, no matter sleep success or not this function will excute\n");
}

static void HT_BeforeSlp2Callback(void *pdata, slpManLpState state) {
    printf("Before Sleep2\n");
	abc = 1110;
	
    printf("Before Sleep2-abc=%ld\n");

#if USE_AON_PIN == 1
	slpManAONIOLatchEn(AonIOLatch_Enable);
#endif

}

static void HT_AfterSlp2Callback(void *pdata, slpManLpState state) {
    printf("Sleep2 Failed, Interrupt Pending. Only sleep failed this function will excute\n");
}

static void HT_FsWrite(void) {
	OSAFILE fp = PNULL;
	uint32_t len;

    fileContent += 1;

	fp = OsaFopen("testFile", "wb");
	len = OsaFwrite(&fileContent, sizeof(fileContent), 1, fp);

	if(len == 1)
		printf("Write Success\n");

	OsaFclose(fp);
}

static void HT_FsRead(void) {
	OSAFILE fp = PNULL;
	uint32_t len;
	
	fp = OsaFopen("testFile", "r");
	len = OsaFread(&fileContent, sizeof(fileContent), 1, fp);
	
	if(len == 1)
    	printf("Read File Success\n");

	OsaFclose(fp);
}

static void HT_UserAppTask(void *arg) {
    uint32_t inParam = 0xAABBCCDD;
    uint32_t cnt;
    slpManSlpState_t slpstate;
    slpManWakeSrc_e wkpSrc = WAKEUP_FROM_POR;

    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    printf("\nSlpman Example!\n");

    appSetCFUN(0);
    appSetEcSIMSleepSync(1);

    slpManSetPmuSleepMode(true,SLP_STATE_MAX,false);

    slpManApplyPlatVoteHandle("SLPMAN",&slpmanSlpHandler);            // apply an app layer vote handle

    slpManRegisterUsrdefinedBackupCb(HT_BeforeHibCallback,&inParam,SLPMAN_HIBERNATE_STATE);
    slpManRegisterUsrdefinedRestoreCb(HT_AfterHibCallback,NULL,SLPMAN_HIBERNATE_STATE);

    slpManRegisterUsrdefinedBackupCb(HT_BeforeSlp1Callback,NULL,SLPMAN_SLEEP1_STATE);
    slpManRegisterUsrdefinedRestoreCb(HT_AfterSlp1Callback,NULL,SLPMAN_SLEEP1_STATE);

    slpManRegisterUsrdefinedBackupCb(HT_BeforeSlp2Callback,NULL,SLPMAN_SLEEP2_STATE);
    slpManRegisterUsrdefinedRestoreCb(HT_AfterSlp2Callback,NULL,SLPMAN_SLEEP2_STATE);

    slpstate = slpManGetLastSlpState();
    wkpSrc = slpManGetWakeupSrc();
    //Modificado
    printf("Wakeup Src: %d\n", wkpSrc);

    if((slpstate == SLP_SLP2_STATE) || (slpstate == SLP_HIB_STATE)) {    // wakeup from sleep 
		HT_FsRead();

        printf("Wakeup From state = %u\n", slpstate);
        printf("uint32_t abc = %d\n", abc);
        printf("File Read: 0x%08X\n", fileContent);

        HT_FsWrite();

        while(1) {               // now app can enter hib, but ps and phy maybe not, so wait here
            osDelay(3000);
        }
    
    } else {
		
        state = EXAMPLE_STEP_1; 

        while(1) {
            switch(state) {
                case EXAMPLE_STEP_1: {

					HT_FsWrite();
						
                    slpManPlatVoteDisableSleep(slpmanSlpHandler, SLP_STATE_MAX);  // sleep deeper than STATE_MAX(include) is prohibited

#if USE_DEEP_SLEEP_TIMER == 1
                    slpManDeepSlpTimerStart(deepslpTimerID, 10000);  //10s
#endif

                    HT_TRACE(UNILOG_PLA_APP, UserAppTask_1, P_SIG, 0, "Example Step1: Keep in Sleep1 state");

                    cnt = 0;

                    while(cnt<20)
                    {
                        cnt++;
                        osDelay(2000);
                    }

                    state = EXAMPLE_STEP_2;
                    break;
                }
                case EXAMPLE_STEP_2: {

                    slpManSlpState_t State;
                    uint8_t cnt;

                    if(slpManCheckVoteState(slpmanSlpHandler, &State, &cnt)==RET_TRUE) {
                        printf("We Can Check Vote State, state=%u, cnt=%u\n",State, cnt);
                    }

                    slpManPlatVoteEnableSleep(slpmanSlpHandler, SLP_STATE_MAX);  // cancel the prohibition of sleep2

                    if(slpManCheckVoteState(slpmanSlpHandler, &State, &cnt)==RET_TRUE) {
                        printf("We Can Check Vote State Again, state=%u, cnt=%u\n",State,cnt);
                    }

                    printf("Now App cancel the prohibition of sleep2\n");

                    slpManDeepSlpTimerStart(deepslpTimerID, 30000);     // create a 30s timer, DeepSleep Timer is always oneshoot

                    while(1) {                // now app can enter hib, but ps and phy maybe not, so wait here
                        osDelay(2000);
                    }
                }

                default: {
                    break;
                }
            }
        }

    }
}

static void HT_AppInit(void *arg) {
    osThreadAttr_t task_attr;

    if(BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_CONTROL) != 0) {
        HAL_UART_RecvFlowControl(false);
    }

    memset(&task_attr,0,sizeof(task_attr));
    memset(usrapp_task_stack, 0xA5,USRAPP_TASK_STACK_SIZE);
    task_attr.name = "Slpman_Example";
    task_attr.stack_mem = usrapp_task_stack;
    task_attr.stack_size = USRAPP_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &usrapp_task;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(HT_UserAppTask, NULL, &task_attr);
}

void main_entry(void) {

    BSP_CommonInit();
    osKernelInitialize();

    setvbuf(stdout,NULL,_IONBF,0);

    registerAppEntry(HT_AppInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }


    while(1);

}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
