 //***********************************************************************************************
 //*  Copyright (c) Federal University of Para, brazil - 2012                                    *
 //*  Developed at the Research Group on Computer Network and Multimedia Communication (GERCOM)  *
 //*  All rights reserved                                                                        *
 //*                                                                                             *
 //*  Permission to use, copy, modify, and distribute this protocol and its documentation for    *
 //*  any purpose, without fee, and without written agreement is hereby granted, provided that   *
 //*  the above copyright notice, and the author appear in all copies of this protocol.          *
 //*                                                                                             *
 //*  Author(s): Kassio Machado <kassio.comp@gmail.com> ,                                        *
 //*             Thiago Oliveira <thiagofdso.ufpa@gmail.com>                                     *
 //***********************************************************************************************

cplusplus {{
#include "PacketId_m.h"
}}

class PacketId;
//id =2
packet LabileRREPPacket extends PacketId{
	bool flagR;
	bool flagA;
	int prefixSz;
	int hopCount;
	int weakLinks;
	string originator;
	unsigned long destinationSeqNum;
	string destinationLABILE;
	double lifetime;
}


