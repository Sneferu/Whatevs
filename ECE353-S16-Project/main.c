//*****************************************************************************
// main.c
// Author: jkrachey@wisc.edu
//*****************************************************************************
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "TM4C123.h"
#include "adc.h"
#include "pc_buffer.h"
#include "gpio_port.h"
#include "interrupts.h"
#include "uart_irqs.h"
#include "eeprom.h"
#include "lcd.h"
#include "timers.h"
#include "wireless.h"

#define   PS2_GPIO_BASE    GPIOE_BASE

#define   PS2_X_PIN_NUM      3
#define   PS2_Y_PIN_NUM      2

#define   PS2_X_DIR_MASK     (1 << PS2_X_PIN_NUM)
#define   PS2_Y_DIR_MASK     (1 << PS2_Y_PIN_NUM)

#define   DIR_BTN_BASE    GPIOF_BASE

#define   DIR_BTN_UP      (1 << 1)
#define   DIR_BTN_DOWN    (1 << 4)
#define   DIR_BTN_LEFT    (1 << 3)
#define   DIR_BTN_RIGHT   (1 << 2)

#define   PS2_BTN         (1 << 0)

#define   PS2_X_PIN_NUM      3
#define   PS2_Y_PIN_NUM      2

#define   PS2_X_DIR_MASK     (1 << PS2_X_PIN_NUM)
#define   PS2_Y_DIR_MASK     (1 << PS2_Y_PIN_NUM)

#define   PS2_ADC_BASE     ADC0_BASE


// Set the analog channel for each direction.  Each analog channel is associated with a
// single GPIO pin.  Make sure to see table 13-1 to see how the GPIO pin maps to an 
// analog channel.
#define   PS2_X_ADC_CHANNEL  1
#define   PS2_Y_ADC_CHANNEL  0


#define		EEPROM_NAME1	0
#define		EEPROM_NAME2	80
#define		EEPROM_NUMBER	160
#define   EEPROM_SCORE 165
#define		EEPROM_MARKER 170

/******************************************************************************
 * Global Variables
 *****************************************************************************/

PC_Buffer UART0_Rx_Buffer;
PC_Buffer UART0_Tx_Buffer;
extern volatile bool ALERT_5_MS;
extern volatile bool ALERT_5_S;

extern volatile bool BTN_PS2;

extern volatile int PACKETS_R;
extern volatile int PACKETS_S;

extern volatile bool BTN_UP;
extern volatile bool BTN_DOWN;
extern volatile bool BTN_LEFT;
extern volatile bool BTN_RIGHT;

extern volatile bool PS2_RIGHT;
extern volatile bool PS2_UP;
extern volatile bool PS2_DOWN;
extern volatile bool PS2_LEFT;


extern volatile bool ALERT_MOVE;

uint8_t myID[]      = { '1', '2', '3', '4', '5'};
uint8_t remoteID[]  = { '5', '4', '3', '2', '1'};
bool amRemote = false;

char output[80];

//*****************************************************************************
//*****************************************************************************
void DisableInterrupts(void)
{
  __asm {
    CPSID  I
  }
}

//*****************************************************************************
//*****************************************************************************
void EnableInterrupts(void)
{
  __asm {
    CPSIE  I
  }
}


//*****************************************************************************
//*****************************************************************************

void init_LED(){
	i2cSetSlaveAddr(EEPROM_I2C_BASE, 0x20, I2C_WRITE); //LED address
 
  // Send the IODIRA Address
  i2cSendByte( EEPROM_I2C_BASE, 0x00, I2C_MCS_START | I2C_MCS_RUN);
 
  // Set PortA to be outputs
  i2cSendByte( EEPROM_I2C_BASE, 0x00, I2C_MCS_RUN | I2C_MCS_STOP);
 
  // Send the IODIRB Address
  i2cSendByte( EEPROM_I2C_BASE, 0x01, I2C_MCS_START | I2C_MCS_RUN);
 
  // Set PortB to be outputs
  i2cSendByte( EEPROM_I2C_BASE, 0x00, I2C_MCS_RUN | I2C_MCS_STOP);
}

void drawLEDs(){
	int i, vals[5] = {0x30, 0x36, 0x36, 0x36, 0x06};
	i2cSetSlaveAddr(EEPROM_I2C_BASE, 0x20, I2C_WRITE); //LED address
	for(i=0;i<5;i++){
		
		while(!ALERT_5_MS);
		//Write column
		i2cSendByte( EEPROM_I2C_BASE, 0x13, I2C_MCS_START | I2C_MCS_RUN);
		i2cSendByte( EEPROM_I2C_BASE, (uint8_t)(1<<i), I2C_MCS_RUN | I2C_MCS_STOP);
	
		i2cSendByte( EEPROM_I2C_BASE, 0x12, I2C_MCS_START | I2C_MCS_RUN);
		i2cSendByte( EEPROM_I2C_BASE, vals[i], I2C_MCS_RUN | I2C_MCS_STOP);
		
		ALERT_5_MS = false; 
	}
}

char* n1 = "Mackenzie Scanlan";
char* n2 = "Marshall Stutz";
char* tn = "37";
void checkForNewBoard(){
	int i;
  eeprom_byte_read(EEPROM_I2C_BASE,EEPROM_MARKER,(uint8_t*)&output[0]);
	if(output[0]==0xA5) return; //Special code
	for(i = 0; i < 18; i++){
    eeprom_byte_write(EEPROM_I2C_BASE,i+EEPROM_NAME1,n1[i]);
  }
	for(i = 0; i < 15; i++){
    eeprom_byte_write(EEPROM_I2C_BASE,i+EEPROM_NAME2,n2[i]);
  }
	for(i = 0; i < 3; i++){
    eeprom_byte_write(EEPROM_I2C_BASE,i+EEPROM_NUMBER,tn[i]);
  }
	eeprom_byte_write(EEPROM_I2C_BASE,EEPROM_MARKER,0xA5);
}

void initializeBoard(){
	
	DisableInterrupts();
	//Enables the four push buttons
	gpio_enable_port(DIR_BTN_BASE);
	gpio_config_digital_enable(DIR_BTN_BASE, DIR_BTN_UP | DIR_BTN_DOWN | DIR_BTN_LEFT | DIR_BTN_RIGHT);
	gpio_config_enable_input(DIR_BTN_BASE, DIR_BTN_UP | DIR_BTN_DOWN | DIR_BTN_LEFT | DIR_BTN_RIGHT);
	gpio_config_enable_pullup(DIR_BTN_BASE, DIR_BTN_UP | DIR_BTN_DOWN | DIR_BTN_LEFT | DIR_BTN_RIGHT);
	
	//Enables the joystick
	gpio_enable_port(PS2_GPIO_BASE);
	gpio_config_enable_input(PS2_GPIO_BASE, PS2_X_DIR_MASK | PS2_Y_DIR_MASK | PS2_BTN);
	gpio_config_digital_enable(PS2_GPIO_BASE, PS2_BTN);
	gpio_config_enable_pullup(PS2_GPIO_BASE, PS2_BTN);
	gpio_config_analog_enable(PS2_GPIO_BASE, PS2_X_DIR_MASK | PS2_Y_DIR_MASK);
  gpio_config_alternate_function(PS2_GPIO_BASE, PS2_X_DIR_MASK | PS2_Y_DIR_MASK);
  
	// Configure the UART
	gpio_enable_port(GPIOA_BASE);
	gpio_config_digital_enable(GPIOA_BASE, PA0 | PA1);
	gpio_config_alternate_function(GPIOA_BASE, PA0 | PA1);
	gpio_config_port_control(GPIOA_BASE, GPIO_PCTL_PA0_M&GPIO_PCTL_PA0_U0RX);
	gpio_config_port_control(GPIOA_BASE, GPIO_PCTL_PA1_M&GPIO_PCTL_PA1_U0TX);
	
		// Configure SCL 
	gpio_enable_port(EEPROM_GPIO_BASE);
	gpio_config_digital_enable(EEPROM_GPIO_BASE, EEPROM_I2C_SCL_PIN);
	gpio_config_alternate_function(EEPROM_GPIO_BASE, EEPROM_I2C_SCL_PIN);
	gpio_config_port_control(EEPROM_GPIO_BASE, EEPROM_I2C_SCL_PCTL_M&EEPROM_I2C_SCL_PIN_PCTL);

	// Configure SDA 
	gpio_config_digital_enable(EEPROM_GPIO_BASE, EEPROM_I2C_SDA_PIN);
	gpio_config_open_drain(EEPROM_GPIO_BASE, EEPROM_I2C_SDA_PIN);
	gpio_config_alternate_function(EEPROM_GPIO_BASE, EEPROM_I2C_SDA_PIN);
	gpio_config_port_control(EEPROM_GPIO_BASE, EEPROM_I2C_SDA_PCTL_M&EEPROM_I2C_SDA_PIN_PCTL);
	
	//Initialize radio
	wireless_initialize();
	
		//Initialize timers
	project_timers_init();

  //Initialize UART
	initialize_uart();

	//  Initialize the I2C peripheral
	initializeI2CMaster(EEPROM_I2C_BASE);
	
	//Initialize adc;
	project_adc_init();
	
	
	//Initialize the screen
	lcd_pins_init();
	lcd_screen_init();
	lcd_clear();
	EnableInterrupts();
	
	wireless_configure_device(myID, remoteID ) ;
	init_LED();
	
	checkForNewBoard();
	
}

void printNames(){
	int i;
	memset(output,0,80);
	for(i = 0; i < 80; i++){
    eeprom_byte_read(EEPROM_I2C_BASE,i+EEPROM_NAME1,(uint8_t*)&output[i]);
  }
	printf("Student 1: ");
	printf(output);
	printf("\n\r");
	for(i = 0; i < 80; i++){
    eeprom_byte_read(EEPROM_I2C_BASE,i+EEPROM_NAME2,(uint8_t*)&output[i]);
  }
	printf("Student 2: ");
	printf(output);
	printf("\n\r");
	for(i = 0; i < 2; i++){
    eeprom_byte_read(EEPROM_I2C_BASE,i+EEPROM_NUMBER,(uint8_t*)&output[i]);
		output[2] = '\0';
  }
	printf("Team Number: ");
	printf(output);
	printf("\n\r");
}

void getInput(char* buffer){
	memset(buffer,0,80);
	scanf("%79[^\n]", buffer);
}

char input[80];

void getNewNames(){
	int i;
	memset(input,0,80);
	printf("\n\rEnter Name 1: ");
	getInput(input);
	for(i = 0; i < 80; i++){
    eeprom_byte_write(EEPROM_I2C_BASE,i+EEPROM_NAME1,input[i]);
  }
	memset(input,0,80);
  printf("\n\rEnter Name 2: ");
	getInput(input);
	for(i = 0; i < 80; i++){
    eeprom_byte_write(EEPROM_I2C_BASE,i+EEPROM_NAME2,input[i]);
  }
	memset(input,0,80);
	printf("\n\rEnter Team Number: ");
	getInput(input);
	for(i = 0; i < 2; i++){
    eeprom_byte_write(EEPROM_I2C_BASE,i+EEPROM_NUMBER,input[i]);
  }
	NVIC_SystemReset();
}

void checkForSpecialConditions(){
	if(BTN_PS2){
			BTN_PS2 = false;
			getNewNames();
	}
	if(ALERT_5_S){
		ALERT_5_S = false;
		printf("Packets Received: %d; Packets Sent: %d \n", PACKETS_R, PACKETS_S);
	}
	if(BTN_LEFT){
		BTN_LEFT = false;;
		wireless_configure_device(remoteID, myID) ;
		amRemote = true;
	}
}

//*****************************************************************************
//*****************************************************************************

/*------SPECIAL SNAKE DEFINES----------*/

#define MAX_SNAKE_SIZE (25*16 - 1)
#define SCREEN_X 102
#define SCREEN_Y 8
#define BOARD_X 25//TODO //Ranges from 1 to BOARD_X
#define BOARD_Y 16//TODO //Ranges from 1 to BOARD_Y
#define PLAY_SIG 0
#define EXIT_SIG 1
#define VICTORY_SIG 0
#define DEFEAT_SIG 1
#define CONTINUE_SIG 2

void genBerry(void);
int update(void);
bool validateOffSnake(uint16_t, bool);
void snakeInit(bool);
int getSnakeLength(void);
uint16_t getSnakeLoc(int);
char getSnakeX(int);
char getSnakeY(int);
uint16_t getSnakeHead(void);
void setSnakeHead(char , char);
void moveSnake(char, char);
int homeScreen(void);
void play(void);
void win(void);
void lose(void);
void setBoard(int, int, char);
char getBoard(int, int);
uint16_t getNextFree(int skip);
int getJoystickX(void);
int getJoystickY(void);
void initGame(void);


/******************************************************************************
 * Global Variables
 *****************************************************************************/
PC_Buffer UART0_Rx_Buffer;
PC_Buffer UART0_Tx_Buffer;


//*****************************************************************************
//*****************************************************************************

typedef struct{
	uint16_t data[MAX_SNAKE_SIZE]; //Top 8 bits: x, Bottom 8 bits: y
	int head; //Point to ONE ABOVE HEAD
	int tail; //Points to tail
} SnakeBuffer;

int berryX;
int berryY;

SnakeBuffer snake;
char board[BOARD_X*BOARD_Y];
uint8_t screen[SCREEN_X * SCREEN_Y];

void clearAll(){
	BTN_UP = false;
	BTN_DOWN = false;
	BTN_LEFT = false;
	BTN_RIGHT = false;

	PS2_UP = false;
	PS2_DOWN = false;
	PS2_LEFT = false;
	PS2_RIGHT = false;
}

void setBoard(int x, int y, char val){
	x = x-1;
	y = y-1;
	board[y*BOARD_X + x] = val;
}

char getBoard(int x, int y){
	x = x-1;
	y = y-1;
	return board[y*BOARD_X + x];
}

void setScreen(int column, int page, uint8_t val){
	screen[page*SCREEN_X + column] = val;
}

uint8_t getScreen(int column, int page){
	return screen[page*SCREEN_X + column];
}

uint16_t getNextFree(int skip){
	static int indexLast = 0;
	int i, x, y;
	for(i=0;i<=skip;){
		indexLast++;
		if(indexLast==BOARD_X*BOARD_Y) indexLast = 0;
		if(!board[indexLast])i++;
	}
	x = (indexLast%BOARD_X) + 1;
	y = (indexLast/BOARD_X) + 1;
	return (uint16_t)(((x&0xFF)<<8) + (y&0xFF));
}

//Initializes the game
void initGame(){
	int j;
	
	while(1){
		
		//Homescreen
		if(homeScreen()==EXIT_SIG) break; //Else play
		//Draw frame
		lcd_clear();
		lcd_set_column(100);
		for(j=0;j<8;j++){
			lcd_set_page(j);
			lcd_write_data(0xFF);
		}
		lcd_set_column(101);
		for(j=0;j<8;j++){
			lcd_set_page(j);
			lcd_write_data(0xFF);
		}
		clearAll();
		//Play
		play();
	}
	lcd_clear();
	
}

//Handles the home screen
int homeScreen(){
	int i;
	char* line1 = "  Snake!";
	char* line2 = "Play";
	char* line3 = "Exit";
	lcd_clear();
	for(i=0;i<7;i++) lcd_write_char_10pts(0, line1[i], i);
	for(i=0;i<4;i++) lcd_write_char_10pts(2, line2[i], i);
	for(i=0;i<4;i++) lcd_write_char_10pts(3, line3[i], i);
	while(!BTN_UP && !BTN_DOWN){
		checkForSpecialConditions();
		WATCHDOG0->ICR = 1;
	}
	if(BTN_UP) return PLAY_SIG;
	else return EXIT_SIG;
}

//Handles gameplay
void play(){
	int sig;
	static bool first = true;
	//Initialize snake
	snakeInit(first);
	first = false;
	while(1){
		sig = update();
		if(sig==DEFEAT_SIG){
			lose();
			break;
		}else if(sig==VICTORY_SIG){
			win();
			break;
		}
		//Else continue
	}
}

void printScore(){
	char input[4];
	char score[4];
	int i, hs;
	score[0] = (((getSnakeLength()-2)/100)%10) + 0x30;
	score[1] = (((getSnakeLength()-2)/10)%10) + 0x30;
	score[2] = ((getSnakeLength()-2)%10) + 0x30;
	score[3] = 0;
	for(i = 0; i < 3; i++){
    eeprom_byte_read(EEPROM_I2C_BASE,i+EEPROM_SCORE,(uint8_t*)&input[i]);
		input[3] = '\0';
  }
	hs = (input[0]-0x30)*100 + (input[1]-0x30)*10 + (input[2]-0x30);
	printf("High Score: ");
	printf(input);
	printf("; Your Score: ");
	printf(score);
	printf("\n\r");
	if(hs<(getSnakeLength()-2)){//New High Score
		for(i = 0; i < 3; i++){
			eeprom_byte_write(EEPROM_I2C_BASE,i+EEPROM_SCORE,score[i]);
		}
	}
}

//VICTORY SCREECH
void win(){
	int i;
	char* line1 = "YOU WIN!:D";
	char* line2 = "UP=EXIT";
	lcd_clear();
	for(i=0;i<10;i++) lcd_write_char_10pts(0, line1[i], i);
	for(i=0;i<7;i++) lcd_write_char_10pts(2, line2[i], i);
	printScore();
	while(!BTN_UP){/*block*/ }
	BTN_UP = false;
}

//Too bad
void lose(){
	int i;
	char* line1 = "SORRY! :(";
	char* line2 = "UP=EXIT";
	lcd_clear();
	for(i=0;i<9;i++) lcd_write_char_10pts(0, line1[i], i);
	for(i=0;i<7;i++) lcd_write_char_10pts(2, line2[i], i);
	printScore();
	while(!BTN_UP){/*block*/ }
	BTN_UP = false;
}



//Initializes the snake.  Allocates memory if alloc is true
void snakeInit(bool alloc){
	snake.head = 0;
	snake.tail = 0;
	memset(board, 0, BOARD_X*BOARD_Y);
	memset(screen, 0, SCREEN_X*SCREEN_Y);
	setSnakeHead(1,1);
	setSnakeHead(2,1);
	genBerry();
}

//Gets the length of the snake
int getSnakeLength(){
	return (snake.head - snake.tail);
}

uint16_t getSnakeLoc(int index){
	return snake.data[(snake.tail+index)%MAX_SNAKE_SIZE];
}

//Returns the x value at an index (tail is 0)
char getSnakeX(int index){
	return (char)((snake.data[(snake.tail+index)%MAX_SNAKE_SIZE])>>8)&0xFF;
}

//Returns the y value at an index (tail is 0)
char getSnakeY(int index){
	return (char)((snake.data[(snake.tail+index)%MAX_SNAKE_SIZE])&0xFF);
}

uint16_t getSnakeHead(){
	return snake.data[(snake.head-1)%MAX_SNAKE_SIZE];
}

//Increments snake length by 1 and sets the head to x-y.  DOES NOT MOVE SNAKE
void setSnakeHead(char x, char y){
	uint16_t newPos = 0;
	uint8_t pix;
	bool isTop = false;
	int xn, yn, i;
	newPos += (uint16_t)y;
	newPos += ((uint16_t)x)<<8;
	snake.data[snake.head%MAX_SNAKE_SIZE] = newPos;
	snake.head++;
	
	//Draw new head
	xn = ((int)x-1);
	yn = ((int)y-1);
	if(yn%2) isTop = true;
	lcd_set_page(7 - yn/2);
	for(i=0;i<4;i++){
		lcd_set_column(xn*4 + i);
		pix = getScreen(xn*4+i, yn/2);
		if(isTop) pix |= 0x0F;
		else pix |= 0xF0;
		lcd_write_data(pix);
		setScreen(xn*4+i, yn/2, pix);
	}
}


//Moves the snake to the location x-y
void moveSnake(char x, char y){
	uint16_t tail, i;
	uint8_t pix;
	setSnakeHead(x, y);
	if(validateOffSnake(((berryX<<8)&0xFF00) + (berryY&0xFF), true)){ //Snake has not encountered the berry
		//Remove snake tail
		tail = snake.data[snake.tail%MAX_SNAKE_SIZE];
		x = ((tail>>8)&0xFF) - 1;
		y = (tail&0xFF) - 1;
		lcd_set_page(7 - y/2);
		for(i = 0; i<4;i++){
			lcd_set_column(x*4 + i);
			pix = getScreen(x*4 + i, y/2);
			if(y%2) pix &= 0xF0;
			else pix &= 0x0F;
			lcd_write_data(pix);
			setScreen(x*4 + i, y/2, pix);
		}
		snake.tail++;
	}else{
		genBerry();
	}
}

//Makes sure that the x-y combo "num" is not within the snake
//If headCount is false, does not test against the head
//Returns false if on snake, true otherwise
bool validateOffSnake(uint16_t num, bool headCount){
	int i, length = getSnakeLength();
	if(!headCount) length--;
	for(i=0;i<length;i++){
		if(num==getSnakeLoc(i)) return false;
	}
	return true;
}

//Updates and draws the snake and the berry, returns code for victory, defeat, or continue
int update(){
	uint16_t pos, pos2;
	uint32_t code=0xDEADBEEF, data;
	wireless_com_status_t status;
	while(!ALERT_MOVE){
		checkForSpecialConditions();
		drawLEDs();
	}
	status = wireless_send_32(false, false, code);
	if(status == NRF24L01_TX_SUCCESS){
		PACKETS_S++;
	}
	status =  wireless_get_32(false, &data);
	if(status == NRF24L01_RX_SUCCESS){
		PACKETS_R++;
		WATCHDOG0->ICR = 1;
	}
	ALERT_MOVE = false;
	pos = getSnakeHead();
	if(PS2_RIGHT && ((pos>>8)&0xFF)!=(BOARD_X)){ //Right and can
		pos += (1<<8);
	}else if(PS2_LEFT && ((pos>>8)&0xFF)!=1){ //Left and can
		pos -= (1<<8);
	}else if(PS2_UP && (pos&0xFF)!=(BOARD_Y)){ //Up and can
		pos += 1;
	}else if(PS2_DOWN && (pos&0xFF)!=1){ //Down and can
		pos -= 1;
	}else{//Try to move current direction, otherwise die
			pos2 = ((uint16_t)getSnakeX(getSnakeLength()-2))<<8;
		  pos2 += getSnakeY(getSnakeLength()-2);
			if(pos>pos2){//right or up
				if((pos-pos2)&0xFF){//up
					if((pos&0xFF)!=(BOARD_Y)) pos += 1; //can go up
					else return DEFEAT_SIG;
				}else{//right
					if(((pos>>8)&0xFF)!=(BOARD_X)) pos += 1<<8;//can go right
					else return DEFEAT_SIG;
				}
			}else{//left or down
				if((pos2-pos)&0xFF){//down
					if((pos&0xFF)!=1) pos -= 1; //can go down
					else return DEFEAT_SIG;
				}else{//left
					if(((pos>>8)&0xFF)!=1) pos -= 1<<8; //can go left
					else return DEFEAT_SIG;
				}
			}
	}
	moveSnake((pos>>8)&0xFF, (pos&0xFF));
	clearAll();
	
	
	if(getSnakeLength()==MAX_SNAKE_SIZE) return VICTORY_SIG;
	
	if(!validateOffSnake(getSnakeHead(), false)){ return DEFEAT_SIG;}//Ran into itself
	
	return CONTINUE_SIG;
	
}

//Generates the berry
void genBerry(){
	uint16_t nextFree;
	uint8_t pix, val;
	int x, y;
	nextFree = getNextFree(BOARD_X*5);
	berryX = (nextFree>>8) & 0xFF;
	berryY = nextFree & 0xFF;
	x = (berryX-1);
	y = (berryY-1);
	if(y%2) val = 0x06;
	else val = 0x60;
	lcd_set_page(7-y/2);
	lcd_set_column(4*x+1);//First column
	pix = getScreen(4*x+1, y/2);
	pix |= val;
	setScreen(4*x+1, y/2, pix);
	lcd_write_data(pix);
	lcd_set_column(4*x+2);//Second column
	pix = getScreen(4*x+2, y/2);
	pix |= val;
	setScreen(4*x+2, y/2, pix);
	lcd_write_data(pix);
}

int 
main(void)
{
	initializeBoard();
	printNames();
 
	initGame();
	
	while(1);

}
