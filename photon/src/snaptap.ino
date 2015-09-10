/* Linksprite */
int a=0x0000;                    //Read Starting address
void sendResetCmd();
void sendTakePhotoCmd();
void sendReadDataCmd();
void stopTakePhotoCmd();
void changeSize(int level);
int chunkSize = 1024;
int chunkIncr = 0x400;

/* TCP */
TCPClient client;
char server[] = "192.168.168.96";
int PORT = 443;
void connectToServer();
int readIntFromTCP(TCPClient);
int waitForACK();

/* Keg */
int statusPin = D7;
int reedSwitchPin = D2;
enum Modes{WAIT, COUNTDOWN, TAKEPIC, GETDATA, POURING};
volatile int mode = WAIT;
int dayCount = 0;
int weekCount = 0;
int monthCount = 0;
int kegCount = 0;
volatile long startPour = 0;
volatile long endPour = -1;
volatile long lastInterruptTime = 0;
void getCounts();

/*Display*/
enum displayModes{KEG, DAY, WEEK, MONTH};
int displayMode = KEG;
int ssPin = A2; //slave select pin
int displaySettingPin = A0;
void clearDisplay();
void sendDisplayString(char*,int size=4);
void setDecimals(byte);
void setCursor(byte);
void updateDisplay();
void setBrightness();
void checkDisplaySetting();


void isr_tapChanged();
void blinkPin(int,int);


void setup() {
    Serial1.begin(38400);
    Serial.begin(115200);
    //Setup Display
    pinMode(ssPin, OUTPUT);
    pinMode(displaySettingPin, INPUT);
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV256);
    clearDisplay();
    setBrightness(25);

    //Setup TCP comms
    connectToServer();
    pinMode(reedSwitchPin, INPUT_PULLUP);
    pinMode(statusPin, OUTPUT);
    if(client.connected()){
      getCounts();
      checkDisplaySetting();
      updateDisplay();
    }else{
      connectToServer();
    }
    //Setup tap switch
    attachInterrupt(reedSwitchPin, isr_tapChanged, CHANGE);

    //Setup Camera
    changeSize(2);
    delay(100);
    sendResetCmd();
    delay(4000);
    changeBaudRate(115200);
    blinkPin(statusPin, 3);
    setDecimals(0b00001000);
}

void loop() {

    switch(mode)
    {
        case WAIT:
        {
            checkDisplaySetting();
            delay(10);
            break;
        }
        case COUNTDOWN:
        {
          clearDisplay();
          sendDisplayString("4", 1);
          delay(500);
          clearDisplay();
          setCursor(0x01);
          sendDisplayString("3", 1);
          delay(500);
          clearDisplay();
          setCursor(0x02);
          sendDisplayString("2", 1);
          delay(500);
          clearDisplay();
          setCursor(0x03);
          sendDisplayString("1", 1);
          delay(500);
          clearDisplay();
          if(mode == COUNTDOWN){
            mode=TAKEPIC;
          }else{
            //Don't take a picture if they already put the tap back
            if(millis()%2 == 0){
              sendDisplayString("nOpE");
            }else{
              sendDisplayString("booo");
            }
            delay(2000);
            clearDisplay();
            setDecimals(0b00001000);
            updateDisplay();
          }
          break;
        }
        case TAKEPIC:
        {
            //Take picture
            setDecimals(0b00000000);
            sendDisplayString("bEEr");
            sendTakePhotoCmd();
            client.write("NEWBEER");
            waitForACK();
            delay(100);
            while(Serial1.available()>0)
            {
                char newChar = Serial1.read();
            }
            mode = GETDATA;
            break;
        }
        case GETDATA:
        {
            //attachInterrupt(D2, isr_tapChanged, FALLING);
            byte byteArr[chunkSize];
            byte incomingByte;
            int j;
            int k;
            int count;
            a=0x0000;
            bool endFlag = false;
            client.write("SNDDATA");
            waitForACK();
            delay(100);
            while(!endFlag){
                digitalWrite(statusPin, !digitalRead(statusPin));
                j = 0;
                k = 0;
                count = 0;
                sendReadDataCmd();
                while(count<chunkSize && !endFlag){
                    if(Serial1.available()>0){
                        incomingByte = Serial1.read();
                        k++;
                        if( k>5 && j<chunkSize && !endFlag){
                            byteArr[j] = incomingByte;
                            if( byteArr[j-1]==0xFF && byteArr[j]==0xD9){
                                endFlag = true;
                            }
                            j++;
                            count++;
                        }
                    }
                }
                k=0;
                while(k<5){
                    if(Serial1.available()>0){
                        k++;
                        incomingByte = Serial1.read();
                    }
                }
                delay(5);
                client.write(byteArr, count);
            }
            digitalWrite(statusPin, HIGH);
            delay(500);
            client.write("ENDDATA");
            waitForACK();
            if(endPour > startPour){
              char temp[20];
              sprintf(temp, "DURATION:%d", endPour-startPour);
              client.write(temp);
              waitForACK();
              getCounts();
              updateDisplay();
              mode = WAIT;
            }else{
              mode = POURING;
            }
            delay(1000);
            stopTakePhotoCmd();
            digitalWrite(statusPin, LOW);
            setDecimals(0b00001000);
            break;
        }
        case POURING:
        {
          if(endPour > startPour){
            char temp[20];
            sprintf(temp, "-DURATION:%d", endPour-startPour);
            client.write(temp);
            waitForACK();
            getCounts();
            updateDisplay();
            mode = WAIT;
          }else{
            delay(10);
          }
        }
        default:
        {
            delay(10);
            break;
        }

    }
}

//connect to tcp server
void connectToServer() {
  if (client.connect(server, PORT)) {
    sendDisplayString("conn");
    delay(500);
  }else{
    sendDisplayString("noco"); //display "no connection" try again in 2 seconds
    delay(2000);
    connectToServer();
  }
}

void getCounts(){
  client.write("GETCOUNTKEG");
  while(client.available()<=0){
      delay(10);
  }
  kegCount = readIntFromTCP();
  client.write("GETCOUNTDAY");
  while(client.available()<=0){
      delay(10);
  }
  dayCount = readIntFromTCP();
  Serial.print("New day count: ");
  Serial.println(dayCount);
  client.write("GETCOUNTWEEK");
  while(client.available()<=0){
      delay(10);
  }
  weekCount = readIntFromTCP();
  client.write("GETCOUNTMONTH");
  while(client.available()<=0){
      delay(10);
  }
  monthCount = readIntFromTCP();
  Serial.print("New month count: ");
  Serial.println(monthCount);
}

int readIntFromTCP(){
  char newChar = 0;
  char buf[10];
  int i = 0;
	while(client.available()>0){
	    newChar = client.read();
	    buf[i]  = newChar;
	    i++;
	}
	int temp = atoi(buf);
	return temp;
}

int waitForACK(){
  long ackTimeout = 10000;
  long start = millis();
  while(client.available()<=0){
    if((millis()-start)>ackTimeout){
      /*connectToServer();
      getCounts();
      updateDisplay();
      mode = WAIT;*/
      sendDisplayString("noco");
      while(1){
        delay(100); //wait for reset
      }
      return -1;
    }
    delay(10);
  }
  while(client.available()>0){
    client.read();
  }
  return 1;
}


void isr_tapChanged(){
    long curr = millis();
    if((curr-lastInterruptTime)>50){ //ignore bouncing signal
      lastInterruptTime = curr;
      if(mode == WAIT){
        mode = COUNTDOWN;
        startPour = millis();
      }else if (mode == COUNTDOWN){
        mode = WAIT;
      }
      else if(mode == GETDATA || mode == POURING){
        if(endPour<startPour){
          endPour = millis();
        }
      }
    }
}

//Send Reset command
void sendResetCmd()
{
      Serial1.write(0x56);
      Serial1.write((byte)0);
      Serial1.write(0x26);
      Serial1.write((byte)0);
}

void blinkPin(int pin, int count){
    for(int i=0; i<count; i++){
        digitalWrite(pin,HIGH);
        delay(100);
        digitalWrite(pin,LOW);
        delay(100);
    }
}

//Send take picture command
void sendTakePhotoCmd()
{
      Serial1.write(0x56);
      Serial1.write((byte)0);
      Serial1.write(0x36);
      Serial1.write(0x01);
      Serial1.write((byte)0);
}

//Read data
void sendReadDataCmd()
{
      uint8_t MH = a/0x100;
      uint8_t ML = a%0x100;
      uint8_t KH = chunkIncr/0x100;
      uint8_t KL = chunkIncr%0x100;
      Serial1.write(0x56);
      Serial1.write((byte)0);
      Serial1.write(0x32);
      Serial1.write(0x0C);
      Serial1.write((byte)0);
      Serial1.write(0x0A);
      Serial1.write((byte)0);
      Serial1.write((byte)0);
      Serial1.write(MH);
      Serial1.write(ML);
      Serial1.write((byte)0);
      Serial1.write((byte)0);
      Serial1.write(KH);
      Serial1.write(KL);
      Serial1.write((byte)0);
      //Serial1.write(0x0A);
      Serial1.write(0x0A);
      //a+=0x20;            //address increases 32bytes according to buffer size
      a+=chunkIncr;  //address increases 64bytes according to buffer size

}

void stopTakePhotoCmd()
{
      Serial1.write(0x56);
      Serial1.write((byte)0);
      Serial1.write(0x36);
      Serial1.write(0x01);
      Serial1.write(0x03);
}

void changeSize(int level)
{
    Serial1.write((byte)0x56);
    Serial1.write((byte)0x00);
    Serial1.write((byte)0x31);
    Serial1.write((byte)0x05);
    Serial1.write((byte)0x04);
    Serial1.write((byte)0x01);
    Serial1.write((byte)0x00);
    Serial1.write((byte)0x19);
    switch(level){
        case 0:
            Serial1.write((byte)0x22); //160x120
            break;
        case 1:
            Serial1.write((byte)0x11); //320x240
            break;
        case 2:
            Serial1.write((byte)0x00); //640x480
            break;
    }
}

void changeBaudRate(unsigned long baudRate){
    Serial1.write((byte)0x56);
    Serial1.write((byte)0x00);
    Serial1.write((byte)0x24);
    Serial1.write((byte)0x03);
    Serial1.write((byte)0x01);
    switch(baudRate){
        case 38400:
            Serial1.write((byte)0x2A);
            Serial1.write((byte)0xF2);
            break;
        case 57600:
            Serial1.write((byte)0x1C);
            Serial1.write((byte)0x4C);
            break;
        case 115200:
            Serial1.write((byte)0x0D);
            Serial1.write((byte)0xA6);
            break;
        default:
            return;
    }
    Serial1.flush();
    delay(2);
    Serial1.end();
    Serial1.begin(baudRate);
}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay()
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x76);  // Clear display command
  digitalWrite(ssPin, HIGH);
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x77);
  SPI.transfer(decimals);
  digitalWrite(ssPin, HIGH);
}

void setCursor(byte cursorPos)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x79);
  SPI.transfer(cursorPos);
  digitalWrite(ssPin, HIGH);
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x7A);  // Set brightness command byte
  SPI.transfer(value);  // brightness data byte
  digitalWrite(ssPin, HIGH);
}

void sendDisplayString(char* toSend, int size)
{
  digitalWrite(ssPin, LOW);
  for (int i=0; i<size; i++)
  {
    SPI.transfer(toSend[i]);
  }
  digitalWrite(ssPin, HIGH);
}

void updateDisplay(){
  int count = 0;
  switch(displayMode){
    case KEG:
      count = kegCount;
      break;
    case DAY:
      count = dayCount;
      break;
    case WEEK:
      count = weekCount;
      break;
    case MONTH:
      count = monthCount;
      break;
  }
  char temp[10];
  sprintf(temp, "%04d", count);
  sendDisplayString(temp);
}

void checkDisplaySetting(){
    double threshLow = 0.25;
    double threshMid = 0.50;
    double threshHigh = 0.75;
    int mode;
    int val = analogRead(displaySettingPin);
    double valPercent = val/(double)4095;
    if(valPercent<threshLow){
      mode = DAY;
    }else if(valPercent>=threshLow && valPercent<threshMid){
      mode = WEEK;
    }else if(valPercent>=threshMid && valPercent<threshHigh){
      mode = MONTH;
    }else{
      mode = KEG;
    }
    if(mode!=displayMode){
      displayMode = mode;
      updateDisplay();
    }
}
