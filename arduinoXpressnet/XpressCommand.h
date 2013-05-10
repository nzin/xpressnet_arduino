#ifndef __XPRESSCOMMAND_H__
#define __XPRESSCOMMAND_H__
#include "HardwareSerial.h"

/**
 * XpressCommand is a class to help write and send Xpressnet command
 * (from an Xpressnet device, not a host)
 *
 * It is also a chain list, because we cannot send commands when we want,
 * so there is facility to store them in a FIFO way, and the entity
 * responsible to send commands, will pop them, and will fetch them
 * one by one (when it receives a "normal inquiry" from the master xpressnet).
 *
 * @author Nicolas Zin <nicolas.zin@gmail.com>
 */
class XpressCommand {
  private:
    unsigned char _command[20];
    int _position;
    XpressCommand *_next;
    static bool _debug;
  
  public:
    XpressCommand();
    virtual ~XpressCommand();
    // add a byte to the payload (max 20 bytes)
    void pushData(unsigned char);
    // send the payload to the serial link (and calculate the last xor byte)
    void writeData(int commandPin,HardwareSerial&serial);
    // helper object: add to the XpressCommand chained list a command
    static void pushStackCommand(XpressCommand *&link, XpressCommand *newCommand);
    // used to walk the chain list
    XpressCommand* next();
    
    // if you want to print payload data to Serial (console)
    static void setDebug(bool debug);
};

#endif
