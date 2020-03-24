#include "pcap.h"
#include "PacketStatistics.h"
#include <windows.h>
#include "Iphlpapi.h"
#include <iostream>
using namespace std;

#pragma comment(lib,"ws2_32")
#pragma comment(lib,"Iphlpapi")

void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data);

int main()
{
	PacketStatistics x;
	pcap_if_t* alldevices = x.LoadDevices();
	int maxdata;
	int i = 0;
	for (pcap_if_t* device = alldevices; device != nullptr; device = device->next)
	{
		printf("%d. %s", ++i, device->name);
		if (device->description)
			printf(" (%s)\n", device->description);
		else
			printf(" (设备描述不可用)\n");
	}
	if (i == 0)
	{
		printf("\n未找到接口，请确认WinPcap已安装。\n");
		return 0;
	}
	int inum;
	printf("请输入输入接口序号 (1-%d):", i);
	scanf_s("%d", &inum);
	x.otherthing = packet_handler;
	x.SelectDevices(inum);
	printf("请输入最大数据流量（字节）：");
	scanf_s("%d", &maxdata);
	x.maxdata = maxdata;
	x.BeginStatistics();

	return 0;

}
/* Callback function invoked by libpcap for every incoming packet */
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data)
{
	struct tm ltime;
	char timestr[32];
	time_t local_tv_sec;

	/*
	 * unused variables
	 */
	(VOID)(param);
	(VOID)(pkt_data);

	/* convert the timestamp to readable format */
	local_tv_sec = header->ts.tv_sec;
	localtime_s(&ltime, &local_tv_sec);
	strftime(timestr, sizeof timestr, "%Y-%m-%d %H:%M:%S", &ltime);

	printf("%s\t", timestr);

	mac_address dst = *(mac_address*)(pkt_data);
	mac_address src = *(mac_address*)(pkt_data + 0x6);
	unsigned int srcaddress = *(unsigned int*)(pkt_data + 0x1A);
	unsigned int dstaddress = *(unsigned int*)(pkt_data + 0x1E);
	printf("%02X-%02X-%02X-%02X-%02X-%02X\t", src.byte[0], src.byte[1], src.byte[2], src.byte[3], src.byte[4], src.byte[5]);
	printf("%d.%d.%d.%d\t", ((unsigned char*)&srcaddress)[0], ((unsigned char*)&srcaddress)[1], ((unsigned char*)&srcaddress)[2], ((unsigned char*)&srcaddress)[3]);
	printf("%02X-%02X-%02X-%02X-%02X-%02X\t", dst.byte[0], dst.byte[1], dst.byte[2], dst.byte[3], dst.byte[4], dst.byte[5]);
	printf("%d.%d.%d.%d\t", ((unsigned char*)&dstaddress)[0], ((unsigned char*)&dstaddress)[2], ((unsigned char*)&dstaddress)[2], ((unsigned char*)&dstaddress)[3]);
	printf("%d\n", header->len);
	
}