#include "bsp_spi.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;
extern DMA_HandleTypeDef hdma_spi1_tx;

void SPI1_DMA_init(uint32_t tx_buf, uint32_t rx_buf, uint16_t num)
{
    SET_BIT(hspi1.Instance->CR2, SPI_CR2_TXDMAEN);
    SET_BIT(hspi1.Instance->CR2, SPI_CR2_RXDMAEN);

    __HAL_SPI_ENABLE(&hspi1);


    //disable DMA
    //失效DMA
    __HAL_DMA_DISABLE(&hdma_spi1_rx);
    
    while(hdma_spi1_rx.Instance->CCR & DMA_CCR_EN)
    {
        __HAL_DMA_DISABLE(&hdma_spi1_rx);
    }

    __HAL_DMA_CLEAR_FLAG(&hdma_spi1_rx, DMA_ISR_TCIF2);

    hdma_spi1_rx.Instance->CPAR = (uint32_t) & (SPI1->DR);
    //memory buffer 1
    //内存缓冲区1
    hdma_spi1_rx.Instance->CMAR = (uint32_t)(rx_buf);
    //data length
    //数据长度
		hdma_spi1_rx.Instance->CNDTR = num;

    __HAL_DMA_ENABLE_IT(&hdma_spi1_rx, DMA_IT_TC);


    //disable DMA
    //失效DMA
    __HAL_DMA_DISABLE(&hdma_spi1_tx);
    
    while(hdma_spi1_tx.Instance->CCR & DMA_CCR_EN)
    {
        __HAL_DMA_DISABLE(&hdma_spi1_tx);
    }


    __HAL_DMA_CLEAR_FLAG(&hdma_spi1_tx, DMA_ISR_TCIF3);

    hdma_spi1_tx.Instance->CPAR = (uint32_t) & (SPI1->DR);
    //memory buffer 1
    //内存缓冲区1
    hdma_spi1_tx.Instance->CMAR = (uint32_t)(tx_buf);
    //data length
    //数据长度
    hdma_spi1_rx.Instance->CNDTR = num;


}

void SPI1_DMA_enable(uint32_t tx_buf, uint32_t rx_buf, uint16_t ndtr)
{
    //disable DMA
    //失效DMA
    __HAL_DMA_DISABLE(&hdma_spi1_rx);
    __HAL_DMA_DISABLE(&hdma_spi1_tx);
    while(hdma_spi1_rx.Instance->CCR & DMA_CCR_EN)
    {
        __HAL_DMA_DISABLE(&hdma_spi1_rx);
    }
    while(hdma_spi1_tx.Instance->CCR & DMA_CCR_EN)
    {
        __HAL_DMA_DISABLE(&hdma_spi1_tx);
    }
    //clear flag
    //清除标志位
    __HAL_DMA_CLEAR_FLAG (hspi1.hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hspi1.hdmarx));
    __HAL_DMA_CLEAR_FLAG (hspi1.hdmarx, __HAL_DMA_GET_HT_FLAG_INDEX(hspi1.hdmarx));
    __HAL_DMA_CLEAR_FLAG (hspi1.hdmarx, __HAL_DMA_GET_TE_FLAG_INDEX(hspi1.hdmarx));
    __HAL_DMA_CLEAR_FLAG (hspi1.hdmarx, __HAL_DMA_GET_GI_FLAG_INDEX(hspi1.hdmarx));
   
		

    __HAL_DMA_CLEAR_FLAG (hspi1.hdmatx, __HAL_DMA_GET_TC_FLAG_INDEX(hspi1.hdmatx));
    __HAL_DMA_CLEAR_FLAG (hspi1.hdmatx, __HAL_DMA_GET_HT_FLAG_INDEX(hspi1.hdmatx));
    __HAL_DMA_CLEAR_FLAG (hspi1.hdmatx, __HAL_DMA_GET_TE_FLAG_INDEX(hspi1.hdmatx));
    __HAL_DMA_CLEAR_FLAG (hspi1.hdmatx, __HAL_DMA_GET_GI_FLAG_INDEX(hspi1.hdmatx));
		

    //set memory address
    //设置数据地址
    hdma_spi1_rx.Instance->CMAR = rx_buf;
    hdma_spi1_tx.Instance->CMAR = tx_buf;
    //set data length
    //设置数据长度
		hdma_spi1_rx.Instance->CNDTR = ndtr;
		hdma_spi1_tx.Instance->CNDTR = ndtr;
    //enable DMA
    //使能DMA
    __HAL_DMA_ENABLE(&hdma_spi1_rx);
    __HAL_DMA_ENABLE(&hdma_spi1_tx);
}




