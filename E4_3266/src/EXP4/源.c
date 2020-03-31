#include <stdio.h>
#include "pcap.h"
#pragma comment (lib, "ws2_32.lib")
// void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data);
/* 4 bytes IP address */
typedef struct ip_address {
	u_char byte1;
	u_char byte2;
	u_char byte3;
	u_char byte4;
}ip_address;

/* IPv4 header */
typedef struct ip_header {
	u_char  ver_ihl;        // Version (4 bits) + Internet header length (4 bits)
	u_char  tos;            // Type of service 
	u_short tlen;           // Total length 
	u_short identification; // Identification
	u_short flags_fo;       // Flags (3 bits) + Fragment offset (13 bits)
	u_char  ttl;            // Time to live
	u_char  proto;          // Protocol
	u_short crc;            // Header checksum
	ip_address  saddr;      // Source address
	ip_address  daddr;      // Destination address
	u_int   op_pad;         // Option + Padding
}ip_header;

/* TCP 报文头*/
typedef struct tcp_header {
	u_short sport;			//源端口号
	u_short dport;			//目的端口号
	u_int sequence;			//序列号
	u_int ack;				//确认号
	u_int reseverd : 4;		//6位中的4位首部长度
	u_int head_length : 4;	//TCP头部长度
	u_char flags : 6;		//6位标志位
	u_int reseverd2 : 2;	//6位中的2位首部长度
	u_short window_size;	//16位窗口大小
	u_short chechsum;		//16位tcp检验和
	u_short urgt_p;			//16位紧急指针
}tcp_header;

char* username;
char* password;
FILE* fp;
pcap_t* adhandle;
int main()
{
	pcap_if_t* allDevices;
	char errbuf[PCAP_ERRBUF_SIZE];
	fopen_s(&fp, "data.csv", "w");
	/*获取设备列表*/
	if (pcap_findalldevs(&allDevices, errbuf) == -1)
	{
		return NULL;
	}
	/*打印设备列表并统计数目*/
	int deviceNum = 0;
	for (pcap_if_t* device = allDevices; device != NULL; device = device->next)
	{
		printf("%d. %s", ++deviceNum, device->name);
		if (device->description)
			printf("(%s)\n", device->description);
		else
			printf("(设备描述不可用)\n");
	}
	if (deviceNum == 0)
	{
		printf("\n未找到接口，请确认WinPcap已安装。\n");
		return 0;
	}
	/*选择适配器*/
	int deviceIndex = 0;
	printf("请输入输入接口序号 (1-%d):", deviceIndex);
	scanf_s("%d", &deviceIndex);
	if (deviceIndex < 1 || deviceIndex > deviceNum)
	{
		pcap_freealldevs(allDevices);
		return -1;
	}
	pcap_if_t* currentDevice = allDevices;
	for (int i = 0; i < deviceIndex - 1; i++)currentDevice = currentDevice->next;
	/*打开设备*/
	if (!(adhandle = pcap_open_live(currentDevice->name,          // name of the device
		65536,            // portion of the packet to capture
						  // 65536 guarantees that the whole packet will be captured on all the link layers
		1,    // promiscuous mode
		1000,             // read timeout
		errbuf            // error buffer
		)))
	{
		return 0;
	}
	/*编译过滤器*/
	u_int netmask;
	struct bpf_program fcode;
	netmask = ((struct sockaddr_in*)(currentDevice->addresses->netmask))->sin_addr.S_un.S_addr;
	if (pcap_compile(adhandle, &fcode, "tcp dst or src port ftp", 1, netmask) < 0)
	{
		fprintf(stderr, "\n无法编译过滤器，请检查语法\n");
		/*释放设备列表*/
		pcap_freealldevs(allDevices);
		return -1;
	}
	/*设置过滤器*/
	if (pcap_setfilter(adhandle, &fcode) < 0)
	{
		fprintf(stderr, "\n设置过滤器时发生错误\n");
		/*释放设备列表*/
		pcap_freealldevs(allDevices);
		return -1;
	}

	int res;
	struct pcap_pkthdr* header;
	const u_char* pkt_data;
	fprintf_s(fp, "时间,源 MAC,源 IP,目标 MAC,目标 IP,登录名,口令,成功与否");
	printf("\n正在监听%s的FTP命令信息，按ESC键退出...\n", currentDevice->description);
	pcap_freealldevs(allDevices);

	/* start the capture */
	while (!(_kbhit() && _getch() == 0x1b)&&(res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0)
	{
		if (res == 0)continue;

		struct tm ltime;
		char timestr[32], src_mac[18], dst_mac[18], src_ip[16], dst_ip[16];
		char* tcp_command_begin, * tcp_command;
		ip_header* ih; // IP头
		tcp_header* th; // TCP头
		unsigned short sport, dport;
		time_t local_tv_sec;

		/* 解析时间戳 */
		local_tv_sec = header->ts.tv_sec;
		localtime_s(&ltime, &local_tv_sec);
		strftime(timestr, sizeof timestr, "\n%Y-%m-%d %H:%M:%S", &ltime);

		/*获取IP头*/
		ih = (ip_header*)(pkt_data + 14);
		/*获取MAC以及IP地址*/
		sprintf_s(dst_mac, 18, "%02X-%02X-%02X-%02X-%02X-%02X",
			*pkt_data, *(pkt_data + 1), *(pkt_data + 2), *(pkt_data + 3), *(pkt_data + 4), *(pkt_data + 5));
		sprintf_s(src_mac, 18, "%02X-%02X-%02X-%02X-%02X-%02X",
			*(pkt_data + 6), *(pkt_data + 7), *(pkt_data + 8), *(pkt_data + 9), *(pkt_data + 10), *(pkt_data + 11));
		sprintf_s(src_ip, 16, "%d.%d.%d.%d", *(pkt_data + 0x1A), *(pkt_data + 0x1B), *(pkt_data + 0x1C), *(pkt_data + 0x1D));
		sprintf_s(dst_ip, 16, "%d.%d.%d.%d", *(pkt_data + 0x1E), *(pkt_data + 0x1F), *(pkt_data + 0x20), *(pkt_data + 0x21));
		/*获取TCP头*/
		ih->tlen = ntohs(ih->tlen);
		th = (tcp_header*)((u_char*)ih + (ih->ver_ihl & 0xf) * 4);
		/*获取端口*/
		sport = ntohs(th->sport);
		dport = ntohs(th->dport);
		/*获取TCP内容*/
		tcp_command_begin = (char*)((u_char*)th + (th->head_length) * 4);
		int command_length = ih->tlen - th->head_length * 4 - (ih->ver_ihl & 0xf) * 4;
		tcp_command = malloc(command_length + 1);
		tcp_command[command_length] = '\0';
		strncpy_s(tcp_command, command_length + 1, tcp_command_begin, command_length);
		if (!strncmp(tcp_command, "USER", 4))
		{
			username = malloc(command_length - 4);
			strncpy_s(username, command_length - 4, tcp_command_begin + 5, command_length - 7);
		}
		else if (!strncmp(tcp_command, "PASS", 4))
		{
			password = malloc(command_length - 4);
			strncpy_s(password, command_length - 4, tcp_command_begin + 5, command_length - 7);
		}
		else if (!strncmp(tcp_command, "530", 3))
		{
			fprintf_s(fp, "%s,%s,%s:%d,%s,%s:%d,%s,%s,FAILED", timestr, src_mac, src_ip, sport, dst_mac, dst_ip, dport, username, password);
		}
		else if (!strncmp(tcp_command, "230", 3))
		{
			fprintf_s(fp, "%s,%s,%s:%d,%s,%s:%d,%s,%s,SUCCEED", timestr, src_mac, src_ip, sport, dst_mac, dst_ip, dport, username, password);
		}
		printf("%s", tcp_command);
	}
	
	fclose(fp);
	return 0;
}