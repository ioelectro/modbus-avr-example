#include <mega8.h>
#include <stdio.h>
#include <delay.h>

#include "modbus/mb.h"
#include "modbus/mb-table.h"

// Voltage Reference: Int., cap. on AREF
#define ADC_VREF_TYPE ((1<<REFS1) | (1<<REFS0) | (0<<ADLAR))

// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
ADMUX=adc_input | ADC_VREF_TYPE;
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
// Start the AD conversion
ADCSRA|=(1<<ADSC);
// Wait for the AD conversion to complete
while ((ADCSRA & (1<<ADIF))==0);
ADCSRA|=(1<<ADIF);
return ADCW;
}

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

int get_adc(char ch)
{
    int i=0,sum=0;
    for(i=0;i<20;i++)
    {   
        sum+=read_adc(ch);
    }
    return (int)(sum/20.0);
}

void main(void)
{
int adc,temp;

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

// ADC initialization
// ADC Clock frequency: 1000/000 kHz
// ADC Voltage Reference: Int., cap. on AREF
ADMUX=ADC_VREF_TYPE;
ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADFR) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
SFIOR=(0<<ACME);

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

mb_slave_address_set(0x01);
mb_set_tx_handler(&send_data);

// Global enable interrupts
#asm("sei")

while (1)
      {
      // Place your code here
       adc=get_adc(0); 
       temp=adc/0.04;  
       mb_table_write(TBALE_Input_Registers,0,temp);
       delay_ms(100);
      }
}
