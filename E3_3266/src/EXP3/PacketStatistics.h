#pragma once
#include "pcap.h"
#include <iphlpapi.h>
#include <map>
#include <fstream>
#include <iostream>
#include <conio.h>
using namespace std;

struct mac_address
{
	u_char byte[6];

	unsigned char operator ==(const mac_address& val) const
	{
		for (int i = 0; i < 6; ++i)
		{
			if (this->byte[i] != val.byte[i])return false;
		}
		return true;
	}

	bool operator < (const mac_address &val) const
	{
		for (int i = 0; i < 6; ++i)
		{
			if (this->byte[i] < val.byte[i])return true;
			else if (this->byte[i] == val.byte[i])continue;
			else return false;
		}
		return false;
	}
	
};

class PacketStatistics
{
public:
	void(*otherthing)(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data);
	pcap_if_t* LoadDevices();
	bool SelectDevices(int deviceIndex);
	bool BeginStatistics();
	int maxdata;
	PacketStatistics();
	~PacketStatistics();
private:
	int current = 0;
	unsigned int ip;
	mac_address mac;
	pcap_if_t* allDevices;
	int deviceNum;
	pcap_if_t* currentDevice;
};
