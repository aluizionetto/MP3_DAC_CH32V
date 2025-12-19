/********************************** (C) COPYRIGHT *******************************
 Processamento de audio mp3 - decoder

 */

#include "debug.h"
#include <math.h>
#include "flash_spi.h"
#include "buffer_fifo.h"

#define MINIMP3_ONLY_MP3
// #define MINIMP3_ONLY_SIMD
#define MINIMP3_NO_SIMD  //garante compatibilidade com outras arquiteturas
// #define MINIMP3_NONSTANDARD_BUT_LOGICAL
// #define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#define MP3_MAX_SAMPLES_PER_FRAME (1152 * 2) // M¨¢ximo de amostras por frame (est¨¦reo)
__attribute__((aligned(4))) int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
#define N_buf (1152)
__attribute__((aligned(4))) uint8_t buf[N_buf]; // buffer de entrada
mp3dec_t mp3d;
__attribute__((aligned(4))) mp3dec_frame_info_t info={};
volatile int32_t nbuf;
volatile int32_t pread = 0;
volatile int32_t frame_size=0;
volatile int32_t flag_hb = 1, flag_cb = 1; //inicia com buffer liberados
volatile int32_t estado_mp3 = 0;
volatile uint32_t is,ks;
uint32_t flash_ok = 1;

#define N_BUF_AUDIO_SAMPLES (1280)
SBUFF audio_samples;
__attribute__((aligned(4))) int16_t ar_audio_samples[N_BUF_AUDIO_SAMPLES];

#define N_FLASH_CHECK (1024) //numero de bytes 0xFF para considerar fim de arquivo

/* Global typedef */

/* Global define */
//const u8 TEXT_Buf[] = {"CH32V305 SPI FLASH W25Qxx"};
//#define SIZE    sizeof(TEXT_Buf)



//#define Num (MINIMP3_MAX_SAMPLES_PER_FRAME/2) //reprodu??o de um canal - mono
#define Num (128) //reprodu??o de um canal - mono
uint16_t DAC_Value[Num];

/*********************************************************************
 * @fn      Dac_Init
 *
 * @brief   Initializes DAC collection.
 *
 * @return  none
 */
void Dac_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure={0};
    DAC_InitTypeDef DAC_InitType={0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE );
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE );

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    DAC_InitType.DAC_Trigger=DAC_Trigger_T4_TRGO;
    DAC_InitType.DAC_WaveGeneration=DAC_WaveGeneration_None;
    DAC_InitType.DAC_OutputBuffer=DAC_OutputBuffer_Enable ;
    DAC_Init(DAC_Channel_2,&DAC_InitType);

    DAC_Cmd(DAC_Channel_2, ENABLE);
    DAC_DMACmd(DAC_Channel_2,ENABLE);
}

/*********************************************************************
 * @fn      DAC1_DMA_INIT
 *
 * @brief   Initializes DMA of DAC1 collection.
 *
 * @return  none
 */
void Dac_Dma_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure={0};
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

    DMA_StructInit( &DMA_InitStructure);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(DAC->R12BDHR2);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&DAC_Value[0];
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = Num;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(DMA2_Channel4, &DMA_InitStructure);

    //habilita iterrup??es para transfer¨ºncia de DMA
    DMA_ITConfig(DMA2_Channel4,DMA_IT_TC,ENABLE);
    DMA_ITConfig(DMA2_Channel4,DMA_IT_HT,ENABLE);
    NVIC_EnableIRQ(DMA2_Channel4_IRQn);

    DMA_Cmd(DMA2_Channel4, ENABLE);
}

//interrup??o para DMA canal 3, Metade do buffer completo
void DMA2_Channel4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA2_Channel4_IRQHandler(void) {

    static uint8_t i =0;

    //interrup??o para metade do buffer
    if(DMA_GetITStatus(DMA2_IT_HT4)) {
        //preenche_buf_dac(0);
        flag_hb = 1;
        for(is = 0; is < Num/2; is++,ks) {
            get_sample_FIFO(&audio_samples, &DAC_Value[is]);
        }

        DMA_ClearITPendingBit(DMA2_IT_HT4);
        GPIO_WriteBit(GPIOC, GPIO_Pin_8, (i == 0) ? (i = Bit_SET) : (i = Bit_RESET));
    }

    if(DMA_GetITStatus(DMA2_IT_TC4)) {

        //preenche_buf_dac(1);
        flag_cb = 1;
        for(is = Num/2,ks =0; is < Num; is++,ks++) {
            get_sample_FIFO(&audio_samples, &DAC_Value[is]);
        }
        DMA_ClearITPendingBit(DMA2_IT_TC4);
        GPIO_WriteBit(GPIOC, GPIO_Pin_8, (i == 0) ? (i = Bit_SET) : (i = Bit_RESET));

    }

}






/*********************************************************************
 * @fn      Timer4_Init
 *
 * @brief   Initializes TIM4
 *
 * @return  none
 */
void Timer4_Init(int fsample)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure={0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = (SystemCoreClock/fsample)-1;
    TIM_TimeBaseStructure.TIM_Prescaler =0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update);
    TIM_Cmd(TIM4, ENABLE);
}


void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_WriteBit(GPIOC, GPIO_Pin_8 | GPIO_Pin_7, Bit_SET);

}



/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    u16 Flash_Model;
    uint32_t k_sample = 0;
    int16_t sam;

    //static uint8_t i;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n",SystemCoreClock);
    printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
    printf("Conversor dac para gerar sinal\r\n");

    GPIO_Toggle_INIT();
    Delay_Ms(200);
    Init_FIFO(&audio_samples, ar_audio_samples, sizeof(uint16_t) ,N_BUF_AUDIO_SAMPLES);

    SPI_Flash_Init();


    Flash_Model = SPI_Flash_ReadID();

    switch(Flash_Model)
    {
    case W25Q80:
        printf("W25Q80 OK!\r\n");

        break;

    case W25Q16:
        printf("W25Q16 OK!\r\n");

        break;

    case W25Q32:
        printf("W25Q32 OK!\r\n");

        break;

    case W25Q64:
        printf("W25Q64 OK!\r\n");

        break;

    case W25Q128:
        printf("W25Q128 OK!\r\n");

        break;

    default:
        flash_ok = 0;
        printf("Fail!\r\n");

        break;
    }


    Delay_Ms(500);
    if (flash_ok){
        printf("Start Read W25Qxx....\r\n");
        //SPI_Flash_Read((uint8_t*)&n_samples_flash, 0x00, 0x04); //
        //printf("N Samples: %d\r\n", n_samples_flash);
    }

    //printf("Start Erase W25Qxx....\r\n");
    //SPI_Flash_Erase_Sector(0);
    //printf("W25Qxx Erase Finished!\r\n");

    //Delay_Ms(500);
    //printf("Start Write W25Qxx....\r\n");
    //SPI_Flash_Write((u8 *)TEXT_Buf, 0, SIZE);
    //printf("W25Qxx Write Finished!\r\n");

    //Delay_Ms(500);
    //printf("Start Read W25Qxx....\r\n");
    //SPI_Flash_Read(datap, 0x0, SIZE);
    //printf("%s\r\n", datap);




    while(1)
    {


        mp3dec_init(&mp3d); //inicializa decodificador
        nbuf = 0;
        pread = 0;
        printf("Iniciando decodificacao...\r\n");

        do {
            printf("Flash p:%d s:%d  n:%d\r\n",pread,N_buf-nbuf,nbuf);
            SPI_Flash_Read(buf+nbuf, pread, N_buf-nbuf);
            pread += (N_buf-nbuf);//atualiza posicao para proxima leitura
            nbuf += (N_buf-nbuf); //bytes lidos
            //printf("decodifica n %d\r\n",nbuf);
            frame_size = mp3dec_decode_frame(&mp3d,buf, nbuf, pcm, &info);
            nbuf -= info.frame_bytes;

            //verfica se memoria flashchegou ao fim
            for(ks = 0; ks < N_FLASH_CHECK; ks++){
                if(buf[ks] != 0xff) break;
            }
            if(ks == N_FLASH_CHECK) break;


            // desloca amsotras remanescentes para inicio do buffer
            //printf("desloca amostras \r\n");
            memmove(buf, buf + info.frame_bytes, nbuf);

            //informacoes do frame
            printf("Frame size=%d, SRate=%d Hz, C=%d, Processado=%d bytes, bitrate %d kbps\r\n", frame_size,info.hz, info.channels, info.frame_bytes, info.bitrate_kbps);



            switch (estado_mp3) {
            case 0:
                if ((info.channels != 0) && (info.hz > 0)  && frame_size > 0) {
                    printf("Inicia repro \r\n");
                    for (k_sample = 0; k_sample < frame_size;k_sample++) {
                        sam = (((int32_t)pcm[k_sample*2] + (int32_t)pcm[k_sample*2+1]) >> 5)+2047;
						 if(sam > 4095) sam = 4095;
                         if(sam < 0) sam = 0;
                        put_sample_FIFO(&audio_samples, &sam);
                    }

                    //preenche buf 1/2
                    for(is = 0; is < Num/2; is++) {
                        get_sample_FIFO(&audio_samples, (void*)&DAC_Value[is]);
                    }
                    flag_hb = 0;

                    //preenche buf 2/2
                    for(is = Num/2; is < Num; is++) {
                        get_sample_FIFO(&audio_samples, (void *)&DAC_Value[is]);
                    }
                    flag_cb = 0;


                    Dac_Init();
                    Dac_Dma_Init();
                    Timer4_Init(info.hz);

                    estado_mp3 = 1;
                }
                break;
            case 1:
                if(frame_size > 0) {
                    while (n_sample_out_FIFO(&audio_samples) < frame_size);
                    for (k_sample = 0; k_sample < frame_size;k_sample++) {
                        sam = (((int32_t)pcm[k_sample*2] + (int32_t)pcm[k_sample*2+1]) >> 5)+2047;
						 if(sam > 4095) sam = 4095;
                         if(sam < 0) sam = 0;
                        put_sample_FIFO(&audio_samples, &sam);
                    }

                }
                else {
                    TIM_Cmd(TIM4, DISABLE);
                    DMA_Cmd(DMA2_Channel4, DISABLE);
                    estado_mp3= 0;
                }
                break;
            }



            //Delay_Ms(100);
        }while(info.frame_bytes);

    }
}

