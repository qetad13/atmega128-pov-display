#include <mega128.h>
#include <delay.h>

#define SER   0  // PORTA.0     DS (Serial Data)
#define SRCLK 1  // PORTA.1     SHCP (Shift Clock)
#define RCLK  2  // PORTA.2     STCP (Latch Clock)

// 변수 선언
volatile unsigned int led_state = 0; // LED 상태를 저장(0부터 5까지 6가지 색상)
int isa_flag = 0;
int heart_flag = 0;
int delay_time = 1;
volatile unsigned int led_shape = 0x0000;
volatile unsigned int uptime = 4;
volatile unsigned int uptime_flag = 0;  // 0이 밝아지기 1이 어두워지기 
volatile unsigned int led_R = 0x0000;  // shiftOutRGB 함수에 전달할 R값
volatile unsigned int led_G = 0xFFFF;  // G값
volatile unsigned int led_B = 0xFFFF;  // B값

// 함수 정의
void update_led_colors();
void bright();
void shift_16_bits(unsigned int data);
void shiftOutRGB(unsigned int r_data, unsigned int g_data, unsigned int b_data);

void update_led_colors() {
    switch(led_state) {
        case 0: // 빨간색
            led_R = led_shape;
            led_G = 0xFFFF;
            led_B = 0xFFFF;
            break;
        case 1: // 초록색
            led_R = 0xFFFF;
            led_G = led_shape;
            led_B = 0xFFFF;
            break;
        case 2: // 파란색
            led_R = 0xFFFF;
            led_G = 0xFFFF;
            led_B = led_shape;
            break;
        case 3: // 빨,초 : 노란색
            led_R = led_shape;
            led_G = led_shape;
            led_B = 0xFFFF;
            break;
        case 4: // 빨, 파 : 마젠타
            led_R = led_shape;
            led_G = 0xFFFF;
            led_B = led_shape;
            break;
        case 5: // 초, 파 : 청록색
            led_R = 0xFFFF;
            led_G = led_shape;
            led_B = led_shape;
            break;
    }
}
void bright() {
    update_led_colors();
    
    shiftOutRGB(led_R, led_G, led_B);
        
    delay_ms(uptime);
    shiftOutRGB(0xFFFF, 0xFFFF, 0xFFFF);
    delay_ms(7-uptime);
}
// 인터럽트
interrupt [EXT_INT0] void ext_int0_isr(void) {
    led_state++;    // 버튼 한 번 누를 때 마다 하나 씩 증가
    if(led_state > 5) { // 청록색 다음에는 다시 빨간색으로 
        led_state = 0;
    }
    
    update_led_colors();
}

interrupt [EXT_INT1] void ext_int1_isr(void) {
    if (uptime_flag == 0) {    
        if (uptime > 0) {
            uptime--;
        }
        
        if (uptime == 0) { 
            uptime_flag = 1;
            uptime++;
        }                           
    }
    if (uptime_flag == 1) { 
        if (uptime < 7) {
            uptime++;
        }
        
        if (uptime == 7) {
            uptime_flag = 0;
            uptime--;
        }
    }
}

interrupt [EXT_INT2] void ext_int2_isr(void) {
    isa_flag = 1;
    heart_flag = 0;
}

interrupt [EXT_INT3] void ext_int3_isr(void) {
    heart_flag = 1;
    isa_flag = 0;
}


// 16비트 데이터를 시프트 해주는 함수
void shift_16_bits(unsigned int data) {
    unsigned char i;

    for(i=0;i<16;i++) {
        // 최상위 비트(MSB)부터 전송
        if(data & 0x8000) PORTA |= (1<<SER);
        else              PORTA &= ~(1<<SER);

        // 시프트 클럭 ↑↓ (데이터 1비트 이동)
        PORTA |= (1<<SRCLK);
        PORTA &= ~(1<<SRCLK);

        data <<= 1; // 다음 비트 준비
    }
}

// 48비트 (16비트 x 3) 데이터 출력 함수
void shiftOutRGB(unsigned int r_data, unsigned int g_data, unsigned int b_data) {
    // Blue 데이터부터 전송, B -> G -> R
    shift_16_bits(b_data); 
    shift_16_bits(g_data);
    shift_16_bits(r_data);

    // 48비트 전송이 모두 완료되면 래치 클럭(RCLK) -> 출력
    PORTA |= (1<<RCLK);
    PORTA &= ~(1<<RCLK);
}

void main(void) {
     // 입출력 포트 설정
     DDRA = 0x07;  // 00000111, PA0,1,2 출력
     PORTA = 0x00;
     DDRD = 0x00;   // PD0,1,2,3 핀 입력
     PORTD = 0X0F; // PD0,1,2,3 핀 풀업저항 활성화

     // 외부 인터럽트 설정
     EIMSK = 0x0F;  // INT 0,1,2,3 인터럽트 활성화
     EICRA = 0xAA;  // INT 0,1,2,3 하강에지
 
     #asm("sei") // 전역 인터럽트 활성화

    update_led_colors(); // led_state(0)에 맞는 R,G,B 값 로드

    // R,G,B 한 번씩 출력
    shiftOutRGB(0x0000, 0xFFFF, 0xFFFF);    
    delay_ms(300);
    shiftOutRGB(0xFFFF, 0xFFFF, 0xFFFF);
    delay_ms(300);    
    shiftOutRGB(0xFFFF, 0x0000, 0xFFFF);    
    delay_ms(300);
    shiftOutRGB(0xFFFF, 0xFFFF, 0xFFFF);
    delay_ms(300);
    shiftOutRGB(0xFFFF, 0xFFFF, 0x0000);
    delay_ms(300);
    
    
    while(1) {
        bright();
        
        if (isa_flag == 1) {
            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xfbef; bright(); delay_ms(delay_time);
            led_shape = 0xf80f; bright(); delay_ms(delay_time);
            led_shape = 0xfbef; bright(); delay_ms(delay_time);
            led_shape = 0xfbef; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);

            led_shape = 0xff9f; bright(); delay_ms(delay_time);
            led_shape = 0xfb6f; bright(); delay_ms(delay_time);
            led_shape = 0xfb6f; bright(); delay_ms(delay_time);
            led_shape = 0xfb6f; bright(); delay_ms(delay_time);
            led_shape = 0xfb6f; bright(); delay_ms(delay_time);
            led_shape = 0xfcff; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);

            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xf81f; bright(); delay_ms(delay_time);
            led_shape = 0xff6f; bright(); delay_ms(delay_time);
            led_shape = 0xff6f; bright(); delay_ms(delay_time);
            led_shape = 0xf81f; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);
        }
        
        if (heart_flag == 1) {
            led_shape = 0xffff; bright(); delay_ms(delay_time);
            led_shape = 0xfe7f; bright(); delay_ms(delay_time);
            led_shape = 0xf81f; bright(); delay_ms(delay_time);
            led_shape = 0xe007; bright(); delay_ms(delay_time);
            led_shape = 0x8001; bright(); delay_ms(delay_time);
            led_shape = 0x0007; bright(); delay_ms(delay_time);
            led_shape = 0x0007; bright(); delay_ms(delay_time);
            led_shape = 0x8001; bright(); delay_ms(delay_time);
            led_shape = 0xe007; bright(); delay_ms(delay_time);
            led_shape = 0xf81f; bright(); delay_ms(delay_time);
            led_shape = 0xfe7f; bright(); delay_ms(delay_time);
            led_shape = 0xffff; bright(); delay_ms(delay_time);
        } 
    }
}