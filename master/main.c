/*
 By Liyanboy74
 https://github.com/liyanboy74
*/

#include <mega8.h>
#include <alcd.h>
#include <stdio.h>
#include <delay.h>

#include "../modbus/mb.h"
#include "../modbus/mb-packet.h"

char LCD_Buffer[64];

int RX1C=0,RX1EC=0;
int RX2C=0,RX2EC=0;
int RX3C=0,RX3EC=0;

void timer_reset()
{
   TCNT0=0x00;
}

// USART Receiver interrupt service routine
interrupt [USART_RXC] void usart_rx_isr(void)
{
    mb_rx_new_data(UDR);
    timer_reset();
}

void USART_Transmit( unsigned char data )
{
/* Wait for empty transmit buffer */
while ( !( UCSRA & (1<<UDRE)) )
;
/* Put data into buffer, sends the data */
UDR = data;
}

void send_data(uint8_t *Data,uint8_t Len)
{
   int i;
   timer_reset();
   for(i=0;i<Len;i++)USART_Transmit(Data[i]);
}

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
   // Reinitialize Timer 0 value
   timer_reset();
   mb_rx_timeout_handler();
}

void master_process(mb_packet_s Packet)
{
    int Data;
    if(Packet.type==MB_PACKET_TYPE_ERROR)
    {
     if(Packet.device_address==0x01)RX1EC++;
     else if(Packet.device_address==0x02)RX2EC++;  
     else if(Packet.device_address==0x03)RX3EC++;    

     sprintf(LCD_Buffer,"ERROR 1:%02d 2:%02d 3:%02d",RX1EC,RX2EC,RX3EC);
     lcd_gotoxy(0,3);
     lcd_puts(LCD_Buffer);
    }
    else if(Packet.func==MB_FUNC_Read_Input_Registers)
    {
     if(Packet.device_address==0x01)
     {
     Data=Packet.Data[1]|(Packet.Data[0]<<8);
     sprintf(LCD_Buffer,"[%02d] A:%02x D:%04d",RX1C++%100,Packet.device_address,Data);
     lcd_gotoxy(0,0);
     lcd_puts(LCD_Buffer);
     }  
     else if(Packet.device_address==0x02)
     {
     Data=Packet.Data[1]|(Packet.Data[0]<<8);
     sprintf(LCD_Buffer,"[%02d] A:%02x D:%04d",RX2C++%100,Packet.device_address,Data);
     lcd_gotoxy(0,1);
     lcd_puts(LCD_Buffer);
     }
     else if(Packet.device_address==0x03)
     {
     Data=Packet.Data[1]|(Packet.Data[0]<<8);
     sprintf(LCD_Buffer,"[%02d] A:%02x D:%04d",RX3C++%100,Packet.device_address,Data);
     lcd_gotoxy(0,2);
     lcd_puts(LCD_Buffer);
     }
    }
}

void main(void)
{

//UART GPIO TX RX Config IO
DDRD=0x02;
PORTD=0x3;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 7/813 kHz
TCCR0=(1<<CS02) | (0<<CS01) | (1<<CS00);
TCNT0=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (0<<TOIE1) | (1<<TOIE0);


// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver: On
// USART Transmitter: On
// USART Mode: Asynchronous
// USART Baud Rate: 9600
UCSRA=(0<<RXC) | (0<<TXC) | (0<<UDRE) | (0<<FE) | (0<<DOR) | (0<<UPE) | (0<<U2X) | (0<<MPCM);
UCSRB=(1<<RXCIE) | (0<<TXCIE) | (0<<UDRIE) | (1<<RXEN) | (1<<TXEN) | (0<<UCSZ2) | (0<<RXB8) | (0<<TXB8);
UCSRC=(1<<URSEL) | (0<<UMSEL) | (0<<UPM1) | (0<<UPM0) | (0<<USBS) | (1<<UCSZ1) | (1<<UCSZ0) | (0<<UCPOL);
UBRRH=0x00;
UBRRL=0x33;

// Alphanumeric LCD initialization
// Connections are specified in the
// Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
// RS - PORTB Bit 0
// RD - PORTB Bit 1
// EN - PORTB Bit 2
// D4 - PORTB Bit 4
// D5 - PORTB Bit 5
// D6 - PORTB Bit 6
// D7 - PORTB Bit 7
// Characters/line: 20
lcd_init(20);

mb_set_tx_handler(&send_data);
mb_set_master_process_handler(&master_process);

// Global enable interrupts
#asm("sei")

lcd_puts("Starting...");
delay_ms(500);

while (1)
      {
       mb_tx_packet_handler(mb_packet_request_read_input_registers(0x01,0x0000,0x0001));
       delay_ms(100);
       mb_tx_packet_handler(mb_packet_request_read_input_registers(0x02,0x0000,0x0001));
       delay_ms(100);
       mb_tx_packet_handler(mb_packet_request_read_input_registers(0x03,0x0000,0x0001));
       delay_ms(100);
      }
}
