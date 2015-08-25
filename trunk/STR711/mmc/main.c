/*********************************************************************
 *
 * Code testing the basic functionality of STR711 (using Olimex H711)
 * The code accesses SD/MMC attached to BSPI1 and communicates with user on UART2
 *
 * (based on ST UART demo, using libraries by Roland Riegel)
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 *********************************************************************
 * FileName:    main.c
 * Depends:
 * Processor:   STR711
 *
 * Author               Date       Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Kubik                9/14/2010  code cleanup
 * Kubik                9/19/2010  Added secondary UART to LCD, SD works
 ********************************************************************/


#include <stdio.h>
#include "71x_lib.h"
#include "gpio.h"
#include "sd_raw.h"
#include "partition.h"
#include "fat.h"

//-------------------------------------------------------------------
// Defines

//--- UART definitions - specifies UART that is used as console
#define UART0_Rx_Pin (0x0001<<8)        /* TQFP 64: pin N° 63 , TQFP 144 pin N° 143 */
#define UART0_Tx_Pin (0x0001<<9)        /* TQFP 64: pin N° 64 , TQFP 144 pin N° 144 */

#define UART1_Rx_Pin (0x0001<<10)       /* TQFP 64: pin N° 1  , TQFP 144 pin N° 1 */
#define UART1_Tx_Pin (0x0001<<11)       /* TQFP 64: pin N° 2  , TQFP 144 pin N° 3 */

#define UART2_Rx_Pin (0x0001<<13)       /* TQFP 64: pin N° 5  , TQFP 144 pin N° 9 */
#define UART2_Tx_Pin (0x0001<<14)       /* TQFP 64: pin N° 6  , TQFP 144 pin N° 10 */

#define UART3_Rx_Pin (0x0001<<1)        /* TQFP 64: pin N° 52 , TQFP 144 pin N° 123 */
#define UART3_Tx_Pin (0x0001<<0)        /* TQFP 64: pin N° 53 , TQFP 144 pin N° 124 */

//--- Modify this to redirect console to another UART
#define Use_UART2

#ifdef Use_UART0
#define  UARTX  UART0
#define  UARTX_Rx_Pin  UART0_Rx_Pin
#define  UARTX_Tx_Pin  UART0_Tx_Pin
#define  UARTX_Periph  UART0_Periph
#endif /* Use_UART0 */

#ifdef Use_UART1
#define  UARTX  UART1
#define  UARTX_Rx_Pin  UART1_Rx_Pin
#define  UARTX_Tx_Pin  UART1_Tx_Pin
#define  UARTX_Periph  UART1_Periph
#endif /* Use_UART1 */

#ifdef Use_UART2
#define  UARTX  UART2
#define  UARTX_Rx_Pin  UART2_Rx_Pin
#define  UARTX_Tx_Pin  UART2_Tx_Pin
#define  UARTX_Periph  UART2_Periph
#endif /* Use_UART2 */

#ifdef Use_UART3
#define  UARTX  UART3
#define  UARTX_Rx_Pin  UART3_Rx_Pin
#define  UARTX_Tx_Pin  UART3_Tx_Pin
#define  UARTX_Periph  UART3_Periph
#endif /* Use_UART3 */

//--- Trigger - this just pulses P1.9 that's used as a trigger for logical analyzer
#define trigger() {u16 i; GPIO_BitWrite(GPIO1, 9, 1); for(i = 0; i < 32; i++); GPIO_BitWrite(GPIO1, 9, 0);}

//--- LED showing the activity of SD card
#define LED_OFF         GPIO_BitWrite(GPIO1, 8, 1);
#define LED_ON          GPIO_BitWrite(GPIO1, 8, 0);

//--- defined elsewhere at the moment
//#define SPI_CS_LOW()    GPIO_BitWrite(GPIO0, 12, 0)
//#define SPI_CS_HIGH()    {for (i=0; i<0x0008; i++); GPIO_BitWrite(GPIO0, 12, 1);}


//---------------------------------------------------------------------------
// Static variables
UART_TypeDef *uart_used;        // This points to UART that printf uses (UARTX for terminal, UART1 for LCD)

//---------------------------------------------------------------------------
// Local functions

//--- Defining this redirects printf to UARTx
int putchar(int ch) {
    u16 i;

    UART_ByteSend(uart_used, (u8 *) & ch);
    return ch;
}

//--- Printing basic info about the inserted SD card
uint8_t print_disk_info(const struct fat_fs_struct * fs) {
    if(!fs)
        return 0;

    struct sd_raw_info disk_info;

    if(!sd_raw_get_info(&disk_info))
        return 0;

    printf("manuf:  0x%2.2X\r\n", disk_info.manufacturer);
    printf("oem:    %s\r\n", disk_info.oem);
    printf("prod:   %s\r\n", disk_info.product);
    printf("rev:    %2.2X\r\n", disk_info.revision);
    printf("serial: 0x%8.8X\r\n", disk_info.serial);
    printf("date:   %d/%d\r\n", disk_info.manufacturing_month, disk_info.manufacturing_year);
    printf("size:   %dMB\r\n", disk_info.capacity / 1024 / 1024);
    printf("copy:   %d\r\n", disk_info.flag_copy);
    printf("wr.pr.: %d %d\r\n", disk_info.flag_write_protect_temp, disk_info.flag_write_protect);
    printf("format: %d\r\n", disk_info.format);
    // following takes a long time and does not work
//    printf("free:   %ld / %ld\r\n", fat_get_fs_free(fs), fat_get_fs_size(fs));

    return 1;
}


//---------------------------------------------------------------------------
// Local functions
int main(void) {
    u16 i, b;
    u32 MCLKval;
    u32 APB1CLKval;
    u32 APB2CLKval;

#ifdef DEBUG
    debug();
#endif

//--- RCCU peripheral configuration
    RCCU_Div2Config(DISABLE);
    RCCU_FCLKConfig(RCCU_DEFAULT);      // Configure FCLK = RCLK /1
//  RCCU_FCLKConfig ( RCCU_RCLK_2 );    // Can't work - for some reasons, it fails when APB1 clock differs from MCLK
    RCCU_PCLKConfig(RCCU_RCLK_8);       // Configure PCLK = RCLK /8
    RCCU_MCLKConfig(RCCU_DEFAULT);      // Configure MCLK clock for the CPU, RCCU_DEFAULT = RCLK /1
    RCCU_PLL1Config(RCCU_PLL1_Mul_12, RCCU_Div_1);      // Configure the PLL1 ( * 12 , / 1 )
    while(RCCU_FlagStatus(RCCU_PLL1_LOCK) == RESET);    // Wait PLL to lock
    RCCU_RCLKSourceConfig(RCCU_PLL1_Output);    // Select PLL1_Output as RCLK clock

    MCLKval = RCCU_FrequencyValue(RCCU_MCLK);
    APB1CLKval = RCCU_FrequencyValue(RCCU_FCLK);
    APB2CLKval = RCCU_FrequencyValue(RCCU_PCLK);
//
// At this step the MCLK = 48 MHz, APB1 clock = 48 MHz and APB2 = 6MHz
// with an external oscilator equal to 4MHz
//

//--- GPIO peripheral configuration

    // Configure trigger GPIO (P1.9)
    GPIO_BitWrite(GPIO1, 9, 0);
    GPIO_Config(GPIO1, 1 << 9, GPIO_OUT_PP);

    // Configure LED GPIO (P1.8, that's the green LED on Olimex H711)
    LED_OFF;
    GPIO_Config(GPIO1, 1 << 8, GPIO_OUT_PP);

    // Configure UART GPIO
    GPIO_Config(GPIO0, UARTX_Tx_Pin, GPIO_AF_PP);
    GPIO_Config(GPIO0, UARTX_Rx_Pin, GPIO_IN_TRI_CMOS);

    // Configure UART1 GPIO (used for display
    GPIO_Config(GPIO0, UART1_Tx_Pin, GPIO_AF_PP);
    GPIO_Config(GPIO0, UARTX_Rx_Pin, GPIO_IN_TRI_CMOS); // not used at the moment

//--- UARTx peripheral configuration - enable it, disable and reset FIFOs, disable loopback, config to 9600/-/8/1, enable RX
    UART_OnOffConfig(UARTX, ENABLE);
    UART_FifoConfig(UARTX, DISABLE);
    UART_FifoReset(UARTX, UART_RxFIFO);
    UART_FifoReset(UARTX, UART_TxFIFO);
    UART_LoopBackConfig(UARTX, DISABLE);
    UART_Config(UARTX, 115200, UART_NO_PARITY, UART_1_StopBits, UARTM_8D);
    UART_RxConfig(UARTX, ENABLE);

//--- UART1 peripheral configuration - enable it, disable and reset FIFOs, disable loopback, config to 9600/-/8/1, disable RX
    UART_OnOffConfig(UART1, ENABLE);
    UART_FifoConfig(UART1, DISABLE);
    UART_FifoReset(UART1, UART_RxFIFO);
    UART_FifoReset(UART1, UART_TxFIFO);
    UART_LoopBackConfig(UART1, DISABLE);
    UART_Config(UART1, 57600, UART_NO_PARITY, UART_1_StopBits, UARTM_8D);
    UART_RxConfig(UART1, DISABLE);

    uart_used = UARTX;                  // Select terminal as output device
    putchar(12);                        // Clear screen

//--- print basic info - mostly to identify what the damn H711 thing is running
    printf("T_MMC " __DATE__ "\r\n");

    printf("Freq: MCLK=%dMHz, APB1=%dMHz, APB2=%dMHz\r\n", MCLKval / 1000000L, APB1CLKval / 1000000L, APB2CLKval / 1000000L);

//--- BSPI1 configuration - first, configure the pins
    // Configure MOSI1, MISO1, and SCLK1 pins as Alternate function Push Pull - those are P0.4-6
    GPIO_Config(GPIO0, 0x0070, GPIO_AF_PP);

    // Configure nSS1 pin mode as Input Weak PU - this is extremely important, otherwise BSPI1 does not work!
    GPIO_Config(GPIO0, 0x0080, GPIO_IPUPD_WP);
    GPIO_BitWrite(GPIO0, 7, 1);         // and pull it up

//--- BSPI1 configuration - first, configure the pins
    // Configure MOSI0, MISO0, and SCLK0 pins as Alternate function Push Pull - those are P0.0-2
    GPIO_Config(GPIO0, 0x0007, GPIO_AF_PP);

    // Configure nSS0 pin mode as Input Weak PU
    GPIO_Config(GPIO0, 0x0008, GPIO_IPUPD_WP);
    GPIO_BitWrite(GPIO0, 3, 1);         // and pull it up

    // Now set card CS to inactive state and configure it as output - this is P0.12
    SPI_CS_HIGH();
    GPIO_Config(GPIO0, 1 << 12, GPIO_OUT_PP);

#if 0
// screw BSPI1!!!

//--- BSPI1 configuration - now, configure the BSPI1 itself
    BSPI_Init(BSPI1);
//    BSPI_ClockDividerConfig(BSPI1, 6);  // Configure Baud rate Frequency: ---> APB1/6
    BSPI_ClockDividerConfig(BSPI1, 240);      // We should start with slow clock but it's not necessary
    BSPI_Enable(BSPI1, ENABLE);
    BSPI_MasterEnable(BSPI1, ENABLE);
    BSPI_ClkActiveHigh(BSPI1, DISABLE); // Configure the clock to be active low
    BSPI_ClkFEdge(BSPI1, DISABLE);      // Enable capturing the first Data sample on the first edge of SCK
    BSPI_8bLEn(BSPI1, ENABLE);          // Set the word length to 8 bit
    BSPI_TrFifoDepth(BSPI1, 1);         // Configure the depth of transmit to 1 word/byte
#endif

//--- BSPI0 configuration - now, configure the BSPI0 itself
    BSPI_BSPI0Conf(ENABLE);              // Enable the BSPI0 interface
    BSPI_Init(BSPI0);
//    BSPI_ClockDividerConfig(BSPI0, 6);  // Configure Baud rate Frequency: ---> APB1/6
    BSPI_ClockDividerConfig(BSPI0, 240);      // We should start with slow clock but it's not necessary
    BSPI_Enable(BSPI0, ENABLE);
    BSPI_MasterEnable(BSPI0, ENABLE);
    BSPI_ClkActiveHigh(BSPI0, DISABLE); // Configure the clock to be active low
    BSPI_ClkFEdge(BSPI0, DISABLE);      // Enable capturing the first Data sample on the first edge of SCK
    BSPI_8bLEn(BSPI0, ENABLE);          // Set the word length to 8 bit
    BSPI_TrFifoDepth(BSPI0, 1);         // Configure the depth of transmit to 1 word/byte

//--- Following code detects the card type and displays basic info and card root directory

//    if(!sd_raw_init()) {
//        printf("MMC/SD initialization failed\r\n");
//        sd_raw_init();
//    }

    printf("MMC/SD initialization:");
    while(!sd_raw_init()) {
        printf(".");
    }
    printf("OK\r\n");

    printf("Switching to high speed\r\n");
    BSPI_ClockDividerConfig(BSPI0, 6);  // Configure Baud rate Frequency: ---> APB1/6

    /* open first partition */
    printf("Opening partition\r\n");
    struct partition_struct *partition = partition_open(sd_raw_read,
                                                        sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
                                                        sd_raw_write,
                                                        sd_raw_write_interval,
#else
                                                        0,
                                                        0,
#endif
                                                        0);

    if(!partition) {
        /* If the partition did not open, assume the storage device
         * is a "superfloppy", i.e. has no MBR.
         */
        partition = partition_open(sd_raw_read, sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
                                   sd_raw_write, sd_raw_write_interval,
#else
                                   0, 0,
#endif
                                   -1);
        if(!partition) {
            printf("opening partition failed\r\n");
        }
    }

    /* open file system */
    printf("Opening filesystem\r\n");
    struct fat_fs_struct *fs = fat_open(partition);

    if(!fs) {
        printf("opening filesystem failed\r\n");
        while(1);
    }

    /* open root directory */
    struct fat_dir_entry_struct directory;

    fat_get_dir_entry_of_path(fs, "/", &directory);

    struct fat_dir_struct *dd = fat_open_dir(fs, &directory);

    if(!dd) {
        printf("opening root directory failed\r\n");
        while(1);
    }

    /* print some card information as a boot message */
    printf("Disk info\r\n");
    print_disk_info(fs);

    /* print directory listing */
    printf("Directory listing\r\n");
    struct fat_dir_entry_struct dir_entry;

    while(fat_read_dir(dd, &dir_entry)) {
        uint8_t spaces = sizeof(dir_entry.long_name) - strlen(dir_entry.long_name) + 2; //was + 4

        printf("%s", dir_entry.long_name);
        putchar(dir_entry.attributes & FAT_ATTRIB_DIR ? '/' : ' ');
        while(spaces--)
            putchar(' ');
        printf("%d\r\n", dir_entry.file_size);
    }

    printf("--- end ---\r\n");
    while(1);

}
