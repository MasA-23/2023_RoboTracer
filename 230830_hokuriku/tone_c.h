//################################################
// ?��?��?��Y?��s?��R?��L?��b?��g(Raspberry Pi Pico Kit) C-Version
// BEEP?��?��?��?��֐� (2022.7.1?��`)
//################################################

//************************************************
// ?��s?��?��?��?��`
//************************************************
#define BEEP        15        // ?��u?��U?��[
//17
//************************************************
// ?��}?��N?��?��?��?��` : ?��?��?��K?��֌W
//************************************************
#define SYS_CLOCK	125000000 // 125MHz
#define DIVISION	100		  // ?��?��?��?��?��?��

// Sound Pitch : ?�����?��?��?��g?��?��
#define TONE_DO		262		  // ?��h
#define TONE_DS		277		  // ?��h#
#define TONE_RE		294		  // ?��?��
#define TONE_RS		311		  // ?��?��#
#define TONE_MI		330		  // ?��~
#define TONE_FA		349		  // ?��t?��@
#define TONE_FS		370		  // ?��t?��@#
#define TONE_SO		392		  // ?��\
#define TONE_SS		415		  // ?��\#
#define TONE_LA		440		  // ?��?��
#define TONE_LS		466		  // ?��?��#
#define TONE_TI		494		  // ?��V?��i?��p?��ꂾ?��ƃe?��B?��j

//************************************************
// ?��O?��?��?��[?��o?��?��?��ϐ�?��?��`
//************************************************
uint beep_slice;		      // Beep?��pPWM slice?��ԍ�
uint beep_channel;            // Beep?��pPWM channnel
int beep_off=false;
//************************************************
// ?��֐�?��v?��?��?��g?��^?��C?��v?��錾
//************************************************
void beep_init( void );
void beep( int freq, int pitch, int len );
//int64_t alarm_callback( alarm_id_t id, void *user_data );
//------------------------------------------------
// Beep?��pPWM?��?��?��?��?��?��
//------------------------------------------------
void beep_init( void )
{
	gpio_set_function( BEEP, GPIO_FUNC_PWM );     // Beep?��?��GPIO?��?��PWM?��ݒ�
	beep_slice = pwm_gpio_to_slice_num( BEEP );   // slice?��ԍ�?��擾
	beep_channel = pwm_gpio_to_channel( BEEP );   // channnel?��擾
	pwm_set_clkdiv( beep_slice, DIVISION );       // ?��?��?��?��?��?��?��ݒ�
}

//------------------------------------------------
// Beep?��֐�
//     freq  = ?��?��{?��?��?��g?��?��?��i?��h?��?��?��~?��j:?��s?��b?��`4?��̎�?��g?��?��[Hz]
//     pitch = ?��?��?��?��?��@4?��?��?�����?��?��?��K?��C5?��?��?��g?��?��?��₷?��?��
//     len   = ?��?��?��̒�?��?��[ms]
//------------------------------------------------
/*int64_t alarm_callback( alarm_id_t id, void *user_data ){
    pwm_set_enabled( beep_slice, false );
    beep_off=false;
	return 0;
}*/
void beep( int freq, int pitch, int len )
{
	if( freq == 0 ){
		//sleep_ms( len );										// 0?��?��?��͂͋x?��?��
	}else{
		//if( beep_off==false ){
	    int beep_freq = freq * pow( 2, pitch - 4 );	            // ?��?��?��g?��?��?��v?��Z
	    int beep_period = SYS_CLOCK / DIVISION / beep_freq;     // ?��?��?��?��?��v?��Z
	    int beep_duty = beep_period / 2;                        // Duty?��?��:50%
	    pwm_set_chan_level( beep_slice, beep_channel, beep_duty );  // Duty?��ݒ�
	    pwm_set_wrap( beep_slice, beep_period );                // ?��?��?��?��(?��?��?��g?��?��)?��ݒ�
	    //beep_off=true;
		pwm_set_enabled( beep_slice, true );// Beep?��?��?��?��
	    //add_alarm_in_ms( 10, alarm_callback, NULL, false );
		//}
	    sleep_ms( len );
	    pwm_set_enabled( beep_slice, false );                   // Beep?��?��~
	}
}