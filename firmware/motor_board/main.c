#include <mega128.h>
#include <delay.h>

// 초기값은 멈춰있는 상태에서 시작이라는 것을 가정
volatile unsigned char motor_speed = 0;     // 현재 모터 속도
volatile unsigned char saved_motor_speed = 160; // 멈추기 직전 속도 저장
volatile unsigned char motor_stop_flag = 1;  // 모터 정지 플래그 (0=동작, 1=정지)

// Timer 0을 8비트 고속 PWM 모드로 초기화
void pwm_init_timer0(void) {
    DDRB.4 = 1;
    TCCR0 = (1 << WGM01) | (1 << WGM00) | (1 << COM01) | (1 << CS01) | (1 << CS00);
    OCR0 = 0; 
}

// 모터 속도 설정
void motor_set_speed(unsigned char speed) {
    OCR0 = speed; 
}

// INT0 - 속도 증가
interrupt [EXT_INT0] void int0_isr(void) {
    if (motor_stop_flag == 0) { 
        if (motor_speed <= 245) {
            motor_speed += 10;
        }
        else {
            motor_speed = 254;
        }
    }
}

// INT1 - 속도 감소
interrupt [EXT_INT1] void int1_isr(void) {
    if (motor_stop_flag == 0) {
        if (motor_speed >= 20) {
            motor_speed -= 10;
        }
        else {
            motor_speed = 1; // 최소 속도
        }
    }   
}

// INT2 - 일시정지 / 재시작
interrupt [EXT_INT2] void int2_isr(void) {
    if (motor_stop_flag == 0) {
        saved_motor_speed = motor_speed;
        motor_speed = 0;
        motor_stop_flag = 1;
    }
    else {
        motor_speed = saved_motor_speed;
        motor_stop_flag = 0;
    }   
}

void main(void) {
    DDRA.0 = 1;
    DDRA.1 = 1;
    
    // 2. PWM 초기화 (Timer 0)
    pwm_init_timer0();
    
    PORTD.0 = 1;
    PORTD.1 = 1;
    PORTD.2 = 1;
    
    EICRA = 0x2a;
    EIMSK = 0x07;
    
    PORTA.0 = 1;    // 모터 방향 설정
    PORTA.1 = 0;
    
    #asm("sei")

    while (1) {
        motor_set_speed(motor_speed);   // 주기적으로 반복하며 인터럽트에서 변경된 속도값을 PWM에 적용
        
        delay_ms(10); 
    }
}