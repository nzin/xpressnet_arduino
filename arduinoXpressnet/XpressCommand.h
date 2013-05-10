#ifndef __XPRESSCOMMAND_H__
#define __XPRESSCOMMAND_H__
#include "HardwareSerial.h"

class XpressCommand {
  private:
    unsigned char _command[20];
    int _position;
    XpressCommand *_next;
    static bool _debug;
  
  public:
    XpressCommand();
    virtual ~XpressCommand();
    void pushData(unsigned char);
    void writeData(int commandPin,HardwareSerial&serial);
    static void pushStackCommand(XpressCommand *&link, XpressCommand *newCommand);
    XpressCommand* next();
    
    static void setDebug(bool debug);
};

#endif
