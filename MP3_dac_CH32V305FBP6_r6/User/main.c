/********************************** (C) COPYRIGHT *******************************
 Processamento de audio mp3 - decoder

 */

#include "debug.h"
#include <math.h>
#include "buffer_fifo.h"
#include "pff/pff.h"
#define MINIMP3_ONLY_MP3
// #define MINIMP3_ONLY_SIMD
#define MINIMP3_NO_SIMD  // garante compatibilidade com outras arquiteturas
// #define MINIMP3_NONSTANDARD_BUT_LOGICAL
// #define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#define MP3_MAX_SAMPLES_PER_FRAME (1152 * 2)  // M¨¢ximo de amostras por frame (est¨¦reo)
__attribute__ ((aligned (4))) int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
#define N_buf (1152)
__attribute__ ((aligned (4))) uint8_t buf[N_buf];  // buffer de entrada
mp3dec_t mp3d;
__attribute__ ((aligned (4))) mp3dec_frame_info_t info = {};
volatile int32_t nbuf;
volatile int32_t pread = 0;
volatile int32_t frame_size = 0;
volatile int32_t flag_hb = 1, flag_cb = 1;  // inicia com buffer liberados
volatile int32_t estado_mp3 = 0;
volatile uint32_t is, ks;
uint32_t flash_ok = 1;

#define N_BUF_AUDIO_SAMPLES (1280)
SBUFF audio_samples;
__attribute__ ((aligned (4))) uint16_t ar_audio_samples[N_BUF_AUDIO_SAMPLES];

//SD
const char dir_raiz[] = "";
FATFS fatfs; /* File system object */
UINT br;
DIR dir;
FILINFO fno;


// #define Num (MINIMP3_MAX_SAMPLES_PER_FRAME/2) //reprodu??o de um canal - mono
#define Num (128)  // reprodu??o de um canal - mono
uint16_t DAC_Value[Num];

/*********************************************************************
 * @fn      Dac_Init
 *
 * @brief   Initializes DAC collection.
 *
 * @return  none
 */
void Dac_Init (void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    DAC_InitTypeDef DAC_InitType = {0};

    RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd (RCC_APB1Periph_DAC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (GPIOA, &GPIO_InitStructure);

    DAC_InitType.DAC_Trigger = DAC_Trigger_T4_TRGO;
    DAC_InitType.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitType.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
    DAC_Init (DAC_Channel_2, &DAC_InitType);

    DAC_Cmd (DAC_Channel_2, ENABLE);
    DAC_DMACmd (DAC_Channel_2, ENABLE);
}

/*********************************************************************
 * @fn      DAC1_DMA_INIT
 *
 * @brief   Initializes DMA of DAC1 collection.
 *
 * @return  none
 */
void Dac_Dma_Init (void) {
    DMA_InitTypeDef DMA_InitStructure = {0};
    RCC_AHBPeriphClockCmd (RCC_AHBPeriph_DMA2, ENABLE);

    DMA_StructInit (&DMA_InitStructure);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (DAC->R12BDHR2);
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

    DMA_Init (DMA2_Channel4, &DMA_InitStructure);

    // habilita iterrup??es para transfer¨ºncia de DMA
    DMA_ITConfig (DMA2_Channel4, DMA_IT_TC, ENABLE);
    DMA_ITConfig (DMA2_Channel4, DMA_IT_HT, ENABLE);
    NVIC_EnableIRQ (DMA2_Channel4_IRQn);

    DMA_Cmd (DMA2_Channel4, ENABLE);
}

// interrup??o para DMA canal 3, Metade do buffer completo
void DMA2_Channel4_IRQHandler (void) __attribute__ ((interrupt ("WCH-Interrupt-fast")));

void DMA2_Channel4_IRQHandler (void) {

    static uint8_t i = 0;

    // interrup??o para metade do buffer
    if (DMA_GetITStatus (DMA2_IT_HT4)) {
        // preenche_buf_dac(0);
        flag_hb = 1;
        for (is = 0; is < Num / 2; is++, ks) {
            get_sample_FIFO (&audio_samples, &DAC_Value[is]);
        }

        DMA_ClearITPendingBit (DMA2_IT_HT4);
        GPIO_WriteBit (GPIOC, GPIO_Pin_8, (i == 0) ? (i = Bit_SET) : (i = Bit_RESET));
    }

    if (DMA_GetITStatus (DMA2_IT_TC4)) {

        // preenche_buf_dac(1);
        flag_cb = 1;
        for (is = Num / 2, ks = 0; is < Num; is++, ks++) {
            get_sample_FIFO (&audio_samples, &DAC_Value[is]);
        }
        DMA_ClearITPendingBit (DMA2_IT_TC4);
        GPIO_WriteBit (GPIOC, GPIO_Pin_8, (i == 0) ? (i = Bit_SET) : (i = Bit_RESET));
    }
}

/*********************************************************************
 * @fn      Timer4_Init
 *
 * @brief   Initializes TIM4
 *
 * @return  none
 */
void Timer4_Init (int fsample) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM4, ENABLE);

    TIM_TimeBaseStructInit (&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = (SystemCoreClock / fsample) - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit (TIM4, &TIM_TimeBaseStructure);

    TIM_SelectOutputTrigger (TIM4, TIM_TRGOSource_Update);
    TIM_Cmd (TIM4, ENABLE);
}

void GPIO_Toggle_INIT (void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init (GPIOC, &GPIO_InitStructure);

    GPIO_WriteBit (GPIOC, GPIO_Pin_8 | GPIO_Pin_7, Bit_SET);


    //Entrada de bot?o para pular faixa
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init (GPIOC, &GPIO_InitStructure);

}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main (void) {
    uint32_t k_sample = 0;
    int16_t sam;
    UINT br;

    // static uint8_t i;
    NVIC_PriorityGroupConfig (NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init (115200);
    Delay_Ms (200);
    printf ("SystemClk:%d\r\n", SystemCoreClock);
    printf ("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
    printf ("Conversor dac para gerar sinal\r\n");

    GPIO_Toggle_INIT();
    Delay_Ms (100);
    Init_FIFO (&audio_samples, ar_audio_samples, sizeof (uint16_t), N_BUF_AUDIO_SAMPLES);

    printf ("Mounting volume.\n\r");
    FRESULT rc = pf_mount (&fatfs);
    if (rc) {
        printf ("Erro1 SD :%u\r\n", rc);
        while (1);
    }

    // printf ("Opening file \"%s\"\n\r", filename);
    // rc = pf_open (filename);
    // if (rc) {
    //     printf ("Erro2 SD :%u\r\n", rc);
    //     while (1);
    // }

    printf ("Dados do cartao SD\n\r");
    // diretorio raiz
    rc = pf_opendir (&dir, dir_raiz);
    if (rc) {
        printf ("Erro2 SD :%u\r\n", rc);
        while (1);
    }

#if 0
    printf ("Lista diretorio...\r\n");
    for (;;) {
        rc = pf_readdir (&dir, &fno); /* Read a directory item */
        if (rc || !fno.fname[0])
            break;                    /* Error or end of dir */
        if (fno.fattrib & AM_DIR)
            printf ("   <dir>  %s\r\n", fno.fname);
        else
            printf ("%8lu  %s\r\n", fno.fsize, fno.fname);
    }
    rc = pf_opendir (&dir, dir_raiz);
#endif

    while (1) {

        rc = pf_readdir (&dir, &fno);


        if (rc || !fno.fname[0]) {
            printf ("Retorna raiz %u %s\r\n", rc, fno.fname);
            rc = pf_opendir (&dir, dir_raiz);  // reeabre diret¨®rio raiz
            continue;                          // n?o encontrou arquivo
        }
        if (fno.fattrib & AM_DIR) {
            printf ("   <dir>  %s\r\n", fno.fname);
            continue;
        }

        printf ("%8lu  %s\r\n", fno.fsize, fno.fname);


        // Verificar se termina com .mp3 (compara??o case-sensitive)
        char *ext = strrchr (fno.fname, '.');
        if (ext && strcmp (ext, ".MP3") == 0) {
            printf ("Processando: %s\n", fno.fname);

            // Abrir arquivo
            if (pf_open (fno.fname) != FR_OK)
                continue;
        } else
            continue;


        mp3dec_init (&mp3d);  // inicializa decodificador
        nbuf = 0;
        pread = 0;
        printf ("Iniciando decodificacao...\r\n");

        do {
            printf ("File p:%d s:%d  n:%d \r\n", pread, N_buf - nbuf, nbuf);
            rc = pf_read (buf + nbuf, N_buf - nbuf, &br); /* Read a chunk of file */
            if (rc || !br) {                              /* Error or end of file */
                pread = 0;
                pf_lseek (0);                             // retorna para inicio do arquivo
                break;
            }
            pread += N_buf - nbuf;
            nbuf += (N_buf - nbuf);  // bytes lidos
            // printf("decodifica n %d\r\n",nbuf);
            frame_size = mp3dec_decode_frame (&mp3d, buf, nbuf, pcm, &info);
            nbuf -= info.frame_bytes;


            // desloca amsotras remanescentes para inicio do buffer
            // printf("desloca amostras \r\n");
            memmove (buf, buf + info.frame_bytes, nbuf);

            // informacoes do frame
            printf ("Fs=%d, SR=%d Hz, C=%d, P=%d bytes, br %d kbps\r\n", frame_size, info.hz, info.channels, info.frame_bytes, info.bitrate_kbps);


            switch (estado_mp3) {
            case 0:
                if ((info.channels != 0) && (info.hz > 0) && frame_size > 0) {
                    // printf ("Inicia repro \r\n");
                    for (k_sample = 0; k_sample < frame_size; k_sample++) {
                        sam = (((int32_t)pcm[k_sample * 2] + (int32_t)pcm[k_sample * 2 + 1]) >> 5) + 2047;
                        if(sam > 4095) sam = 4095;
                        if(sam < 0) sam = 0;
                        put_sample_FIFO (&audio_samples, &sam);
                    }

                    // preenche buf 1/2
                    for (is = 0; is < Num / 2; is++) {
                        get_sample_FIFO (&audio_samples, (void *)&DAC_Value[is]);
                    }
                    flag_hb = 0;

                    // preenche buf 2/2
                    for (is = Num / 2; is < Num; is++) {
                        get_sample_FIFO (&audio_samples, (void *)&DAC_Value[is]);
                    }
                    flag_cb = 0;


                    Dac_Init();
                    Dac_Dma_Init();
                    Timer4_Init (info.hz);

                    estado_mp3 = 1;
                }
                break;
            case 1:
                if (frame_size > 0) {
                    while (n_sample_out_FIFO (&audio_samples) < frame_size);
                    // stereo
                    if (info.channels == 2) {
                        for (k_sample = 0; k_sample < frame_size; k_sample++) {
                            sam = (((int32_t)pcm[k_sample * 2] + (int32_t)pcm[k_sample * 2 + 1]) >> 5) + 2047;
                            if(sam > 4095) sam = 4095;
                            if(sam < 0) sam = 0;
                            put_sample_FIFO (&audio_samples, &sam);
                        }
                    }
                    // mono
                    else {
                        for (k_sample = 0; k_sample < frame_size; k_sample++) {
                            sam = (pcm[k_sample] >> 4) + 2047;
                            if(sam > 4095) sam = 4095;
                            if(sam < 0) sam = 0;
                            put_sample_FIFO (&audio_samples, &sam);
                        }
                    }

                } else {
                    TIM_Cmd (TIM4, DISABLE);
                    DMA_Cmd (DMA2_Channel4, DISABLE);
                    estado_mp3 = 0;
                }
                break;
            }

            //verifca bot?o para pular faixa
            if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_6) == RESET){
                TIM_Cmd (TIM4, DISABLE);
                DMA_Cmd (DMA2_Channel4, DISABLE);
                Delay_Ms(10);
                while(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_6) == RESET);
                break;

            }


            // Delay_Ms(100);
        } while (info.frame_bytes);
    }
}
