#include "XpressCommand.h"
#include <Arduino.h>

bool XpressCommand::_debug=false;

XpressCommand::XpressCommand() {
  _next=NULL;
  _position=0;
}



XpressCommand::~XpressCommand() {
}



void XpressCommand::pushData(unsigned char data) {
  if (_position<19) {
    _command[_position++]=data;
  }
}



void XpressCommand::writeData(int commandPin,HardwareSerial&serial) {
  unsigned char xorByte=0;
  digitalWrite(commandPin,HIGH);
  for(int i=0;i<_position;i++) {
    serial.write(_command[i]);
    xorByte ^= _command[i];
  }
  serial.write(xorByte);
  
  // wait flush
  delayMicroseconds(220*(_position+1));
  
  digitalWrite(commandPin,LOW);
  
  if (_debug) {
    for(int i=0;i<_position;i++) {
      Serial.print((unsigned char)_command[i],HEX);
      Serial.print(' ');
    }
    Serial.println((unsigned char)xorByte,HEX);
  }
}


 
void XpressCommand::pushStackCommand(XpressCommand *&link, XpressCommand *newCommand) {
  if (link==NULL) {
    link=newCommand;
    return;
  }
  while (link->_next!=NULL) {
    link=link->_next;
  }
  link->_next=newCommand;
}



XpressCommand* XpressCommand::next() {
  return _next;
}



void XpressCommand::setDebug(bool debug) {
  _debug=debug;
}


