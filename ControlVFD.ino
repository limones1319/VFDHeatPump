#include <SoftwareSerial.h>

#define rxPin 02
#define txPin 03

// Set up a new SoftwareSerial object
SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin);

byte forward_command[] = {0x01, 0x06, 0x20, 0x00, 0x00, 0x01};
byte stop_command[] = {0x01, 0x06, 0x20, 0x00, 0x00, 0x05};
byte status_command[] = {0x01, 0x03, 0x30, 0x00, 0x00, 0x00};
byte vfd_fault_command[] = {0x01, 0x03, 0x80, 0x00, 0x00, 0x00};
byte comm_fault_command[] = {0x01, 0x03, 0x80, 0x01, 0x00, 0x00};
byte comm_source_running_command[] = {0x01, 0x06, 0x00, 0x02, 0x00, 0x02}
byte keypad_source_running_command[] = {0x01, 0x06, 0x00, 0x02, 0x00, 0x00}

bool debug = false;
bool prompt = false;

void setup()  {
    // Define pin modes for TX and RX
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    
    // Set the baud rate for the SoftwareSerial object
    mySerial.begin(9600);
    Serial.begin(9600);
}

void loop() {
	String input_command = read_serial_input();

	if (input_command.equals("STATUS")) {
		Serial.print("Received command: ");
		Serial.println(input_command);
		byte status = get_status(status_command, sizeof(status_command));
	} else if (input_command.equals("VFDFAULT")) {
		Serial.print("Received command: ");
		Serial.println(input_command);
		byte status = get_status(vfd_fault_command, sizeof(vfd_fault_command));
	} else if (input_command.equals("COMMFAULT")) {
		Serial.print("Received command: ");
		Serial.println(input_command);
		byte status = get_status(comm_fault_command, sizeof(comm_fault_command));
	} else if (input_command.equals("START")) {
		Serial.print("Received command: ");
		Serial.println(input_command);
		byte status = get_status(forward_command, sizeof(forward_command));
	} else if (input_command.equals("STOP")) {
		Serial.print("Received command: ");
		Serial.println(input_command);
		byte status = get_status(stop_command, sizeof(stop_command));
	} else if (input_command.equals("DEBUG")) {
	    prompt = true;
	    Serial.print("Received command: ");
	    if (debug == false) {
	        debug = true;
	        Serial.print(input_command);
	        Serial.println(" is ON");
	    } else {
	        debug = false;
	        Serial.print(input_command);
	        Serial.println(" is OFF");
	    }
	} else if (input_command.length()>1) {
	    Serial.println("UNKNOWN COMMAND");
	} else if (!prompt) {
        Serial.println("READY. ");
        Serial.print("#");
        prompt = true;
	}
}

bool check_crc(byte response[], size_t size) {
    byte check_array[size-2];
    memcpy(check_array, response, size-2);
    unsigned int crc = crc_cal_value(check_array, size-2);

    if (debug) {
        Serial.print("DEBUG: CRC low bytes response - crc: ");
        Serial.print(response[size-2]);
        Serial.print(" - ");
        Serial.println(lowByte(crc));
        Serial.print("DEBUG: CRC high bytes response - crc: ");
        Serial.print(response[size-1]);
        Serial.print(" - ");
        Serial.println(highByte(crc));
    }

    if (lowByte(crc)==response[size-2] and highByte(crc)==response[size-1]) {
        return true;
    } else {
        return false;
    }
}

byte get_status(byte command[], int sizeOfCommand) {

    prompt = false;
    // Set the status command
    byte temporary_command[sizeOfCommand+2];
	memcpy(temporary_command, command, sizeOfCommand);

    if (debug) {
    	Serial.print("DEBUG: CMD: ");
	    monitor_print_array(command, sizeOfCommand);
    }

	unsigned int crc = crc_cal_value(command,sizeOfCommand);

    if (debug) {
    	Serial.print("DEBUG: CRC: ");
	    Serial.println(crc,HEX);
    }

	unsigned char low_byte = crc & 0xff;
	unsigned char high_byte = (crc>>8) & 0xff;
	temporary_command[sizeOfCommand] = (int) low_byte;
	temporary_command[sizeOfCommand+1] = (int) high_byte;

	if (debug) {
	    Serial.print("DEBUG: Sending command: ");
    	monitor_print_array(temporary_command,sizeof(temporary_command)/sizeof(temporary_command[1]));
	}

	int size = sizeof(temporary_command)/sizeof(temporary_command[1]);
	byte vfd_response[8];

	int response_array_length = send_command(vfd_response, temporary_command, size);

    if (check_crc(vfd_response, response_array_length)) {
        Serial.println("CRC: OK.");
        Serial.print("STATUS: Received: ");
        monitor_print_array(vfd_response, response_array_length);
    } else {
        Serial.println("ERROR: CRC Failed! ");
    }
}

String read_serial_input() {
    String input_str;

    while (Serial.available()) {
        input_str = Serial.readString();  //read until timeout
        input_str.trim();                        // remove any \r \n whitespace at the end of the String
        Serial.read();
    }

	return input_str;
}

void monitor_print_array(byte array_to_print[], size_t size) {
	int i = 0;
	for(size_t i=0;i<size;i++) {
		Serial.print(convert_to_hex(array_to_print[i]));
	}

    if(debug) {
	    Serial.println("");
    	Serial.print("DEBUG: Size of the array: ");
    }
	Serial.println(size);
}

String convert_to_hex(byte hex_value) {
	String hex_string;
	if (hex_value<16) {
		hex_string = " 0x0";
	} else {
		hex_string = " 0x";
	}

	String result = hex_string + String(hex_value,HEX);
	return result;
}

int send_command(byte vfd_response[], byte command[], int sizeOfCommand) {
	int bytes_sent = 0;

	if (debug == true) Serial.print("DEBUG: Command sent: ");

	for (int i=0; i<sizeOfCommand; i++) {
		mySerial.write(command[i]);
		if (debug == true) {
		    Serial.print(command[i]);
		    Serial.print(".");
		}
	}

	if (debug == true) Serial.println(" ");

	delayMicroseconds(2917);
	int i = 0;

	if (debug == true) Serial.print("DEBUG: Response received: ");

	while (mySerial.available()) {
		int byte_response = mySerial.read();
		vfd_response[i] = byte_response;
		if (debug == true) {
		    Serial.print(byte_response);
		    Serial.print(".");
		}
		i++;
    }

    if (debug == true) Serial.println(" ");

    return (i);
}

/*
byte read_response(void) {
	delay(2);
	int i = 0;
	while (mySerial.available() > 0) {
		int byte_response = mySerial.read();
		vfd_response[i] = byte_response;
		Serial.print(byte_response);
		Serial.print(".");
		i++;
  }
	return vfd_response; 
}
*/

unsigned int crc_cal_value(unsigned char *data_value,int data_length) {
  int i;
  unsigned int crc_value = 0xffff;
	while(data_length--) {
		crc_value ^= *data_value++;
		for(i=0;i<8;i++) {
	  	if(crc_value&0x0001)
	    	crc_value = (crc_value>>1)^0xa001;
	    else
	    	crc_value = crc_value>>1;
	  }
	} 
	return(crc_value);
}
