#include <msp430.h>
#include <libemb/serial/serial.h>
#include <libemb/conio/conio.h>
#include <stdint.h>

#include "dtc.h"
#include "eeprom.h"

// STRUCT (AKA "CLASS") FOR A LOCATION
typedef struct loc {
	char key;
	char sum;
	char name[32];
	char desc[216];
	char exits[6];
} loc;

// THIS WILL PROBABLY BE USEFUL
const char hex[] = { "0123456789ABCDEF" };

// GLOBAL VARIABLES
loc current;
uint16_t heading;

//Initialize location data
char buffer[32];
int index = 1;


// PROTOTYPES
void read_loc(uint16_t);
void print_loc(void);

int main(void){
	WDTCTL  = WDTPW|WDTHOLD;
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL  = CALDCO_1MHZ;

	P2SEL  &= ~(BIT6 | BIT7);
	P2SEL2 &= ~(BIT6 | BIT7);

	// MORE PIN INITIALIZATION HERE
	P3DIR |= BIT6;
	P3OUT &= ~BIT6;
	P3DIR |= BIT2;
	P3OUT &= ~BIT2;

	P1DIR &= ~BIT3;
	P1REN |= BIT3;
	P1OUT |= BIT3;
	P1IES |= BIT3;
	P1IFG &= ~BIT3;
	P1IE  |= BIT3;

	P2DIR &= ~BIT7;
	P2REN |= BIT7;
	P2OUT |= BIT7;
        P2IES |= BIT7;
        P2IFG &= ~BIT7;
        P2IE  |= BIT7;

	// INITIALIZE THE EEPROM AND SERIAL LINE
	eepromInit();
	serial_init(9600);

	// DISPLAY GALACTIC CENTER LOCATION AT START
	read_loc(0x0000);
	print_loc();

	// ENABLE INTERRUPTS
	__eint();

	// INITIALIZE THE POTENTIOMETER
	initialize_dtc(INCH_4, &heading);
	for(;;) {
		//get heading
		uint8_t most_sig_8 = (heading >> 2) & 0xFF;

		uint8_t high_nibble = (most_sig_8 >> 4) & 0x0F;
    		uint8_t low_nibble  = most_sig_8 & 0x0F;

		char high_char = hex[high_nibble];
		char low_char = hex[low_nibble];
		// DISPLAY CURRENT HEADING BYTE
		cio_printf(">> %c%c", high_char, low_char);
		cio_printf("\b\b\b\b\b");
		__delay_cycles(25000);
	}

	return 0;
}

void read_loc(uint16_t a) {
	// READ DATA FROM EEPROM
	//key
	eepromRead(a, (uint8_t *)&current.key);
	a++;

	//checksum
        eepromRead(a, (uint8_t *)&current.sum);
        a++;

	uint8_t check = (current.sum  ^ current.key) & 0xFF;
	if (check == 0xFF) {
		P3OUT |= BIT2;
		P3OUT &= ~BIT6;
	} else {
		P3OUT |= BIT6;
		P3OUT &= ~BIT2;
	}

	//name
	for(int i = 0; i < 32; i++){
		eepromRead(a, (uint8_t *)&current.name[i]);
		a++;
		current.name[i] ^= current.key;
	}

        //description
        for(int i = 0; i < 216; i++){
                eepromRead(a, (uint8_t *)&current.desc[i]);
                a++;
                current.desc[i] ^= current.key;
        }

        //exits
        for(int i = 0; i < 6; i++){
                eepromRead(a, (uint8_t *)&current.exits[i]);
                a++;
                current.exits[i] ^= current.key;
        }

}

void print_loc(void) {
	// OUTPUT CURRENT LOCATION NICELY
	//line 1
	cio_printf("@ %s", current.name);

	//print location array
	cio_printf(" { ");
	for (int k = 0; k < (index - 1); k++){
		char high = hex[(buffer[k] >> 4) & 0x0F];
		char low  = hex[buffer[k] & 0x0F];
		cio_printf("%c%c -> ", high, low);
	}
        char high = hex[(buffer[index - 1] >> 4) & 0x0F];
        char low  = hex[buffer[index - 1] & 0x0F];

	cio_printf("%c%c }\n\r", high, low);

	//line 2
	cio_printf("%s\n\r", current.desc);

	//line 3
	cio_printf("SCANNED HEADINGS:\n\r\t");

	//line 4
    	for(int j = 0; j < 6; j++) {
                char high = hex[(current.exits[j] >> 4) & 0x0F];
                char low  = hex[current.exits[j] & 0x0F];

        	cio_printf("%c%c", high, low);

        // Print a comma after all except the last exit
        	if (j < 5) {
            		cio_printf(", ");
        	}
    	}

    	cio_printf("\n\r");
}

#pragma vector=PORT1_VECTOR
__interrupt void backtrack(void)
{
	if (index > 1){
	  // MOVE BACKWARD INTERRUPT
		index--;
		buffer[index] = '\0';

	        uint16_t past_loc = (buffer[index - 1] << 5) & 0x1FE0;



        	read_loc(past_loc);
        	print_loc();
	}

	// DEBOUNCE ROUTINE
    while (!(BIT3 & P1IN));
    __delay_cycles(32000);
    P1IFG &= ~BIT3;
}

#pragma vector=PORT2_VECTOR
__interrupt void engage(void)
{
	// MOVE FORWARD INTERRUPT
        uint8_t most_sig_8 = (heading >> 2) & 0xFF;

	buffer[index++] = most_sig_8;

	uint16_t next_loc = (most_sig_8 << 5) & 0x1FE0;

	read_loc(next_loc);
	print_loc();

	// DEBOUNCE ROUTINE
    while (!(BIT7 & P2IN));
    __delay_cycles(32000);
    P2IFG &= ~BIT7;
}
