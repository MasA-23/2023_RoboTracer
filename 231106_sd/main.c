#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ff.h"
#include "diskio.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define UART_ID uart1
#define BAUD_RATE 115200

#define UART_TX_PIN 20
#define UART_RX_PIN 21


char  sbuff[64];
#define DEF_FATBUFF  1024
char  buff_fattest[ DEF_FATBUFF ];


int  fat_test_init( void )
{
    DSTATUS  ret;
    int  result = 0;

    ret = disk_initialize( 0 );
    if( ret & STA_NOINIT ) {
        result = -1;
    }

    return  result;
}


int  fat_test_read( char *buff, int bsize )
{
    FRESULT  ret;
    FATFS  fs;
    FIL  fil;
    UINT  rdsz ;

    ret = f_mount( &fs, "", 0 );
    if( ret != FR_OK ) {
        return  -1;
    }
    ret = f_open( &fil, "test.txt", FA_READ );
    if( ret != FR_OK ) {
        return  -2;
    }

    ret = f_read( &fil, buff, (UINT)bsize, &rdsz );
    if( ret != FR_OK ) {
        return  -3;
    }

    f_close( &fil );

    return  (int)rdsz;
}


int  fat_test_write( char *filename, char *buff, int size )
{
    FRESULT  ret;
    FATFS  fs;
    FIL  fil;
    UINT  wsize ;

    ret = f_mount( &fs, "", 1 );
    if( ret != FR_OK ) {
        return  -1;
    }
    ret = f_open( &fil, filename, FA_WRITE|FA_CREATE_ALWAYS );
    if( ret != FR_OK ) {
        return  -2;
    }

    ret = f_write( &fil, buff, (UINT)size, &wsize );
    if( ret != FR_OK ) {
        return  -3;
    }

    f_close( &fil );

    return  (int)wsize;
}


int  main( void )
{   
    stdio_init_all();

    int  ret;
    int  wsize;

    uart_init( UART_ID, BAUD_RATE );
    gpio_set_function( UART_TX_PIN, GPIO_FUNC_UART );
    gpio_set_function( UART_RX_PIN, GPIO_FUNC_UART );

    uart_puts( UART_ID, "\n----- START fatfs on pico -----\n" );
    printf("\n----- START fatfs on pico -----\n");

    ret = fat_test_init();
    if( ret != 0 ) {
        uart_puts( UART_ID, "fat_test_init()  ERROR!\n" );
        printf("fat_test_init()  ERROR!\n");
    }
    ret = fat_test_read( buff_fattest, DEF_FATBUFF );
    
    //for(int i=0;i<8;i++){
        buff_fattest[10]=0x01;
        buff_fattest[11]=0x02;
        buff_fattest[12]=0x03;
        buff_fattest[13]=0x04;
        buff_fattest[14]=0x05;
        buff_fattest[15]=0x06;
    //}
    
    sprintf( sbuff, "fat_test_read()  ret = %d\n", ret );
    uart_puts( UART_ID, sbuff );
    printf("fat_test_read()  ret = %d\n", ret);

    if( ret > 0 ) {
        wsize = ret;
        ret = fat_test_write( "w_test1.txt", buff_fattest, wsize );
        sprintf(sbuff, "fat_test_write()  ret = %d\n", ret );
        uart_puts( UART_ID, sbuff );

        ret = fat_test_write( "w_test2.txt", buff_fattest, wsize );
        sprintf(sbuff, "fat_test_write()  ret = %d\n", ret );
        uart_puts( UART_ID, sbuff );
    }
    buff_fattest[ 10 ] = 0xd;
    buff_fattest[ 11 ] = 0xa;
    buff_fattest[ 12 ] = 0x0;
    uart_puts( UART_ID, buff_fattest );

    return  0;
}
