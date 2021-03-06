//
// Generated file, do not edit! Created by nedtool 4.6 from src/node/application/geoSync/DataPacket.msg.
//

#ifndef _DATAPACKET_M_H_
#define _DATAPACKET_M_H_

#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0406
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



// cplusplus {{
#include "GeoSyncPacketType_m.h"
// }}

/**
 * Class generated from <tt>src/node/application/geoSync/DataPacket.msg:22</tt> by nedtool.
 * <pre>
 * packet DataPacket extends GeoSyncPacketType
 * {
 * }
 * </pre>
 */
class DataPacket : public ::GeoSyncPacketType
{
  protected:

  private:
    void copy(const DataPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const DataPacket&);

  public:
    DataPacket(const char *name=NULL, int kind=0);
    DataPacket(const DataPacket& other);
    virtual ~DataPacket();
    DataPacket& operator=(const DataPacket& other);
    virtual DataPacket *dup() const {return new DataPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, DataPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, DataPacket& obj) {obj.parsimUnpack(b);}


#endif // ifndef _DATAPACKET_M_H_

