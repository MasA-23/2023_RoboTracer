//################################################
// 標準機改良プログラム
// 23/08/30~
//
//################################################
#include <stdio.h>                      // C言語用ヘッダ読込み
#include <stdlib.h>                     // C言語用ヘッダ読込み
#include <math.h>                       // C言語用ヘッダ読込み
#include "pico/stdlib.h"                // ラズピコ用ヘッダ読込み
#include "hardware/i2c.h"               // I2C通信用ヘッダ読込み
#include "hardware/pwm.h"               // PWM用ヘッダ読込み
#include "hardware/adc.h"               // A/D変換用ヘッダ読込み
#include "LCD_c.h"                      // LCD用自作関数ヘッダ読込み
#include "tone_c.h"                     // Beep用自作関数ヘッダ読込み

#include "pico/multicore.h"             // multicoreヘッダ読み込み
#include "hardware/spi.h"               //spi通信用ヘッダ読み込み
#include "mcp3208_c.h"                  //ADコンバータ
#include "gyro_c.h"                     //ジャイロ

//************************************************
// ピン定義
//************************************************
#define LED_PICO    25        // Pico上のLED : Active High

#define MOT_L_STEP       22     //左ステップ数取得ピン
#define MOT_R_STEP       17     //右ステップ数取得ピン

#define SW_L        9          // 左スイッチ   : Active Low
#define SW_C        13        // 中央スイッチ : Active Low
#define SW_R        14        // 右スイッチ   : Active Low

#define ADC0        26        // A/Dコンバータ : ADC0 : GPIO 26
#define ADC1        27        // A/Dコンバータ : ADC1 : GPIO 27
#define ADC2        28        // A/Dコンバータ : ADC2 : GPIO 28

#define SEN_R       1         // ADC0 : 右センサ
//#define SEN_F       1         // ADC2 : 前センサ
#define SEN_L       2         // ADC2 : 左センサ/電源電圧(共用)

#define MARKER_R    10        // P21 : 右マーカー（スタート・ゴールマーカー）
#define MARKER_L    16         // P22 : 左マーカー（コーナーマーカー）

#define MOT_EN      8         // モータ電源 ON/OFF
#define MOT_L_CLK   3         // 左モータ : パルス入力
#define MOT_L_DIR   4         // 左モータ : 方向指示
#define MOT_R_CLK   11        // 右モータ : パルス入力
#define MOT_R_DIR   12        // 右モータ : 方向指示

//************************************************
// マクロ定義
//************************************************
// スイッチ関連
#define SW_ON       0         // SWは負論理なので0でON
#define SW_OFF      1         // SWは負論理なので1でOFF
#define SW_WAIT     300       // チャタリング防止用の待ち時間(ms)
// モード関連
#define MODE_MAX    6         // 動作モード数
#define DISP        0         // モード表示
#define EXEC        1         // モード実行
// センサ関連
#define SEN_ON      1         // センサ用LED点灯
#define SEN_OFF     0         // センサ用LED消灯
#define SEN_WAIT    5         // センサ発光時間調整
#define MARKER_ON   0         // マーカーON
#define MARKER_OFF  1         // マーカーOFF
// モータ関連
#define MOT_ON      1         // モータ電源 ON
#define MOT_OFF     0         // モータ電源 OFF
#define MOT_L_FWD   1         // 左モータ前進
#define MOT_L_BACK  0         // 左モータ後進
#define MOT_R_FWD   0         // 右モータ前進
#define MOT_R_BACK  1         // 右モータ後進
//#define MOT_ACC     10        // モータ加速度
#define MOT_SPD_INIT    100   // モータ起動速度

// tera termモード関連
#define VALUE_MODE_MAX    6         // 動作モード数
#define VALUE_DISP        0         // モード表示
#define VALUE_EXEC        1         // モード実行
//************************************************
// グローバル変数定義
//************************************************
int tick_count = 0;           // 1msカウンター用変数
int Mode = 0;                 // 現在モード格納用
int value_Mode = 0;           // 現在モード格納用
int mot_r_slice;              // 右モータ スライス番号
int mot_l_slice;              // 左モータ スライス番号
int mot_r_channel;            // 右モータ チャンネル番号
int mot_l_channel;            // 左モータ チャンネル番号
int mot_r_freq;               // 右モータ 周波数用カウント数
int mot_l_freq;               // 左モータ 周波数用カウント数
int mot_r_duty;               // 右モータ duty用カウント数
int mot_l_duty;               // 左モータ duty用カウント数
float mot_spd_r = 0;            // 右モータ速度
float mot_spd_l = 0;            // 左モータ速度
int target_spd = 0;           // モータ目標速度
int target_spd_straight = 0;
//int target_spd_moto=0;
// volatile : コンパイラの最適化を抑制するための命令
volatile float target_spd_r = 0;  // モータ目標速度　右
volatile float target_spd_l = 0;  // モータ目標速度　左

int sen_ref_f = 400;

float MOT_ACC=10;

//パラメータ選択用変数
int param=0;
int b=0;

//line sensorを使うフラグ
bool line_sensor_flg=false;

//ラインセンサ関連
volatile int sen_adc[10]={0,0,0,0,0,0,0,0,0};
volatile int sen_val[10]={0,0,0,0,0,0,0,0,0};
int rl[10]={0,0,0,0,0,0,0,0,0};
int rl_pre[10]={0,0,0,0,0,0,0,0,0};
bool out_r=false,out_l=false;
volatile int sen_val_c[10]={0,0,0,0,0,0,0,0,0,0};

//キャリブレーション
volatile int sen_val_min[10]={999,999,999,999,999,999,999,999,999,999};
volatile int sen_val_max[10]={0,0,0,0,0,0,0,0,0,0};

//ジャイロ
short gyro_offset_cnt=0;
float gyro_offset=0;
int gyro_offset_sum=0;

volatile float degree=0;
volatile float pregy=0;

float gyro_drift=0;
bool gyro_print_flg=false;
int tick_count_gyro=0;
int gyro_cnt=0;
bool gyro_flg=false;

//トレース中のフラグ
bool trace_flg=false;

//割り込み
int ms_count_for=0;

//トレース用
int trace_time_count=0;
int cross_count=0;
bool cross_flg=false;
bool cross_demo=false;
short trace_number=1;
int cross=0;

//制御
float dev=0;
float dev_pre=0;

float k2=0;
float k3=0;
float k4=0;
float k5=0;
float k5_x=0;
float k5_single=0;

float control=0;

float kp=0;
float kd=0;

//ステップ数
volatile int STEP_L=0;
volatile int STEP_R=0;

//マッピング
//volatile int step_array_l[6000];
//volatile int step_array_r[6000];

//volatile float degree_array[6000];

volatile float angular_velocity[12000];
volatile float angular_velocity_ave[12000];

volatile float x_pos[12000];
volatile float y_pos[12000];

volatile float marker_coordinate_x[300][2];
volatile float marker_coordinate_y[300][2];

volatile float section_angular_velocity[150];
volatile float section_distance[150];

volatile float distance_sum=0;

//マーカ
int marker_L_count=0;
short marker_coordinate_cnt=0;

//二次走行
int max_spd=0;
int min_spd=0;

int acc=0;

float range_distance[150];
float acc_distance[150];

int acc_cnt=0;
int dec_cnt=0;

float acc_spd=0;
float dec_spd=0;

bool acc_flg=false;
bool dec_flg=false;

short distance_cnt=0;
int marker_r_cnt=0;

short acc_section_flg[150];

int acc_space=0;
int dec_space=0;

float section_v_max[150];
short start_spd=10;

float revision_distance[150];
//************************************************
// 関数プロトタイプ宣言
//************************************************
void IO_init( void );
void set_param( void );
void disp_voltage( void );

bool tick0( struct repeating_timer *rt );// Timer割込み関数
bool tick1( struct repeating_timer *rt );// Timer割込み関数

void change_mode( int add );
void exec_mode( void );
void value_change_mode( int add );
void value_exec_mode( void );

void mode0( int com );
void mode1( int com );
void mode2( int com );
void mode3( int com );
void mode4( int com );
void mode5( int com );

void mot_l_drive( void );
void mot_r_drive( void );

//パラメータ調整用関数
void select_param( void );

//キャリブレーション用関数
void line_calib( void );
void auto_calibracion( void );

//linesensor calibration
long map(long x, long in_min, long in_max, long out_min, long out_max);

//step
void cnt_step_L( uint gpio, uint32_t events );
void cnt_step_R( uint gpio, uint32_t events );

//gyro calibration
void gyro_calib( void );

//reset value
void reset_value( void );

//second run
void second_run( void );
void second_acc_dec( void );
void coordinate_calculation( void );
void second_marker( void );
void coordinate( void );

//tera term
void value_mode0( int com );
void value_mode1( int com );
void value_mode2( int com );
void value_mode3( int com );
void value_mode4( int com );
void value_mode5( int com );

//================================================
//  core1 main
//================================================
void core1_main(void) {

    //ステップ数カウント関連初期化
    gpio_set_irq_enabled_with_callback(MOT_L_STEP,0x8u,false,&cnt_step_L);
    gpio_set_irq_enabled(MOT_L_STEP,0x8u,true);

    struct repeating_timer timer;                       // タイマー割込み用変数
    add_repeating_timer_ms( -3, tick1, NULL, &timer );  // 50us割込み設定

    while( true ) {

        if( line_sensor_flg == true ){
            adc_select_input( SEN_R );
            sen_adc[9] = adc_read(); 
            adc_select_input( SEN_L );
            sen_adc[8] = adc_read();
            sen_adc[7] = readADC(2);
            sen_adc[6] = readADC(1);
            sen_adc[5] = readADC(0);
            sen_adc[4] = readADC(3);
            sen_adc[3] = readADC(4);
            sen_adc[2] = readADC(7);
            sen_adc[1] = readADC(6);
            sen_adc[0] = readADC(5);

        }
    }
}

//================================================
// core0 main
//================================================
int main( void ){                      // メイン関数
    
	IO_init();							// I/O初期化
    LCD_init();                         // LCD初期化
    beep_init();                        // Beep初期化
    setup_SPI();                        //adコンバータ
    MPU6050_Reset();                    //gyro
    
    struct repeating_timer timer;       // タイマー割込み用変数
    add_repeating_timer_us( -50, tick0, NULL, &timer );  // 50us割込み設定

    multicore_launch_core1( core1_main );   //multicore

    reset_value();
    
    beep( TONE_DO,5,50);

    change_mode( 0 );                   // まず初期画面にする = mode0

    while( true )                       // 無限ループ
    {
        if( gpio_get( SW_R ) == SW_ON ){// 右SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            change_mode( +1 );          // モードを1つ上げる
        }
        if( gpio_get( SW_L ) == SW_ON ){// 左SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            change_mode( -1 );          // モードを1つ下げる
        }
        if( gpio_get( SW_C ) == SW_ON ){// 中央SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            exec_mode();                // モード実行
            Mode = 0;
            change_mode( 0 );           // 実行後は初期画面に戻す
        }
    }
}

//------------------------------------------------
// Timer割込み1
//------------------------------------------------
bool tick1( struct repeating_timer *rt ){

    if( gyro_flg==true ){

        MPU6050_ReadData(gyro);

        degree += ( ( pregy +  ( ((float)gyro[2]-gyro_offset ) / 16.4 ) ) * 0.003 / 2 ) - gyro_drift;
        pregy = (float)((gyro[2] - gyro_offset) / 16.4);

        gyro_offset_cnt++;
        gyro_offset_sum+=gyro[2];

    }
    if( gyro_print_flg==true ){
        //printf("%d\n",gyro[2]);
        //printf("%9.6lf\n",( temp / 340.0 ) + 36.53);
        printf("%9.6lf\n",degree);
    }
    LCD_print(1,0,"");
}

//------------------------------------------------
// Timer割込み0
//------------------------------------------------
bool tick0( struct repeating_timer *rt ){
    
    int dev_l = 0, dev_r = 0;           //偏差計算用
    
    //msカウンタ
    ms_count_for++;
    if( ms_count_for >= 20 ){
        ms_count_for=0;
        
        if( trace_flg==1 ){

            //gyroセンサ値取得カウント
            tick_count_gyro++;

            //トレース秒数カウント
            trace_time_count++;

            //交差フラグ強制解除用カウント
            cross_count++;


            if( acc_flg==true ) acc_spd += acc;
            else if( dec_flg==true ) dec_spd += acc;

            if( start_spd < target_spd && trace_time_count ){
                start_spd=start_spd+4;
            }
        }
    }
    
    
    tick_count++;                       // カウンタの更新
    if( tick_count == 8 )               // 0-7の8カウントで1周 50us*8=0.4ms
        tick_count = 0;                 // 各処理は0.4us周期で実行される
    
    switch( tick_count )
    {
        // タスク0 : LCD更新
        case 0: LCD_disp();                       // LCD1文字更新
                

                LCD_print(1,0,"");
                break;

        // タスク1
        case 1: //センサ値の加工　0-999

                for ( int i=0; i<10; i++ ){
                    sen_val[i]=sen_adc[i]*999/4095;
                    
                    if( trace_flg==1 ){
                        if( sen_val[i] < sen_val_min[i] ) sen_val_min[i]=sen_val[i];
                        if( sen_val[i] > sen_val_max[i] ) sen_val_max[i]=sen_val[i];
                    }

                }
                LCD_print(1,0,"");
                break;

        // タスク4
        case 2: //キャリブレーション

                if( trace_flg==1 ){
                    for( int i=0; i<10; i++ ){

                        sen_val[i]=map(sen_val[i],sen_val_min[i],sen_val_max[i],0,999 );

                        if( sen_val[i]<0 ) sen_val[i]=0;
                        if( sen_val[i]>999 ) sen_val[i]=999;

                    }
                }
                LCD_print(1,0,"");
                break;

        // タスク5
        case 3: 
                for( int i=0; i<10; i++ ){
                    //反応センサの保存
                    rl_pre[i]=rl[i];
                    //リセット
                    rl[i]=0;
                    sen_val_c[i]=0;
                    //閾値と比較
                    if( sen_val[i] > sen_ref_f ) rl[i]=1;
                } 
                LCD_print(1,0,"");
                break;

        // タスク5
        case 4: 
                if( trace_flg==1 ){
                    
                    //ラインセンサによる交差Flag
                    if( rl[0]==1 && rl[7]==1 ){
                        cross_count=0;
                        rl[0]=2,rl[7]=2;
                        
                        cross_flg=true;
                        cross_demo=true;
                    }
                
                    //交差ブレ防止
                    if(  rl[3]==1 &&  rl[4]==1 ){
                        rl[9]=2;
                        rl[7]=2;
                        rl[0]=2;
                        rl[8]=2;
                    }


                    //マーカー処理
                    if( ( rl[8]==1 && rl[0]==1 ) && ( rl_pre[8]==1 && rl_pre[0]==1 ) ) rl[5]=2,rl[6]=2,rl[7]=2,rl[9]=2;
                    if( ( rl[7]==1 && rl[9]==1 ) && ( rl_pre[7]==1 && rl_pre[9]==1 ) ) rl[2]=2,rl[1]=2,rl[0]=2,rl[8]=2;

                    if( ( rl[0]==1 && rl[1]==1 ) && ( rl_pre[0]==1 && rl_pre[1]==1 ) ) rl[6]=2,rl[7]=2,rl[9]=2;
                    if( ( rl[6]==1 && rl[7]==1 ) && ( rl_pre[6]==1 && rl_pre[7]==1 ) ) rl[1]=2,rl[0]=2,rl[8]=2;

                    if( ( rl[1]==1 && rl[2]==1 ) && ( rl_pre[1]==1 && rl_pre[2]==1 ) ) rl[7]=2,rl[9]=2;
                    if( ( rl[5]==1 && rl[6]==1 ) && ( rl_pre[5]==1 && rl_pre[6]==1 ) ) rl[0]=2,rl[8]=2;



                    //外センサ単体反応時のゲイン調整
                    if( rl[0]==0 && rl[8]==1 ){
                        k5=k5_single;
                    }else if( rl[7]==0 && rl[9]==1 ){
                        k5=k5_single;
                    }else{
                        k5=k5_x;
                    }


                    //センサ外対応
                    if( rl[3]+rl[2]+rl[1]+rl[0]+rl[8]==0 && rl_pre[8]==1 ){
                        out_r=true;
                    }else if( rl[4]+rl[5]+rl[6]+rl[7]+rl[9]==0 && rl_pre[9]==1 ){
                        out_l=true;
                    }else{
                        out_l=false;
                        out_r=false;
                    }

                    if( out_r == true ) rl[1]=2,rl[2]=2,rl[3]=2,rl[4]=2,rl[5]=2,rl[6]=2,rl[7]=2,rl[9]=2;
                    if( out_l == true ) rl[4]=2,rl[5]=2,rl[6]=2,rl[0]=2,rl[1]=2,rl[2]=2,rl[3]=2,rl[8]=2;
                }
                LCD_print(1,0,"");
                break;

        // タスク5
        case 5: //アナログ値を保存
                for( int i=0; i<10; i++ ){
                    if( rl[i]!=2 ){
                        sen_val_c[i]=sen_val[i];
                    }
                    if( rl[i]==2 ) sen_val_c[i]=130;
                }
                LCD_print(1,0,"");
                break;

        // タスク5
        case 6: if( trace_flg==1 ){

                //偏差
                dev = ( sen_val_c[9]*k5 + sen_val_c[7]*k4 + sen_val_c[6]*k3 + sen_val_c[5]*k2 + sen_val_c[4] ) - ( sen_val_c[8]*k5 + sen_val_c[0]*k4 + sen_val_c[1]*k3 + sen_val_c[2]*k2 + sen_val_c[3] );
                
                //センサ外のとき，前回の偏差を使用する
                if( rl[8]+rl[0]+rl[1]+rl[2]+rl[3]+rl[4]+rl[5]+rl[6]+rl[7]+rl[9]==0 ){
                    dev=dev_pre;
                }

                //PD
                control = ( kp * dev ) + ( ( ( dev-dev_pre ) / 0.0004 ) * kd );

                //目標速度設定
                //センサ外処理
                if( out_r==true ){
                    target_spd_r = target_spd - control;
                    target_spd_l=0;

                    rl[8]=1;

                }else if( out_l==true ){
                    target_spd_r=0;
                    target_spd_l = target_spd + control;

                    rl[9]=1;

                }else{
                    if( start_spd < target_spd && trace_time_count < 10 ){
                        target_spd_l = start_spd + control;
                        target_spd_r = start_spd - control;
                    }else{
                        target_spd_l = target_spd + control;
                        target_spd_r = target_spd - control;
                    }
                }
                
                dev_pre=dev;
                
                }
                LCD_print(1,0,"");
                break;

        // タスク6 : モータ速度制御
        case 7: // 左モータ加減速処理
                dev_l = target_spd_l - mot_spd_l; // 目標速度までの偏差
                if( dev_l > 0 )                   // 偏差が正＝速度が不足＝加速
                    mot_spd_l += MOT_ACC;
                else if( dev_l < 0 )              // 偏差が負＝速度が過剰＝減速
                    mot_spd_l -= MOT_ACC;

                // 右モータ加減速処理
                dev_r = target_spd_r - mot_spd_r; // 目標速度までの偏差
                if( dev_r > 0 )                   // 偏差が正＝速度が不足＝加速
                    mot_spd_r += MOT_ACC;
                else if( dev_r < 0 )              // 偏差が負＝速度が過剰＝減速
                    mot_spd_r -= MOT_ACC;
                
                // 起動速度以下の場合の処理
                if( mot_spd_l < MOT_SPD_INIT )    // 左モータの現在速度が起動速度以下
                    if( target_spd_l == 0 )       // 目標速度が0の時は現在速度を0に落とす
                        mot_spd_l = 0;
                    else                          // 目標速度が0以外(動かそうとしている)時は起動速度に引き上げる
                        mot_spd_l = MOT_SPD_INIT;
                if( mot_spd_r < MOT_SPD_INIT )    // 右モータの現在速度が起動速度以下
                    if( target_spd_r == 0 )       // 目標速度が0の時は現在速度を0に落とす
                        mot_spd_r = 0;
                    else                          // 目標速度が0以外(動かそうとしている)時は起動速度に引き上げる
                        mot_spd_r = MOT_SPD_INIT;
                
                // 左右モータ指示
                mot_l_drive();                    // 速度mot_spd_lで左モータ駆動
                mot_r_drive();                    // 速度mot_spd_rで右モータ駆動
                LCD_print(1,0,"");
                
                break; 
        
        default:break;
    }
}

//------------------------------------------------
// I/O初期化
//------------------------------------------------
void IO_init( void )
{
    stdio_init_all();                   // USB初期化

    gpio_init( LED_PICO );              // LED_PICO:GP25初期化
    gpio_set_dir( LED_PICO, GPIO_OUT ); // LED_PICO:GP25出力設定
    
    gpio_init( SW_L );                  // SW_L:GP13初期化
    gpio_init( SW_C );                  // SW_C:GP14初期化
    gpio_init( SW_R );                  // SW_R:GP15初期化

    gpio_set_dir( SW_L, GPIO_IN );      // SW_L:GP13入力設定
    gpio_set_dir( SW_C, GPIO_IN );      // SW_C:GP14入力設定
    gpio_set_dir( SW_R, GPIO_IN );      // SW_R:GP15入力設定

    gpio_pull_up( SW_L );               // SW_L:GP13プルアップ有効化
    gpio_pull_up( SW_C );               // SW_C:GP14プルアップ有効化
    gpio_pull_up( SW_R );               // SW_R:GP15プルアップ有効化

    gpio_init( MARKER_R );              // MARKER_R:GP21初期化
    gpio_init( MARKER_L );              // MARKER_L:GP22初期化

    gpio_set_dir( MARKER_R, GPIO_IN );  // MARKER_R:GP21入力設定
    gpio_set_dir( MARKER_L, GPIO_IN );  // MARKER_L:GP22入力設定

    gpio_pull_up( MARKER_R );           // MARKER_R:GP21プルアップ有効化
    gpio_pull_up( MARKER_L );           // MARKER_L:GP22プルアップ有効化

    //LCD　I2C 初期化
    i2c_init( LCD_I2C, LCD_BAUDRATE );
    gpio_set_function( LCD_SDA, GPIO_FUNC_I2C );
    gpio_set_function( LCD_SCL, GPIO_FUNC_I2C );
    gpio_pull_up( LCD_SDA );
    gpio_pull_up( LCD_SCL );

    //ADC　初期化
    adc_init();                         // ADCのハードウェア初期化
    adc_gpio_init( ADC0 );              // ADC0のGPIOをADCで使用するために設定
    adc_gpio_init( ADC1 );              // ADC1のGPIOをADCで使用するために設定
    adc_gpio_init( ADC2 );              // ADC2のGPIOをADCで使用するために設定

    //Motor　初期化
    gpio_init( MOT_EN );                // MOT_EN:GP8初期化
    gpio_set_dir( MOT_EN, GPIO_OUT );   // MOT_EN:GP8出力設定
    gpio_init( MOT_L_DIR);              // MOT_L_DIR:GP4初期化
    gpio_set_dir( MOT_L_DIR, GPIO_OUT );// MOT_L_DIR:GP4出力設定
    gpio_init( MOT_R_DIR );             // MOT_R_DIR:GP12初期化
    gpio_set_dir( MOT_R_DIR, GPIO_OUT );// MOT_R_DIR:GP12出力設定

    gpio_set_function( MOT_L_CLK, GPIO_FUNC_PWM );          // MOT_L_CLK:GP3 PWM設定
    gpio_set_function( MOT_R_CLK, GPIO_FUNC_PWM );          // MOT_R_CLK:GP11 PWM設定
    mot_l_slice = pwm_gpio_to_slice_num( MOT_L_CLK );       // MOT_L スライス番号取得
    mot_r_slice = pwm_gpio_to_slice_num( MOT_R_CLK );       // MOT_R スライス番号取得
    mot_l_channel = pwm_gpio_to_channel( MOT_L_CLK );       // MOT_L チャンネル番号取得
    mot_r_channel = pwm_gpio_to_channel( MOT_R_CLK );       // MOT_R チャンネル番号取得
    pwm_set_clkdiv( mot_l_slice, DIVISION );                // MOT_L 分周比設定
    pwm_set_clkdiv( mot_r_slice, DIVISION );                // MOT_R 分周比設定

    //gyro初期化
    gpio_set_function(gyro_SCL, GPIO_FUNC_I2C);
    gpio_set_function(gyro_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(gyro_SCL);
    gpio_pull_up(gyro_SDA);
    i2c_init(gyro_i2c, 100*1000);

    //ステップ数カウント関連初期化
    gpio_set_irq_enabled_with_callback(MOT_R_STEP,0x8u,false,&cnt_step_R);
    gpio_set_irq_enabled(MOT_R_STEP,0x8u,true);

}

//------------------------------------------------
// モード表示
//------------------------------------------------
void change_mode( int add ){
    Mode += add;                        // 引数でモードを移動
    if( Mode >= MODE_MAX )  Mode = 0;   // モードが最大値を超えている場合は0に戻す
    if( Mode < 0 )  Mode = MODE_MAX - 1;// モードが負の場合はモードを最大値に設定

    if     ( Mode == 0 ) mode0( DISP );  // Mode0表示
    else if( Mode == 1 ) mode1( DISP );  // Mode1表示
    else if( Mode == 2 ) mode2( DISP );  // Mode2表示
    else if( Mode == 3 ) mode3( DISP );  // Mode3表示
    else if( Mode == 4 ) mode4( DISP );  // Mode4表示
    else if( Mode == 5 ) mode5( DISP );  // Mode5表示
    
}

//------------------------------------------------
// モード実行
//------------------------------------------------
void exec_mode( void ){
    if     ( Mode == 0 ) mode0( EXEC );  // Mode0実行
    else if( Mode == 1 ) mode1( EXEC );  // Mode1実行
    else if( Mode == 2 ) mode2( EXEC );  // Mode2実行
    else if( Mode == 3 ) mode3( EXEC );  // Mode3実行
    else if( Mode == 4 ) mode4( EXEC );  // Mode4実行
    else if( Mode == 5 ) mode5( EXEC );  // Mode5実行
    
}

//------------------------------------------------
// Mode1 : センサチェック
//------------------------------------------------
void mode1( int com )
{
    // 表示の場合
    if( com == DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "1: Sensor Check " );
        LCD_print( 2, 0, "                " );
        return;                     // 以下の実行処理をしないで戻る
    }
    LCD_clear(0);
    // 実行の場合

    line_sensor_flg=true;

    do{

        for ( int i=0; i<10; i++ ){
            sen_val[i]=sen_adc[i]*999/4095;
        }

        for( int i=0; i<10; i++ ){
            sen_val[i]=map(sen_val[i],sen_val_min[i],sen_val_max[i],0,999 );
            if( sen_val[i]<0 ){
                sen_val[i]=0;
            }
        }

        /*LCD_num_out( 1, 5, sen_val[8], 3 );
        LCD_num_out( 1, 9, sen_val[9], 3 );*/

        LCD_num_out( 1, 0, sen_val[7], 3 );
        LCD_num_out( 1, 4, sen_val[6], 3 );
        LCD_num_out( 1, 8, sen_val[5], 3 );
        LCD_num_out( 1, 12, sen_val[4], 3 );

        LCD_num_out( 2, 0, sen_val[3], 3 );
        LCD_num_out( 2, 4, sen_val[2], 3 );
        LCD_num_out( 2, 8, sen_val[1], 3 );
        LCD_num_out( 2, 12, sen_val[0], 3 );
        
    }while( gpio_get( SW_C ) == SW_OFF );         // 中央SW入力判断
    line_sensor_flg=false;
    
    sleep_ms( 1000 );
    LCD_clear( 2 );                               // LCD2行目クリア
}

//------------------------------------------------
// Mode2 : モータチェック
//------------------------------------------------
void mode2( int com )
{
    // 表示の場合
    if( com == DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "2: Motor Check  " );
        LCD_print( 2, 0, "                " );
        return;                     // 以下の実行処理をしないで戻る
    }

    // 実行の場合
    LCD_print( 1, 0, "                             " );
    LCD_print( 2, 0, " SPD =          " );
    gpio_put( MOT_EN, MOT_ON );         // モータEnable
    gpio_put( MOT_L_DIR, MOT_L_FWD );   // 左モータ方向指示 : 正転
    gpio_put( MOT_R_DIR, MOT_R_FWD );   // 右モータ方向指示 : 正転

    target_spd = 0;                     // 目標速度リセット
    trace_flg=true;
    line_sensor_flg=true;

    while( true ){

        //LCD_num_out( 1,1,STEP_L,6);
        //LCD_num_out( 1,8,STEP_R,6);
        
        LCD_num_out( 2, 7, target_spd, 4 );
        if( gpio_get( SW_R ) == SW_ON ){// 右SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            target_spd += 100;          // 加速
            target_spd_r = target_spd;
            target_spd_l = target_spd;
        }
        if( gpio_get( SW_L ) == SW_ON ){// 左SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            target_spd -= 100;          // 減速
            target_spd_r = target_spd;
            target_spd_l = target_spd;
        }
        if( gpio_get( SW_C ) == SW_ON ){// 中央SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            target_spd = 0;             // 停止
            target_spd_r = target_spd;
            target_spd_l = target_spd;
            while( mot_spd_l != 0 || mot_spd_r != 0 );    // 完全停止待ち
            gpio_put( MOT_EN, MOT_OFF );// モータEnable OFF
            return;
        }
        
    }

    trace_flg=false;
    line_sensor_flg=false;
}

//------------------------------------------------
// Mode0 : ライントレース
//------------------------------------------------
void mode0( int com )
{
    if( com == DISP )  // DISPモードの場合
    {
        // モード内容表示
        LCD_print( 1, 0, "0: Line Trace  " );
        LCD_print( 2, 0, "                " );
        return;                     // 以下の実行処理をしないで戻る
    }

    // 実行の場合
    LCD_clear(0);
    
    //パラメータの選択
    select_param();

    //二次走行の計算
    if( trace_number >= 2 ){

        //区間ごとの計算
        for( int i=1; i<=marker_coordinate_cnt+1; i++ ){
            
            //直線
            if( -30 < section_angular_velocity[i] && section_angular_velocity[i] < 30 ){
                
                float acc_dec_distance=0,section_distance=0,max_spd_mms=0,min_spd_mms=0,acc_mms=0,acc_time=0;

                //値の単位変換
                max_spd_mms = max_spd * 0.4045;
                min_spd_mms = min_spd * 0.4045;
                acc_mms = acc * 0.4045 * 1000;

                //加減速に必要な距離
                acc_dec_distance = ( ( ( max_spd_mms * max_spd_mms ) - ( min_spd_mms * min_spd_mms ) ) / acc_mms );
                
                //区間距離の算出
                section_distance = range_distance[i] -  range_distance[i-1];

                //設定最高速度まで加減速できる距離がある場合
                if( section_distance > acc_dec_distance + dec_space + acc_space ){

                    /*float ulm_distance;     //uniform linear motion
                    ulm_distance = section_distance - acc_dec_distance;*/

                    //加速距離の保存
                    acc_distance[i] = ( acc_dec_distance / 2 );

                    acc_section_flg[i] = 1;

                //設定最高速度まで加減速できる距離がない場合
                }else if( section_distance > dec_space + acc_space ){

                    float v_max=0;

                    v_max = sqrt( 2 * acc_mms * ( ( section_distance / 2 ) - dec_space - acc_space ) + ( min_spd_mms * min_spd_mms ) );
                    section_v_max[i] = (int)(v_max / 0.4045);

                    //加速距離の保存
                    acc_distance[i] = ( acc_dec_distance / 2 );

                    acc_section_flg[i] = 2;

                }

            //曲線
            }else{

            }
        }
    }
/*
    //補正区間の算出
    if( trace_number >= 2 ){
        int j=0;
        float section_distance=0;

        for(int i=1;i<marker_coordinate_cnt+1;i++){
            
            if(){

            }


            
            section_distance = range_distance[i] - range_distance[i-1];
            
            revision_distance[j] = range_distance[i] - 50;
            revision_distance[j+1] = range_distance[i] + 50;

            j=+2;

        }
    }
*/
    gpio_put( MOT_EN, MOT_ON );                   // モータEnable
    gpio_put( MOT_L_DIR, MOT_L_FWD );             // 左モータ方向指示 : 正転
    gpio_put( MOT_R_DIR, MOT_R_FWD );             // 右モータ方向指示 : 正転

    sleep_ms(1000);

    LCD_clear(0);
    
    //走行用変数リセット
    line_sensor_flg=true;
    trace_flg=true;
    trace_time_count=0;
    gyro_print_flg=gyro_flg=false;
    tick_count_gyro=0;
    
    int marker_sound=0;
    
    int trace_start_count_pre=0;
    marker_L_count=0;
    
    int start_count_flg=false;

    marker_r_cnt=0;
    
    distance_sum=0;

    //test
    pregy=0;
    degree=0;
    STEP_L=0;
    STEP_R=0;
    distance_cnt=0;
    
    start_spd=0;

    do{

        //加減速
        second_acc_dec();
        //1周目座標計算
        coordinate_calculation();
        //一回目マーカの記憶
        second_marker();

        
        if( tick_count_gyro >= 3 && trace_number>=2 ){
            tick_count_gyro = 0;

            float delta_distance;
            delta_distance = ( STEP_L*0.4045 + STEP_R*0.4045 ) / 2;
            STEP_R=0,STEP_L=0;

            distance_sum += delta_distance;
            
        }

        //スタート
        if( marker_r_cnt == 0 ) trace_time_count=0,pregy=0,degree=0,STEP_L=0,STEP_R=0,cross_demo=false;

        if( cross_count > 1000 ) cross_flg=false;
        
        //クロス強制解除
        if( cross_count >= 700 &&  cross_flg==true  ) cross_flg=false;

        //ゴール
        if( gpio_get( MARKER_R ) == MARKER_ON ){
            marker_sound++;
            beep( TONE_DO,6,10);
        }else{
            if( marker_sound >= 1 ){
                marker_sound=0;

                marker_r_cnt++;

                if( marker_r_cnt >= 2){

                    if( cross_flg==false ){
                        trace_flg = 0;

                        //最後の区間距離保存
                        marker_coordinate_cnt++;
                        marker_coordinate_x[marker_coordinate_cnt][1]=gyro_cnt;
                        marker_coordinate_y[marker_coordinate_cnt][1]=gyro_cnt;

                        range_distance[marker_coordinate_cnt] = distance_sum;

                        target_spd_l=1200;
                        target_spd_r=1200;
                        sleep_ms(500);
                        target_spd_l=500;
                        target_spd_r=500;
                        sleep_ms(500);
                        target_spd_l=0;
                        target_spd_r=0;
                        
                        break;
                    }else{
                        cross_flg=false;
                    }
                }
            }
        }


    }while( gpio_get( SW_C ) == SW_OFF );         // 中央SW入力判断
    sleep_ms( SW_WAIT );                          // チャタリング防止

    trace_time_count=0;
    trace_number++;
    target_spd_r = 0;
    target_spd_l = 0;
    line_sensor_flg=false;

    while( mot_spd_l != 0 || mot_spd_r != 0 );    // 完全停止待ち
    gpio_put( MOT_EN, MOT_OFF );                  // モータEnable OFF


    //角速度の移動平均を算出
    if(trace_number==2){

        for(int n=0;n<gyro_cnt-50;n++){

            float angular_velocity_sum=0;

            for(int i=n;i<n+50;i++){
                angular_velocity_sum += angular_velocity[i];
            }

            angular_velocity_ave[n+50] = angular_velocity_sum / 50;

        }

        for( int i=0;i<50;i++){
            angular_velocity_ave[i] = angular_velocity[i];
        }

        //区間中央の角速度を保存
        for( int i=1;i<marker_coordinate_cnt+1;i++){

            int section_middle_number;
            section_middle_number = (int)( ( marker_coordinate_x[i][1] + marker_coordinate_x[i-1][1] ) / 2 );

            section_angular_velocity[i] = angular_velocity_ave[section_middle_number];

        }
    }
}

//------------------------------------------------
// Mode3 : gyro calib
//------------------------------------------------
void mode3( int com )
{ 
    if( com == DISP )  // DISPモードの場合
    {
        // モード内容表示
        LCD_print( 1, 0, "3: Gyro Calib   " );
        LCD_print( 2, 0, "          " );
        return;                     // 以下の実行処理をしないで戻る
    }
    // 実行の場合
    LCD_clear(0);
    
    LCD_print( 1, 0, "   Gyro Calib   " );
    LCD_print(2,1,"Cancel    Next");

    while(true){
        if( gpio_get( SW_L ) == SW_ON ){
            sleep_ms( SW_WAIT );
            break;
        }
        if( gpio_get( SW_R ) == SW_ON ){
            sleep_ms( SW_WAIT );
            
            LCD_clear(0);
            LCD_print(2,0," Calibrating...");

            gyro_calib();

            //tera termへ1分間の角度の出力
            /*gyro_print_flg=gyro_flg=true;
            sleep_ms(60000);*/

            gyro_print_flg=gyro_flg=false;
            break;
        }
    }
    LCD_clear(0);
    
}

//------------------------------------------------
// Mode4 : linesensor calib
//------------------------------------------------
void mode4( int com )
{
    if( com == DISP )  // DISPモードの場合
    {
        // モード内容表示
        LCD_print( 1, 0, "4: Sensor Calib  " );
        LCD_print( 2, 0, "                " );
        return;                     // 以下の実行処理をしないで戻る
    }
    // 実行の場合
    LCD_clear(0);

    //キャリブレーション選択
    LCD_print( 1, 0, "  Sensor Calib  " );
    LCD_print(2,1,"Cancel    Next");

    while(true){
        if( gpio_get( SW_L ) == SW_ON ){
            sleep_ms( SW_WAIT );
            break;
        }
        if( gpio_get( SW_R ) == SW_ON ){
            sleep_ms( SW_WAIT );
            
            LCD_clear(0);
            LCD_print(2,0," Calibrating...");

            line_sensor_flg=true;

            for( int i=0; i<5000; i++ ){
                line_calib();
                sleep_ms( 1 );
            }

            line_sensor_flg=false;

            break;
        }
    }
    LCD_clear(0);


}

//------------------------------------------------
// Mode5 : 値の通信
//------------------------------------------------
void mode5( int com ){

    if( com == DISP )  // DISPモードの場合
    {
        // モード内容表示
        LCD_print( 1, 0, "5:tera term  " );
        LCD_print( 2, 0, "                " );
        return;                     // 以下の実行処理をしないで戻る
    }
    
    sleep_ms( SW_WAIT );
    while( true )                       // 無限ループ
    {
        if( gpio_get( SW_R ) == SW_ON ){// 右SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            value_change_mode( +1 );          // モードを1つ上げる
        }
        if( gpio_get( SW_L ) == SW_ON ){// 左SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            value_change_mode( -1 );          // モードを1つ下げる
        }
        if( gpio_get( SW_C ) == SW_ON ){// 中央SW入力判断
            sleep_ms( SW_WAIT );        // チャタリング防止
            value_exec_mode();                // モード実行
            value_Mode = 0;
            value_change_mode( 0 );           // 実行後は初期画面に戻す
        }
    }
}

//------------------------------------------------
// tera term モード表示
//------------------------------------------------
void value_change_mode( int add ){
    value_Mode += add;                        // 引数でモードを移動
    if( value_Mode >= VALUE_MODE_MAX )  value_Mode = 0;   // モードが最大値を超えている場合は0に戻す
    if( value_Mode < 0 )  value_Mode = VALUE_MODE_MAX - 1;// モードが負の場合はモードを最大値に設定

    if     ( value_Mode == 0 ) value_mode0( VALUE_DISP );  // Mode0表示
    else if( value_Mode == 1 ) value_mode1( VALUE_DISP );  // Mode1表示
    else if( value_Mode == 2 ) value_mode2( VALUE_DISP );  // Mode2表示
    else if( value_Mode == 3 ) value_mode3( VALUE_DISP );  // Mode3表示
    else if( value_Mode == 4 ) value_mode4( VALUE_DISP );  // Mode4表示
    else if( value_Mode == 5 ) value_mode5( VALUE_DISP );  // Mode5表示
    
}

//------------------------------------------------
// tera term モード実行
//------------------------------------------------
void value_exec_mode( void ){
    if     ( value_Mode == 0 ) value_mode0( VALUE_EXEC );  // Mode0実行
    else if( value_Mode == 1 ) value_mode1( VALUE_EXEC );  // Mode1実行
    else if( value_Mode == 2 ) value_mode2( VALUE_EXEC );  // Mode2実行
    else if( value_Mode == 3 ) value_mode3( VALUE_EXEC );  // Mode3実行
    else if( value_Mode == 4 ) value_mode4( VALUE_EXEC );  // Mode4実行
    else if( value_Mode == 5 ) value_mode5( VALUE_EXEC );  // Mode5実行
    
}

//------------------------------------------------
// value Mode0 : xy座標
//------------------------------------------------
void value_mode0( int com ){

    // 表示の場合
    if( com == VALUE_DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "0: x,y position  " );
        LCD_print( 2, 0, "                " );
        return;                     // 以下の実行処理をしないで戻る
    }

    LCD_clear(0);
    LCD_print(1,4,"coordinate x");
    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=0; i<gyro_cnt; i++ ){
        printf( "%lf\n",x_pos[i]);
    }

    LCD_clear(0);
    LCD_print(1,4,"coordinate y");
    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=0; i<gyro_cnt; i++ ){
        printf( "%lf\n",y_pos[i]);
    }

    return;

}

//------------------------------------------------
// value Mode1 : 角速度
//------------------------------------------------
void value_mode1( int com ){

    // 表示の場合
    if( com == VALUE_DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "1: angular velocity" );
        LCD_print( 2, 0, "                " );
        return;                     // 以下の実行処理をしないで戻る
    }

    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=0; i<gyro_cnt; i++ ){
        printf( "%lf\n",angular_velocity[i]);
    }
}

//------------------------------------------------
// value Mode2 : 区間角速度
//------------------------------------------------
void value_mode2( int com ){

    // 表示の場合
    if( com == VALUE_DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "2: section angular " );
        LCD_print( 2, 0, "      velocity     " );
        return;                     // 以下の実行処理をしないで戻る
    }

    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=1; i<marker_coordinate_cnt+1; i++ ){
        printf( "%lf\n",section_angular_velocity[i]);
    }
}

//------------------------------------------------
// value Mode3 : 移動平均をとった区間角速度
//------------------------------------------------
void value_mode3( int com ){

    // 表示の場合
    if( com == VALUE_DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "3: section angular " );
        LCD_print( 2, 0, "  velocity ave     " );
        return;                     // 以下の実行処理をしないで戻る
    }

    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=1; i<gyro_cnt; i++ ){
        printf( "%lf\n",angular_velocity_ave[i]);
    }
}

//------------------------------------------------
// value Mode4 : 区間距離
//------------------------------------------------
void value_mode4( int com ){

    // 表示の場合
    if( com == VALUE_DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "4: section distance " );
        LCD_print( 2, 0, "                   " );
        return;                     // 以下の実行処理をしないで戻る
    }

    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=1; i<marker_coordinate_cnt+1; i++ ){
        float section_distance;
        section_distance = range_distance[i] -  range_distance[i-1];
        printf( "%lf\n",section_distance);
    }
}

//------------------------------------------------
// value Mode5 : マーカ座標
//------------------------------------------------
void value_mode5( int com ){

    // 表示の場合
    if( com == VALUE_DISP )
    {
        // モード内容表示
        LCD_print( 1, 0, "5: marker coordinate " );
        LCD_print( 2, 0, "                   " );
        return;                     // 以下の実行処理をしないで戻る
    }

    LCD_clear(0);
    LCD_print(1,4,"marker x coordinate");
    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=0; i<marker_coordinate_cnt+1; i++ ){
        printf( "%lf\n",marker_coordinate_x[i][0]);
    }

    LCD_clear(0);
    LCD_print(1,4,"marker y coordinate");
    do{
    }while(gpio_get( SW_C ) == SW_OFF);
    sleep_ms(SW_WAIT);
    for( int i=0; i<marker_coordinate_cnt+1; i++ ){
        printf( "%lf\n",marker_coordinate_y[i][0]);
    }
}

//------------------------------------------------
// gyro calibration
//------------------------------------------------
void gyro_calib( void )
{
    //gyro offset
    gyro_flg=true;
    sleep_ms(3000);
    
    degree=0,pregy=0;
    gyro_offset_cnt=0;
    sleep_ms(3000);
    
    //gyro_flg=false;
    gyro_offset = gyro_offset_sum / gyro_offset_cnt;
    
    beep( TONE_MI,5,100);
    beep( TONE_DO,4,100);

    sleep_ms(500);
    
    //gyro drift
    degree=0;
    pregy=0;
    gyro_offset_cnt=0;

    do{LCD_print(1,0,"");}while( gyro_offset_cnt <= 999 );

    gyro_drift = degree / 1000;

    beep( TONE_DO,5,100);
    beep( TONE_RE,6,100);

    //表示
    gyro_flg=false;
    degree=0,pregy=0;

}

//------------------------------------------------
// linesensor calibration
//------------------------------------------------
void line_calib( void ){

    //センサ値0-999
    for( int i=0; i<10; i++ ){
        sen_val[i]=sen_adc[i]*999/4095;
    }

    //最小値の取得
    for( int i=0; i<10; i++ ){
        if( sen_val[i] < sen_val_min[i] ){
            sen_val_min[i]=sen_val[i];
        }
    }
    
    //最大値の取得
    for( int i=0; i<10; i++ ){
        if( sen_val[i] > sen_val_max[i] ){
            sen_val_max[i]=sen_val[i];
        }
    }
    
}

//------------------------------------------------
// 二次走行加減速
//------------------------------------------------
void second_acc_dec( void ){

    //加減速
    if( trace_number>=2 && trace_time_count > 0 ){
        
        //一区間の範囲
        if( range_distance[distance_cnt] <= distance_sum && distance_sum <= range_distance[distance_cnt+1] ){

            if( acc_section_flg[distance_cnt+1] == 1 ){

                //加速範囲
                if( range_distance[distance_cnt] + acc_space <= distance_sum && distance_sum <= ( range_distance[distance_cnt+1] - acc_distance[distance_cnt+1] - dec_space ) ){
                    
                    if( acc_cnt == 0 ){
                        acc_flg=true;
                        dec_cnt=0;
                        dec_spd=0;
                        dec_flg=false;
                    }

                    if( target_spd >= max_spd ){
                        target_spd = max_spd;
                    }else{
                        target_spd = min_spd + acc_spd;
                    }
                    
                    acc_cnt++;

                //減速範囲
                }else if( range_distance[distance_cnt+1] - acc_distance[distance_cnt+1] - dec_space <= distance_sum && distance_sum <= range_distance[distance_cnt+1] ){
                    
                    if( dec_cnt == 0 ){
                        dec_flg=true;
                        acc_cnt=0;
                        acc_spd=0;
                        acc_flg=false;
                    }
                    
                    if( target_spd <= min_spd ){
                        target_spd = min_spd;
                    }else{
                        target_spd = max_spd - dec_spd;
                    }
                    
                    dec_cnt++;
                    
                //等速範囲
                }else{

                    acc_flg=dec_flg=false;
                    target_spd      = min_spd;

                }

            }else if( acc_section_flg[distance_cnt+1] == 2 ){

                //加速範囲
                if( range_distance[distance_cnt] + acc_space <= distance_sum && distance_sum <= ( range_distance[distance_cnt+1] - acc_distance[distance_cnt+1] - dec_space ) ){
                    
                    if( acc_cnt == 0 ){
                        acc_flg=true;
                        dec_cnt=0;
                        dec_spd=0;
                        dec_flg=false;
                    }

                    if( target_spd >= section_v_max[distance_cnt+1] ){
                        target_spd = section_v_max[distance_cnt+1];
                    }else{
                        target_spd = min_spd + acc_spd;
                    }
                    
                    acc_cnt++;

                //減速範囲
                }else if( range_distance[distance_cnt+1] - acc_distance[distance_cnt+1] - dec_space <= distance_sum && distance_sum <= range_distance[distance_cnt+1] ){
                    
                    if( dec_cnt==0 ){
                        dec_flg=true;
                        acc_cnt=0;
                        acc_spd=0;
                        acc_flg=false;
                    }
                    
                    if( target_spd <= min_spd ){
                        target_spd = min_spd;
                    }else{
                        target_spd = section_v_max[distance_cnt+1] - dec_spd;
                    }
                    
                    dec_cnt++;
                    
                //等速範囲
                }else{
                    acc_flg=dec_flg=false;
                    target_spd = min_spd;
                }
                
            }else{
                acc_flg=dec_flg=false;
                target_spd = min_spd;
            }

        //区間の更新
        }else{
            
            distance_cnt++;
            if( marker_coordinate_cnt <= distance_cnt ){

            }else{
                //beep( TONE_DO,7,20);
            }
        }
    }
        
}

//------------------------------------------------
// 1周目座標計算
//------------------------------------------------
void coordinate_calculation( void ){

    //1周目座標計算
    if( trace_number==1 && marker_r_cnt >= 1 ){
        if( tick_count_gyro >= 3 ){
            tick_count_gyro = 0;


            MPU6050_ReadData( gyro );
            degree += ( ( pregy +  ( (float)( gyro[2] - gyro_offset ) / 16.4 ) ) * 0.003 / 2 ) - gyro_drift;
            pregy = (float)( ( gyro[2] - gyro_offset ) / 16.4);


            //角速度の保存
            angular_velocity[gyro_cnt] = (float)(( gyro[2] - gyro_offset ) / 16.4 );
            
            //xy座標の算出
            float delta_distance;
            float degree_rad;
            delta_distance = ( STEP_L * 0.4045 + STEP_R * 0.4045 ) / 2;
            STEP_R=0,STEP_L=0;

            distance_sum += delta_distance;
            
            degree_rad = degree * M_PI / 180;

            if( gyro_cnt == 0 ){
                x_pos[gyro_cnt] = sin(degree_rad) * delta_distance;
                y_pos[gyro_cnt] = cos(degree_rad) * delta_distance;
            }else{
                x_pos[gyro_cnt] = x_pos[gyro_cnt-1] + sin(degree_rad) * delta_distance;
                y_pos[gyro_cnt] = y_pos[gyro_cnt-1] + cos(degree_rad) * delta_distance;
            }

            //値のセット
            gyro_cnt++;

        }
    }
}

//------------------------------------------------
// マーカ記憶
//------------------------------------------------
void second_marker( void ){

    //一回目トレースの記憶
    if( trace_number==1 ){
        if( gpio_get( MARKER_L ) == MARKER_ON ){
            marker_L_count++;
        }else{
            if( marker_L_count > 1 ){
                marker_L_count = 0;

                if( cross_demo == false ){

                    marker_coordinate_cnt++;
                    
                    /*marker_coordinate_x[marker_coordinate_cnt][0]=x_pos[gyro_cnt-1];
                    marker_coordinate_y[marker_coordinate_cnt][0]=y_pos[gyro_cnt-1];*/

                    marker_coordinate_x[marker_coordinate_cnt][1]=gyro_cnt;
                    marker_coordinate_y[marker_coordinate_cnt][1]=gyro_cnt;

                    range_distance[marker_coordinate_cnt] = distance_sum;
                    
                }else{
                    cross_demo = false;
                }
            }
        }
    }
}

//------------------------------------------------
// 値の変換用関数
//------------------------------------------------
long map(long x, long in_min, long in_max, long out_min, long out_max){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//------------------------------------------------
// 値のリセット
//------------------------------------------------
void reset_value( void ){

    //値のリセット
    for(int i=0; i<12000; i++ ){
        x_pos[i]=0;
        y_pos[i]=0;
        //step_array_l[i]=0;
        //step_array_r[i]=0;
        //degree_array[i]=0;
        angular_velocity[i]=0;
    }

    for( int i=0; i<300; i++ ){
        for( int n=0; n<2; n++ ){
            marker_coordinate_x[i][n]=0;
            marker_coordinate_y[i][n]=0;
        }
    }

    for( int i=0; i<150; i++ ){
        range_distance[i]=0;
        section_distance[i]=0;
        section_angular_velocity[i]=0;
        acc_distance[i]=0;
        acc_section_flg[i]=0;
        section_v_max[i]=0;
        revision_distance[i]=0;
    }

}
//------------------------------------------------
// モータSTEP数のカウント
//------------------------------------------------
void cnt_step_L( uint gpio, uint32_t events )
{
    if( gpio == MOT_L_STEP && events == 8u ){
        gpio_set_irq_enabled(MOT_L_STEP,0x8u,false);
        STEP_L++;
        gpio_set_irq_enabled(MOT_L_STEP,0x8u,true);
    }
}
void cnt_step_R( uint gpio, uint32_t events )
{
    if( gpio == MOT_R_STEP && events == 8u ){
        gpio_set_irq_enabled(MOT_R_STEP,0x8u,false);
        STEP_R++;
        gpio_set_irq_enabled(MOT_R_STEP,0x8u,true);
    }
}
//------------------------------------------------
// モータ駆動関数 : 左モータ : 速度mot_spd_lで駆動
//------------------------------------------------
void mot_l_drive( void )
{
    if( mot_spd_l <= 0 ){
        // モータ停止指示
        pwm_set_enabled( mot_l_slice, false );		        // 左モータOFF
    }else{
        // モータ駆動指示
        mot_l_freq = SYS_CLOCK / DIVISION / mot_spd_l;      // 指定周波数からPWMのカウンタ値を計算
        mot_l_duty = mot_l_freq / 2;                        // PWMのカウンタ値からDuty50%を計算
        pwm_set_chan_level( mot_l_slice, mot_l_channel, mot_l_duty );    // 左モータDuty比設定
        pwm_set_wrap( mot_l_slice, mot_l_freq );            // 左モータ周波数設定
        pwm_set_enabled( mot_l_slice, true );		        // 左モータON
    }
}

//------------------------------------------------
// モータ駆動関数 : 右モータ : 速度mot_spd_rで駆動
//------------------------------------------------
void mot_r_drive( void )
{
    if( mot_spd_r <= 0 ){
        // モータ停止指示
        pwm_set_enabled( mot_r_slice, false );		        // 右モータOFF
    }else{
        // モータ駆動指示
        mot_r_freq = SYS_CLOCK / DIVISION / mot_spd_r;      // 指定周波数からPWMのカウンタ値を計算
        mot_r_duty = mot_r_freq / 2;                        // PWMのカウンタ値からDuty50%を計算
        pwm_set_chan_level( mot_r_slice, mot_r_channel, mot_r_duty );    // 右モータDuty比設定
        pwm_set_wrap( mot_r_slice, mot_r_freq );            // 右モータ周波数設定
        pwm_set_enabled( mot_r_slice, true );		        // 右モータON
    }
}

//************************************************
// パラメータ設定
//************************************************
void select_param ( void ){
    LCD_clear(0);
    param=0;
    LCD_print( 1,0,"serch");
    LCD_print( 2,0," <             >");
    while( true ){

        //パラメータ選択
        if( gpio_get( SW_R ) == SW_ON ){
            sleep_ms( SW_WAIT );
            param++;

            //表示上限加減処理
            if(param>8){
                param=0;
            }
            if(param<0){
                param=8;
            }

            //パラメータ表示
            LCD_clear(0);
            
            if( param==0 ){
                LCD_print( 1,0,"serch");
            }
            if( param==1 ){
                LCD_print( 1,0,"m 2000  M 3000  acc 9");
            }
            if( param==2 ){
                LCD_print( 1,0,"m 2000  M 3500  acc 9");
            }
            if( param==3 ){
                LCD_print( 1,0,"m 2000  M 4000  acc 10");
            }
            if( param==4 ){
                LCD_print( 1,0,"m 2000  M 4500  acc 9");
            }
            if( param==5 ){
                LCD_print( 1,0,"m 2200  M 4000  acc 10");
            }
            if( param==6 ){
                LCD_print( 1,0,"m 2400  M 4000  acc 9");
            }
            if( param==7 ){
                LCD_print( 1,0,"m 2200  M 4500  acc 9");
            }
            if( param==8 ){
                LCD_print( 1,0,"m 2300  M 4500  acc 9");
            }
        }
        if( gpio_get( SW_L ) == SW_ON ){
            sleep_ms( SW_WAIT );
            param--;
            //表示上限加減処理
            if(param>8){
                param=0;
            }
            if(param<0){
                param=8;
            }

            //パラメータ表示
            LCD_clear(0);
            
            if( param==0 ){
                LCD_print( 1,0,"serch");
            }
            if( param==1 ){
                LCD_print( 1,0,"m 2000  M 3000  acc 9");
            }
            if( param==2 ){
                LCD_print( 1,0,"m 2000  M 3500  acc 9");
            }
            if( param==3 ){
                LCD_print( 1,0,"m 2000  M 4000  acc 10");
            }
            if( param==4 ){
                LCD_print( 1,0,"m 2000  M 4500  acc 9");
            }
            if( param==5 ){
                LCD_print( 1,0,"m 2200  M 4000  acc 10");
            }
            if( param==6 ){
                LCD_print( 1,0,"m 2400  M 4000  acc 9");
            }
            if( param==7 ){
                LCD_print( 1,0,"m 2200  M 4500  acc 9");
            }
            if( param==8 ){
                LCD_print( 1,0,"m 2300  M 4500  acc 9");
            }
            
        }
        
        if( gpio_get( SW_C ) == SW_ON ){
            sleep_ms( SW_WAIT );
            set_param();
            break;
        }
    }
}

//************************************************
// パラメータ設定
//************************************************
void set_param( void ){
    
    
    /*if( param==0 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.25;
        kd=0.0001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2000;

    }*/

    //2400のパラメータ
    /*
    if( param==0 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.24;
        kd=0.00001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd= 2400;      

    }*/
    if( param==0 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.24;
        kd=0.000005;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd= 2000;      

    }
    if( param==1 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.25;
        kd=0.0001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2000;

        min_spd=2000;
        max_spd=3000;
        acc=9;

        acc_space=50;
        dec_space=50;
    }
    if( param==2 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.25;
        kd=0.0001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2000;

        min_spd=2000;
        max_spd=3500;
        acc=9;

        acc_space=50;
        dec_space=50;
    }
    if( param==3 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.25;
        kd=0.0001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2000;

        min_spd=2000;
        max_spd=4000;
        acc=10;

        acc_space=50;
        dec_space=50;
    }
    if( param==4 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.25;
        kd=0.0001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2000;

        min_spd=2000;
        max_spd=4500;
        acc=9;

        acc_space=50;
        dec_space=50;
    }
    if( param==5 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.25;
        kd=0.0001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2200;

        min_spd=2200;
        max_spd=4000;
        acc=10;

        acc_space=50;
        dec_space=50;
    }
    if( param==6 ){
        
        b=2;
        MOT_ACC=10;
        kp=0.26;
        kd=0.0001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2400;

        min_spd=2400;
        max_spd=4000;
        acc=9;

        acc_space=50;
        dec_space=50;
    }
    if( param==7 ){
        
        b=2;
        MOT_ACC=9;
        kp=0.26;
        kd=0.000001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.6;
        k5_single=15;
        
        target_spd      = 2200;

        min_spd=2200;
        max_spd=4500;
        acc=9;

        acc_space=50;
        dec_space=50;
    }
    if( param==8 ){
        
        b=2;
        MOT_ACC=9;
        kp=0.25;
        kd=0.000001;
        
        k2=1.5;
        k3=1.8;
        k4=2.1;
        k5_x=2.5;
        k5_single=15;
        
        target_spd      = 2400;

        min_spd=2400;
        max_spd=4500;
        acc=9;

        acc_space=70;
        dec_space=70;
    }
}