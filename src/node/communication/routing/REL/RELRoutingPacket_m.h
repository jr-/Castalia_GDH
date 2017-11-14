//
// Generated file, do not edit! Created by nedtool 4.6 from src/node/communication/routing/REL/RELRoutingPacket.msg.
//

#ifndef _RELROUTINGPACKET_M_H_
#define _RELROUTINGPACKET_M_H_

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
 * Class generated from <tt>src/node/communication/routing/REL/RELRoutingPacket.msg:21</tt> by nedtool.
 * <pre>
 * //id =1
 * packet RELRREQPacket extends PacketId
 * {
 *     int hopCount;
 *     int linkCost;
 *     int rreqID;
 *     int energy;
 *     string sourceREL;
 *     unsigned long sourceSeqNum;
 *     unsigned long destinationSeqNum;
 *     string destinationREL;
 * }
 * </pre>
 */
class RELRREQPacket : public ::PacketId
{
  protected:
    int hopCount_var;
    int linkCost_var;
    int rreqID_var;
    int energy_var;
    opp_string sourceREL_var;
    unsigned long sourceSeqNum_var;
    unsigned long destinationSeqNum_var;
    opp_string destinationREL_var;

  private:
    void copy(const RELRREQPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RELRREQPacket&);

  public:
    RELRREQPacket(const char *name=NULL, int kind=0);
    RELRREQPacket(const RELRREQPacket& other);
    virtual ~RELRREQPacket();
    RELRREQPacket& operator=(const RELRREQPacket& other);
    virtual RELRREQPacket *dup() const {return new RELRREQPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getHopCount() const;
    virtual void setHopCount(int hopCount);
    virtual int getLinkCost() const;
    virtual void setLinkCost(int linkCost);
    virtual int getRreqID() const;
    virtual void setRreqID(int rreqID);
    virtual int getEnergy() const;
    virtual void setEnergy(int energy);
    virtual const char * getSourceREL() const;
    virtual void setSourceREL(const char * sourceREL);
    virtual unsigned long getSourceSeqNum() const;
    virtual void setSourceSeqNum(unsigned long sourceSeqNum);
    virtual unsigned long getDestinationSeqNum() const;
    virtual void setDestinationSeqNum(unsigned long destinationSeqNum);
    virtual const char * getDestinationREL() const;
    virtual void setDestinationREL(const char * destinationREL);
};

inline void doPacking(cCommBuffer *b, RELRREQPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, RELRREQPacket& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>src/node/communication/routing/REL/RELRoutingPacket.msg:32</tt> by nedtool.
 * <pre>
 * //id =5
 * packet RELHelloMessage extends PacketId
 * {
 * }
 * </pre>
 */
class RELHelloMessage : public ::PacketId
{
  protected:

  private:
    void copy(const RELHelloMessage& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RELHelloMessage&);

  public:
    RELHelloMessage(const char *name=NULL, int kind=0);
    RELHelloMessage(const RELHelloMessage& other);
    virtual ~RELHelloMessage();
    RELHelloMessage& operator=(const RELHelloMessage& other);
    virtual RELHelloMessage *dup() const {return new RELHelloMessage(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, RELHelloMessage& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, RELHelloMessage& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>src/node/communication/routing/REL/RELRoutingPacket.msg:36</tt> by nedtool.
 * <pre>
 * //id =4
 * packet RELRREPAckPacket extends PacketId
 * {
 * }
 * </pre>
 */
class RELRREPAckPacket : public ::PacketId
{
  protected:

  private:
    void copy(const RELRREPAckPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RELRREPAckPacket&);

  public:
    RELRREPAckPacket(const char *name=NULL, int kind=0);
    RELRREPAckPacket(const RELRREPAckPacket& other);
    virtual ~RELRREPAckPacket();
    RELRREPAckPacket& operator=(const RELRREPAckPacket& other);
    virtual RELRREPAckPacket *dup() const {return new RELRREPAckPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, RELRREPAckPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, RELRREPAckPacket& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>src/node/communication/routing/REL/RELRoutingPacket.msg:41</tt> by nedtool.
 * <pre>
 * //id =3
 * packet RELRERRPacket extends PacketId
 * {
 *     int destCount;
 *     string unreachableDestAddr[];
 *     unsigned long unreachableDestSeqNum[];
 * }
 * </pre>
 */
class RELRERRPacket : public ::PacketId
{
  protected:
    int destCount_var;
    opp_string *unreachableDestAddr_var; // array ptr
    unsigned int unreachableDestAddr_arraysize;
    unsigned long *unreachableDestSeqNum_var; // array ptr
    unsigned int unreachableDestSeqNum_arraysize;

  private:
    void copy(const RELRERRPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RELRERRPacket&);

  public:
    RELRERRPacket(const char *name=NULL, int kind=0);
    RELRERRPacket(const RELRERRPacket& other);
    virtual ~RELRERRPacket();
    RELRERRPacket& operator=(const RELRERRPacket& other);
    virtual RELRERRPacket *dup() const {return new RELRERRPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getDestCount() const;
    virtual void setDestCount(int destCount);
    virtual void setUnreachableDestAddrArraySize(unsigned int size);
    virtual unsigned int getUnreachableDestAddrArraySize() const;
    virtual const char * getUnreachableDestAddr(unsigned int k) const;
    virtual void setUnreachableDestAddr(unsigned int k, const char * unreachableDestAddr);
    virtual void setUnreachableDestSeqNumArraySize(unsigned int size);
    virtual unsigned int getUnreachableDestSeqNumArraySize() const;
    virtual unsigned long getUnreachableDestSeqNum(unsigned int k) const;
    virtual void setUnreachableDestSeqNum(unsigned int k, unsigned long unreachableDestSeqNum);
};

inline void doPacking(cCommBuffer *b, RELRERRPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, RELRERRPacket& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>src/node/communication/routing/REL/RELRoutingPacket.msg:48</tt> by nedtool.
 * <pre>
 * //id =7
 * packet RELRADVPacket extends PacketId
 * {
 *     int energy;
 *     string originator;
 * 
 * }
 * </pre>
 */
class RELRADVPacket : public ::PacketId
{
  protected:
    int energy_var;
    opp_string originator_var;

  private:
    void copy(const RELRADVPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RELRADVPacket&);

  public:
    RELRADVPacket(const char *name=NULL, int kind=0);
    RELRADVPacket(const RELRADVPacket& other);
    virtual ~RELRADVPacket();
    RELRADVPacket& operator=(const RELRADVPacket& other);
    virtual RELRADVPacket *dup() const {return new RELRADVPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getEnergy() const;
    virtual void setEnergy(int energy);
    virtual const char * getOriginator() const;
    virtual void setOriginator(const char * originator);
};

inline void doPacking(cCommBuffer *b, RELRADVPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, RELRADVPacket& obj) {obj.parsimUnpack(b);}


#endif // ifndef _RELROUTINGPACKET_M_H_

