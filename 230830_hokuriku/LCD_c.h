//################################################
// ラズピコキット(Raspberry Pi Pico Kit) C-Version
// LCD自作関数 (2022.7.1～)
//################################################

//************************************************
// ピン定義
//************************************************
#define LCD_I2C     i2c0
#define LCD_SDA     20        // I2C�ｿｽﾊ信�ｿｽFSDA
#define LCD_SCL     21        // I2C�ｿｽﾊ信�ｿｽFSCL
//************************************************
// マクロ定義
//************************************************
#define DEVICE      0x3e      // LCDデバイスのアドレス
#define COM_ADD     0x00      // コマンドを送信するアドレス
#define DAT_ADD     0x40      // データを送信するアドレス
#define ALL_CLR     0x01      // 命令 : 全クリア
#define ADD_SET     0x80      // 命令 : DDRAMアドレスセット 0x80+指定した場所
#define L1TOP       0x00      // Line1の先頭アドレス (DDRAM)
#define L2TOP       0x40      // Line2の先頭アドレス (DDRAM)
#define LCD_BAUDRATE        (700 * 1000)

//************************************************
// グローバル変数定義
//************************************************
unsigned char lcd_line[ 2 ][ 16 ];  // LCD表示文字列 : 2行x16文字
int disp_cnt = 0;

//************************************************
// 関数プロトタイプ宣言
//************************************************
void LCD_init( void );
void LCD_write( char lcd_addr, char lcd_data );
void LCD_disp( void );
void LCD_clear(int line);
void LCD_print( int line, int LCD_pt, unsigned char *pt );
void LCD_num_out( int line, int pos, int num, int digit );

//------------------------------------------------
// LCD初期化
//------------------------------------------------
void LCD_init( void )
{
	// LCD初期化用データ：数値の意味はデータシート参照
	char orders[ 9 ] = { 0x38, 0x39, 0x14, 0x73, 0x56, 0x6c, 0x38, 0x01, 0x0c };

    sleep_ms( 40 );
    for( int i = 0; i < 6; i++ ){
        LCD_write( COM_ADD, orders[i] );
        sleep_ms( 1 );
    }
    sleep_ms( 200 );
    for( int i = 6; i < 9; i++ ){
        LCD_write( COM_ADD, orders[i] );
        sleep_ms( 1 );
    }
    LCD_clear( 0 );		                // LCD表示全クリア
}

//------------------------------------------------
// LCD書き込み
//------------------------------------------------
void LCD_write( char lcd_addr, char lcd_data )
{
    char lcd_send_data[ 2 ];
    lcd_send_data[ 0 ] = lcd_addr;
    lcd_send_data[ 1 ] = lcd_data;
    i2c_write_blocking( LCD_I2C, DEVICE, lcd_send_data, 2, false);
}

//------------------------------------------------
// LCD表示  34回の呼び出しで全画面更新
//------------------------------------------------
void LCD_disp( void )
{
    disp_cnt ++;                        // 呼び出し回数カウント(1-34)
    if( disp_cnt == 35 )
        disp_cnt = 1;                   // 34回呼び出したらカウンタクリア

    // タイマ割り込み併用時，1回の通信で1コマンドor1データにしないと
    // 通信障害が発生する可能性があるので注意
    char disp_command;                  // コマンド格納用
    char disp_data;                     // 送信データ格納用
    
    // 呼び出し回数に応じてコマンドとデータをセット
    // 1-16回目 : line1 16文字
    if( disp_cnt >= 1 && disp_cnt <= 16 ){
        disp_command = DAT_ADD;         // データ送信
        disp_data = lcd_line[ 0 ][ disp_cnt - 1 ];    // 送信する１文字をセット
    }
    // 17回目 : カーソルをline2の先頭に移動するだけ
    else if( disp_cnt == 17 ){
        disp_command = COM_ADD;         // インストラクション（液晶への指示）送信
        disp_data = ADD_SET + L2TOP;    // 送信する指示をセット : 0xc0
    }
    // 18-33回目 : line2 16文字
    else if( disp_cnt >= 18 && disp_cnt <= 33 ){
        disp_command = DAT_ADD;         // データ送信
        disp_data = lcd_line[ 1 ][ disp_cnt - 18 ];   // 送信する１文字をセット
    }
    // 34回目 : カーソルをline1の先頭に戻すだけ
    else if( disp_cnt == 34 ){
        disp_command = COM_ADD;         // インストラクション（液晶への指示）送信
        disp_data = ADD_SET + L1TOP;    // 送信する指示をセット : 0x8f
    }
    
    LCD_write( disp_command, disp_data );         // LCD送信
}

//------------------------------------------------
//  LCDクリア
//------------------------------------------------
void LCD_clear(int line)
{
    if( line == 1 || line == 2 )                  // 1 or 2 行目どちらか指定の時
    	for( int i = 0; i < 16; i++ )
	        lcd_line[ line - 1 ][ i ] = ' ';      // 指定行クリア
    else 										  // line = 1, 2 以外 = 全クリア指示
    	for( int j = 0; j < 2; j++ )
	    	for( int i = 0; i < 16; i++ )
	    	    lcd_line[ j ][ i ] = ' ';         // 全クリア
}

//------------------------------------------------
//  LCD文字セット　line = 1 or 2 / pos = 0 - 15
//    使用例 : LCD_print( 行番号line, 先頭位置pos, "文字列" )
//------------------------------------------------
void LCD_print( int line, int pos, unsigned char *pt )
{
	unsigned char send_data;			// 半角カナの加工後データ格納用

    if( line == 1 || line == 2 )		// 正しい行指定の場合のみ文字セット
        while( *pt ){
        	// 半角カナ処理
        	if( *pt <= 0x80 )			// ASCIIコード範囲内外，つまりそのままでOK
        		send_data = *pt;
        	else{						// ASCIIコード範囲外，つまり半角カナ
        		pt += 2;			    // UTF-8で半角カナは3byte．頭2byte無視して3byte目を使用
        		if( *pt > 0xa0 )		// 半角カナ前半(～ｿ)までは3byte目がそのままANKコード
        			send_data = *pt;
        		else					// 後半(ﾀ～)は3byte目に0x40を加算するとANKコードと一致
        			send_data = *pt + 0x40;
        	}
        	// LCD表示用配列に加工後のデータをセット
        	lcd_line[ line - 1 ][ pos++ ] = send_data;
        	pt++;						// ポインタを1つ後ろへ
		}
}

//------------------------------------------------
//  LCD数値セット　line = 1 or 2 / pos = 0 - 15 / 数値5桁まで対応
//    使用例 : LCD_num_out( 行番号line, 先頭位置pos, 数値, 桁数 )
//------------------------------------------------
void LCD_num_out( int line, int pos, int num, int digit )
{
    int div;
    if( line == 1 || line == 2 ){				  // 正しい行指定の場合のみ数値セット
	    if( digit > 4 ){ div = 10000; lcd_line[ line - 1 ][ pos++ ] = '0' + num / div; num %= div; }
	    if( digit > 3 ){ div = 1000;  lcd_line[ line - 1 ][ pos++ ] = '0' + num / div; num %= div; }
	    if( digit > 2 ){ div = 100;   lcd_line[ line - 1 ][ pos++ ] = '0' + num / div; num %= div; }
	    if( digit > 1 ){ div = 10;    lcd_line[ line - 1 ][ pos++ ] = '0' + num / div; num %= div; }
	                                  lcd_line[ line - 1 ][ pos++ ] = '0' + num;
    }
}
