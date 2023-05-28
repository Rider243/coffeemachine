// #ifndef F_CPU

#define F_CPU 4000000UL // clock speed is 4MHz

// endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define LCD_Dir DDRD   /* Define LCD data port direction */
#define LCD_Port PORTD /* Define LCD data port */
#define RS PD0		   /* Define Register Select pin */
#define EN PD1
#define mucnuoc PA0
#define relay_bom PC0
#define relay_dun PC1

#define relay_van1 PC6
#define relay_van2 PC7

#define btn_1cf PB0
#define btn_start PB1
#define btn_2cf PB2
volatile unsigned dem = 0;
volatile unsigned tinh = 0;
volatile int soly = 0;
volatile int check = 0;
volatile int onecup = 150; // o=35;
volatile int twocup = 250; // t=70;

volatile int flowrate = 0;
volatile float flowratemililits = 0.0;
volatile float totalrate = 0.0;

volatile float flowmeter = 0.0;
volatile float calibrationFactor = 4.5;

void LCD_Command(unsigned char cmnd)
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0); /* sending upper nibble */
	LCD_Port &= ~(1 << RS);						  /* RS=0, command reg. */
	LCD_Port |= (1 << EN);						  /* Enable pulse */
	_delay_us(1);
	LCD_Port &= ~(1 << EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4); /* sending lower nibble */
	LCD_Port |= (1 << EN);
	_delay_us(1);
	LCD_Port &= ~(1 << EN);
	_delay_ms(2);
}

void LCD_Char(unsigned char data)
{
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0); /* sending upper nibble */
	LCD_Port |= (1 << RS);						  /* RS=1, data reg. */
	LCD_Port |= (1 << EN);
	_delay_us(1);
	LCD_Port &= ~(1 << EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (data << 4); /* sending lower nibble */
	LCD_Port |= (1 << EN);
	_delay_us(1);
	LCD_Port &= ~(1 << EN);
	_delay_ms(2);
}

void LCD_Init(void) /* LCD Initialize function */
{
	LCD_Dir = 0xFF; /* Make LCD port direction as o/p */
	_delay_ms(20);	/* LCD Power ON delay always >15ms */

	LCD_Command(0x02); /* send for 4 bit initialization of LCD  */
	LCD_Command(0x28); /* 2 line, 5*7 matrix in 4-bit mode */
	LCD_Command(0x0c); /* Display on cursor off*/
	LCD_Command(0x06); /* Increment cursor (shift cursor to right)*/
	LCD_Command(0x01); /* Clear display screen*/
	_delay_ms(2);
}

void LCD_String(char *str) /* Send string to LCD function */
{
	int i;
	for (i = 0; str[i] != 0; i++) /* Send each char of string till the NULL */
	{
		LCD_Char(str[i]);
	}
}

void LCD_String_xy(char row, char pos, char *str) /* Send string to LCD with xy position */
{
	if (row == 0 && pos < 16)
		LCD_Command((pos & 0x0F) | 0x80); /* Command of first row and required position<16 */
	else if (row == 1 && pos < 16)
		LCD_Command((pos & 0x0F) | 0xC0); /* Command of first row and required position<16 */
	LCD_String(str);					  /* Call LCD string function */
}

void LCD_Clear()
{
	LCD_Command(0x01); /* Clear display */
	_delay_ms(2);
	LCD_Command(0x80); /* Cursor at home position */
}
/////////////////////////////////////////////////

void bom_nuoc()
{
	LCD_Command(0x01);
	LCD_String("Pumping   "); /* Write string on 1st line of LCD*/
	DDRC = 0xff;
	PORTC = PORTC | (1 << relay_bom);
	PORTC = PORTC | (1 << relay_van1);
	_delay_ms(5000);
}

void xanuoc()
{
	if (PINB & (1 << btn_1cf))
	{
		PORTC = PORTC & ~(1 << relay_van2);
	}
	else
	{
		PORTC = PORTC | (1 << relay_van2);
		PORTC = PORTC & ~(1 << relay_van1);
	}
}

void cho_lenh()
{
	LCD_Command(0x01);
	LCD_String("Ready        "); /* Write string on 1st line of LCD*/
}

void pha_1cafe()
{
	if (soly == 1)
	{
		PORTC = PORTC | (1 << relay_van2);
		PORTC = PORTC | (1 << relay_bom);
		totalrate = totalrate + flowratemililits;
		flowratemililits = 0;
		LCD_Command(0x01);
		LCD_String("Making single         ");
		if (totalrate >= onecup)
		{
			totalrate = 0;
			PORTC = PORTC & ~(1 << relay_bom);
			PORTC = PORTC & ~(1 << relay_van2);
			soly = 0;
			check = 1;
		}
	}
}

void pha_2cafe()
{
	if (soly == 2)
	{
		PORTC = PORTC | (1 << relay_van2);
		PORTC = PORTC | (1 << relay_bom);
		LCD_Command(0x01);
		LCD_String("Making double");
		totalrate = totalrate + flowratemililits;
		flowratemililits = 0;
		if (totalrate >= twocup)
		{
			totalrate = 0;
			PORTC = PORTC & ~(1 << relay_bom);
			PORTC = PORTC & ~(1 << relay_van2);
			soly = 0;
			check = 1;
		}
	}
}
void btn()
{
	if (PINB & (1 << btn_1cf)) // if PIN5 of port C is high

	{
		cho_lenh();
	}
	else
	{
		_delay_ms(200);
		soly = 1;
		flowmeter = 0;
	}

	if (PINB & (1 << btn_2cf))
	{
	}
	else
	{
		_delay_ms(200);
		soly = 2;
		flowmeter = 0;
	}
}

void bao_loi()
{
	LCD_Command(0x01);
	LCD_String("ERROR ");	/* Write string on 1st line of LCD*/
	LCD_Command(0xC0);		/* Go to 2nd line*/
	LCD_String("NO WATER"); /* Write string on 2nd line*/
	PORTC = PORTC & ~(1 << relay_bom);
	PORTC = PORTC & ~(1 << relay_dun);
}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 130; // luu gia tri dem
	dem++;
}

ISR(INT0_vect)
{
	flowmeter++;
}

ISR(TIMER1_OVF_vect)
{

	flowratemililits = flowmeter * 10 / 27;
	flowmeter = 0;
	TCNT1 = 59285; // gan gia tri khoi tao cho T/C1
}

int main(void) // main starts

{
	TCCR0 = (1 << CS00) | (1 << CS01); // dung bo chia 64
	TCNT0 = 130;					   // thanh ghi luu gia tri dem theo cong thuc 255-(time*xungclock)/bo chia
	TIMSK |= (1 << TOIE0);			   // cho phep ngat
	MCUCR |= (1 << ISC11) | (1 << ISC01);
	GICR |= (1 << INT0);

	TCCR1B = (1 << CS00) | (1 << CS01); // bo chia 64
	TCNT1 = 59285;						// gan gia tri khoi tao cho T/C1
	TIMSK = (1 << TOIE1);				// cho phep ngat khi co tran o T/C1

	sei();

	DDRC = 0xff;
	; // Make pin 4 of port D as a output
	PORTC = 0x00;
	DDRC = 0xff;
	DDRA = 0x00;
	DDRB = 0x00; // Make pin 5 of port B as a input
	DDRD = 0x00;
	PORTD = 0xff;
	LCD_Init(); /* Initialization of LCD*/

	while (1) // initialize while loop

	{
		pha_1cafe();
		pha_2cafe();

		if (PINB & (1 << btn_start))
		{
			// LCD_Command(0x01);
		}
		else
		{
			_delay_ms(200);
			check++;
		}

		if (check % 2 == 1)
		{

			if (PINA & (1 << mucnuoc) && a == 0)
			{
				dem = 0;
				tinh = 0;
				LCD_Command(0x01);
				PORTC = PORTC & ~(1 << relay_bom);
				PORTC = PORTC & ~(1 << relay_van1);
				PORTC = PORTC | (1 << relay_dun);
				btn();
			}

			if (PINA & (1 != mucnuoc))

			{
			}

			else
			{
				bom_nuoc();
				xanuoc();
				PORTC = PORTC & ~(1 << relay_dun);
				if (dem >= 15000)
				{
					tinh++;
					dem = 0;
					if (tinh >= 10)
					{
						bao_loi();
						break;
					}
				}
			}
		}
		else
		{
			LCD_Command(0x01);
		}

	} // while loop ends

} // main end
