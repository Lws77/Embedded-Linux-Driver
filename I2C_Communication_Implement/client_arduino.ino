#include<stdio.h>
#include<Wire.h>

#define ARDUINO_ADDR 0x8
#define PIR_PIN_10 10
#define LED_PIN_9 9

int i=0;
int dataMode=0;
int PIRMode=0;

/**
 * The setup function 
 */
void setup() {  
  pinMode(LED_BUILTIN, OUTPUT);         	// initialize digital pin LED_BUILTIN as an OUTPUT.
  pinMode(LED_PIN_9, OUTPUT);         		// initialize digital pin 9 as an OUTPUT.
  pinMode(PIR_PIN_10, INPUT);			// initialize digital pin 10 as an INPUT.
  Wire.begin(ARDUINO_ADDR);
  Wire.onReceive(receiveEvent);         	// Register event
  Wire.onRequest(requestEvent);         	// Register event
  Serial.begin(9600);
  printf_begin();
  Serial.println("Serial Strat\n");
}

/**
 * The loop function 
 */
void loop() {
	/* read from PIR senser */
	PIRMode=digitalRead(PIR_PIN_10);
	printf("dataMode=%d\n", PIRMode);
	
	/* set LED_PIN_9 */
	if(PIRMode){
		digitalWrite(LED_PIN_9, HIGH);
	}else{
		digitalWrite(LED_PIN_9, LOW);
	}
	delay(500);
}


int serial_putc(char c, FILE *) {
	Serial.write(c);
}
void printf_begin(void){
	fdevopen(&serial_putc, 0);
}

/**
 * receiveEvent 
 */
void receiveEvent(int nbyte){
  while (Wire.available()) {
    Serial.println("read i2c wire\n");
    char c = Wire.read();
    Serial.println(c);
    }
}

/**
 * requestEvent 
 */
void requestEvent(){
	switch (PIRMode){
		case 0:
		/*write to master*/
			Wire.write("L",1);
			break;
		case 1:
		/* write to master */
			Wire.write("H",1);
			break;
		default:
			Wire.write("n",1);
	}
}


/**
  FILE *  fdevopen (int(*put)(char, FILE *), int(*get)(FILE *))
*/
