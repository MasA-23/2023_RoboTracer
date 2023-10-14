//################################################
// ラズピコキット(Raspberry Pi Pico Kit) C-Version
// GYRO自作関数 (2023.1.14～)
//################################################
//#pragma once
//************************************************
// ピン定義
//************************************************
#define gyro_i2c i2c1
#define gyro_SCL 19
#define gyro_SDA 18

//************************************************
// マクロ定義
//************************************************
#define MPU6050_ADDRESS 0x68

//************************************************
// グローバル変数定義
//************************************************
//int16_t accelerometer[3], gyro[3] , temp;
int16_t gyro[3];
//************************************************
// 関数プロトタイプ宣言
//************************************************
static void MPU6050_Reset();
//static void MPU6050_ReadData(int16_t accelerometer[3], int16_t gyro[3] , int16_t *temp );
static void MPU6050_ReadData(int16_t gyro[3]);

//------------------------------------------------
// gyro初期化
//------------------------------------------------
static void MPU6050_Reset()
{
    uint8_t reg[] = {0x6B, 0x00};
    i2c_write_blocking(gyro_i2c, MPU6050_ADDRESS, reg, sizeof(reg), false );

    uint8_t leg[] = {0x1B, 0x18};
    i2c_write_blocking(gyro_i2c, MPU6050_ADDRESS,leg, sizeof(leg), false );
    
}

//------------------------------------------------
// gyro読み込み
//------------------------------------------------
static void MPU6050_ReadData( int16_t gyro[3])
{
    uint8_t buffer[6];

    // reading the accelerometer data
    uint8_t reg = 0x3B;
    /*
    i2c_write_blocking(gyro_i2c, MPU6050_ADDRESS, &reg, sizeof(reg), false );
    i2c_read_blocking(gyro_i2c, MPU6050_ADDRESS, buffer, sizeof(buffer), false );

    accelerometer[0] = (buffer[0] << 8) | buffer[1];
    accelerometer[1] = (buffer[2] << 8) | buffer[3];
    accelerometer[2] = (buffer[4] << 8) | buffer[5];
*/


    //Gyro data
    reg = 0x43;
    i2c_write_blocking(gyro_i2c, MPU6050_ADDRESS, &reg, sizeof(reg), false );
    i2c_read_blocking(gyro_i2c, MPU6050_ADDRESS, buffer, sizeof(buffer), false );

    //gyro[0] = (buffer[0] << 8) | buffer[1];
    //gyro[1] = (buffer[2] << 8) | buffer[3];
    gyro[2] = ( (buffer[4] << 8) | buffer[5] ); 

    

    // read the temperture data
    /*reg = 0x41;
    i2c_write_blocking(gyro_i2c, MPU6050_ADDRESS, &reg, sizeof(reg), true );
    i2c_read_blocking(gyro_i2c, MPU6050_ADDRESS, buffer, sizeof(buffer), false );
    
    *temp = (buffer[0] << 8) | buffer[1];*/
    
}
