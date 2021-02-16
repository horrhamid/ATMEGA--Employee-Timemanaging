#define F_CPU 8000000L
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/eeprom.h>
// keys
#define KEY_PRT 	PORTD
#define KEY_DDR		DDRD
#define KEY_PIN		PIND
#define CLOCK_PORT  PORTC
#define CLOCK_DDR   DDRC

/// start lcd
#define LCD_DPRT PORTB
#define LCD_DDDR DDRB
#define LCD_DPIN PINB
#define LCD_CPRT PORTA
#define LCD_CDDR DDRA
#define LCD_CPIN PINA
#define LCD_RS 7
#define LCD_RW 6
#define LCD_EN 5
/////


void lcdCommand( unsigned char cmnd )
{
	LCD_DPRT = cmnd;
	LCD_CPRT &= ~ (1 << LCD_RS);
	LCD_CPRT &= ~ (1 << LCD_RW);
	LCD_CPRT |= (1 << LCD_EN);
	_delay_us(1);
	LCD_CPRT &= ~ (1 << LCD_EN);
	_delay_us(100);
}
void lcdData( unsigned char data )
{
	LCD_DPRT = data;
	LCD_CPRT |= (1 << LCD_RS);
	LCD_CPRT &= ~(1 << LCD_RW);
	LCD_CPRT |= (1 << LCD_EN);
	_delay_us(1);
	LCD_CPRT &= ~(1 << LCD_EN);
	_delay_us(100);
}
void lcd_init()
{
	LCD_DDDR = 0xFF;
	LCD_CDDR = 0xFF;
	LCD_CPRT &= ~(1 << LCD_EN);
	
	_delay_us(2000);
	lcdCommand(0x38);
	lcdCommand(0x0E);
	lcdCommand(0x01);
	_delay_us(2000);
	lcdCommand(0x06);
}
void lcd_gotoxy(unsigned char x , unsigned char y)
{
	unsigned char firstCharAdr[] = {0x80,0xC0,0x94,0xD4};
	lcdCommand(firstCharAdr[y - 1] + x - 1);
	_delay_us(100);
}
void lcd_print(char* str)
{
	unsigned char i = 0;
	while(str[i] != 0)
	{
		_delay_ms(25);
		lcdData(str[i]);
		i++;
	}
}

void lcd_print_without_delay(char* str)
{
	unsigned char i = 0;
	while(str[i] != 0)
	{
		lcdData(str[i]);
		i++;
	}
}



/////// end lcd


// most dell
unsigned char userCount = 2;


// EEPROM MEM
unsigned char EEMEM myVar[10][5]; 

// FUNCTIONS 
void bootup();

//
unsigned char keypad[4][4] = {	{'7','4','1',' '},
{'8','5','2','0'},
{'9','6','3','='},
{'/','*','-','+'}	};

unsigned char colloc, rowloc;

char keyfind()
{
	while(1)
	{
		KEY_DDR = 0xF0;           /* set port direction as input-output */
		KEY_PRT = 0xFF;

		do
		{
			KEY_PRT &= 0x0F;      /* mask PORT for column read only */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F); /* read status of column */
		}while(colloc != 0x0F);
		
		do
		{
			do
			{
				_delay_ms(20);             /* 20ms key debounce time */
				colloc = (KEY_PIN & 0x0F); /* read status of column */
				}while(colloc == 0x0F);        /* check for any key press */
				
				_delay_ms (40);	            /* 20 ms key debounce time */
				colloc = (KEY_PIN & 0x0F);
			}while(colloc == 0x0F);

			/* now check for rows */
			KEY_PRT = 0xEF;            /* check for pressed key in 1st row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 0;
				break;
			}

			KEY_PRT = 0xDF;		/* check for pressed key in 2nd row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 1;
				break;
			}
			
			KEY_PRT = 0xBF;		/* check for pressed key in 3rd row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 2;
				break;
			}

			KEY_PRT = 0x7F;		/* check for pressed key in 4th row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 3;
				break;
			}
		}

		if(colloc == 0x0E)
		return(keypad[rowloc][0]);
		else if(colloc == 0x0D)
		return(keypad[rowloc][1]);
		else if(colloc == 0x0B)
		return(keypad[rowloc][2]);
		else
		return(keypad[rowloc][3]);
	}
//

int authenticate(unsigned char cards_count , char* card_number , char* entered_password, int nu,int np)
{
	int user = 0;
	int pass = 0;
	unsigned char time;
	char username;
	char password;
	for (int j = 0 ; j<nu; j++)
	{
		user += (((int)card_number[j]) * pow(10,(nu - (j + 1))));
	}
	for (int j = 0 ; j<np; j++)
	{
		pass += (((int)entered_password[j]) * pow(10,(nu - (j + 1))));
	}
	
	for (unsigned char i = 0 ; i < cards_count ; i++)
	{
		username = 	eeprom_read_byte(&myVar[i][0]);
		
		if(username == user)
		{
			password = 	eeprom_read_byte(&myVar[i][1]);
			if (pass == password)
			{
				// set time
				if (eeprom_read_byte(&myVar[i][2]) == 0)
				{
					time = CLOCK_PORT;
					eeprom_write_byte(&myVar[i][3],time);
					eeprom_write_byte(&myVar[i][2],1);
					return 1;
					
				}else{
					time = CLOCK_PORT;
					eeprom_write_byte(&myVar[i][4],time);
					eeprom_write_byte(&myVar[i][2],0);
					return 2;
					
				}
				
			}
			return 0;
		} 
	}
	return 0;
}


void init_admin()
{
	eeprom_write_byte(&myVar[0][0],'1');
	_delay_ms(20);
	eeprom_write_byte(&myVar[0][1],'1');
	_delay_ms(20);
	eeprom_write_byte(&myVar[0][2],0);
	_delay_ms(20);
	// init 1 user
	
	eeprom_write_byte(&myVar[1][0],96);
	_delay_ms(20);
	eeprom_write_byte(&myVar[1][1],44);
	_delay_ms(20);
	eeprom_write_byte(&myVar[1][2],0);
	_delay_ms(20);
}


void bootup()
{
	CLOCK_DDR = 0xFF;
	CLOCK_PORT = 0xFF;
	char opening1[50] = "Project - AVR Micro controller";
	char opening2[30] = "SADEGHI - POORYEGANE";
	init_admin();
	lcd_init();
	
	lcd_gotoxy(1 , 1);
	lcd_print(opening1);
	
	lcd_gotoxy(1 , 2);
	lcd_print(opening2);
	
	_delay_ms(1500);
	lcdCommand(0x01);
}

int main(void)
{
	int status = 0;
	unsigned char first_round_for_backspace_option = 0;
	unsigned char first_round_for_space_option = 1;
	char current_key;
	char card_number [30] = "";
	char password [30] = "";
	//char amount_str [30] = "";
	int n_u = 0,n_p = 0;
	char b;
	bootup();
	
   	while(1)
   	{
	   	L1:
		   memset(card_number, 0, 30);
		   memset(password, 0, 30);
	   	if(status == 0 /* first time ever */)
	   	{
		   	lcd_print("Enter Your Card Number : ");
		   	lcd_gotoxy(1 , 2);
			n_u = 0;
		   	while(1)
		   	{
			   	current_key = keyfind(); //card_number
			   	if(current_key == '=')
			   	{
				   	lcdCommand(0x01);
				   	lcd_gotoxy(1 , 1);
				   	lcd_print("Your Card : ");
				   	lcd_gotoxy(1 , 2);
				   	lcd_print(card_number);
				   	_delay_ms(1000);
				   	
				   	lcdCommand(0x01);
				   	lcd_gotoxy(1 , 1);
				   	lcd_print("Enter Your Password : ");
				   	lcd_gotoxy(1 , 2);
				   	
				   	first_round_for_backspace_option = 0; // re-init
				   	first_round_for_space_option = 1; // re-init
					n_p = 0;
				   	while(1)
				   	{
					   	current_key = keyfind(); //password
					   	if(current_key == '=')
					   	{
						   	b = authenticate(userCount , card_number , password, n_u,n_p);
						   	lcdCommand(0x01);
							if (b == 1)
							{
								lcd_gotoxy(1 , 1);
								lcd_print("Logged in Successfully -- Welcome");
								_delay_ms(1000);
								lcdCommand(0x01);
								
								status =0;
								goto L1;
							}
							else if (b == 2)
							{
								lcd_gotoxy(1 , 1);
								lcd_print("Logged out Successfully -- GoodBye");
								_delay_ms(1000);
								lcdCommand(0x01);
								status =0;
								goto L1;
							}
							else
							{
								lcd_gotoxy(1 , 1);
								lcd_print("Wrong username or password");
								lcd_gotoxy(1 , 2);
								_delay_ms(500);
								goto L1;	
							}
						     	
					   	}
					   	else if (current_key == '-')
					   	{

						   	lcdCommand(0x04);
						   	_delay_us(100);
						   	lcdData(' ');
						   	_delay_us(100);
						   	lcdCommand(0x06);
						   	_delay_us(100);
						   	

						   	if (first_round_for_backspace_option != 0)
						   	{
							   	//truncating the deleted number
							   	unsigned char password_len = strlen(password);
							   	password[password_len - 1] = 0;
						   	}
						   	else if(first_round_for_backspace_option == 0)
						   	{
							   	first_round_for_space_option = 0;
							   	first_round_for_backspace_option = 1;
						   	}


					   	}
					   	else
					   	{
						   	if(first_round_for_space_option == 0)
						   	{
							   	first_round_for_space_option = 1;
							   	first_round_for_backspace_option = 0;
						   	}
						   	else if(first_round_for_space_option != 0)
						   	{
							   	strncat(password , &current_key , 1);
							   	lcdData('*');
								n_p++;
						   	}
					   	}

				   	}
			   	}
			   	else if (current_key == '-')
			   	{

				   	lcdCommand(0x04);
				   	_delay_us(100);
				   	lcdData(' ');
				   	_delay_us(100);
				   	lcdCommand(0x06);
				   	_delay_us(100);
				   	
				   	if (first_round_for_backspace_option != 0)
				   	{
					   	//truncating the deleted number
					   	unsigned char card_number_len = strlen(card_number);
					   	card_number[card_number_len - 1] = 0;
				   	}
				   	else if(first_round_for_backspace_option == 0)
				   	{
					   	first_round_for_space_option = 0;
					   	first_round_for_backspace_option = 1;
				   	}
				   	

			   	}
			   	else if (current_key == ' ')
			   	{
					
				   	lcdCommand(0x01);
				   	_delay_us(100);
				   	lcd_gotoxy(1 , 1);
				   	_delay_us(100);
				   	lcd_print("Enter Your Card Number : ");
				   	_delay_us(100);
				   	lcd_gotoxy(1 , 2);
				   	strcpy(card_number , "");
				   	
			   	}
			   	else
			   	{
				   	if(first_round_for_space_option != 0)
				   	{
					   	strncat(card_number , &current_key , 1);
					   	lcdData(current_key);
						n_u++;
				   	}
				   	else if(first_round_for_space_option == 0)
				   	{
					   	first_round_for_space_option = 1;
					   	first_round_for_backspace_option = 0;
				   	}
			   	}

		   	}
	   	}	
   	}
}

