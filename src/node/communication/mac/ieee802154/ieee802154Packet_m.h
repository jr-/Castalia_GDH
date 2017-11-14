//
// Generated file, do not edit! Created by nedtool 4.6 from src/node/communication/mac/ieee802154/ieee802154Packet.msg.
//

#ifndef _IEEE802154PACKET_M_H_
#define _IEEE802154PACKET_M_H_

#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0406
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



// cplusplus {{
#include "MacPacket_m.h"
// }}

/**
 * Enum generated from <tt>src/node/communication/mac/ieee802154/ieee802154Packet.msg:19</tt> by nedtool.
 * <pre>
 * enum IEEE802154Packet_type
 * {
 * 
 *     IEEE802154_BEACON_PACKET = 1001;
 *     IEEE802154_ASSOCIATE_PACKET = 1002;
 *     IEEE802154_DATA_PACKET = 1003;
 *     IEEE802154_ACK_PACKET = 1004;
 *     IEEE802154_GTS_REQUEST_PACKET = 1005;
 * }
 * </pre>
 */
enum IEEE802154Packet_type {
    IEEE802154_BEACON_PACKET = 1001,
    IEEE802154_ASSOCIATE_PACKET = 1002,
    IEEE802154_DATA_PACKET = 1003,
    IEEE802154_ACK_PACKET = 1004,
    IEEE802154_GTS_REQUEST_PACKET = 1005
};

/**
 * Class generated from <tt>src/node/communication/mac/ieee802154/ieee802154Packet.msg:27</tt> by nedtool.
 * <pre>
 * packet IEEE802154Packet extends MacPacket
 * {
 *     int IEEE802154PacketType @enum(IEEE802154Packet_type);
 *     bool ackRequest;
 *     char seqNum;
 * }
 * </pre>
 */
class IEEE802154Packet : public ::MacPacket
{
  protected:
    int IEEE802154PacketType_var;
    bool ackRequest_var;
    char seqNum_var;

  private:
    void copy(const IEEE802154Packet& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const IEEE802154Packet&);

  public:
    IEEE802154Packet(const char *name=NULL, int kind=0);
    IEEE802154Packet(const IEEE802154Packet& other);
    virtual ~IEEE802154Packet();
    IEEE802154Packet& operator=(const IEEE802154Packet& other);
    virtual IEEE802154Packet *dup() const {return new IEEE802154Packet(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getIEEE802154PacketType() const;
    virtual void setIEEE802154PacketType(int IEEE802154PacketType);
    virtual bool getAckRequest() const;
    virtual void setAckRequest(bool ackRequest);
    virtual char getSeqNum() const;
    virtual void setSeqNum(char seqNum);
};

inline void doPacking(cCommBuffer *b, IEEE802154Packet& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, IEEE802154Packet& obj) {obj.parsimUnpack(b);}


#endif // ifndef _IEEE802154PACKET_M_H_

