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
char server[] = "192.168.168.165";
int PORT = 443;
int connectToServer();
int readIntFromTCP(TCPClient);

/* Keg */
int statusPin = D7;
enum Modes{WAIT, TAKEPIC, GETDATA};
int mode = WAIT;
int dayCount = 0;
int kegCount = 0;


void isr_tapChanged();
void blinkPin(int,int);


void setup() {
    Serial1.begin(38400);
    Serial.begin(115200);
    connectToServer();
    pinMode(D2, INPUT);
    pinMode(statusPin, OUTPUT);
    digitalWrite(D2, HIGH);
    if(client.connected()){
        client.write("GETCOUNTKEG");
        delay(50);
        while(client.available()<=0){
            delay(10);
        }
        kegCount = readIntFromTCP();

        Serial.print("New keg count: ");
        Serial.println(kegCount);
        client.write("GETCOUNTDAY");
        delay(50);
        while(client.available()<=0){
            delay(10);
        }
        dayCount = readIntFromTCP();

        Serial.print("New day count: ");
        Serial.println(dayCount);

    }else{
        digitalWrite(statusPin, HIGH);
    }
    attachInterrupt(D2, isr_tapChanged, FALLING);
    changeSize(2);
    delay(100);
    sendResetCmd();
    delay(4000);
    changeBaudRate(115200);
    blinkPin(statusPin, 3);
}

void loop() {

    switch(mode)
    {
        case WAIT:
        {
            //do nothing
            delay(10);
            break;
        }
        case TAKEPIC:
        {
            //Take picture
            sendTakePhotoCmd();
            client.write("NEWBEER");
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
            byte byteArr[chunkSize];
            byte incomingByte;
            int j;
            int k;
            int count;
            a=0x0000;
            bool endFlag = false;
            client.write("SNDDATA");
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
            client.write("ENDDATA");
            client.write("DURATION:5");
            delay(3000);
            stopTakePhotoCmd();
            attachInterrupt(D2, isr_tapChanged, FALLING);
            digitalWrite(statusPin, LOW);
            mode = WAIT;
            break;
        }
        default:
        {
            delay(10);
            break;
        }

    }
}

//connect to tcp server
int connectToServer() {
  digitalWrite(statusPin, HIGH);
  if (client.connect(server, PORT)) {
      digitalWrite(statusPin, LOW);
      return 1; // successfully connected
  } else {
     digitalWrite(statusPin, LOW);
     return -1; // failed to connect
  }
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

void isr_tapChanged(){
    //Set display to "bEEr"
    //Start timer
    if(mode == WAIT){
        mode = TAKEPIC;
        detachInterrupt(D2);
    }else if(mode == TAKEPIC){
        //do nothing
    }else if(mode == GETDATA){
        //do nothing
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
