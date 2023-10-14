//################################################
// ?øΩ?øΩ?øΩY?øΩs?øΩR?øΩL?øΩb?øΩg(Raspberry Pi Pico Kit) C-Version
// BEEP?øΩ?øΩ?øΩ?øΩ÷êÔøΩ (2022.7.1?øΩ`)
//################################################

//************************************************
// ?øΩs?øΩ?øΩ?øΩ?øΩ`
//************************************************
#define BEEP        15        // ?øΩu?øΩU?øΩ[
//17
//************************************************
// ?øΩ}?øΩN?øΩ?øΩ?øΩ?øΩ` : ?øΩ?øΩ?øΩK?øΩ÷åW
//************************************************
#define SYS_CLOCK	125000000 // 125MHz
#define DIVISION	100		  // ?øΩ?øΩ?øΩ?øΩ?øΩ?øΩ

// Sound Pitch : ?øΩ˚´Ä?øΩ?øΩ?øΩg?øΩ?øΩ
#define TONE_DO		262		  // ?øΩh
#define TONE_DS		277		  // ?øΩh#
#define TONE_RE		294		  // ?øΩ?øΩ
#define TONE_RS		311		  // ?øΩ?øΩ#
#define TONE_MI		330		  // ?øΩ~
#define TONE_FA		349		  // ?øΩt?øΩ@
#define TONE_FS		370		  // ?øΩt?øΩ@#
#define TONE_SO		392		  // ?øΩ\
#define TONE_SS		415		  // ?øΩ\#
#define TONE_LA		440		  // ?øΩ?øΩ
#define TONE_LS		466		  // ?øΩ?øΩ#
#define TONE_TI		494		  // ?øΩV?øΩi?øΩp?øΩÍÇæ?øΩ∆Ée?øΩB?øΩj

//************************************************
// ?øΩO?øΩ?øΩ?øΩ[?øΩo?øΩ?øΩ?øΩœêÔøΩ?øΩ?øΩ`
//************************************************
uint beep_slice;		      // Beep?øΩpPWM slice?øΩ‘çÔøΩ
uint beep_channel;            // Beep?øΩpPWM channnel
int beep_off=false;
//************************************************
// ?øΩ÷êÔøΩ?øΩv?øΩ?øΩ?øΩg?øΩ^?øΩC?øΩv?øΩÈåæ
//************************************************
void beep_init( void );
void beep( int freq, int pitch, int len );
//int64_t alarm_callback( alarm_id_t id, void *user_data );
//------------------------------------------------
// Beep?øΩpPWM?øΩ?øΩ?øΩ?øΩ?øΩ?øΩ
//------------------------------------------------
void beep_init( void )
{
	gpio_set_function( BEEP, GPIO_FUNC_PWM );     // Beep?øΩ?øΩGPIO?øΩ?øΩPWM?øΩ›íÔøΩ
	beep_slice = pwm_gpio_to_slice_num( BEEP );   // slice?øΩ‘çÔøΩ?øΩÊìæ
	beep_channel = pwm_gpio_to_channel( BEEP );   // channnel?øΩÊìæ
	pwm_set_clkdiv( beep_slice, DIVISION );       // ?øΩ?øΩ?øΩ?øΩ?øΩ?øΩ?øΩ›íÔøΩ
}

//------------------------------------------------
// Beep?øΩ÷êÔøΩ
//     freq  = ?øΩ?øΩ{?øΩ?øΩ?øΩg?øΩ?øΩ?øΩi?øΩh?øΩ?øΩ?øΩ~?øΩj:?øΩs?øΩb?øΩ`4?øΩÃéÔøΩ?øΩg?øΩ?øΩ[Hz]
//     pitch = ?øΩ?øΩ?øΩ?øΩ?øΩ@4?øΩ?øΩ?øΩ˚´Ä?øΩ?øΩ?øΩK?øΩC5?øΩ?øΩ?øΩg?øΩ?øΩ?øΩ‚Ç∑?øΩ?øΩ
//     len   = ?øΩ?øΩ?øΩÃíÔøΩ?øΩ?øΩ[ms]
//------------------------------------------------
/*int64_t alarm_callback( alarm_id_t id, void *user_data ){
    pwm_set_enabled( beep_slice, false );
    beep_off=false;
	return 0;
}*/
void beep( int freq, int pitch, int len )
{
	if( freq == 0 ){
		//sleep_ms( len );										// 0?øΩ?øΩ?øΩÕÇÕãx?øΩ?øΩ
	}else{
		//if( beep_off==false ){
	    int beep_freq = freq * pow( 2, pitch - 4 );	            // ?øΩ?øΩ?øΩg?øΩ?øΩ?øΩv?øΩZ
	    int beep_period = SYS_CLOCK / DIVISION / beep_freq;     // ?øΩ?øΩ?øΩ?øΩ?øΩv?øΩZ
	    int beep_duty = beep_period / 2;                        // Duty?øΩ?øΩ:50%
	    pwm_set_chan_level( beep_slice, beep_channel, beep_duty );  // Duty?øΩ›íÔøΩ
	    pwm_set_wrap( beep_slice, beep_period );                // ?øΩ?øΩ?øΩ?øΩ(?øΩ?øΩ?øΩg?øΩ?øΩ)?øΩ›íÔøΩ
	    //beep_off=true;
		pwm_set_enabled( beep_slice, true );// Beep?øΩ?øΩ?øΩ?øΩ
	    //add_alarm_in_ms( 10, alarm_callback, NULL, false );
		//}
	    sleep_ms( len );
	    pwm_set_enabled( beep_slice, false );                   // Beep?øΩ?øΩ~
	}
}