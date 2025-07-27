#include "UserTcpServer.h"
#include "../utils/log/BaseLog.h"
#include "proto/FusionPathDatas.pb.h"
#include "proto/v2x.pb.h"
#include "../utils/ThreadSafeQueue.h"
#include "TcpRecv.h"
#include "../utils/common.h"
//#include "ConvertCam.h"
#include "MqttSubscriber.h"
#include "../utils/nodeInfo.h"


using namespace cn::seisys::v2x::pb;
cn::ThreadSafeQueue<cn::seisys::v2x::pb::CamData>camSendQueue_;
cn::ThreadSafeQueue<cn::seisys::v2x::pb::CamData>simplifiedCamQueue_;

uint8_t msgNum = 0;
void ConvertCam(cn::seisys::rbx::comm::bean::multi::FusionPathDatas* fusion, uint8_t msgNum)
{
	try {
		//string camStr = "";
		cn::seisys::v2x::pb::CamData cam;
		cam.set_type(utility::node::nodeInfo_.camType_);
		cam.set_ver(utility::node::nodeInfo_.camVer_.c_str());
		cam.set_msgcnt(msgNum);
		cam.set_timestamp(fusion->datatime());
		cam.set_scenetype(static_cast<cn::seisys::v2x::pb::SceneType>(utility::node::nodeInfo_.sceneType_));//cn::seisys::v2x::pb::SCENE_TYPE_URBAN
		cam.set_toalgorithmtime(fusion->sendtime());
		cam.set_deviceid(utility::node::nodeInfo_.deviceId_.c_str());//配置读入 嘉松北路-金昌西路 31011400000100000131011400068_310114000680MEC000057CLD000001
		cam.set_mapdeviceid(utility::node::nodeInfo_.mapDevId_.c_str());  //配置读入  嘉松北路-金昌西路 1.2.156.28896.1.04.121130000.31180000.06010141.00000263
		auto mecPos = new Position3D();
		mecPos->set_lat(utility::node::nodeInfo_.lat_);//配置读入 嘉松北路-金昌西路 313050472
		mecPos->set_lon(utility::node::nodeInfo_.lon_);//配置读入 嘉松北路-金昌西路 1212170908
		mecPos->set_ele(utility::node::nodeInfo_.ele_);//配置读入 嘉松北路-金昌西路 23
		cam.set_allocated_refpos(mecPos);
		for (int i = 0; i < fusion->pathlist_size(); i++)
		{
			auto participant = cam.add_ptclist();
			participant->set_ptcid(fusion->pathlist(i).objid());
			participant->set_ptctype(static_cast<cn::seisys::v2x::pb::ParticipantType>(fusion->pathlist(i).objtype()));
			participant->set_datasource(cn::seisys::v2x::pb::INTEGRATED);
			participant->set_vehicletype(static_cast<cn::seisys::v2x::pb::VehicleType>(fusion->pathlist(i).objkind()));
			string deviceIdList = "";
			for (int j = 0; j < fusion->pathlist(i).oriposdatalist_size(); j++)
			{
				if (j == 0)
				{
					deviceIdList = fusion->pathlist(i).oriposdatalist(j).deviceid();
				}
				else
				{
					deviceIdList = deviceIdList + "," + fusion->pathlist(i).oriposdatalist(j).deviceid();
				}
			}
			participant->set_deviceidlist(deviceIdList);
			participant->set_timestamp(fusion->datatime());
			if (fusion->pathlist(i).has_objposdatas())
			{
				auto pos = new Position3D();
				int32_t lat = fusion->pathlist(i).objposdatas().lattitude() * 1e7;
				int32_t lon = fusion->pathlist(i).objposdatas().longitude() * 1e7;
				//DEBUGLOG("before transfer lat:{},lon:{}", lat, lon);
				pos->set_lat(lat);
				pos->set_lon(lon);
				//DEBUGLOG("after transfer lat:{},lon:{}", pos->lat(), pos->lon());
				pos->set_ele(fusion->pathlist(i).objposdatas().elevation());
				participant->set_allocated_ptcpos(pos);
				participant->set_speed(fusion->pathlist(i).objposdatas().speed());
				participant->set_heading(fusion->pathlist(i).objposdatas().speedheading());
				auto participantSize = new ParticipantSize();
				participantSize->set_width(fusion->pathlist(i).objposdatas().objwidth());
				participantSize->set_length(fusion->pathlist(i).objposdatas().objlength());
				participantSize->set_height(fusion->pathlist(i).objposdatas().objheight());
				participant->set_allocated_ptcsize(participantSize);
			}
			if (fusion->pathlist(i).has_objvisiondata())
			{
				participant->set_plateno(fusion->pathlist(i).objvisiondata().plateno());
				participant->set_platecolor(static_cast<cn::seisys::v2x::pb::ParticipantData::PlateColor>(fusion->pathlist(i).objvisiondata().platecolor()));
			}
		}
		if (camSendQueue_.Size() > MAX_MSG)
		{
			camSendQueue_.Empty();
		}
		camSendQueue_.Push(cam);
		// 发送简化cam队列入库cam结构化数据
		if (simplifiedCamQueue_.Size() > MAX_MSG)
		{
			simplifiedCamQueue_.Empty();
		}
		simplifiedCamQueue_.Push(cam);

	}
	catch (const exception& e) {
		ERRORLOG("Convert Cam:错误:{}!", e.what());
	}

}
void UserServiceHandler::onSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf)
{
	//ConvertCam convert;
	TcpRecv dataTcpRecv;
	//std::vector<std::string> data;
	//Poco::Net::SocketBufVec buffer = Poco::Net::Socket::makeBufVec(data);
	char buffer[65535] = { 0, };
	char tmp_buffer[65535] = { 0, };
	//两个缓冲区，header 定长，数据变长，需要通过两次接收
	//char header[26];
	//char* fusion;
	//char check[4];
	//第一次接收协议相关的头
	//recv_len_ = socket_.receiveBytes(header, 26);
	//全部取下来,再做处理
	recv_len_ = socket_.receiveBytes(buffer, 26);

	if (recv_len_ <= 0) return;
	INFOLOG("receiveBytes length:{}.", recv_len_);
	
	int limit = 26;
	int try_count_max = 100;
	int try_count = 0;
	bool retFlag = false;
	retFlag = recvAllData(limit, tmp_buffer, try_count, buffer, try_count_max);
	if (!retFlag) return;

	PacketHeader_1 packetHead;
	memcpy(&(packetHead.header), buffer, 2);
	memcpy(&(packetHead.protoType), buffer + 2, 1);
	memcpy(&(packetHead.protoVersion), buffer + 3, 1);
	memcpy(packetHead.fusionAlNums, buffer + 4, 15);
	//fusionAlNums[15] = '\0'; // 添加字符串的终止符
	memcpy(&(packetHead.msgNum), buffer + 19, 1);
	memcpy(&(packetHead.token), buffer + 20, 2);
	memcpy(&(packetHead.msgLength), buffer + 22, 4);

	packetHead.header = ntohs(packetHead.header);
	packetHead.token = ntohs(packetHead.token);
	packetHead.msgLength = ntohl(packetHead.msgLength);

	INFOLOG("Header header:{}, protoType:{},protoVersion:{}, fusionAlNums:{}, msgNum:{}, token:{}, msgLength:{},", packetHead.header, packetHead.protoType, packetHead.protoVersion,\
		packetHead.fusionAlNums, packetHead.msgNum, packetHead.token, packetHead.msgLength);


	//第二次接收 pb字节流
	if (packetHead.header != 0xFAEB || packetHead.msgLength <= 0 || packetHead.msgLength > 65000) {

		ERRORLOG("packetHead.msgLength:{}, recv msg failed.", packetHead.msgLength);
		//char buff[65535];
		//recv_len_ = socket_.receiveBytes((void*)buff,65534);
		return;
	}

	limit = 26 + packetHead.msgLength + 4;

	//recv_len_ = socket_.receiveBytes(buffer+26, limit-26);

	try_count = 0;
	retFlag = false;
	retFlag = recvAllData(limit, tmp_buffer, try_count, buffer, try_count_max);
	if (!retFlag) return;


	auto fusion = new char[packetHead.msgLength];
	//recv_len_ = socket_.receiveBytes((void*)fusion, packetHead.msgLength);
	memcpy(fusion, buffer + 26, packetHead.msgLength);

	//第三次接收校验码、结束码
	//recv_len_ = socket_.receiveBytes(check, 4);
	PacketCheck checkStruct;
	memcpy(&(checkStruct.checkCode), buffer + 26 + packetHead.msgLength, 2);
	memcpy(&(checkStruct.endCode), buffer + 26 + packetHead.msgLength + 2, 2);
	checkStruct.checkCode = ntohs(checkStruct.checkCode);
	checkStruct.endCode = ntohs(checkStruct.endCode);

	//校验码校验,结束码判定
	if (checkStruct.endCode != 0xECFD)
	{
		ERRORLOG("Invalid EndCode");
		delete fusion;
		return;
	}

	int dataLen = packetHead.msgLength + 26;
	//auto data = (unsigned char*)malloc(sizeof(dataLen));//packetHead.msgLength + 26
	auto data = new unsigned char[dataLen];
	memcpy(data, buffer, 26);
	memcpy(data + 26, fusion, packetHead.msgLength);
	if (checkStruct.checkCode != dataTcpRecv.N_CRC16(data, 26 + packetHead.msgLength)) {
		ERRORLOG("Invalid CheckCode");
		delete fusion;
		delete data;
		return;
	}
	else
	{
		cn::seisys::rbx::comm::bean::multi::FusionPathDatas fusionPbData;
		bool result = false;
		try {
			result = fusionPbData.ParseFromArray(fusion, packetHead.msgLength);
		}
		catch (...) {
			ERRORLOG("Failed to parse protobuf message 1");
		}
		delete data;
		delete fusion;
		if (result)
		{
			//if (TcpRecv::fusionQueue_.Size() > MAX_MSG)
			//{
			//	TcpRecv::fusionQueue_.Empty();
			//}
			////DEBUGLOG("FusionData:{}", fusionPbData.DebugString());
			//TcpRecv::fusionQueue_.Push(fusionPbData);
			ConvertCam(&fusionPbData, msgNum);
			msgNum++;
			//return;
		}
		//else {
			//ERRORLOG("Failed to parse protobuf message 2");
		//}
	}

	// Fusion数据接收并解析
	


	// do some user work here
	// do some user work here
	// do some user work here
	// sleep(1); // 不要sleep,不要花时间处理数据,先把数据拷贝到链表里,放其他线程单独处理数据。

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
}

bool UserServiceHandler::recvAllData(int& limit, char  tmp_buffer[65535], int& try_count, char  buffer[65535], int try_count_max)
{
	int tmp_recv_len = 0;
	while (recv_len_ < limit) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		tmp_recv_len = socket_.receiveBytes(tmp_buffer, 65535);
		if (tmp_recv_len <= 0) {
			INFOLOG("receiveBytes length <=0, recv msg failed. tyr:{}", try_count);
		}
		else {
			memcpy(buffer + recv_len_, tmp_buffer, tmp_recv_len);
			recv_len_ += tmp_recv_len;
			INFOLOG("receiveBytes length:{}.tyr:{}", recv_len_, try_count);
		}
		try_count++;
		if (try_count > try_count_max) {
			ERRORLOG("recv failed with length:{}, return", recv_len_);
			return false;
		}
	}
	return true;
}
