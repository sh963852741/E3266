#include "PacketStatistics.h"



pcap_if_t* PacketStatistics::LoadDevices()
{
	char errbuf[PCAP_ERRBUF_SIZE];

	/*获取设备列表*/
	if (pcap_findalldevs(&allDevices, errbuf) == -1)
	{
		return nullptr;
	}
	int i = 0;
	for (pcap_if_t* device = allDevices; device != nullptr; device = device->next)
	{
		++i;	
	}
	deviceNum = i;
	return allDevices;
}

bool PacketStatistics::SelectDevices(int deviceIndex)
{

	if (deviceIndex < 1 || deviceIndex > deviceNum)return false;

	/*选择适配器*/
	currentDevice = allDevices;
	for (int i = 0; i < deviceIndex - 1; i++)currentDevice = currentDevice->next;

}


bool PacketStatistics::BeginStatistics()
{
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* adhandle;
	/*打开设备*/
	if (!(adhandle = pcap_open_live(currentDevice->name,          // name of the device
		65536,            // portion of the packet to capture
						  // 65536 guarantees that the whole packet will be captured on all the link layers
		1,    // promiscuous mode
		1000,             // read timeout
		errbuf            // error buffer
	)))
	{
		return false;
	}
	printf("\n正在监听%s，按ESC退出监听...\n", currentDevice->description);

	/*初始化设置*/
	struct pcap_pkthdr* header;
	const u_char* pkt_data;
	int res;
	map<unsigned int,unsigned int> dataInByip;
	map<unsigned int, unsigned int> dataOutByip;
	map<mac_address, unsigned int> dataInBymac;
	map<mac_address, unsigned int> dataOutBymac;
	unsigned int dataLenPerMin = 0;
	ofstream writeFile("./data.csv", ios::out);
	writeFile << "时间,源MAC,源IP,目标MAC,目标IP,帧长度（字节）" << endl;
	unsigned long start = GetTickCount64(); // 开始计时
	while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) //开始循环获取数据帧
	{
		if (res == 0)continue;
		
		unsigned int length = header->len;
		mac_address dst = *(mac_address*)(pkt_data);
		mac_address src = *(mac_address*)(pkt_data + 0x6);
		unsigned int srcaddress = *(unsigned int*)(pkt_data + 0x1A); // 用4字节的unsigned int表示IP地址
		unsigned int dstaddress = *(unsigned int*)(pkt_data + 0x1E); // 用4字节的unsigned int表示IP地址

		/*开始统计数据*/
		int x = sizeof(unsigned int);
		if (mac == dst) // 如果是本机MAC则记录源地址
		{
			if (dataInBymac.find(src) == dataInBymac.end())dataInBymac.emplace(pair<mac_address, unsigned int>(src, length)); // 如果找不到记录就新建
			else dataInBymac[src] += length;
		}
		else // 如果不是本机MAC则记录目的地址
		{
			if(dataOutBymac.find(dst) == dataOutBymac.end())dataOutBymac.emplace(pair<mac_address, unsigned int>(dst, length));
			else dataOutBymac[dst] += length;
		}
		if (ip == dstaddress) // 如果是本机MAC则记录源地址
		{
			if (dataInByip.find(dstaddress) == dataInByip.end())dataInByip.emplace(pair<unsigned int, unsigned int>(dstaddress, length)); // 如果找不到记录就新建
			else dataInByip[dstaddress] += length;
		}
		else // 如果不是本机ip则记录目的地址
		{
			if (dataOutByip.find(dstaddress) == dataOutByip.end())dataOutByip.emplace(pair<unsigned int, unsigned int>(dstaddress, length));
			else dataOutByip[dstaddress] += length;
		}
		dataLenPerMin += length;

		/*将记录写入文件*/
		const unsigned char* myIP;
		struct tm ltime;
		char timestr[32];
		time_t local_tv_sec;

		/*转换时间格式*/
		local_tv_sec = header->ts.tv_sec;
		localtime_s(&ltime, &local_tv_sec);
		strftime(timestr, sizeof timestr, "%Y-%m-%d %H:%M:%S", &ltime);
		
		char info[18];
		sprintf(info, "%02X-%02X-%02X-%02X-%02X-%02X", src.byte[0], src.byte[1], src.byte[2], src.byte[3], src.byte[4], src.byte[5]);
		writeFile << timestr << ',' << info;
		myIP = (const unsigned char*)&srcaddress;
		writeFile << ','  << (int)myIP[0] << '.' << (int)myIP[1] << '.' << (int)myIP[2] << '.' << (int)myIP[3] << ',';
		sprintf(info, "%02X-%02X-%02X-%02X-%02X-%02X", dst.byte[0], dst.byte[1], dst.byte[2], dst.byte[3], dst.byte[4], dst.byte[5]);
		writeFile << info;
		myIP = (const unsigned char*)&dstaddress;
		writeFile << ','  << (int)myIP[0] << '.' << (int)myIP[1] << '.' << (int)myIP[2] << '.' << (int)myIP[3] << ',';
		writeFile << length << endl;

		// otherthing(nullptr, header, pkt_data);
		if (_kbhit())
		{
			char key = _getch();
			if (key == 27)
			{
				writeFile.close();
				break;
			}
		}

		/*每过一段时间输出数据*/
		if ((GetTickCount64()-start) / 5000) //设置统计周期
		{
			++current;
			cout << "统计周期中共监听到数据：" << dataLenPerMin << "字节" << endl;
			cout << "-------------------------------------------------" << endl;
			cout << "目标为本机Mac的数据" << endl;
			// cout.setf(iostream::hex);
			for (map<mac_address, unsigned int>::iterator i = dataInBymac.begin(); i != dataInBymac.end(); i++)
			{
				cout << "源MAC：" <<hex<< (int)i->first.byte[0] << '-' << (int)i->first.byte[1] << '-' << (int)i->first.byte[2]
					<< '-' << (int)i->first.byte[3] << '-' << (int)i->first.byte[4] << '-' << (int)i->first.byte[5] << '\t' <<dec<<(int)i->second << "字节" << endl;
			}
			cout << "-------------------------------------------------" << endl;
			cout << "目标为本机IP的数据：" << endl;
			for (map<unsigned int, unsigned int>::iterator i = dataInByip.begin(); i != dataInByip.end(); i++)
			{
				const unsigned char* myIP = (const unsigned char*)&i->first;
				cout << "源IP：" << dec << (int)myIP[0] << '.' << (int)myIP[1] << '.' << (int)myIP[2] << '.' << (int)myIP[3] << '\t'<<dec << i->second << "字节" << endl;
			}
			cout << "-------------------------------------------------" << endl;
			cout << "目标为其他MAC的数据：" << endl;
			for (map<mac_address, unsigned int>::iterator i = dataOutBymac.begin(); i != dataOutBymac.end(); i++)
			{
				cout << "目标MAC：" << hex << (int)i->first.byte[0] << '-' << (int)i->first.byte[1] << '-' << (int)i->first.byte[2]
					<< '-' << (int)i->first.byte[3] << '-' << (int)i->first.byte[4] << '-' << (int)i->first.byte[5] << '\t' <<dec<<(int)i->second << "字节" << endl;
			}
			cout << "-------------------------------------------------" << endl;
			cout << "目标为其他IP的数据：" << endl;
			for (map<unsigned int, unsigned int>::iterator i = dataOutByip.begin(); i != dataOutByip.end(); i++)
			{
				const unsigned char* myIP = (const unsigned char*)&i->first;
				cout << "目标IP：" << dec << (int)myIP[0] << '.' << (int)myIP[1] << '.' << (int)myIP[2] << '.' << (int)myIP[3] << '\t' << dec << i->second << "字节" << endl;
			}
			if(dataLenPerMin> maxdata)
			{
				cout << "本周期流量超出" << maxdata << "字节的限制!" << endl;
			}
			cout << "=================================================" << endl;

			/*清空统计记录*/
			dataInByip.clear();
			dataOutByip.clear();
			dataInBymac.clear();
			dataOutBymac.clear();
			dataLenPerMin = 0;

			/*重新开始计时*/
			start = GetTickCount64();
		}
	}
	if (res == -1) {
		return false;
	}

	return true;
}

PacketStatistics::PacketStatistics()
{
	u_char ucLocalMac[6];   //本机MAC地址
	DWORD dwLocalIp = 0;     //本机Ip
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	ULONG uLen = 0;

	//为适配器申请内存
	GetAdaptersInfo(pAdapterInfo, &uLen);
	pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, uLen);

	//取得适配器的结构信息
	if (GetAdaptersInfo(pAdapterInfo, &uLen) == ERROR_SUCCESS)
	{
		if (pAdapterInfo != NULL)
		{
			memcpy(&mac, pAdapterInfo->Address, 6);
			ip = inet_addr(pAdapterInfo->IpAddressList.IpAddress.String);
		}
	}

}

PacketStatistics::~PacketStatistics()
{
	pcap_freealldevs(allDevices);
}

