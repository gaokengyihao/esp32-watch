#include "spi.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

spi_device_handle_t spi_handle;//设备句柄。在向设备发送传输事务时使用

/*atk_md0240屏 spi通信引脚初始化*/
void spi_gpio_init()
{
    /*硬件spi配置*/
    spi_bus_config_t spi_initstruct = {0};
    spi_initstruct.mosi_io_num = 11;
    spi_initstruct.miso_io_num = -1;
    spi_initstruct.sclk_io_num = 12;
    spi_initstruct.quadwp_io_num = -1;
    spi_initstruct.quadhd_io_num = -1;
    spi_initstruct.data4_io_num = -1;
    spi_initstruct.data5_io_num = -1;
    spi_initstruct.data6_io_num = -1;
    spi_initstruct.data7_io_num = -1;
    spi_initstruct.data_io_default_level = 0;
    spi_initstruct.max_transfer_sz = 240 * 40 * 2;//最大传输字节 //cpu为64字节 //dma可将数组全发
    spi_initstruct.dma_burst_size = 0;//默认每次搬32字节
    spi_initstruct.intr_flags = 0;//中断优先级
    spi_bus_initialize(SPI2_HOST, &spi_initstruct, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t spi_devicestruct = {0};
    spi_devicestruct.mode = 0;//spi模式
    spi_devicestruct.queue_size = 4;//最多事务排队数量
    spi_devicestruct.clock_speed_hz = 80*1000*1000;
    spi_devicestruct.spics_io_num = tft_spi_cs_bit;
    spi_bus_add_device(SPI2_HOST, &spi_devicestruct, &spi_handle);

    /*软件spi配置*/
    // gpio_config_t gpio_struct = {0};
    // gpio_struct.pin_bit_mask = (1ULL << tft_spi_cs_bit) | (1ULL << spi_sda_bit) | (1ULL << spi_clk_bit);
    // gpio_struct.mode = GPIO_MODE_OUTPUT;
    // gpio_config(&gpio_struct);

    // spi_cs_writebit(1);//cs上拉
    // spi_clk_writebit(0);//clk置0
}

void spi_sendbit(unsigned char bit)
{
    spi_sda_writebit(bit);
    spi_clk_writebit(1);
    esp_rom_delay_us(1);
    spi_clk_writebit(0);
    esp_rom_delay_us(1);
}

/*软件spi*/
// void spi_sendbyte(unsigned char byte)
// {
//     spi_cs_writebit(0);//片选

//     for (unsigned char i = 0; i < 8; i++)
//     {
//         if (byte & (0x80 >> i))
//         {
//             spi_sendbit(1);
//         }
//         else
//         {
//             spi_sendbit(0);
//         }
//     }
    
//     spi_cs_writebit(1);//片选结束
// }

/*硬件spi*/
void spi_sendbyte(unsigned char byte)
{
    spi_transaction_t spi_sendtruct = {0};
    spi_sendtruct.flags = SPI_TRANS_USE_TXDATA;
    spi_sendtruct.length = 8;
    spi_sendtruct.tx_data[0] = byte;
    
    spi_device_polling_transmit(spi_handle, &spi_sendtruct);
}

// void spi_sendbuf(uint8_t *sendbuf, uint16_t sendbuf_length)
// {
//     uint16_t length = sendbuf_length;//需发送的总字节数
//     uint16_t n;//当次发送字节数
//     uint8_t * p = sendbuf;//指向当前发送部分

//     while(length > 0)//发送完成后退出
//     {
//         n = (length > 64) ? 64 : length;//限制最大64字节

//         spi_transaction_t spi_sendtruct = {0};
//         spi_sendtruct.length = n * 8;
//         spi_sendtruct.tx_buffer = p;
//         spi_device_polling_transmit(spi_handle, &spi_sendtruct);

//         length -= n;//减去已发送字节
//         p += n;//指向上次发送部分
//     }
    
// }

/*硬件spi+dma*/
void spi_sendbuf(uint8_t *sendbuf, uint16_t sendbuf_length)
{
    spi_transaction_t spi_sendtruct = {0};
    spi_sendtruct.length = sendbuf_length * 8;
    spi_sendtruct.tx_buffer = sendbuf;
    spi_device_polling_transmit(spi_handle, &spi_sendtruct);
}
