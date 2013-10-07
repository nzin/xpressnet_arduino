#include <stdbool.h>
#include "XpressCommand.h"

#define TRUE (0==0)
#define FALSE (0==1)


void sendSwitchCommand(int address,int up, int activate);
/********************* TCO state *******************/
// pin id, turnout id, isLeftOn1 (i.e. reverse the command state), currentState
int button[][4]={
  {31, 37, LOW, LOW},
  {32, 38, HIGH,LOW},
  {33, 12, HIGH,LOW},
  {34, 40, LOW, LOW},
};

void initiateTCOState() {
  int state;
  for (int i=0;i<sizeof(button)/(4*sizeof(int));i++) {
    state=digitalRead(button[i][0]);
    if (LOW==button[i][2]) {
      sendSwitchCommand(button[i][1],0,1);
      sendSwitchCommand(button[i][1],0,0);
    } else {
      sendSwitchCommand(button[i][1],1,1);
      sendSwitchCommand(button[i][1],1,0);
    }
  }
}

void checkTCOState() {
  int state;
  for (int i=0;i<sizeof(button)/(4*sizeof(int));i++) {
    state=digitalRead(button[i][0]);
    // state changed
    if (state!=button[i][3]) {
      Serial.print("state (");
      Serial.print(state);
      Serial.print(") changed for button");
      Serial.println(button[i][1]);
      if (state==button[i][2]) {
        sendSwitchCommand(button[i][1],0,1);
        sendSwitchCommand(button[i][1],0,0);
      } else {
        sendSwitchCommand(button[i][1],1,1);
        sendSwitchCommand(button[i][1],1,0);
      }
      button[i][3]=state;
    }
  }
}

/********************* xpressnet state machine *******************/
#define MYADDRESS 4
#define GIO_RXTX 7

// first stage decoding
#define OPERATION_RESYNC -1
#define OPERATION_NONE 0
#define OPERATION_NORMAL_INQUIRY 1
#define OPERATION_REQUEST_ACKNOWLEDGEbMENT 2
#define OPERATION_BROADCAST 3
#define OPERATION_FEEDBACK_BROADCAST 4
#define OPERATION_SERVICE_MODE 5
#define OTHER_DEVICE 20
int currentOperation=0;



unsigned char payload[16];
int dataPosition=-1;
int headerType=0;
int packetSize=0;
int xorCheck;

// second stage: state
int trackon=TRUE;
XpressCommand *stack=NULL;

void printPayload(int size) {
  for (int i=0;i<packetSize;i++) {
    Serial.print(payload[i],HEX);
    Serial.print(' ');
  }
    Serial.println();
}



void decodeStage2() {
  
  if (currentOperation==OPERATION_BROADCAST && headerType==1) {
    //track status
    if (payload[0]==1) trackon=TRUE;
    if (payload[0]==0) trackon=FALSE;
    
  } else if (currentOperation==OPERATION_BROADCAST && headerType==8) {
    // emergency stop
    if (payload[0]==0);
    
  } else if (currentOperation==OPERATION_BROADCAST && headerType==6) {  
    // entering service mode
    if (payload[0]==2);
    
  } else if (currentOperation==OPERATION_FEEDBACK_BROADCAST && headerType==4) {
    // feedback broadcast
    
  } else if (currentOperation==OPERATION_SERVICE_MODE && headerType==6) {
    if (payload[0]==0x10); // Service Mode response for Register and Paged Mode
    if (payload[0]==0x11); // Programming info. "Command station ready "
    if (payload[0]==0x12); // Programming info. " short-circuit "
    if (payload[0]==0x13); // Programming info. "Data byte not found"
    if (payload[0]==0x14); // Service Mode response for Direct CV mode
    if (payload[0]==0x1f); // Programming info. "Command station busy"
    if (payload[0]==0x21); // Software-version
    if (payload[0]==0x22); // Command station status indication response
    if (payload[0]==0x80) { Serial.println("transfer error"); } // Transfer Errors
    if (payload[0]==0x81); // Command station busy response
    if (payload[0]==0x82); // Instruction not supported by command station
    
  } else if (currentOperation==OPERATION_SERVICE_MODE && headerType==0x4) {
    // Accessory Decoder information response
    
  } else if (currentOperation==OPERATION_SERVICE_MODE && headerType==0x8) {
    // Locomotive is available for operation
    
  } else if (currentOperation==OPERATION_SERVICE_MODE && headerType==0xa) {
    // Locomotive is being operated by another device
    
  } else if (currentOperation==OPERATION_SERVICE_MODE && headerType==0xe) {
    // Locomotive information normal locomotive & many more!
  }
  
}


// Accessory Decoder information request
void sendAccessoryDecoderInformationRequest(int decoderAddress) {
  unsigned char data[4];
  data[0]=0x42;
  data[1]=decoderAddress>>2;
  data[2]=0x80+((decoderAddress>>1) & 1);
  data[3]=data[0]^data[1]^data[2];
  
  digitalWrite(GIO_RXTX,HIGH);
  Serial1.write9bit(data[0]);
  Serial1.write9bit(data[1]);
  Serial1.write9bit(data[2]);
  Serial1.write9bit(data[3]);
  delayMicroseconds(650);
  digitalWrite(GIO_RXTX,LOW);
  
  Serial.print("sent:");
  Serial.print((unsigned char)data[0],HEX);
  Serial.print(' ');
  Serial.print((unsigned char)data[1],HEX);
  Serial.print(' ');
  Serial.print((unsigned char)data[2],HEX);
  Serial.print(' ');
  Serial.println((unsigned char)data[3],HEX);
}



void sendAcknowledegmentResponse() {
  digitalWrite(GIO_RXTX,HIGH);
  Serial1.write(0x20);
  Serial1.write(0x20);
  delayMicroseconds(400);
  digitalWrite(GIO_RXTX,LOW);
}



void sendResumeOperationsRequest() {
  XpressCommand *c=new XpressCommand();
  c->pushData(0x21);
  c->pushData(0x81);
  XpressCommand::pushStackCommand(stack,c);
}



void sendStopOperationsRequest() {
  XpressCommand *c=new XpressCommand();
  c->pushData(0x21);
  c->pushData(0x80);
  XpressCommand::pushStackCommand(stack,c);
}



void sendStopallLocomotivesRequest() {
  XpressCommand *c=new XpressCommand();
  c->pushData(0x80);
  XpressCommand::pushStackCommand(stack,c);
}



// Accessory Decoder operation request
void sendSwitchCommand(int address,int up, int activate) {
  int rest=address%4;
  address>>=2;
  
  XpressCommand *c=new XpressCommand();
  c->pushData(0x52);
  c->pushData(address);
  c->pushData(0x80+(activate?8:0)+(rest<<1)+(up==1?1:0));
  XpressCommand::pushStackCommand(stack,c);
}



// return true if the byte is OK
int checkParity(int data) {
  int i,p=0;
  for (i=0;i<8;i++) {
    if ((data&1)!=0) p=!p;
    data >>=1;
  }
  return (p==0);
}


long count=0;
long ope=0;

int turnout=37; // 38 -1 car on compte a partir de 0

long initDcc=0;
void poolEvent() {
  XpressCommand *ptr;
  if (stack!=NULL) {
    ptr=stack;
    ptr->writeData(GIO_RXTX,Serial1);
    stack=ptr->next();
    delete ptr;
  }
  
  checkTCOState();
  
//  return;
//  count++;
  if (count==0) {
    count=millis();
  }
  if (millis()>count+500) {
    count=millis();
    switch(ope++) {
      case 0:
        sendSwitchCommand(turnout,1,1);
        sendSwitchCommand(turnout,1,0);
//        count=millis()-1800;
      Serial.println("send 0");
        break;
      case 1:
//        sendAccessoryDecoderInformationRequest(turnout);
//        count=millis()-1800;
        break;
      case 2:
//        sendResumeOperationsRequest();
//        sendAccessoryDecoderInformationRequest(turnout);
        break;
      case 3:
//        count=millis()-1800;
      Serial.println("send 3");
        break;
      case 4:
//        sendAccessoryDecoderInformationRequest(turnout);
//        count=millis()-1800;
        break;
      case 5:
//        sendAccessoryDecoderInformationRequest(turnout);
        break;
      case 6:
        sendSwitchCommand(turnout,0,1);
        sendSwitchCommand(turnout,0,0);
      Serial.println("send 6");
        break;
      case 7:
//        sendAccessoryDecoderInformationRequest(turnout);
        break;
      case 8:
//        sendAccessoryDecoderInformationRequest(turnout);
        break;
      case 9:
//        sendStopOperationsRequest();
        Serial.println("send 9");
        break;
      case 10:
//        sendAccessoryDecoderInformationRequest(turnout);
        break;
      case 11:
//        sendAccessoryDecoderInformationRequest(turnout);
        ope=0;
      break;
    }
//    sendStopOperationsRequest();
  }
}



int decodeXpressnet(int data) {
  int address;
  
  // after a XOR problem
  if (currentOperation==OPERATION_RESYNC) {
    address=data&0x1f;
    if ( ! (((data & 0x60) == 0x40) && (address==MYADDRESS))) { // P10AAAAA
      Serial.print("state=");
      Serial.print(currentOperation);
      Serial.print(" data=");
      Serial.println(data,HEX);
      return FALSE;
    }
    currentOperation=OPERATION_NONE;
  }
  
  // other xpress device?
  if (data<=255 && currentOperation==OPERATION_NONE) {
    Serial.println();
      currentOperation=OTHER_DEVICE;
      dataPosition=-1;
  }
  if (currentOperation==OTHER_DEVICE) {
    Serial.print(data,HEX); Serial.print(' '); }
  
  // call byte
  if (currentOperation==OPERATION_NONE) {
    // check parity
    if (checkParity(data&0xff)==FALSE) {
        Serial.print("bad parity : ");
        Serial.print(data&0xff,HEX);
        Serial.print(" ");
        Serial.print(data,HEX);
        Serial.println("");
      return 0;
    }
    
    address=data&0x1f;
    // Normal inquiry
    if ( ((data & 0x60) == 0x40) && (address==MYADDRESS)) { // P10AAAAA
//      Serial.println("normal inquiry");
      poolEvent();
      return TRUE;
      
      // Request Acknowledgement from Device
    } else if (((data & 0x60) == 0x00) && (address==MYADDRESS)) { // P00A AAAA
      Serial.println("request acknoledgement\n");
      sendAcknowledegmentResponse();
      return TRUE;
      
      // broadcast
    } else if ((data & 0x7f) == 0x60) { // P110 0000
      Serial.println("broadcast\n");
      currentOperation=OPERATION_BROADCAST;
      dataPosition=-1;
      return TRUE;
      
      // feedback broadcast
    } else if (data & 0x7f == 0x40) { // P100 0000
      Serial.println("feedback broadcast\n");
      currentOperation=OPERATION_FEEDBACK_BROADCAST;
      dataPosition=-1;
      return TRUE;
      
    } else if (((data & 0x60) == 0x60) && (address==MYADDRESS)) { // P11A AAAAA
      Serial.println("service mode\n");
      // Service Mode information response
      currentOperation=OPERATION_SERVICE_MODE;
      dataPosition=-1;
      return TRUE;
    }
//    Serial.print("other: ");
//    Serial.println(address);
    return FALSE;
  // after a call byte
  } else {
//    Serial.println("follow up");
    if (dataPosition==-1) { // HEADER
      xorCheck=data&0xff;
      headerType=(data>>4) & 0xf;
      packetSize=data&0xf;
      dataPosition=0;
    } else if (dataPosition<packetSize) { // PAYLOAD
      payload[dataPosition++]=data & 0xff;
      xorCheck^=(data&0xff);
    } else { //XOR
      if (xorCheck!=(data&0xff)) {
        Serial.println("incorrect XOR byte");
        currentOperation=OPERATION_RESYNC;
        dataPosition=-1;
        return FALSE;
      }
      //treat
      Serial.print("header=");
      Serial.println((unsigned char)((headerType<<4)+packetSize),HEX);
      printPayload(packetSize);
      Serial.println();
      if (currentOperation!=OTHER_DEVICE) {
        decodeStage2();
      }
      currentOperation=OPERATION_NONE;
    }
    return TRUE;
  }
}



void setup() {
  pinMode(GIO_RXTX,OUTPUT);
  digitalWrite(GIO_RXTX,HIGH);
  delay(1000);
  digitalWrite(GIO_RXTX,LOW);
  // put your setup code here, to run once:
  Serial.begin (115200);  // debugging prints
  Serial1.begin (62500, true);  // 9 bit mode
//  Serial1.begin (62500, false);  // 9 bit mode
  Serial.println ("--- starting ---");
}



void loop() {

//  delay(10);
//  sendSwitchCommand(39,0,0);
  // put your main code here, to run repeatedly: 


  if (Serial1.available ()) {
    decodeXpressnet((int) Serial1.read ());
    
//    Serial.println ((int) Serial1.read (), HEX);  

  }
}
