ECE 353 Snake Project

Authors:
	Mackenzie Scanlan
	Marshal Stutz

Team:
	37

Game:
	Snake

Executive Summary:
	This project is an implementation of the classic "Snake" game, in which a digital serpent crosses the LCD screen,
	eating randomly-generated berries and growing larger each time, while trying not to hit the walls or run into himself.
	Furthermore, this game makes perpetual contact with another board in the vicinity, and will not run for long unless the
	"friend" is present.  The errata segment at the bottom shows any problems still extant, and what our solutions for them
	are.

Project Description.

	In order to implement the snake game, we perfected and used several of the driver files designed throughout the ECE 353 class.

	The game begins at reset by entering a title screen.  The game prints on the UART screen who it thinks you are, based on your last entry.
	At any time during either this screen or gameplay, you may press the joystick down hard.  This will prompt you to enter two new names
	and a new team number, and then reset the game to the starting screen.

	Meanwhile, the LCD screen prompt you to either play by pressing the up button, or exit by pressing the down button.
	This is the home screen, and you will return to it whenever a game ends or the board resets.

	If you press the left button here, it will set you to be Board A.  If you do not, you are Board B.  Once you begin actual gameplay, the other
	board must also begin.  The boards will communicate as long as the game is in progress, and will shut down once they lose contact for a long
	enough time (10 seconds).

	The snake begins in the bottom left corner moving right.  Use the joystick components to send him in different directions, and try to eat the berry.
	He will grow longer when he does so and a new berry will generate somewhere else.  If you run into the walls or yourself, you will lose and the game
	will end.  If you fill the entire screen with your immense length, you will win and the game will end.  Whenever the game ends due to victory or
	defeat, your score and previous high score will be printed to the screen.  They are stored in the EEPROM device.
	
Implementation:

	The joystick is controlled via the ADC0 SS0.  The comparators generate interrupts which are used to determine when it has moved too far in any
	direction.  ADC1 SS3 monitors the potentiometer value, but as we had no use for it in our game, the actual data in the FIFO is never used.

	The UART runs at 115200 baud and generates interrupts for transmission and reciept.

	The LCD forms the crux of gameplay, and features both text and moving, controlled images.

	The radio maintains contact between the devices, sending the "DEADBEEF" code between them.  When a packet is recieved, the 10 second watchdog timer
	resets, allowing normal operation to continue.  When the devices lose contact, this timer eventually resets the boards.

	The SysTick timer is used to debounce the push buttons.  Timer 0A activates both ADC's and runs at 1 interrupt per 500 microseconds.
	Timer 1 runs at 1 interrupt per 5 seconds, and prints packet statistics to the UART screen. 

	The EEPROM contains data on student names and the team number, which can be reset by pressing the PS2 button at any point during normal operation.
	Keep in mind, only ten seconds are given to reset the data after the watchdog timer begins.  Furthermore, the EEPROM stores data on previous high
	scores.

Video:

	https://youtu.be/VQC4Wi9rouY

Errata:

	The snake can be guided by simply moving the joystick in the up, left, and down directions.  However, it must be HELD DOWN to go right.

	The snake can double back on itself, and will lose if it does so.  This is not a bug, it is a feature.

	
	 