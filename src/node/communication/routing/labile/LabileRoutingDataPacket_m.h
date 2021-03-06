//
// Generated file, do not edit! Created by nedtool 4.6 from src/node/communication/routing/labile/LabileRoutingDataPacket.msg.
//

#ifndef _LABILEROUTINGDATAPACKET_M_H_
#define _LABILEROUTINGDATAPACKET_M_H_

#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0406
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



// cplusplus {{
#include "PacketId_m.h"
// }}

/**
 * Class generated from <tt>src/node/communication/routing/labile/LabileRoutingDataPacket.msg:20</tt> by nedtool.
 * <pre>
 * //id =6
 * packet LabileDataPacket extends PacketId
 * {
 *     string destinationLABILE;
 * }
 * </pre>
 */
class LabileDataPacket : public ::PacketId
{
  protected:
    opp_string destinationLABILE_var;

  private:
    void copy(const LabileDataPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const LabileDataPacket&);

  public:
    LabileDataPacket(const char *name=NULL, int kind=0);
    LabileDataPacket(const LabileDataPacket& other);
    virtual ~LabileDataPacket();
    LabileDataPacket& operator=(const LabileDataPacket& other);
    virtual LabileDataPacket *dup() const {return new LabileDataPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual const char * getDestinationLABILE() const;
    virtual void setDestinationLABILE(const char * destinationLABILE);
};

inline void doPacking(cCommBuffer *b, LabileDataPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, LabileDataPacket& obj) {obj.parsimUnpack(b);}


#endif // ifndef _LABILEROUTINGDATAPACKET_M_H_

