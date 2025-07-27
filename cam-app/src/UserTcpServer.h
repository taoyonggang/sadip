#pragma once
//
// CamTcpServer.cpp
//
// This sample demonstrates the SocketReactor and SocketAcceptor classes.
//
#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0


#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/ParallelSocketAcceptor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/NObserver.h"
#include "Poco/Exception.h"
#include "Poco/Thread.h"
#include "Poco/FIFOBuffer.h"
#include "Poco/Delegate.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"

#include "Poco/LocalDateTime.h"
#include "Poco/DateTime.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeParser.h"
#include "../utils/log/BaseLog.h"
//#include "FusionPathDatas.pb.h"
//#include "../../utils/ThreadSafeQueue.h"
//#include "TcpRecv.h"
#include <thread>


#include <iostream>

//using namespace cn::seisys::rbx::comm::bean::multi;

//cn::ThreadSafeQueue<cn::seisys::rbx::comm::bean::multi::FusionPathDatas> FusionQueue;
//cn::ThreadSafeQueue<std::string> FusionQueue;

//const int MAX_MSG = 20000;
//
//struct PacketHeader_1
//{
//	uint16_t header;        // 报文头
//	uint8_t protoType;      //协议类型
//	uint8_t protoVersion;   //协议版本
//	char fusionAlNums[15];  //融合算法编号
//	uint8_t msgNum;         //报文序号
//	uint16_t token;         // token
//	uint32_t msgLength;     // 报文长度 
//};
//
//struct Packet
//{
//	char* msg;  //报文内容
//};
//
//struct PacketCheck
//{
//	uint16_t checkCode;  // CRC16校验
//	uint16_t endCode;    // 结束码
//};
//
//// CRC16校验码验证
//unsigned char auchCRCHi[] = {
//	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
//	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40 };
//
//unsigned char auchCRCLo[] = {
//	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
//	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40 };
//unsigned int N_CRC16(unsigned char* updata, unsigned int len)
//{
//	unsigned char uchCRCHi = 0xff;
//	unsigned char uchCRCLo = 0xff;
//	unsigned int uindex;
//	while (len--)
//	{
//		uindex = uchCRCHi ^ *updata++;
//		uchCRCHi = uchCRCLo ^ auchCRCHi[uindex];
//		uchCRCLo = auchCRCLo[uindex];
//	}
//	return (uchCRCHi << 8 | uchCRCLo);
//}

class UserServiceHandler
{
public:
	UserServiceHandler(Poco::Net::StreamSocket& socket, Poco::Net::SocketReactor& reactor) :
		socket_(socket), reactor_(reactor)
	{
		Poco::Util::Application& app = Poco::Util::Application::instance();
		app.logger().information("Connection from " + socket.peerAddress().toString());

		recv_buf_ = (uint8_t*)malloc(BUFFER_SIZE);
		send_buf_ = (uint8_t*)malloc(BUFFER_SIZE);

		recv_len_ = 0;
		send_len_ = 0;

		reactor_.addEventHandler(socket_, Poco::NObserver<UserServiceHandler, Poco::Net::ReadableNotification>(*this, &UserServiceHandler::onSocketReadable));
		reactor_.addEventHandler(socket_, Poco::NObserver<UserServiceHandler, Poco::Net::ShutdownNotification>(*this, &UserServiceHandler::onSocketShutdown));

	}

	~UserServiceHandler()
	{
		Poco::Util::Application& app = Poco::Util::Application::instance();
		try
		{
			app.logger().information("Disconnecting " + socket_.peerAddress().toString());
		}
		catch (...)
		{
		}
		reactor_.removeEventHandler(socket_, Poco::NObserver<UserServiceHandler, Poco::Net::ReadableNotification>(*this, &UserServiceHandler::onSocketReadable));
		reactor_.removeEventHandler(socket_, Poco::NObserver<UserServiceHandler, Poco::Net::WritableNotification>(*this, &UserServiceHandler::onSocketWritable));
		reactor_.removeEventHandler(socket_, Poco::NObserver<UserServiceHandler, Poco::Net::ShutdownNotification>(*this, &UserServiceHandler::onSocketShutdown));

		if (recv_buf_)
		{
			free(recv_buf_);
			recv_buf_ = NULL;
		}
		if (send_buf_)
		{
			free(send_buf_);
			send_buf_ = NULL;
		}
	}


	void onSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf);
	bool recvAllData(int& limit, char  tmp_buffer[65535], int& try_count, char  buffer[65535], int try_count_max);
	//{
		////DataTcpRecv dataTcpRecv;
		//////两个缓冲区，header 定长，数据变长，需要通过两次接收
		////char header[26];
		//////char* fusion;
		////char check[4];
		//////第一次接收协议相关的头
		////recv_len_ = socket_.receiveBytes(header, 26);
		////PacketHeader_1 packetHead;
		////memcpy(&(packetHead.header), header, 2);
		////memcpy(&(packetHead.protoType), header + 2, 1);
		////memcpy(&(packetHead.protoVersion), header + 3, 1);
		////memcpy(packetHead.fusionAlNums, header + 4, 15);
		//////fusionAlNums[15] = '\0'; // 添加字符串的终止符
		////memcpy(&(packetHead.msgNum), header + 19, 1);
		////memcpy(&(packetHead.token), header + 20, 2);
		////memcpy(&(packetHead.msgLength), header + 22, 4);

		////packetHead.header = ntohs(packetHead.header);
		////packetHead.token = ntohs(packetHead.token);
		////packetHead.msgLength = ntohl(packetHead.msgLength);

		//////第二次接收 pb字节流
		////char* fusion = new char[packetHead.msgLength];
		////recv_len_ = socket_.receiveBytes(fusion, packetHead.msgLength);

		//////第三次接收校验码、结束码
		////recv_len_ = socket_.receiveBytes(check, 4);
		////PacketCheck checkStruct;
		////memcpy(&(checkStruct.checkCode), check, 2);
		////memcpy(&(checkStruct.endCode), check + 2, 2);
		////checkStruct.checkCode = ntohs(checkStruct.checkCode);
		////checkStruct.endCode = ntohs(checkStruct.endCode);

		//////验证数据：协议头、校验码、结束码、pb数据是否正确编码
		////if (packetHead.header != 0xFAEB)
		////{
		////	ERRORLOG("Header: {} Error!", packetHead.header);
		////	return;
		////}
		//////校验码校验，结束码判定
		////unsigned char* data = (unsigned char*)malloc(sizeof(packetHead.msgLength + 26));
		////memcpy(data, header, 26);
		////memcpy(data + 26, fusion, packetHead.msgLength);
		////if (checkStruct.checkCode != dataTcpRecv.N_CRC16(data, 26 + packetHead.msgLength)) {
		////	ERRORLOG("Invalid CheckCode");
		////	return;
		////}
		////delete[] data;
		////if (checkStruct.endCode != 0xECFD)
		////{
		////	ERRORLOG("Invalid EndCode");
		////	return;
		////}
		////// Fusion数据接收并解析
		////cn::seisys::rbx::comm::bean::multi::FusionPathDatas fusionPbData;
		////if (!fusionPbData.ParseFromArray(fusion, packetHead.msgLength))
		////{
		////	ERRORLOG("Failed to parse protobuf message");
		////	return;
		////}
		////delete[] fusion;
		////if (FusionQueue.Size() > MAX_MSG)
		////{
		////	FusionQueue.Empty();
		////}
		////FusionQueue.Push(fusionPbData);
		// do some user work here
		// do some user work here
		// do some user work here
		// sleep(1); // 不要sleep，不要花时间处理数据，先把数据拷贝到链表里，放其他线程单独处理数据。

		// example
		// receive string
		// return server date
		//printf("RECV - %s\n\r", recv_buf_);

		//组包好后调用DDS发送函数或者压入队列

		//Poco::LocalDateTime now;
		//std::string time_str = Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::ISO8601_FRAC_FORMAT);

		////
		//send_len_ = sprintf((char*)send_buf_, "%s, SVR - %s\n\r", recv_buf_, time_str.c_str());
		//send_buf_[send_len_++] = '\0';

		//// en send
		//reactor_.addEventHandler(socket_, Poco::NObserver<UserServiceHandler, Poco::Net::WritableNotification>(*this, &UserServiceHandler::onSocketWritable));
	//}


	void onSocketWritable(const Poco::AutoPtr<Poco::Net::WritableNotification>& pNf)
	{
		if (send_len_ > 0)
		{
			socket_.sendBytes(send_buf_, send_len_);

		}
		//
		reactor_.removeEventHandler(socket_, Poco::NObserver<UserServiceHandler, Poco::Net::WritableNotification>(*this, &UserServiceHandler::onSocketWritable));
	}

	void onSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification>& pNf)
	{
		delete this;
	}

private:
	enum
	{
		BUFFER_SIZE = 65535
	};

	Poco::Net::StreamSocket   socket_;
	Poco::Net::SocketReactor& reactor_;

	uint8_t* recv_buf_;        // 接收缓存区
	uint8_t* send_buf_;        // 发送缓存区

	int    recv_len_;
	int send_len_;
};

class UserTcpServer : public Poco::Util::ServerApplication
{
public:
	UserTcpServer(Poco::Net::SocketAddress& socket_addr)
	{
		socket_addr_ = &socket_addr;
	}

	~UserTcpServer()
	{
	}

protected:
	void initialize(Application& self)
	{
		Poco::Util::ServerApplication::initialize(self);
	}

	void uninitialize()
	{
		Poco::Util::ServerApplication::uninitialize();
	}

	int main(const std::vector<std::string>& args)
	{
		// set-up a server socket
		Poco::Net::ServerSocket svs(*socket_addr_);
		svs.setReceiveBufferSize(1024*1024);
		//svs.setNoDelay(true);
		// set-up a SocketReactor...
		Poco::Net::SocketReactor reactor;
		// ... and a SocketAcceptor
		Poco::Net::ParallelSocketAcceptor<UserServiceHandler, Poco::Net::SocketReactor> acceptor(svs, reactor);
		// run the reactor in its own thread so that we can wait for 
		// a termination request
		Poco::Thread thread;
		thread.start(reactor);
		// wait for CTRL-C or kill
		Poco::Util::ServerApplication::waitForTerminationRequest();
		// Stop the SocketReactor
		reactor.stop();
		thread.join();
		ERRORLOG("SocketReactor exit after join");

		return Poco::Util::Application::EXIT_OK;
	}

private:
	Poco::Net::SocketAddress* socket_addr_;
};

//int main(int argc, char** argv)
//{
//    int server_port = 8089;
//    std::string server_ip("localhost");
//    Poco::Net::SocketAddress socket_addr(server_ip.c_str(), server_port);
//
//
//    UserTcpServer app(socket_addr);
//    return app.run(argc, argv);
//}
