
#include "log/BaseLog.h"
#include "DataInsert.h"

std::tm* DataInsert::gettm(long long timestamp)
{
	auto milli = timestamp + (long long)8 * 60 * 60 * 1000;
	auto mTime = std::chrono::milliseconds(milli);
	auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
	auto tt = std::chrono::system_clock::to_time_t(tp);
	std::tm* now = std::gmtime(&tt);

	return now;
}

void DataInsert::camInsert(CamData camData, DbBase* db, SnowFlake* g_sf) // 缺少数据库连接类型的一个参数,假定为：DbBase *db
{
    long long camId = 0;
    if (camData.id() == 0)
    {
        camId = g_sf->nextId();
    }
    else
    {
        camId = camData.id();
    }
    int ret = 1;
    srand((unsigned)time(NULL));
    int splitId = rand() % 10000;
    string allSql;
    char camSql[4096];
    int camLat = 0;
    int camLon = 0;
    int camEle = 0;
    int spatMsgcnt = 0;
    uint64_t spatTimestamp = 0;
    if (camData.has_refpos())
    {
        camLat = camData.refpos().lat();
        camLon = camData.refpos().lon();
        camEle = camData.refpos().ele();
    }
    if (camData.has_roadsignalstate())
    {
        spatMsgcnt = camData.roadsignalstate().msgcnt();
        spatTimestamp = camData.roadsignalstate().timestamp();
    }

    sprintf(camSql, "INSERT INTO `cam_data`(`id`,`split_id`,`type`,`ver`,`msg_cnt`,\
	`timestamp`,`device_id`,`map_device_id`,`lat`,`lon`,\
	`ele`,`scene_type_id`,`spat_msg_cnt`,`spat_timestamp`,`to_algorithm_time`,\
	`to_databus_time`,`to_cloud_time`)\
	VALUE\
	(%lld,%d,%d,'%s',%d,\
	%lld,'%s','%s',%d,%d,\
	%d,%d,%d,%lld,%lld,\
	%lld,%lld);",
    camId, splitId, camData.type(), camData.ver().c_str(), camData.msgcnt(),
    camData.timestamp(), camData.deviceid().c_str(), camData.mapdeviceid().c_str(), camLat, camLon,
    camEle, camData.scenetype(), spatMsgcnt, spatTimestamp, camData.toalgorithmtime(),
    camData.todatabustime(), camData.tocloudtime());

    // 需在头文件中加入日志头文件、数据库相关头文件
     ret=db->excuteSql(camSql);
     if (ret<0)
     {
     	ERRORLOG("Insert cam_data Fail,SQL:{}",camSql);
     }

    //  ParticipantData
    string ptclistSqls = "";
    for (int i = 0; i < camData.ptclist_size(); i++)
    {
        char ptclistSql[40960];
        long long ptclistId =0;
        if (camData.ptclist(i).id() == 0)
        {
            ptclistId = g_sf->nextId();
        }
        else
        {
            ptclistId = camData.ptclist(i).id();
        }
        int ptclistLat = 0;
        int ptclistLon = 0;
        int ptclistEle = 0;
        int sectionId = 0;
        int laneId = 0;
        string linkName = "";
        int ptclistPosconfid = 0;
        int ptclistEleconfid = 0;
        int ptclistSpeedCfd = 0;
        int ptclistHeadingCfd = 0;
        int ptclistSteerCfd = 0;
        int acceLat = 0;
        int acceLon = 0;
        int acceVert = 0;
        int acceYaw = 0;
        int latAcceConfid = 0;
        int lonAcceConfid = 0;
        int vertAcceConfid = 0;
        int yawAcceConfid = 0;
        int ptclistWidth = 0;
        int ptclistLength = 0;
        int ptclistHeight = 0;
        int ptclistWidthConfid = 0;
        int ptclistLengthConfid = 0;
        int ptclistHeightConfid = 0;
        int regionId = 0;
        int nodeId = 0;
        int upstreamRegionId = 0;
        int upstreamNodeId = 0;
        if (camData.ptclist(i).has_ptcpos())
        {
            ptclistLat = camData.ptclist(i).ptcpos().lat();
            ptclistLon = camData.ptclist(i).ptcpos().lon();
            ptclistEle = camData.ptclist(i).ptcpos().ele();
        }
        if (camData.ptclist(i).has_maplocation())
        {
            sectionId = camData.ptclist(i).maplocation().sectionid();
            laneId = camData.ptclist(i).maplocation().laneid();
            linkName = camData.ptclist(i).maplocation().linkname();
            if (camData.ptclist(i).maplocation().has_nodeid())
            {
                regionId = camData.ptclist(i).maplocation().nodeid().region();
                nodeId = camData.ptclist(i).maplocation().nodeid().nodeid();
            }
            if (camData.ptclist(i).maplocation().has_upstreamnodeid())
            {
                upstreamRegionId = camData.ptclist(i).maplocation().upstreamnodeid().region();
                upstreamNodeId = camData.ptclist(i).maplocation().upstreamnodeid().nodeid();
            }
        }
        if (camData.ptclist(i).has_posconfid())
        {
            ptclistPosconfid = camData.ptclist(i).posconfid().posconfid();
            ptclistEleconfid = camData.ptclist(i).posconfid().eleconfid();
        }
        if (camData.ptclist(i).has_motionconfid())
        {
            ptclistSpeedCfd = camData.ptclist(i).motionconfid().speedcfd();
            ptclistHeadingCfd = camData.ptclist(i).motionconfid().headingcfd();
            ptclistSteerCfd = camData.ptclist(i).motionconfid().steercfd();
        }
        if (camData.ptclist(i).has_accelset())
        {
            acceLat = camData.ptclist(i).accelset().lat();
            acceLon = camData.ptclist(i).accelset().lon();
            acceVert = camData.ptclist(i).accelset().vert();
            acceYaw = camData.ptclist(i).accelset().yaw();
        }
        if (camData.ptclist(i).has_accelerationconfid())
        {
            latAcceConfid = camData.ptclist(i).accelerationconfid().lataccelconfid();
            lonAcceConfid = camData.ptclist(i).accelerationconfid().lonaccelconfid();
            vertAcceConfid = camData.ptclist(i).accelerationconfid().verticalaccelconfid();
            yawAcceConfid = camData.ptclist(i).accelerationconfid().yawrateconfid();
        }
        if (camData.ptclist(i).has_ptcsize())
        {
            ptclistWidth = camData.ptclist(i).ptcsize().width();
            ptclistLength = camData.ptclist(i).ptcsize().length();
            ptclistHeight = camData.ptclist(i).ptcsize().height();
        }
        if (camData.ptclist(i).has_ptcsizeconfid())
        {
            ptclistWidthConfid = camData.ptclist(i).ptcsizeconfid().widthconfid();
            ptclistLengthConfid = camData.ptclist(i).ptcsizeconfid().lengthconfid();
            ptclistHeightConfid = camData.ptclist(i).ptcsizeconfid().heightconfid();
        }

        if (i == 0)
        {
            sprintf(ptclistSql, "INSERT INTO `participant_data` (`id`,`split_id`,`ptc_id`,`ptc_type_id`,`data_source_id`,\
			`device_id_list`,`timestamp`,`lat`,`lon`,`ele`,\
			`link_name`,`section_id`,`lane_id`,`pos_confid_id`,`ele_confid_id`,\
			`speed`,`heading`,`speed_cfd_id`,`heading_cfd_id`,`steer_cfd_id`,\
			`acceleration_lat`,`acceleration_lon`,`acceleration_vert`,`acceleration_yaw`,`lon_accel_confid_id`,\
			`lat_accel_confid_id`,`vertical_accel_confid_id`,`yaw_rate_confid_id`,`width`,`length`,\
			`height`,`vehicle_band`,`vehicle_type_id`,`plate_no`,`plate_type_id`,\
			`plate_color_id`,`vehicle_color_id`,`width_confid_id`,`length_confid_id`,`height_confid_id`,\
			`ptc_type_ext_id`,`ptc_type_ext_confid`,`status_duration`,`tracking`,`cam_data_id`,\
			`node_region_id`,`node_id`,`upstream_node_region_id`,`upstream_node_node_id`,`time_confidence_id`,`device_id`)\
			VALUES");
            ptclistSqls = ptclistSqls + ptclistSql;
        }

        sprintf(ptclistSql, "(%lld,%d,%lld,%d,%d,'%s',%lld,%d,%d,%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\
		%d,%d,%d,%d,%d,%d,'%s',%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%lld,%d,%d,%d,%d,%d,'%s'),",
        ptclistId, splitId, camData.ptclist(i).ptcid(), camData.ptclist(i).ptctype(), camData.ptclist(i).datasource(),
        camData.ptclist(i).deviceidlist().c_str(), camData.ptclist(i).timestamp(), ptclistLat, ptclistLon, ptclistEle,
        linkName.c_str(), sectionId, laneId, ptclistPosconfid, ptclistEleconfid,
        camData.ptclist(i).speed(), camData.ptclist(i).heading(), ptclistSpeedCfd, ptclistHeadingCfd, ptclistSteerCfd,
        acceLat, acceLon, acceVert, acceYaw, lonAcceConfid,
        latAcceConfid, vertAcceConfid, yawAcceConfid, ptclistWidth, ptclistLength,
        ptclistHeight, camData.ptclist(i).vehicleband().c_str(), camData.ptclist(i).vehicletype(), camData.ptclist(i).plateno().c_str(), camData.ptclist(i).platetype(),
        camData.ptclist(i).platecolor(), camData.ptclist(i).vehiclecolor(), ptclistWidthConfid, ptclistLengthConfid, ptclistHeightConfid,
        camData.ptclist(i).ptctypeext(), camData.ptclist(i).ptctypeextconfid(), camData.ptclist(i).statusduration(), camData.ptclist(i).tracking(), camId,
        regionId, nodeId, upstreamRegionId, upstreamNodeId, camData.ptclist(i).timeconfidence(), camData.deviceid().c_str());

        ptclistSqls += ptclistSql;

        if (camData.ptclist(i).has_polygon())
        {
            string polySqls = "";
            for (int j = 0; j < camData.ptclist(i).polygon().pos_size(); j++)
            {
                char polySql[40960];
                long long polyId = g_sf->nextId();

                if (j == 0)
                {
                    sprintf(polySql, "INSERT INTO `participant_data_position_3_d`(`id`,`split_id`,`lat`,`lon`,`ele`,`participant_data_id`)\
					VALUES");
                    polySqls += polySql;
                }

                sprintf(polySql, "(%lld,%d,%d,%d,%d,%lld),",
                        polyId, splitId, camData.ptclist(i).polygon().pos(j).lat(), camData.ptclist(i).polygon().pos(j).lon(), camData.ptclist(i).polygon().pos(j).ele(), ptclistId);

                polySqls += polySql;
            }

            if (polySqls.length() > 0)
            {
                polySqls[polySqls.length() - 1] = ';';
            }

            if (polySqls.length() > 10)
            {
                 //需在头文件中加入日志头文件、数据库相关头文件
                 ret=db->excuteSql(polySqls);
                 if (ret<0)
                 {
                 	ERRORLOG("Insert participant_data_position_3_d Fail,SQL:{}",polySqls);
                 }
            }
        }

        string pathHistorySqls = "";
        for (int k = 0; k < camData.ptclist(i).pathhistory_size(); k++)
        {
            char pathHistorySql[40960];
            long long pathHistoryId = g_sf->nextId();
            int pathLat = 0;
            int pathLon = 0;
            int pathEle = 0;
            int pathPosConfid = 0;
            int pathEleConfid = 0;
            if (camData.ptclist(i).pathhistory(k).has_pos())
            {
                pathLat = camData.ptclist(i).pathhistory(k).pos().lat();
                pathLon = camData.ptclist(i).pathhistory(k).pos().lon();
                pathEle = camData.ptclist(i).pathhistory(k).pos().ele();
            }
            if (camData.ptclist(i).pathhistory(k).has_posconfid())
            {
                pathPosConfid = camData.ptclist(i).pathhistory(k).posconfid().posconfid();
                pathEleConfid = camData.ptclist(i).pathhistory(k).posconfid().eleconfid();
            }

            if (k == 0)
            {
                sprintf(pathHistorySql, "INSERT INTO `path_history_point`(`id`,`split_id`,`lat`,`lon`,`ele`,\
				`time_offset`,`speed`,`heading`,`pos_confid_id`,`ele_confid_id`,`participant_data_id`)VALUES");
                pathHistorySqls = pathHistorySqls + pathHistorySql;
            }

            sprintf(pathHistorySql, "(%lld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%lld),",
                    pathHistoryId, splitId, pathLat, pathLon, pathEle,
                    camData.ptclist(i).pathhistory(k).timeoffset(), camData.ptclist(i).pathhistory(k).speed(), camData.ptclist(i).pathhistory(k).heading(), pathPosConfid, pathEleConfid,
                    ptclistId);
            pathHistorySqls += pathHistorySql;
        }

        if (pathHistorySqls.length() > 0)
        {
            pathHistorySqls[pathHistorySqls.length() - 1] = ';';
        }

        if (pathHistorySqls.length() > 10)
        {
            // 需在头文件中加入日志头文件、数据库相关头文件
             ret=db->excuteSql(pathHistorySqls);
             if (ret<0)
             {
             	ERRORLOG("Insert path_history_point Fail,SQL:{}",pathHistorySqls);
             }
        }
    }

    if (ptclistSqls.length() > 0)
    {
        ptclistSqls[ptclistSqls.length() - 1] = ';';
    }

    if (ptclistSqls.length() > 10)
    {
        // 需在头文件中加入日志头文件、数据库相关头文件
         ret=db->excuteSql(ptclistSqls);
         if (ret<0)
         {
         	ERRORLOG("Insert Participant Data Fail,SQL:{}",ptclistSqls);
         }
    }

	// rte_database- Table
	int rteSize = camData.rtelist_size();
	string rteSqls = "";
	for (int i = 0; i < rteSize; i++)
	{
		//删除手动下发事件
		if (camData.rtelist(i).datasource() == 9 || camData.rtelist(i).datasource() == 12)
		{
			continue;
		}
		char rteSql[10240];
		long long rteDataId = 0;
		if (camData.rtelist(i).id() == 0)
		{
			rteDataId = g_sf->nextId();
		}
		else
		{
			rteDataId = camData.rtelist(i).id();
		}
		int rteId = camData.rtelist(i).rteid();
		int rteType = camData.rtelist(i).rtetype();
		string description = camData.rtelist(i).description();
		string deviceIdList = camData.rtelist(i).deviceidlist();
		int lon = 0; // Position3D
		int lat = 0; // Position3D
		int ele = 0; // Position3D
		if (camData.rtelist(i).has_rtepos())
		{
			lat = camData.rtelist(i).rtepos().lat();
			lon = camData.rtelist(i).rtepos().lon();
			ele = camData.rtelist(i).rtepos().ele();
		}
		int nodeRegionId = 0;		  // MapLocation   nodeId
		int nodeId = 0;				  // MapLocation   nodeId
		string linkName = "";		  // MapLocation
		int upstreamNodeRegionId = 0; // MapLocation   upstreamNodeId
		int upstreamNodeNodeId = 0;	  // MapLocation   upstreamNodeId
		int sectionId = 0;			  // MapLocation
		int laneId = 0;				  // MapLocation
		if (camData.rtelist(i).has_maplocation())
		{
			nodeRegionId = camData.rtelist(i).maplocation().nodeid().region();
			nodeId = camData.rtelist(i).maplocation().nodeid().nodeid();
			linkName = camData.rtelist(i).maplocation().linkname().c_str();
			upstreamNodeRegionId = camData.rtelist(i).maplocation().upstreamnodeid().region();
			upstreamNodeNodeId = camData.rtelist(i).maplocation().upstreamnodeid().nodeid();
			sectionId = camData.rtelist(i).maplocation().sectionid();
			laneId = camData.rtelist(i).maplocation().laneid();
		}
		int eventRadius = camData.rtelist(i).eventradius();
		long long startTime = 0;			 // RsiTimeDetails
		long long endTime = 0;			 // RsiTimeDetails
		int endTimeConfidenceId = 0; // RsiTimeDetails enum
		if (camData.rtelist(i).has_timedetails())
		{
			startTime = camData.rtelist(i).timedetails().starttime();
			endTime = camData.rtelist(i).timedetails().endtime();
			endTimeConfidenceId = camData.rtelist(i).timedetails().endtimeconfidence();
		}
		string priority = camData.rtelist(i).priority();
		int eventConfid = camData.rtelist(i).eventconfid();
		string eventImages = camData.rtelist(i).eventimages();
		string eventVideos = camData.rtelist(i).eventvideos();
		int eventSourceId = camData.rtelist(i).eventsource(); // enum
		int dataSourceId = camData.rtelist(i).datasource();	  // enum

		if (i == 0)
		{
			sprintf(rteSql, "INSERT INTO `rte_data` (`id`,`split_id`,`rte_id`,`rte_type`,`description`,\
			`device_id_list`,`lat`,`lon`,`ele`,`node_region_id`,\
			`node_id`,`link_name`,`upstream_node_region_id`,`upstream_node_node_id`,`section_id`,\
			`lane_id`,`event_radius`,`start_time`,`end_time`,`priority`,\
			`event_confid`,`event_images`,`event_videos`,`event_source_id`,`data_source_id`,\
			`end_time_confidence_id`,`cam_data_id`,`device_id`)VALUES");
			rteSqls += rteSql;
		}

		sprintf(rteSql, "(%lld,%d,%d,%d,'%s',\
		'%s',%d,%d,%d,%d,\
		%d,'%s',%d,%d,%d,\
		%d,%d,%lld,%lld,'%s',\
		%d,'%s','%s',%d,%d,\
		%d,%lld,'%s'),",
			rteDataId, splitId, rteId, rteType, description.c_str(),
			deviceIdList.c_str(), lat, lon, ele, nodeRegionId,
			nodeId, linkName.c_str(), upstreamNodeRegionId, upstreamNodeNodeId, sectionId,
			laneId, eventRadius, startTime, endTime, priority.c_str(),
			eventConfid, eventImages.c_str(), eventVideos.c_str(), eventSourceId, dataSourceId,
			endTimeConfidenceId, camId, camData.deviceid().c_str());
		rteSqls += rteSql;

		// rte_data_reference_path- Table
		long long forwardRteDataId = g_sf->nextId();
		string rteReferencePathSqls = "";
		for (int j = 0; j < camData.rtelist(i).referencepath_size(); j++)
		{
			char rteReferencePathSql[10240];
			long long rteReferencePathId = g_sf->nextId();
			int pathRadius = camData.rtelist(i).referencepath(j).pathradius();

			if (j == 0)
			{
				sprintf(rteReferencePathSql, "INSERT INTO `rte_data_reference_path` \
				(`id`,`split_id`,`path_radius`,`rte_data_id`,`forward_rte_data_id`)VALUES");
				rteReferencePathSqls += rteReferencePathSql;
			}

			sprintf(rteReferencePathSql, "(%lld,%d,%d,%lld,%lld),",
				rteReferencePathId, splitId, pathRadius, rteDataId, forwardRteDataId);

			rteReferencePathSqls += rteReferencePathSql;

			// rte_data_reference_path_position_3_d   -Table
			string activepathSqls = "";
			for (int k = 0; k < camData.rtelist(i).referencepath(j).activepath_size(); k++)
			{
				char activepathSql[10240];
				long long activepathId = g_sf->nextId();
				int activepathLon = camData.rtelist(i).referencepath(j).activepath(k).lon();
				int activepathLat = camData.rtelist(i).referencepath(j).activepath(k).lat();
				int activepathEle = camData.rtelist(i).referencepath(j).activepath(k).ele();

				if (k == 0)
				{
					sprintf(activepathSql, "INSERT INTO `rte_data_reference_path_position_3_d`\
					(`id`,`split_id`,`lon`,`lat`,`ele`,`rte_data_reference_path_id`)VALUES");
					activepathSqls += activepathSql;
				}

				sprintf(activepathSql, "(%lld,%d,%d,%d,%d,%lld),",
					activepathId, splitId, activepathLon, activepathLat, activepathEle, rteReferencePathId);

				activepathSqls += activepathSql;
			}
			if (activepathSqls.length() > 0)
			{
				activepathSqls[activepathSqls.length() - 1] = ';';
			}
			if (activepathSqls.length() > 10)
			{
				ret = db->excuteSql(activepathSqls);
				if (ret < 0)
				{
					ERRORLOG("Insert Activepath Data Fail,SQL:{}", activepathSqls);
				}
			}
		}

		if (rteReferencePathSqls.length() > 0)
		{
			rteReferencePathSqls[rteReferencePathSqls.length() - 1] = ';';
		}

		if (rteReferencePathSqls.length() > 10)
		{
			ret = db->excuteSql(rteReferencePathSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert RteReferencePath Data Fail,SQL:{}", rteReferencePathSqls);
			}
		}

		// rte_data_reference_link -Table
		string rteReferenceLinkSqls = "";
		for (int j = 0; j < camData.rtelist(i).referencelinks_size(); j++)
		{
			char rteReferenceLinkSql[10240];
			long long rteReferenceLinkId = g_sf->nextId();
			int upstreamNodeRegionId = 0; // upstreamNodeId
			int upstreamNodeId = 0;		  // upstreamNodeId
			if (camData.rtelist(i).referencelinks(j).has_upstreamnodeid())
			{
				upstreamNodeRegionId = camData.rtelist(i).referencelinks(j).upstreamnodeid().region();
				upstreamNodeId = camData.rtelist(i).referencelinks(j).upstreamnodeid().nodeid();
			}
			int downstreamNodeRegionId = 0; // downstreamNodeId
			int downstreamNodeId = 0;		// downstreamNodeId
			if (camData.rtelist(i).referencelinks(j).has_upstreamnodeid())
			{
				downstreamNodeRegionId = camData.rtelist(i).referencelinks(j).downstreamnodeid().region();
				downstreamNodeId = camData.rtelist(i).referencelinks(j).downstreamnodeid().nodeid();
			}
			int referenceLanes = 0; // referenceLanes
			if (camData.rtelist(i).referencelinks(j).has_referencelanes())
			{
				referenceLanes = camData.rtelist(i).referencelinks(j).referencelanes().referencelanes();
			}

			if (j == 0)
			{
				sprintf(rteReferenceLinkSql, "INSERT INTO `rte_data_reference_link`\
				(`id`,`split_id`,`upstream_node_region_id`,`upstream_node_id`,`downstream_node_region_id`,\
				`downstream_node_id`,`reference_lanes`,`rte_data_id`,`forward_rte_data_id`)VALUES");
				rteReferenceLinkSqls += rteReferenceLinkSql;
			}

			sprintf(rteReferenceLinkSql, "(%lld,%d,%d,%d,%d,%d,%d,%lld,%lld),",
				rteReferenceLinkId, splitId, upstreamNodeRegionId, upstreamNodeId, downstreamNodeRegionId,
				downstreamNodeId, referenceLanes, rteDataId, forwardRteDataId);

			rteReferenceLinkSqls += rteReferenceLinkSql;
		}

		if (rteReferenceLinkSqls.length() > 0)
		{
			rteReferenceLinkSqls[rteReferenceLinkSqls.length() - 1] = ';';
		}

		if (rteReferenceLinkSqls.length() > 10)
		{
			ret = db->excuteSql(rteReferenceLinkSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert RteReferenceLink Data Fail,SQL:{}", rteReferenceLinkSqls);
			}
		}

		// obj_id_value   -Table
		string eventObjIdSqls = "";
		for (int j = 0; j < camData.rtelist(i).eventobjid_size(); j++)
		{
			char eventObjIdSql[10240];
			long long eventObjIdId = g_sf->nextId();
			uint64_t ptcId = camData.rtelist(i).eventobjid(j).ptcid();
			uint64_t obsId = camData.rtelist(i).eventobjid(j).obsid();
			int roleId = camData.rtelist(i).eventobjid(j).role();

			if (j == 0)
			{
				sprintf(eventObjIdSql, "INSERT INTO `obj_id_value`\
				(`id`,`split_id`,`ptc_id`,`obs_id`,`role_id`,\
				`rte_data_id`,`forward_rte_data_id`VALUES");

				eventObjIdSqls += eventObjIdSql;
			}

			sprintf(eventObjIdSql, "(%lld,%d,%lld,%lld,%d,%lld,%lld),",
				eventObjIdId, splitId, ptcId, obsId, roleId,
				rteDataId, forwardRteDataId);

			eventObjIdSqls += eventObjIdSql;
		}

		if (eventObjIdSqls.length() > 0)
		{
			eventObjIdSqls[eventObjIdSqls.length() - 1] = ';';
		}

		if (eventObjIdSqls.length() > 10)
		{
			ret = db->excuteSql(eventObjIdSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert EventObjId Data Fail,SQL:{}", eventObjIdSqls);
			}
		}
	}

	if (rteSqls.length() > 0)
	{
		rteSqls[rteSqls.length() - 1] = ';';
	}

	if (rteSqls.length() > 10)
	{
		ret = db->excuteSql(rteSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert Rte Data Fail,SQL:{}", rteSqls);
		}
	}

	// rts_data  -Table
	string rtsDataSqls = "";
	for (int i = 0; i < camData.rtslist_size(); i++)
	{
		if (camData.rtslist(i).datasource() == 9 || camData.rtslist(i).datasource() == 12)
		{
			continue;
		}
		char rtsDataSql[10240];
		long long rtsDataId = g_sf->nextId();
		int rtsId = camData.rtslist(i).rtsid();
		int rtsType = camData.rtslist(i).rtstype();
		long long dataSource = camData.rtslist(i).datasource();
		string priority = camData.rtslist(i).priority();
		int lon = 0; // Position3D
		int lat = 0; // Position3D
		int ele = 0; // Position3D
		if (camData.rtslist(i).has_rtspos())
		{
			lat = camData.rtslist(i).rtspos().lat();
			lon = camData.rtslist(i).rtspos().lon();
			ele = camData.rtslist(i).rtspos().ele();
		}
		long long startTime = 0;		   // RsiTimeDetails
		long long endTime = 0;		   // RsiTimeDetails
		int endTimeConfidence = 0; // RsiTimeDetails enum
		if (camData.rtslist(i).has_timedetails())
		{
			startTime = camData.rtslist(i).timedetails().starttime();
			endTime = camData.rtslist(i).timedetails().endtime();
			endTimeConfidence = camData.rtslist(i).timedetails().endtimeconfidence();
		}
		string description = camData.rtslist(i).description();
		int pathRadius = camData.rtslist(i).pathradius();

		if (i == 0)
		{
			sprintf(rtsDataSql, "INSERT INTO `rts_data`\
			(`id`,`split_id`,`rts_id`,`rts_type`,`priority`,\
			`lat`,`lon`,`ele`,`start_time`,`end_time`,\
			`description`,`path_radius`,`data_source_id`,`end_time_confidence_id`,`cam_data_id`,`device_id`)VALUES");

			rtsDataSqls += rtsDataSql;
		}

		sprintf(rtsDataSql, "(%lld,%d,%d,%d,'%s',%d,%d,%d,%lld,%lld,'%s',%d,%lld,%d,%lld,'%s'),",
			rtsDataId, splitId, rtsId, rtsType, priority.c_str(),
			lat, lon, ele, startTime, endTime,
			description.c_str(), pathRadius, dataSource, endTimeConfidence, camId, camData.deviceid().c_str());

		rtsDataSqls += rtsDataSql;

		// rts_data_reference_path -Table
		long long forwardRtsDataId = g_sf->nextId();
		string rtsReferencePathSqls = "";
		for (int j = 0; j < camData.rtslist(i).refpathlist_size(); j++)
		{
			char rtsReferencePathSql[10240];
			long long rtsReferencePathId = g_sf->nextId();
			int pathRadius = camData.rtslist(i).refpathlist(j).pathradius();

			if (j == 0)
			{
				sprintf(rtsReferencePathSql, "INSERT INTO `rts_data_reference_path`\
				(`id`,`split_id`,`path_radius`,`rts_data_id`,`forward_rts_data_id`)VALUES");

				rtsReferencePathSqls += rtsReferencePathSql;
			}

			sprintf(rtsReferencePathSql, "(%lld,%d,%d,%lld,%lld),",
				rtsReferencePathId, splitId, pathRadius, rtsDataId, forwardRtsDataId);

			rtsReferencePathSqls += rtsReferencePathSql;

			// rts_data_reference_path_position_3_d   -Table
			string activepathSqls = "";
			for (int k = 0; k < camData.rtslist(i).refpathlist(j).activepath_size(); k++)
			{
				char activepathSql[10240];
				long long activepathId = g_sf->nextId();
				int activepathLon = camData.rtelist(i).referencepath(j).activepath(k).lon();
				int activepathLat = camData.rtelist(i).referencepath(j).activepath(k).lat();
				int activepathEle = camData.rtelist(i).referencepath(j).activepath(k).ele();

				if (k == 0)
				{
					sprintf(activepathSql, "INSERT INTO `rts_data_reference_path_position_3_d`\
					(`id`,`split_id`,`lon`,`lat`,`ele`,`rts_data_reference_path_id`)VALUES");

					activepathSqls += activepathSql;
				}

				sprintf(activepathSql, "(%lld,%d,%d,%d,%d,%lld),",
					activepathId, splitId, activepathLon, activepathLat, activepathEle, rtsReferencePathId);

				activepathSqls += activepathSql;
			}

			if (activepathSqls.length() > 0)
			{
				activepathSqls[activepathSqls.length() - 1] = ';';
			}

			if (activepathSqls.length() > 10)
			{
				ret = db->excuteSql(activepathSqls);
				if (ret < 0)
				{
					ERRORLOG("Insert Activepath Data Fail,SQL:{}", activepathSqls);
				}
			}
		}

		if (rtsReferencePathSqls.length() > 0)
		{
			rtsReferencePathSqls[rtsReferencePathSqls.length() - 1] = ';';
		}

		if (rtsReferencePathSqls.length() > 10)
		{
			ret = db->excuteSql(rtsReferencePathSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert RtsReferencePath Data Fail,SQL:{}", rtsReferencePathSqls);
			}
		}

		// rts_data_reference_link -Table
		string rtsReferenceLinkSqls = "";
		for (int j = 0; j < camData.rtslist(i).reflinklist_size(); j++)
		{
			char rtsReferenceLinkSql[10240];
			long long rtsReferenceLinkId = g_sf->nextId();
			int upstreamNodeRegionId = 0; // upstreamNodeId
			int upstreamNodeId = 0;		  // upstreamNodeId
			if (camData.rtslist(i).reflinklist(j).has_upstreamnodeid())
			{
				upstreamNodeRegionId = camData.rtslist(i).reflinklist(j).upstreamnodeid().region();
				upstreamNodeId = camData.rtslist(i).reflinklist(j).upstreamnodeid().nodeid();
			}
			int downstreamNodeRegionId = 0; // downstreamNodeId
			int downstreamNodeId = 0;		// downstreamNodeId
			if (camData.rtslist(i).reflinklist(j).has_upstreamnodeid())
			{
				downstreamNodeRegionId = camData.rtslist(i).reflinklist(j).downstreamnodeid().region();
				downstreamNodeId = camData.rtslist(i).reflinklist(j).downstreamnodeid().nodeid();
			}
			int referenceLanes = 0; // referenceLanes
			if (camData.rtslist(i).reflinklist(j).has_referencelanes())
			{
				referenceLanes = camData.rtslist(i).reflinklist(j).referencelanes().referencelanes();
			}

			if (j == 0)
			{
				sprintf(rtsReferenceLinkSql, "INSERT INTO `rts_data_reference_link`\
				(`id`,`split_id`,`upstream_node_region_id`,`upstream_node_id`,`downstream_node_region_id`,\
				`downstream_node_id`,`reference_lanes`,`rts_data_id`,`forward_rts_data_id`)VALUES");

				rtsReferenceLinkSqls += rtsReferenceLinkSql;
			}

			sprintf(rtsReferenceLinkSql, "(%lld,%d,%d,%d,%d,%d,%d,%lld,%lld),",
				rtsReferenceLinkId, splitId, upstreamNodeRegionId, upstreamNodeId, downstreamNodeRegionId,
				downstreamNodeId, referenceLanes, rtsDataId, forwardRtsDataId);

			rtsReferenceLinkSqls += rtsReferenceLinkSql;
		}

		if (rtsReferenceLinkSqls.length() > 0)
		{
			rtsReferenceLinkSqls[rtsReferenceLinkSqls.length() - 1] = ';';
		}

		if (rtsReferenceLinkSqls.length() > 10)
		{
			ret = db->excuteSql(rtsReferenceLinkSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert rtsReferenceLink Data Fail,SQL:{}", rtsReferenceLinkSqls);
			}
		}
	}

	if (rtsDataSqls.length() > 0)
	{
		rtsDataSqls[rtsDataSqls.length() - 1] = ';';
	}

	if (rtsDataSqls.length() > 10)
	{
		ret = db->excuteSql(rtsDataSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert Rts Data Fail,SQL:{}", rtsDataSqls);
		}
	}

	// traffic_flow -Table
	string trafficFlowSqls = "";
	for (int i = 0; i < camData.trafficflow_size(); i++)
	{
		char trafficFlowSql[10240];
		long long trafficFlowId = g_sf->nextId();
		int nodeRegionId = 0; // nodeId NodeReferenceId
		int nodeId = 0;		  // nodeId NodeReferenceId
		if (camData.trafficflow(i).has_nodeid())
		{
			nodeRegionId = camData.trafficflow(i).nodeid().region();
			nodeId = camData.trafficflow(i).nodeid().nodeid();
		}
		long long genTime = camData.trafficflow(i).gentime();
		int jhiInterval = 0;	 // statType TrafficFlowStatType
		long long cycleStartTime = 0; // sequence TrafficFlowStatBySignalCycle TrafficFlowStatType
		long long cycleEndTime = 0;	 // sequence TrafficFlowStatBySignalCycle TrafficFlowStatType
		int cycleTime = 0;		 // sequence TrafficFlowStatBySignalCycle TrafficFlowStatType
		if (camData.trafficflow(i).has_stattype())
		{
			if (camData.trafficflow(i).stattype().has_interval())
			{
				jhiInterval = camData.trafficflow(i).stattype().interval().interval();
			}
			if (camData.trafficflow(i).stattype().has_sequence())
			{
				cycleStartTime = camData.trafficflow(i).stattype().sequence().cyclestarttime();
				cycleEndTime = camData.trafficflow(i).stattype().sequence().cycleendtime();
				cycleTime = camData.trafficflow(i).stattype().sequence().cycletime();
			}
		}

		if (i == 0)
		{
			sprintf(trafficFlowSql, "INSERT INTO `traffic_flow`\
			(`id`,`split_id`,`node_region_id`,`node_id`,`gen_time`,\
			`jhi_interval`,`cycle_start_time`,`cycle_end_time`,`cycle_time`,`cam_data_id`,`device_id`)VALUES");

			trafficFlowSqls += trafficFlowSql;
		}

		sprintf(trafficFlowSql, "(%lld,%d,%d,%d,%lld,%d,%lld,%lld,%d,%lld,'%s'),",
			trafficFlowId, splitId, nodeRegionId, nodeId, genTime,
			jhiInterval, cycleStartTime, cycleEndTime, cycleTime, camId, camData.deviceid().c_str());

		trafficFlowSqls += trafficFlowSql;

		// traffic_flow_stat -Table
		string trafficFlowStatSqls = "";
		for (int j = 0; j < camData.trafficflow(i).stats_size(); j++)
		{
			char trafficFlowStatSql[10240];
			long long trafficFlowStatId = g_sf->nextId();
			/* mapElement TrafficFlowStatMapElement 拆 路网元素 */
			/* detectorArea DetectorArea 拆 交通流感知区间*/
			int areaId = 0;	  // detectorArea DetectorArea / mapElement TrafficFlowStatMapElement
			long long setTime = 0; // detectorArea DetectorArea / mapElement TrafficFlowStatMapElement
			/** polygon TrafficFlowStatPosition3D 子集 一对多 */
			int detectorAreaNodeRegionId = 0; // 交通流感路口节点地区id
			int detectorAreaNodeNodeId = 0;	  // 交通流感路口节点id
			int detectorAreaLaneId = 0;		  // detectorArea DetectorArea / mapElement TrafficFlowStatMapElement

			/* laneStatInfo LaneStatInfo  展 */
			int laneId = 0;								 // laneStatInfo LaneStatInfo / mapElement TrafficFlowStatMapElement
			int laneLinkUpstreamNodeRegionId = 0;		 // Lane关联的路段上游节点地区id
			int laneLinkUpstreamNodeNodeId = 0;			 // Lane关联的路段上游节点id
			string laneLinkName = "";					 // Lane关联的路段名
			int laneLinkNodeRegionId = 0;				 // Lane关联的Link关联的路口节点地区id
			int laneLinkNodeNodeId = 0;					 // Lane关联的Link关联的路口节点id
			string laneLinkExtId = "";					 // Lane关联的路段拓展id、保证全局唯一
			int laneSectionId = 0;						 // Lane关联的路段区段SectionId
			int laneSectionLinkUpstreamNodeRegionId = 0; // Lane关联的Section关联的Link路段上游节点地区id
			int laneSectionLinkUpstreamNodeNodeId = 0;	 // Lane关联的Section关联的Link路段上游节点id
			string laneSectionLinkName = "";			 // Lane关联的Section关联的Link路段名
			int laneSectionLinkNodeRegionId = 0;		 // Lane关联的Section关联的Link关联的路口节点Node地区id
			int laneSectionLinkNodeNodeId = 0;			 // Lane关联的Section关联的Link关联的路口节点Nodeid
			string laneSectionLinkExtId = "";			 // Lane关联的Section关联的路段Link的拓展id、保证全局唯一
			string laneSectionExtId = "";				 // Lane关联的路段区段Section拓展Id、保证全局唯一
			string laneExtId = "";						 // 车道  拓展ID、保证全局唯一

			/*sectionStatInfo SectionStatInfo*/
			int sectionId = 0;						 // sectionStatInfo SectionStatInfo / mapElement TrafficFlowStatMapElement
			int sectionLinkUpstreamNodeRegionId = 0; // Section关联的Link关联的路口节点Node地区id
			int sectionLinkUpstreamNodeNodeId = 0;	 // Section关联的Link关联的路口节点NodeId
			string sectionLinkName = "";			 // Section关联的Link路段名
			int sectionLinkNodeRegionId = 0;		 // Section关联的Link关联的路口节点Node地区id
			int sectionLinkNodeNodeId = 0;			 // Section关联的Link关联的路口节点NodeId
			string sectionLinkExtId = "";			 // Section关联的Link关联的拓展ID
			string sectionExtId = "";				 // 路段区段分段Section拓展ID、保证全局唯一

			/* linkStatInfo LinkStatInfo 拆 有向路段对象*/
			/* upstreamNodeId NodeReferenceId 拆 */
			int linkUpstreamNodeNodeId = 0;	  // upstreamNodeId NodeReferenceId / linkStatInfo LinkStatInfo / mapElement TrafficFlowStatMapElement
			int linkUpstreamNodeRegionId = 0; // upstreamNodeId NodeReferenceId / linkStatInfo LinkStatInfo / mapElement TrafficFlowStatMapElement
			string linkName = "";			  // linkStatInfo LinkStatInfo / mapElement TrafficFlowStatMapElement
			int linkNodeRegionId = 0;		  // link关联的路口节点node地区id
			int linkNodeNodeId = 0;			  // link关联的路口节点nodeId
			string linkExtId = "";			  // 有向路段link对象拓展ID、保证全局唯一

			/* nodeStatInfo NodeStatInfo 展 路口对象*/
			/* nodeId NodeReferenceId 展 上游节点Id */
			int nodeRegionId = 0; // nodeId NodeReferenceId / nodeStatInfo NodeStatInfo / mapElement TrafficFlowStatMapElement
			int nodeId = 0;		  // nodeId NodeReferenceId / nodeStatInfo NodeStatInfo / mapElement TrafficFlowStatMapElement

			/* movementStatInfo MovementStatInfo 拆 转向对象 */
			/* remoteIntersection NodeReferenceId 拆 下游路口编号*/
			int movementRemoteIntersectionNodeRegionId = 0; // remoteIntersection NodeReferenceId / movementStatInfo MovementStatInfo / mapElement TrafficFlowStatMapElement
			int movementRemoteIntersectionNodeId = 0;		// remoteIntersection NodeReferenceId / movementStatInfo MovementStatInfo / mapElement TrafficFlowStatMapElement
			int movementNodeRegionId = 0;					// 转向对象本路口地区id
			int movementNodeNodeId = 0;						// 转向对象本路口节点id
			string movementExtId = "";						// 转向对象拓展ID、保证全局唯一
			long long mapElementType = 0;						// mapElement类型

			/* turnDirection ManeuverDesc 枚举  转向信息 */
			long long turnDirection = 0; // turnDirection ManeuverDesc / movementStatInfo MovementStatInfo / mapElement TrafficFlowStatMapElement

			if (camData.trafficflow(i).stats(j).has_mapelement())
			{
				if (camData.trafficflow(i).stats(j).mapelement().has_detectorarea())
				{
					mapElementType = 1;
					areaId = camData.trafficflow(i).stats(j).mapelement().detectorarea().areaid();
					setTime = camData.trafficflow(i).stats(j).mapelement().detectorarea().settime();
					if (camData.trafficflow(i).stats(j).mapelement().detectorarea().has_nodeid())
					{
						detectorAreaNodeRegionId = camData.trafficflow(i).stats(j).mapelement().detectorarea().nodeid().region();
						detectorAreaNodeNodeId = camData.trafficflow(i).stats(j).mapelement().detectorarea().nodeid().nodeid();
					}
					detectorAreaLaneId = camData.trafficflow(i).stats(j).mapelement().detectorarea().laneid();

					// traffic_flow_stat_position_3_d
					if (camData.trafficflow(i).stats(j).mapelement().detectorarea().has_polygon())
					{
						string trafficFlowStatPosSqls = "";
						for (int k = 0; k < camData.trafficflow(i).stats(j).mapelement().detectorarea().polygon().pos_size(); k++)
						{
							char trafficFlowStatPosSql[10240];
							long long trafficFlowStatPosId = g_sf->nextId();
							int trafficFlowStatPosLon = camData.trafficflow(i).stats(j).mapelement().detectorarea().polygon().pos(k).lon();
							int trafficFlowStatPosLat = camData.trafficflow(i).stats(j).mapelement().detectorarea().polygon().pos(k).lat();
							int trafficFlowStatPosEle = camData.trafficflow(i).stats(j).mapelement().detectorarea().polygon().pos(k).ele();

							if (k == 0)
							{
								sprintf(trafficFlowStatPosSql, "INSERT INTO `traffic_flow_stat_position_3_d`\
								(`id`,`split_id`,`lon`,`lat`,`ele`,`traffic_flow_stat_id`)VALUES");

								trafficFlowStatPosSqls += trafficFlowStatPosSql;
							}

							sprintf(trafficFlowStatPosSql, "(%lld,%d,%d,%d,%d,%lld),",
								trafficFlowStatPosId, splitId, trafficFlowStatPosLon, trafficFlowStatPosLat, trafficFlowStatPosEle, trafficFlowStatId);

							trafficFlowStatPosSqls += trafficFlowStatPosSql;
						}

						if (trafficFlowStatPosSqls.length() > 0)
						{
							trafficFlowStatPosSqls[trafficFlowStatPosSqls.length() - 1] = ';';
						}

						if (trafficFlowStatPosSqls.length() > 10)
						{
							ret = db->excuteSql(trafficFlowStatPosSqls);
							if (ret < 0)
							{
								ERRORLOG("Insert TrafficFlowStatPos Data Fail,SQL:{}", trafficFlowStatPosSqls);
							}
						}
					}
				}
				else if (camData.trafficflow(i).stats(j).mapelement().has_lanestatinfo())
				{
					mapElementType = 2;
					laneId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().laneid();
					if (camData.trafficflow(i).stats(j).mapelement().lanestatinfo().has_linkstatinfo())
					{
						if (camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().has_upstreamnodeid())
						{
							laneLinkUpstreamNodeRegionId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().upstreamnodeid().region();
							laneLinkUpstreamNodeNodeId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().upstreamnodeid().nodeid();
						}
						laneLinkName = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().name();
						if (camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().has_nodestatinfo())
						{
							laneLinkNodeRegionId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().nodestatinfo().nodeid().region();
							laneLinkNodeNodeId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().nodestatinfo().nodeid().nodeid();
						}
						laneLinkExtId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().linkstatinfo().extid();
					}
					if (camData.trafficflow(i).stats(j).mapelement().lanestatinfo().has_sectionstatinfo())
					{
						laneSectionId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().sectionid();
						if (camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().has_linkstatinfo())
						{
							if (camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().has_upstreamnodeid())
							{
								laneSectionLinkUpstreamNodeRegionId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().upstreamnodeid().region();
								laneSectionLinkUpstreamNodeNodeId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().upstreamnodeid().nodeid();
							}
							laneSectionLinkName = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().name();
							if (camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().has_nodestatinfo())
							{
								laneSectionLinkNodeRegionId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().nodestatinfo().nodeid().region();
								laneSectionLinkNodeNodeId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().nodestatinfo().nodeid().nodeid();
							}
							laneSectionLinkExtId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().linkstatinfo().extid();
						}
						laneSectionExtId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().sectionstatinfo().extid();
					}
					laneExtId = camData.trafficflow(i).stats(j).mapelement().lanestatinfo().extid();
				}

				else if (camData.trafficflow(i).stats(j).mapelement().has_sectionstatinfo())
				{
					mapElementType = 3;
					sectionId = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().sectionid();
					if (camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().has_linkstatinfo())
					{
						if (camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().has_upstreamnodeid())
						{
							sectionLinkUpstreamNodeRegionId = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().upstreamnodeid().region();
							sectionLinkUpstreamNodeNodeId = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().upstreamnodeid().nodeid();
						}
						sectionLinkName = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().name();
						if (camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().has_nodestatinfo())
						{
							sectionLinkNodeRegionId = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().nodestatinfo().nodeid().region();
							sectionLinkNodeNodeId = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().nodestatinfo().nodeid().nodeid();
						}
						sectionLinkExtId = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().linkstatinfo().extid();
					}
					sectionExtId = camData.trafficflow(i).stats(j).mapelement().sectionstatinfo().extid();
				}

				else if (camData.trafficflow(i).stats(j).mapelement().has_linkstatinfo())
				{
					mapElementType = 4;
					if (camData.trafficflow(i).stats(j).mapelement().linkstatinfo().has_upstreamnodeid())
					{
						linkUpstreamNodeRegionId = camData.trafficflow(i).stats(j).mapelement().linkstatinfo().upstreamnodeid().region();
						linkUpstreamNodeNodeId = camData.trafficflow(i).stats(j).mapelement().linkstatinfo().upstreamnodeid().nodeid();
					}
					linkName = camData.trafficflow(i).stats(j).mapelement().linkstatinfo().name();
					if (camData.trafficflow(i).stats(j).mapelement().linkstatinfo().has_nodestatinfo())
					{
						linkNodeNodeId = camData.trafficflow(i).stats(j).mapelement().linkstatinfo().nodestatinfo().nodeid().nodeid();
						linkNodeRegionId = camData.trafficflow(i).stats(j).mapelement().linkstatinfo().nodestatinfo().nodeid().region();
					}
					linkExtId = camData.trafficflow(i).stats(j).mapelement().linkstatinfo().extid();
				}

				else if (camData.trafficflow(i).stats(j).mapelement().has_nodestatinfo())
				{
					mapElementType = 5;
					if (camData.trafficflow(i).stats(j).mapelement().nodestatinfo().has_nodeid())
					{
						nodeRegionId = camData.trafficflow(i).stats(j).mapelement().nodestatinfo().nodeid().region();
						nodeId = camData.trafficflow(i).stats(j).mapelement().nodestatinfo().nodeid().nodeid();
					}
				}

				else if (camData.trafficflow(i).stats(j).mapelement().has_movementstatinfo())
				{
					mapElementType = 6;
					if (camData.trafficflow(i).stats(j).mapelement().movementstatinfo().has_remoteintersection())
					{
						movementRemoteIntersectionNodeRegionId = camData.trafficflow(i).stats(j).mapelement().movementstatinfo().remoteintersection().region();
						movementRemoteIntersectionNodeId = camData.trafficflow(i).stats(j).mapelement().movementstatinfo().remoteintersection().nodeid();
					}
					if (camData.trafficflow(i).stats(j).mapelement().movementstatinfo().has_nodestatinfo())
					{
						movementNodeRegionId = camData.trafficflow(i).stats(j).mapelement().movementstatinfo().nodestatinfo().nodeid().region();
						movementNodeNodeId = camData.trafficflow(i).stats(j).mapelement().movementstatinfo().nodestatinfo().nodeid().nodeid();
					}
					movementExtId = camData.trafficflow(i).stats(j).mapelement().movementstatinfo().extid();
					turnDirection = camData.trafficflow(i).stats(j).mapelement().movementstatinfo().turndirection();
				}
			}
			long long ptcType = camData.trafficflow(i).stats(j).ptctype();
			long long vehicleType = camData.trafficflow(i).stats(j).vehicletype();
			long long timestamp = camData.trafficflow(i).stats(j).timestamp();
			int volume = camData.trafficflow(i).stats(j).volume();
			int speedPoint = camData.trafficflow(i).stats(j).speedpoint();
			int speedArea = camData.trafficflow(i).stats(j).speedarea();
			int density = camData.trafficflow(i).stats(j).density();
			int travelTime = camData.trafficflow(i).stats(j).traveltime();
			int delay = camData.trafficflow(i).stats(j).delay();
			int queueLength = camData.trafficflow(i).stats(j).queuelength();
			int queueInt = camData.trafficflow(i).stats(j).queueint();
			int congestion = camData.trafficflow(i).stats(j).congestion();
			/* trafficFlowExtensionStat TrafficFlowExtension 拆 */
			/* laneIndex LaneIndexAdded  子集 一对多 除通用交通指标之外的车道级交通指标*/
			/* linkIndex LinkIndexAdded  子集 一对多 除通用交通指标之外的进口道级交通指标*/
			/* movementIndex MovementIndexAdded  子集 一对多 除通用交通指标之外的转向级交通指标*/
			/* nodeIndex NodeIndexAdded 子集 一对多 除通用交通指标之外的路口级交通指标*/
			/* signalIndex SignalControlIndexAdded 子集 一对多 交叉口信控评价的可选拓展统计指标*/
			int timeHeadway = camData.trafficflow(i).stats(j).timeheadway();
			int spaceHeadway = camData.trafficflow(i).stats(j).spaceheadway();
			int stopNums = camData.trafficflow(i).stats(j).stopnums();

			if (j == 0)
			{
				sprintf(trafficFlowStatSql, "INSERT INTO `traffic_flow_stat`\
				(`id`,`split_id`,`area_id`,`set_time`,`detector_area_node_region_id`,\
				`detector_area_node_node_id`,`detector_area_lane_id`,`lane_id`,`lane_link_upstream_node_region_id`,`lane_link_upstream_node_node_id`,\
				`lane_link_name`, `lane_link_node_region_id`,`lane_link_node_node_id`, `lane_link_ext_id`,\
				`lane_section_id`,  `lane_section_link_upstream_node_region_id`, `lane_section_link_upstream_node_node_id`,`lane_section_link_name`, `lane_section_link_node_region_id`,\
				`lane_section_link_node_node_id`, `lane_section_link_ext_id`, `lane_section_ext_id`,`lane_ext_id`, `section_id`,\
				`section_link_upstream_node_region_id`, `section_link_upstream_node_node_id`, `section_link_name`,`section_link_node_region_id`, `section_link_node_node_id`,\
				`section_link_ext_id`,`section_ext_id`, `link_upstream_node_region_id`, `link_upstream_node_node_id`,`link_name`, `link_node_region_id`,\
				`link_node_node_id`, `link_ext_id`, `node_region_id`,`node_id`, `movement_remote_intersection_node_region_id`,\
				`movement_remote_intersection_node_id`, `movement_node_region_id`, `movement_node_node_id`,`movement_ext_id`, `timestamp`,\
				`volume`,`speed_point`,`speed_area`,`density`,`travel_time`,\
				`delay`,`queue_length`,`queue_int`,`congestion`,`time_headway`,\
				`space_headway`,`stop_nums`,`ptc_type_id`,`vehicle_type_id`,`maneuver_id`,\
				`map_element_type_id`,`traffic_flow_id`,`device_id`)VALUES");

				trafficFlowStatSqls += trafficFlowStatSql;
			}

			sprintf(trafficFlowStatSql, "(%lld,%d,%d,%lld,%d,\
			%d,%d,%d,'%d',%d,\
			'%s',%d,%d,'%s',\
			%d,%d,%d,'%s',%d,\
			%d,'%s','%s','%s',%d,\
			%d,%d,'%s',%d,%d,\
			'%s','%s',%d,%d,'%s',%d,\
			%d,'%s',%d,%d,%d,\
			%d,%d,%d,'%s',%lld,\
			%d,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,\
			%d,%d,%lld,%lld,%lld,%lld,%lld,'%s'),",
				trafficFlowStatId, splitId, areaId, setTime, detectorAreaNodeRegionId,
				detectorAreaNodeNodeId, detectorAreaLaneId, laneId, laneLinkUpstreamNodeRegionId, laneLinkUpstreamNodeNodeId,
				laneLinkName.c_str(), laneLinkNodeRegionId, laneLinkNodeNodeId, laneLinkExtId.c_str(),
				laneSectionId, laneSectionLinkUpstreamNodeRegionId, laneSectionLinkUpstreamNodeNodeId, laneSectionLinkName.c_str(), laneSectionLinkNodeRegionId,
				laneSectionLinkNodeNodeId, laneSectionLinkExtId.c_str(), laneSectionExtId.c_str(), laneExtId.c_str(), sectionId,
				sectionLinkUpstreamNodeRegionId, sectionLinkUpstreamNodeNodeId, sectionLinkName.c_str(), sectionLinkNodeRegionId, sectionLinkNodeNodeId,
				sectionLinkExtId.c_str(), sectionExtId.c_str(), linkUpstreamNodeRegionId, linkUpstreamNodeNodeId, linkName.c_str(), linkNodeRegionId,
				linkNodeNodeId, linkExtId.c_str(), nodeRegionId, nodeId, movementRemoteIntersectionNodeRegionId,
				movementRemoteIntersectionNodeId, movementNodeRegionId, movementNodeNodeId, movementExtId.c_str(), timestamp,
				volume, speedPoint, speedArea, density, travelTime,
				delay, queueLength, queueInt, congestion, timeHeadway,
				spaceHeadway, stopNums, ptcType, vehicleType, turnDirection,
				mapElementType, trafficFlowId, camData.deviceid().c_str());

			trafficFlowStatSqls += trafficFlowStatSql;

			// lane_index_added -Table
			if (camData.trafficflow(i).stats(j).has_trafficflowextension())
			{
				string laneIndexAddSqls = "";
				for (int k = 0; k < camData.trafficflow(i).stats(j).trafficflowextension().laneindex_size(); k++)
				{
					char laneIndexAddSql[10240];
					long long laneIndexAddId = g_sf->nextId();
					long long laneIndexAddTimestamp = camData.trafficflow(i).stats(j).trafficflowextension().laneindex(k).timestamp();
					int laneCapacity = camData.trafficflow(i).stats(j).trafficflowextension().laneindex(k).lanecapacity();
					int laneSaturation = camData.trafficflow(i).stats(j).trafficflowextension().laneindex(k).lanesaturation();
					int laneSpaceOccupy = camData.trafficflow(i).stats(j).trafficflowextension().laneindex(k).lanespaceoccupy();
					int laneTimeOccupy = camData.trafficflow(i).stats(j).trafficflowextension().laneindex(k).lanetimeoccupy();
					int laneAvgGrnQueue = camData.trafficflow(i).stats(j).trafficflowextension().laneindex(k).laneavggrnqueue();
					int laneGrnUtilization = camData.trafficflow(i).stats(j).trafficflowextension().laneindex(k).lanegrnutilization();

					if (k == 0)
					{
						sprintf(laneIndexAddSql, "INSERT INTO `lane_index_added`\
						(`id`,`split_id`,`timestamp`,`lane_capacity`,`lane_saturation`,\
						`lane_time_occupy`,`lane_space_occupy`,`lane_avg_grn_queue`,`lane_grn_utilization`,`traffic_flow_stat_id`)VALUES");

						laneIndexAddSqls += laneIndexAddSql;
					}

					sprintf(laneIndexAddSql, "(%lld,%d,%lld,%d,%d,%d,%d,%d,%d,%lld),",
						laneIndexAddId, splitId, laneIndexAddTimestamp, laneCapacity, laneSaturation,
						laneTimeOccupy, laneSpaceOccupy, laneAvgGrnQueue, laneGrnUtilization, trafficFlowStatId);

					laneIndexAddSqls += laneIndexAddSql;
				}

				if (laneIndexAddSqls.length() > 0)
				{
					laneIndexAddSqls[laneIndexAddSqls.length() - 1] = ';';
				}

				if (laneIndexAddSqls.length() > 10)
				{
					ret = db->excuteSql(laneIndexAddSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert LaneIndexAdd Data Fail,SQL:{}", laneIndexAddSqls);
					}
				}

				// link_index_added -Table
				string linkIndexAddSqls = "";
				for (int k = 0; k < camData.trafficflow(i).stats(j).trafficflowextension().linkindex_size(); k++)
				{
					char linkIndexAddSql[10240];
					long long linkIndexAddId = g_sf->nextId();
					long long linkIndexAddTimestamp = camData.trafficflow(i).stats(j).trafficflowextension().linkindex(k).timestamp();
					int linkCapacity = camData.trafficflow(i).stats(j).trafficflowextension().linkindex(k).linkcapacity();
					int linkSaturation = camData.trafficflow(i).stats(j).trafficflowextension().linkindex(k).linksaturation();
					int linkSpaceOccupy = camData.trafficflow(i).stats(j).trafficflowextension().linkindex(k).linkspaceoccupy();
					int linkTimeOccupy = camData.trafficflow(i).stats(j).trafficflowextension().linkindex(k).linktimeoccupy();
					int linkAvgGrnQueue = camData.trafficflow(i).stats(j).trafficflowextension().linkindex(k).linkavggrnqueue();
					int linkGrnUtilization = camData.trafficflow(i).stats(j).trafficflowextension().linkindex(k).linkgrnutilization();

					if (k == 0)
					{
						sprintf(linkIndexAddSql, "INSERT INTO `link_index_added`\
						(`id`,`split_id`,`timestamp`,`link_capacity`,`link_saturation`,\
						`link_time_occupy`,`link_space_occupy`,`link_avg_grn_queue`,`link_grn_utilization`,`traffic_flow_stat_id`)VALUES");

						linkIndexAddSqls += linkIndexAddSql;
					}

					sprintf(linkIndexAddSql, "(%lld,%d,%lld,%d,%d,%d,%d,%d,%d,%lld),",
						linkIndexAddId, splitId, linkIndexAddTimestamp, linkCapacity, linkSaturation,
						linkTimeOccupy, linkSpaceOccupy, linkAvgGrnQueue, linkGrnUtilization, trafficFlowStatId);

					linkIndexAddSqls += linkIndexAddSql;
				}

				if (linkIndexAddSqls.length() > 0)
				{
					linkIndexAddSqls[linkIndexAddSqls.length() - 1] = ';';
				}

				if (linkIndexAddSqls.length() > 10)
				{
					ret = db->excuteSql(linkIndexAddSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert LinkIndexAdd Data Fail,SQL:{}", linkIndexAddSqls);
					}
				}

				// movement_index_added -Table
				string movementIndexAddSqls = "";
				for (int k = 0; k < camData.trafficflow(i).stats(j).trafficflowextension().movementindex_size(); k++)
				{
					char movementIndexAddSql[10240];
					long long movementIndexAddId = g_sf->nextId();
					long long movementIndexAddTimestamp = camData.trafficflow(i).stats(j).trafficflowextension().movementindex(k).timestamp();
					int movementCapacity = camData.trafficflow(i).stats(j).trafficflowextension().movementindex(k).movementcapacity();
					int movementSaturation = camData.trafficflow(i).stats(j).trafficflowextension().movementindex(k).movementsaturation();
					int movementSpaceOccupy = camData.trafficflow(i).stats(j).trafficflowextension().movementindex(k).movementspaceoccupy();
					int movementTimeOccupy = camData.trafficflow(i).stats(j).trafficflowextension().movementindex(k).movementtimeoccupy();
					int movementAvgGrnQueue = camData.trafficflow(i).stats(j).trafficflowextension().movementindex(k).movementavggrnqueue();
					int movementGrnUtilization = camData.trafficflow(i).stats(j).trafficflowextension().movementindex(k).movementgrnutilization();

					if (k == 0)
					{
						sprintf(movementIndexAddSql, "INSERT INTO `movement_index_added`\
						(`id`,`split_id`,`timestamp`,`movement_capacity`,`movement_saturation`,\
						`movement_time_occupy`,`movement_space_occupy`,`movement_avg_grn_queue`,`movement_grn_utilization`,`traffic_flow_stat_id`)VALUES");

						movementIndexAddSqls += movementIndexAddSql;
					}

					sprintf(movementIndexAddSql, "(%lld,%d,%lld,%d,%d,%d,%d,%d,%d,%lld),",
						movementIndexAddId, splitId, movementIndexAddTimestamp, movementCapacity, movementSaturation,
						movementTimeOccupy, movementSpaceOccupy, movementAvgGrnQueue, movementGrnUtilization, trafficFlowStatId);

					movementIndexAddSqls += movementIndexAddSql;
				}

				if (movementIndexAddSqls.length() > 0)
				{
					movementIndexAddSqls[movementIndexAddSqls.length() - 1] = ';';
				}

				if (movementIndexAddSqls.length() > 10)
				{
					ret = db->excuteSql(movementIndexAddSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert MovementIndexAdd Data Fail,SQL:{}", movementIndexAddSqls);
					}
				}

				// node_index_added -Table
				string nodeIndexAddSqls = "";
				for (int k = 0; k < camData.trafficflow(i).stats(j).trafficflowextension().nodeindex_size(); k++)
				{
					char nodeIndexAddSql[10240];
					long long nodeIndexAddId = g_sf->nextId();
					long long nodeIndexAddTimestamp = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).timestamp();
					int nodeCapacity = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).nodecapacity();
					int nodeSaturation = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).nodesaturation();
					int nodeSpaceOccupy = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).nodespaceoccupy();
					int nodeTimeOccupy = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).nodetimeoccupy();
					int nodeAvgGrnQueue = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).nodeavggrnqueue();
					int nodeGrnUtilization = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).nodegrnutilization();
					int demandIndex = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).demandindex();
					int supplyIndex = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).supplyindex();
					int theoryIndex = camData.trafficflow(i).stats(j).trafficflowextension().nodeindex(k).theoryindex();

					if (k == 0)
					{
						sprintf(nodeIndexAddSql, "INSERT INTO `node_index_added`\
						(`id`,`split_id`,`timestamp`,`node_capacity`,`node_saturation`,\
						`node_time_occupy`,`node_space_occupy`,`node_avg_grn_queue`,`node_grn_utilization`,`demand_index`,\
						`supply_index`,`theory_index`,`traffic_flow_stat_id`)VALUES");

						nodeIndexAddSqls += nodeIndexAddSql;
					}

					sprintf(nodeIndexAddSql, "(%lld,%d,%lld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%lld),",
						nodeIndexAddId, splitId, nodeIndexAddTimestamp, nodeCapacity, nodeSaturation,
						nodeTimeOccupy, nodeSpaceOccupy, nodeAvgGrnQueue, nodeGrnUtilization, demandIndex,
						supplyIndex, theoryIndex, trafficFlowStatId);

					nodeIndexAddSqls += nodeIndexAddSql;
				}

				if (nodeIndexAddSqls.length() > 0)
				{
					nodeIndexAddSqls[nodeIndexAddSqls.length() - 1] = ';';
				}

				if (nodeIndexAddSqls.length() > 10)
				{
					ret = db->excuteSql(nodeIndexAddSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert NodeIndexAdd Data Fail,SQL:{}", nodeIndexAddSqls);
					}
				}

				// signal_control_index_added -Table
				string signalIndexAddSqls = "";
				for (int k = 0; k < camData.trafficflow(i).stats(j).trafficflowextension().signalindex_size(); k++)
				{
					char signalIndexAddSql[10240];
					long long signalIndexAddId = g_sf->nextId();
					int signalPhaseId = camData.trafficflow(i).stats(j).trafficflowextension().signalindex(k).phaseid();
					int signalGreenStartQueue = camData.trafficflow(i).stats(j).trafficflowextension().signalindex(k).greenstartqueue();
					int signalRedStartQueue = camData.trafficflow(i).stats(j).trafficflowextension().signalindex(k).redstartqueue();
					int signalGreenUtilization = camData.trafficflow(i).stats(j).trafficflowextension().signalindex(k).greenutilization();

					if (k == 0)
					{
						sprintf(signalIndexAddSql, "INSERT INTO `signal_control_index_added`\
						(`id`,`split_id`,`phase_id`,`green_start_queue`,`red_start_queue`,\
						`green_utilization`,`traffic_flow_stat_id`)VALUES");

						signalIndexAddSqls += signalIndexAddSql;
					}

					sprintf(signalIndexAddSql, "(%lld,%d,%d,%d,%d,%d,%lld),",
						signalIndexAddId, splitId, signalPhaseId, signalGreenStartQueue, signalRedStartQueue,
						signalGreenUtilization, trafficFlowStatId);

					signalIndexAddSqls += signalIndexAddSql;
				}

				if (signalIndexAddSqls.length() > 0)
				{
					signalIndexAddSqls[signalIndexAddSqls.length() - 1] = ';';
				}

				if (signalIndexAddSqls.length() > 10)
				{
					ret = db->excuteSql(signalIndexAddSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert SignalIndexAdd Data Fail,SQL:{}", signalIndexAddSqls);
					}
				}
			}
		}

		if (trafficFlowStatSqls.length() > 0)
		{
			trafficFlowStatSqls[trafficFlowStatSqls.length() - 1] = ';';
		}

		if (trafficFlowStatSqls.length() > 10)
		{
			ret = db->excuteSql(trafficFlowStatSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert TrafficFlowStat Data Fail,SQL:{}", trafficFlowStatSqls);
			}
		}
	}

	if (trafficFlowSqls.length() > 0)
	{
		trafficFlowSqls[trafficFlowSqls.length() - 1] = ';';
	}

	if (trafficFlowSqls.length() > 10)
	{
		ret = db->excuteSql(trafficFlowSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert TrafficFlow Data Fail,SQL:{}", trafficFlowSqls);
		}
	}

	// intersection_state (spat) -Table
	if (camData.has_roadsignalstate())
	{
		string intersectionStateSqls = "";
		for (int i = 0; i < camData.roadsignalstate().intersections_size(); i++)
		{
			char intersectionStateSql[10240];
			long long intersectionStateId = g_sf->nextId();
			int nodeRegionId = 0; // NodeReferenceId
			int nodeId = 0;		  // NodeReferenceId
			if (camData.roadsignalstate().intersections(i).has_intersectionid())
			{
				nodeRegionId = camData.roadsignalstate().intersections(i).intersectionid().region();
				nodeId = camData.roadsignalstate().intersections(i).intersectionid().nodeid();
			}
			string status = camData.roadsignalstate().intersections(i).status();
			long long intersectionStateTimestamp = camData.roadsignalstate().intersections(i).timestamp();
			long long intersectionStateTimeConfidence = camData.roadsignalstate().intersections(i).timeconfidence();

			if (i == 0)
			{
				sprintf(intersectionStateSql, "INSERT INTO `intersection_state`\
				(`id`,`split_id`,`node_region_id`,`node_id`,`status`,\
				`timestamp`,`time_confidence_id`,`cam_data_id`,`device_id`)VALUES");

				intersectionStateSqls += intersectionStateSql;
			}

			sprintf(intersectionStateSql, "(%lld,%d,%d,%d,'%s',%lld,%lld,%lld,'%s'),",
				intersectionStateId, splitId, nodeRegionId, nodeId, status.c_str(),
				intersectionStateTimestamp, intersectionStateTimeConfidence, camId, camData.deviceid().c_str());

			intersectionStateSqls += intersectionStateSql;

			// Phase -Table
			string phaseDataSqls = "";
			for (int j = 0; j < camData.roadsignalstate().intersections(i).phases_size(); j++)
			{
				char phaseDataSql[10240];
				long long phaseDataId = g_sf->nextId();
				int phaseId = camData.roadsignalstate().intersections(i).phases(j).id();

				if (j == 0)
				{
					sprintf(phaseDataSql, "INSERT INTO `phase`\
					(`id`,`split_id`,`phase_id`,`intersection_state_id`,`device_id`)VALUES");

					phaseDataSqls += phaseDataSql;
				}

				sprintf(phaseDataSql, "(%lld,%d,%d,%lld,'%s'),",
					phaseDataId, splitId, phaseId, intersectionStateId, camData.deviceid().c_str());

				phaseDataSqls += phaseDataSql;

				// phase_state -Table
				string phaseStateSqls = "";
				for (int k = 0; k < camData.roadsignalstate().intersections(i).phases(j).phasestates_size(); k++)
				{
					char phaseStateSql[10240];
					long long phaseStateId = g_sf->nextId();
					long long lightId = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).light();
					int startTime = 0;
					int minEndTime = 0;
					int maxEndTime = 0;
					int likelyEndTime = 0;
					long long phasesStateTimeConfidence = 0;
					int nextStartTime = 0;
					int nextDuration = 0;
					if (camData.roadsignalstate().intersections(i).phases(j).phasestates(k).has_timing())
					{
						startTime = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).timing().starttime();
						minEndTime = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).timing().minendtime();
						maxEndTime = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).timing().maxendtime();
						likelyEndTime = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).timing().likelyendtime();
						phasesStateTimeConfidence = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).timing().timeconfidence();
						nextStartTime = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).timing().nextstarttime();
						nextDuration = camData.roadsignalstate().intersections(i).phases(j).phasestates(k).timing().nextduration();
					}
					if (k == 0)
					{
						sprintf(phaseStateSql, "INSERT INTO `phase_state`\
						(`id`,`split_id`,`start_time`,`min_end_time`,`max_end_time`,\
						`likely_end_time`,`next_start_time`,`next_duration`,`light_id`,`time_confidence_id`,`phase_id`,`device_id`)VALUES");

						phaseStateSqls += phaseStateSql;
					}

					sprintf(phaseStateSql, "(%lld,%d,%d,%d,%d,%d,%d,%d,%lld,%lld,%lld,'%s'),",
						phaseStateId, splitId, startTime, minEndTime, maxEndTime,
						likelyEndTime, nextStartTime, nextDuration, lightId, phasesStateTimeConfidence, phaseDataId, camData.deviceid().c_str());

					phaseStateSqls += phaseStateSql;
				}
				if (phaseStateSqls.length() > 0)
				{
					phaseStateSqls[phaseStateSqls.length() - 1] = ';';
				}

				if (phaseStateSqls.length() > 10)
				{
					ret = db->excuteSql(phaseStateSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert PhaseState Data Fail,SQL:{}", phaseStateSqls);
					}
				}
			}

			if (phaseDataSqls.length() > 0)
			{
				phaseDataSqls[phaseDataSqls.length() - 1] = ';';
			}

			if (phaseDataSqls.length() > 10)
			{
				ret = db->excuteSql(phaseDataSqls);
				if (ret < 0)
				{
					ERRORLOG("Insert PhaseData Data Fail,SQL:{}", phaseDataSqls);
				}
			}
		}

		if (intersectionStateSqls.length() > 0)
		{
			intersectionStateSqls[intersectionStateSqls.length() - 1] = ';';
		}

		if (intersectionStateSqls.length() > 10)
		{
			ret = db->excuteSql(intersectionStateSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert IntersectionState Data Fail,SQL:{}", intersectionStateSqls);
			}
		}
	}

	// signal_scheme -Table
	string signalSchemeSqls = "";
	for (int i = 0; i < camData.signalschemelist_size(); i++)
	{
		char signalSchemeSql[10240];
		long long signalSchemeId = g_sf->nextId();
		int signalSchemeNodeRegionId = 0; // nodeId NodeReferenceId
		int signalSchemeNodeId = 0;		  // nodeId NodeReferenceId
		if (camData.signalschemelist(i).has_nodeid())
		{
			signalSchemeNodeRegionId = camData.signalschemelist(i).nodeid().region();
			signalSchemeNodeId = camData.signalschemelist(i).nodeid().nodeid();
		}
		int optimType = camData.signalschemelist(i).optimtype();
		long long signalSchemetimestamp = camData.signalschemelist(i).timestamp();

		if (i == 0)
		{
			sprintf(signalSchemeSql, "INSERT INTO `signal_scheme`\
			(`id`,`split_id`,`node_region_id`,`node_id`,`optim_type`,`timestamp`,`cam_data_id`,`device_id`)VALUES");

			signalSchemeSqls += signalSchemeSql;
		}

		sprintf(signalSchemeSql, "(%lld,%d,%d,%d,%d,%lld,%lld,'%s'),",
			signalSchemeId, splitId, signalSchemeNodeRegionId, signalSchemeNodeId, optimType, signalSchemetimestamp, camId, camData.deviceid().c_str());

		signalSchemeSqls += signalSchemeSql;

		// optimDataList OptimData 子集  一对多*/
		// optim_data -Table
		string optimSqls = "";
		for (int j = 0; j < camData.signalschemelist(i).optimdatalist_size(); j++)
		{
			char optimSql[10240];
			long long optimId = g_sf->nextId();
			/* optimTimeType OptimTimeType 拆 优化时段类型*/
			/* single SingleTimeSpan 单次优化时段 */
			long long optimStartTime = 0; //
			long long optimEndTime = 0;	 //
			/* periodic PeriodicTimeSpan 拆 周期优化时段划分*/
			int monthFilter = 0;
			int dayFilter = 0;
			int weekdayFilter = 0;
			/* fromTimePoint LocalTimePoint 拆 开始时刻*/
			int fromhh = 0;
			int frommm = 0;
			int fromss = 0;
			/* toTimePoint LocalTimePoint 拆 结束时刻*/
			int tohh = 0;
			int tomm = 0;
			int toss = 0;
			if (camData.signalschemelist(i).optimdatalist(j).has_optimtimetype())
			{
				if (camData.signalschemelist(i).optimdatalist(j).optimtimetype().has_single())
				{
					optimStartTime = camData.signalschemelist(i).optimdatalist(j).optimtimetype().single().starttime();
					optimEndTime = camData.signalschemelist(i).optimdatalist(j).optimtimetype().single().endtime();
				}
				if (camData.signalschemelist(i).optimdatalist(j).optimtimetype().has_periodic())
				{
					monthFilter = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().monthfilter();
					dayFilter = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().dayfilter();
					weekdayFilter = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().weekdayfilter();
					if (camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().has_fromtimepoint())
					{
						fromhh = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().fromtimepoint().hh();
						frommm = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().fromtimepoint().mm();
						fromss = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().fromtimepoint().ss();
					}
					if (camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().has_totimepoint())
					{
						tohh = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().totimepoint().hh();
						tomm = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().totimepoint().mm();
						toss = camData.signalschemelist(i).optimdatalist(j).optimtimetype().periodic().totimepoint().ss();
					}
				}
			}
			int optimCycleTime = camData.signalschemelist(i).optimdatalist(j).optimcycletime();
			int minCycleTime = camData.signalschemelist(i).optimdatalist(j).mincycletime();
			int maxCycleTime = camData.signalschemelist(i).optimdatalist(j).maxcycletime();
			string coorPhase = camData.signalschemelist(i).optimdatalist(j).coorphase();
			int offset = camData.signalschemelist(i).optimdatalist(j).offset();

			if (j == 0)
			{
				sprintf(optimSql, "INSERT INTO `optim_data`\
				(`id`,`split_id`,`start_time`,`end_time`,`month_filter`,\
				`day_filter`,`weekday_filter`,`fromhh`,`frommm`,`fromss`,\
				`tohh`,`tomm`,`toss`,`optim_cycle_time`,`min_cycle_time`,\
				`max_cycle_time`,`coor_phase`,`offset`,`signal_scheme_id`,`device_id`)VALUES");

				optimSqls += optimSql;
			}

			sprintf(optimSql, "(%lld,%d,%lld,%lld,%d,\
            %d,%d,%d,%d,%d,\
            %d,%d,%d,%d,%d,\
            %d,'%s',%d,%lld,'%s'),",
				optimId, splitId, optimStartTime, optimEndTime, monthFilter,
				dayFilter, weekdayFilter, fromhh, frommm, fromss,
				tohh, tomm, toss, optimCycleTime, minCycleTime,
				maxCycleTime, coorPhase.c_str(), offset, signalSchemeId, camData.deviceid().c_str());

			optimSqls += optimSql;

			/* optimPhaseList OptimPhase 子集  一对多*/
			// optim_phase -Table
			string optimPhaseSqls = "";
			for (int k = 0; k < camData.signalschemelist(i).optimdatalist(j).optimphaselist_size(); k++)
			{
				char optimPhaseSql[10240];
				long long optimPhaseId = g_sf->nextId();
				int phaseId = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).phaseid();
				int phaseOrder = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).order();
				int phaseTime = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).phasetime();
				int phaseGreen = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).green();
				int phaseYellowTime = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).phaseyellowtime();
				int phaseAllRedTime = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).phaseallredtime();
				int phaseMinGreen = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).mingreen();
				int phaseMaxGreen = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).maxgreen();

				if (k == 0)
				{
					sprintf(optimPhaseSql, "INSERT INTO `optim_phase`\
					(`id`,`split_id`,`phase_id`,`jhi_order`,`phase_time`,\
					`green`,`phase_yellow_time`,`phase_all_red_time`,`min_green`,`max_green`,`optim_data_id`,`device_id`)VALUES");

					optimPhaseSqls += optimPhaseSql;
				}

				sprintf(optimPhaseSql, "(%lld,%d,%d,%d,%d,\
                %d,%d,%d,%d,%d,%lld,'%s'),",
					optimPhaseId, splitId, phaseId, phaseOrder, phaseTime,
					phaseGreen, phaseYellowTime, phaseAllRedTime, phaseMinGreen, phaseMaxGreen, optimId, camData.deviceid().c_str());

				optimPhaseSqls += optimPhaseSql;

				/* movementsId OptimPhaseMovementEx 子集  一对多 优化方案对应转向信息*/
				// optim_phase_movement_ex -Table
				string movementsSqls = "";
				for (int m = 0; m < camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).movementid_size(); m++)
				{
					char movementsSql[10240];
					long long movementsId = g_sf->nextId();
					int movementsNodeRegionId = 0;
					int movementsNodeId = 0;
					if (camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).movementid(m).has_remoteintersection())
					{
						movementsNodeRegionId = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).movementid(m).remoteintersection().region();
						movementsNodeId = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).movementid(m).remoteintersection().nodeid();
					}
					int movementsPhaseId = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).movementid(m).phaseid();
					long long movementsTurnDirectionId = camData.signalschemelist(i).optimdatalist(j).optimphaselist(k).movementid(m).turndirection();

					if (m == 0)
					{
						sprintf(movementsSql, "INSERT INTO `optim_phase_movement_ex`\
						(`id`,`split_id`,`node_region_id`,`node_id`,`phase_id`,\
						`turn_direction_id`,`optim_phase_id`,`device_id`)VALUES");

						movementsSqls += movementsSql;
					}

					sprintf(movementsSql, "(%lld,%d,%d,%d,%d,%lld,%lld,'%s'),",
						movementsId, splitId, movementsNodeRegionId, movementsNodeId, movementsPhaseId,
						movementsTurnDirectionId, optimPhaseId, camData.deviceid().c_str());

					movementsSqls += movementsSql;
				}

				if (movementsSqls.length() > 0)
				{
					movementsSqls[movementsSqls.length() - 1] = ';';
				}

				if (movementsSqls.length() > 10)
				{
					ret = db->excuteSql(movementsSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert Movements Data Fail,SQL:{}", movementsSqls);
					}
				}
			}

			if (optimPhaseSqls.length() > 0)
			{
				optimPhaseSqls[optimPhaseSqls.length() - 1] = ';';
			}

			if (optimPhaseSqls.length() > 10)
			{
				ret = db->excuteSql(optimPhaseSqls);
				if (ret < 0)
				{
					ERRORLOG("Insert optimPhase Data Fail,SQL:{}", optimPhaseSqls);
				}
			}
		}

		if (optimSqls.length() > 0)
		{
			optimSqls[optimSqls.length() - 1] = ';';
		}

		if (optimSqls.length() > 10)
		{
			ret = db->excuteSql(optimSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert Optim Data Fail,SQL:{}", optimSqls);
			}
		}
	}
	if (signalSchemeSqls.length() > 0)
	{
		signalSchemeSqls[signalSchemeSqls.length() - 1] = ';';
	}

	if (signalSchemeSqls.length() > 10)
	{
		ret = db->excuteSql(signalSchemeSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert SignalScheme Data Fail,SQL:{}", signalSchemeSqls);
		}
	}
	
	// DetectedRegion
	string camPolygonSqls = "";
	for (int i = 0; i < camData.detectedregion_size(); i++)
	{
		char camPolygonSql[10240];
		long long camPolygonId = g_sf->nextId();

		if (i == 0)
		{
			sprintf(camPolygonSql, "INSERT INTO `cam_data_polygon`\
			(`id`,`split_id`,`cam_data_id`)VALUES");

			camPolygonSqls += camPolygonSql;
		}

		sprintf(camPolygonSql, "(%lld,%d,%lld),",
			camPolygonId, splitId, camId);

		camPolygonSqls += camPolygonSql;

		// cam_data_polygon_position_3_d -Table
		string camPolygonPosSqls = "";
		for (int j = 0; j < camData.detectedregion(i).pos_size(); j++)
		{
			char camPolygonPosSql[10240];
			long long camPolygonPosId = g_sf->nextId();
			int camPolygonPosLon = camData.detectedregion(i).pos(j).lon();
			int camPolygonPosLat = camData.detectedregion(i).pos(j).lat();
			int camPolygonPosEle = camData.detectedregion(i).pos(j).ele();
			if (j == 0)
			{
				sprintf(camPolygonPosSql, "INSERT INTO `cam_data_polygon_position_3_d`\
				(`id`,`split_id`,`lon`,`lat`,`ele`,`cam_data_polygon_id`)VALUES");
				camPolygonPosSqls += camPolygonPosSql;
			}

			sprintf(camPolygonPosSql, "(%lld,%d,%d,%d,%d,%lld),",
				camPolygonPosId, splitId, camPolygonPosLon, camPolygonPosLat, camPolygonPosEle, camPolygonId);

			camPolygonPosSqls += camPolygonPosSql;
		}

		if (camPolygonPosSqls.length() > 0)
		{
			camPolygonPosSqls[camPolygonPosSqls.length() - 1] = ';';
		}

		if (camPolygonPosSqls.length() > 10)
		{
			ret = db->excuteSql(camPolygonPosSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert CamPolygonPos Data Fail,SQL:{}", camPolygonPosSqls);
			}
		}
	}

	if (camPolygonSqls.length() > 0)
	{
		camPolygonSqls[camPolygonSqls.length() - 1] = ';';
	}

	if (camPolygonSqls.length() > 10)
	{
		ret = db->excuteSql(camPolygonSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert CamPolygon Data Fail,SQL:{}", camPolygonSqls);
		}
	}

	// ObstacleData 
	string obslistSqls = "";
	for (int i = 0; i < camData.obstaclelist_size(); i++)
	{
		char obslistSql[40960];
		long long obslistId = g_sf->nextId();
		int obsLat = 0;
		int obsLon = 0;
		int obsEle = 0;
		int obsPosconfid = 0;
		int obsEleconfid = 0;
		int obsSectionId = 0;
		int obsLaneId = 0;
		string obsLinkName = "";
		int obsRegionId = 0;
		int obsNodeId = 0;
		int obsUpstreamRegionId = 0;
		int obsUpstreamNodeId = 0;
		int obsSpeedCfd = 0;
		int obsHeadingCfd = 0;
		int obsSteerCfd = 0;
		int obsAcceLat = 0;
		int obsAcceLon = 0;
		int obsAcceVert = 0;
		int obsAcceYaw = 0;
		int obsWidth = 0;
		int obsLength = 0;
		int obsHeight = 0;
		int obsWidthConfid = 0;
		int obsLengthConfid = 0;
		int obsHeightConfid = 0;
		if (camData.obstaclelist(i).has_obspos())
		{
			obsLat = camData.obstaclelist(i).obspos().lat();
			obsLon = camData.obstaclelist(i).obspos().lon();
			obsEle = camData.obstaclelist(i).obspos().ele();
		}
		if (camData.obstaclelist(i).has_posconfid())
		{
			obsPosconfid = camData.obstaclelist(i).posconfid().posconfid();
			obsEleconfid = camData.obstaclelist(i).posconfid().eleconfid();
		}
		if (camData.obstaclelist(i).has_maplocation())
		{
			obsSectionId = camData.obstaclelist(i).maplocation().sectionid();
			obsLaneId = camData.obstaclelist(i).maplocation().laneid();
			obsLinkName = camData.obstaclelist(i).maplocation().linkname();
			if (camData.obstaclelist(i).maplocation().has_nodeid())
			{
				obsRegionId = camData.obstaclelist(i).maplocation().nodeid().region();
				obsNodeId = camData.obstaclelist(i).maplocation().nodeid().nodeid();
			}
			if (camData.obstaclelist(i).maplocation().has_upstreamnodeid())
			{
				obsUpstreamRegionId = camData.obstaclelist(i).maplocation().upstreamnodeid().region();
				obsUpstreamNodeId = camData.obstaclelist(i).maplocation().upstreamnodeid().nodeid();
			}
		}
		if (camData.obstaclelist(i).has_motionconfid())
		{
			obsSpeedCfd = camData.obstaclelist(i).motionconfid().speedcfd();
			obsHeadingCfd = camData.obstaclelist(i).motionconfid().headingcfd();
			obsSteerCfd = camData.obstaclelist(i).motionconfid().steercfd();
		}
		if (camData.obstaclelist(i).has_acceleration())
		{
			obsAcceLat = camData.obstaclelist(i).acceleration().lat();
			obsAcceLon = camData.obstaclelist(i).acceleration().lon();
			obsAcceVert = camData.obstaclelist(i).acceleration().vert();
			obsAcceYaw = camData.obstaclelist(i).acceleration().yaw();
		}
		if (camData.obstaclelist(i).has_size())
		{
			obsWidth = camData.obstaclelist(i).size().width();
			obsLength = camData.obstaclelist(i).size().length();
			obsHeight = camData.obstaclelist(i).size().height();
		}
		if (camData.obstaclelist(i).has_obssizeconfid())
		{
			obsWidthConfid = camData.obstaclelist(i).obssizeconfid().widthconfid();
			obsLengthConfid = camData.obstaclelist(i).obssizeconfid().lengthconfid();
			obsHeightConfid = camData.obstaclelist(i).obssizeconfid().heightconfid();
		}

		if (i == 0)
		{
			sprintf(obslistSql, "INSERT INTO `obstacle_data`(`id`,`split_id`,`obs_id`,`obs_type_cfd`,`timestamp`,\
			`device_id_list`,`lat`,`lon`,`ele`,`node_region_id`,\
			`node_id`,`link_name`,`upstream_node_region_id`,`upstream_node_node_id`,`section_id`,\
			`lane_id`,`speed`,`heading`,`ver_speed`,`acceleration_lon`,\
			`acceleration_lat`,`acceleration_vert`,`acceleration_yaw`,`width`,`length`,\
			`height`,`tracking`,`obs_type_id`,`obs_source_id`,`pos_confid_id`,\
			`ele_confid_id`,`speed_cfd_id`,`heading_cfd_id`,`steer_cfd_id`,`ver_speed_confid_id`,\
			`width_confid_id`,`length_confid_id`,`height_confid_id`,`cam_data_id`)VALUES");
			obslistSqls += obslistSql;
		}

		sprintf(obslistSql, "(%lld,%d,%lld,%d,%lld,\
		'%s',%d,%d,%d,%d,\
		%d,'%s',%d,%d,%d,\
		%d,%d,%d,%d,%d,\
		%d,%d,%d,%d,%d,\
		%d,%d,%d,%d,%d,\
		%d,%d,%d,%d,%d,\
		%d,%d,%d,%lld),",
			obslistId, splitId, camData.obstaclelist(i).obsid(), camData.obstaclelist(i).obstype(), camData.obstaclelist(i).timestamp(),
			camData.obstaclelist(i).deviceidlist().c_str(), obsLat, obsLon, obsEle, obsRegionId,
			obsNodeId, obsLinkName.c_str(), obsUpstreamRegionId, obsUpstreamNodeId, obsSectionId,
			obsLaneId, camData.obstaclelist(i).speed(), camData.obstaclelist(i).heading(), camData.obstaclelist(i).verspeed(), obsAcceLon,
			obsAcceLat, obsAcceVert, obsAcceYaw, obsWidth, obsLength,
			obsHeight, camData.obstaclelist(i).tracking(), camData.obstaclelist(i).obstype(), camData.obstaclelist(i).obssource(), obsPosconfid,
			obsEleconfid, obsSpeedCfd, obsHeadingCfd, obsSteerCfd, camData.obstaclelist(i).verspeedconfid(),
			obsWidthConfid, obsLengthConfid, obsHeightConfid, camId);

		obslistSqls += obslistSql;

		if (camData.obstaclelist(i).has_polygon())
		{
			string obspolySqls = "";
			for (int j = 0; j < camData.obstaclelist(i).polygon().pos_size(); j++)
			{
				char obspolySql[4096];
				long long obspolyId = g_sf->nextId();

				if (j == 0)
				{
					sprintf(obspolySql, "INSERT INTO `obstacle_data_position_3_d`(`id`,`split_id`,`lat`,`lon`,`ele`,`obstacle_data_id`)VALUES");
					obspolySqls += obspolySql;
				}

				sprintf(obspolySql, "(%lld,%d,%d,%d,%d,%lld),",
					obspolyId, splitId, camData.obstaclelist(i).polygon().pos(j).lat(), camData.obstaclelist(i).polygon().pos(j).lon(), camData.obstaclelist(i).polygon().pos(j).ele(), obslistId);

				obspolySqls += obspolySql;
			}

			if (obspolySqls.length() > 0)
			{
				obspolySqls[obspolySqls.length() - 1] = ';';
			}

			if (obspolySqls.length() > 10)
			{
				ret = db->excuteSql(obspolySqls);
				if (ret < 0)
				{
					ERRORLOG("Insert Obspoly Data Fail,SQL:{}", obspolySqls);
				}
			}
		}
	}

	if (obslistSqls.length() > 0)
	{
		obslistSqls[obslistSqls.length() - 1] = ';';
	}

	if (obslistSqls.length() > 10)
	{
		ret = db->excuteSql(obslistSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert Obslist Data Fail,SQL:{}", obslistSqls);
		}
	}

	// bsm_data -Table not motified
	string bsmSqls = "";
	for (int i = 0; i < camData.bsmlist_size(); i++)
	{
		char bsmSql[10240];
		long long bsmId = g_sf->nextId();
		string obuId = camData.bsmlist(i).obuid();
		string plateNo = camData.bsmlist(i).plateno();
		long long bsmTimestamp = camData.bsmlist(i).timestamp();
		/* pos Position3D 拆分*/
		int bsmLon = 0;
		int bsmLat = 0;
		int bsmEle = 0;
		if (camData.bsmlist(i).has_pos())
		{
			bsmLon = camData.bsmlist(i).pos().lon();
			bsmLat = camData.bsmlist(i).pos().lat();
			bsmEle = camData.bsmlist(i).pos().ele();
		}
		/* posConfid PositionConfidenceSet 拆分 障碍物位置精度*/
		int bsmPosConfidId = 0;
		int bsmEleConfidId = 0;
		if (camData.bsmlist(i).has_posconfid())
		{
			bsmPosConfidId = camData.bsmlist(i).posconfid().posconfid();
			bsmEleConfidId = camData.bsmlist(i).posconfid().eleconfid();
		}
		/* posAccuracy PositionAccuracy 拆分 椭圆模型表示的GNSS系统精度*/
		int semiMajor = 0;
		int semiMinor = 0;
		int orientation = 0;
		if (camData.bsmlist(i).has_posaccuracy())
		{
			semiMajor = camData.bsmlist(i).posaccuracy().semimajor();
			semiMinor = camData.bsmlist(i).posaccuracy().semiminor();
			orientation = camData.bsmlist(i).posaccuracy().orientation();
		}
		/* acceleration AccelerationSet4Way 拆 四轴加速度 */
		int bsmAccelerationLon = 0;
		int bsmAccelerationLat = 0;
		int bsmAccelerationVert = 0;
		int bsmAccelerationYaw = 0;
		if (camData.bsmlist(i).has_acceleration())
		{
			bsmAccelerationLon = camData.bsmlist(i).acceleration().lon();
			bsmAccelerationLat = camData.bsmlist(i).acceleration().lat();
			bsmAccelerationVert = camData.bsmlist(i).acceleration().vert();
			bsmAccelerationYaw = camData.bsmlist(i).acceleration().yaw();
		}
		int transmission = camData.bsmlist(i).transmission();
		int bsmSpeed = camData.bsmlist(i).speed();
		int bsmHeading = camData.bsmlist(i).heading();
		int steeringWheelAngle = camData.bsmlist(i).steeringwheelangle();
		/* motionConfid MotionConfidenceSet 运动状态精度  拆 */
		int bsmSpeedCfd = 0;
		int bsmHeadingCfd = 0;
		int bsmSteerCfd = 0;
		if (camData.bsmlist(i).has_motionconfid())
		{
			bsmSpeedCfd = camData.bsmlist(i).motionconfid().speedcfd();
			bsmHeadingCfd = camData.bsmlist(i).motionconfid().headingcfd();
			bsmSteerCfd = camData.bsmlist(i).motionconfid().steercfd();
		}
		/* brakes BrakeSystemStatus 拆 定义车辆的刹车系统状态,包括了8种不同类型的状态 */
		int brakePadel = 0;
		int wheelBrakes = 0;
		int traction = 0;
		int abs = 0;
		int scs = 0;
		int brakeBoost = 0;
		int auxBrakes = 0;
		int brakeControl = 0;
		if (camData.bsmlist(i).has_brakes())
		{
			brakePadel = camData.bsmlist(i).brakes().brakepadel();
			wheelBrakes = camData.bsmlist(i).brakes().wheelbrakes();
			traction = camData.bsmlist(i).brakes().traction();
			abs = camData.bsmlist(i).brakes().abs();
			scs = camData.bsmlist(i).brakes().scs();
			brakeBoost = camData.bsmlist(i).brakes().brakeboost();
			auxBrakes = camData.bsmlist(i).brakes().auxbrakes();
			brakeControl = camData.bsmlist(i).brakes().brakecontrol();
		}
		/* throttle ThrottleSystemStatus 拆  油门系统状态 */
		/**油门踩踏强度 百分比：0~100%,精度0.1%*/
		int throttleControl = 0;
		int throttlePadel = 0;
		int wheelThrottles = 0;
		if (camData.bsmlist(i).has_throttle())
		{
			throttleControl = camData.bsmlist(i).throttle().thorttlecontrol();
			throttlePadel = camData.bsmlist(i).throttle().throttlepadel();
			wheelThrottles = camData.bsmlist(i).throttle().wheelthrottles();
		}
		/* size VehicleSize 拆分*/
		int bsmVehicleSizeWidth = 0;
		int bsmVehicleSizeLength = 0;
		int bsmVehicleSizeHeight = 0;
		if (camData.bsmlist(i).has_size())
		{
			bsmVehicleSizeWidth = camData.bsmlist(i).size().width();
			bsmVehicleSizeLength = camData.bsmlist(i).size().length();
			bsmVehicleSizeHeight = camData.bsmlist(i).size().height();
		}
		int bsmVehicleType = camData.bsmlist(i).vehicletype();
		int bsmFuelType = camData.bsmlist(i).fueltype();
		int driveModedriveStatus = camData.bsmlist(i).drivemodedrivestatus();
		int emergencyStatus = camData.bsmlist(i).emergencystatus();
		int bsmLight = camData.bsmlist(i).light();
		int bsmWiper = camData.bsmlist(i).wiper();
		int bsmOutofControl = camData.bsmlist(i).outofcontrol();
		int bsmEndurance = camData.bsmlist(i).endurance();

		if (i == 0)
		{
			sprintf(bsmSql, "INSERT INTO `bsm_data`\
			(`id`,`split_id`,`obu_id`,`plate_no`,`timestamp`,\
			`lat`,`lon`,`ele`,`semi_major`,`semi_minor`,\
			`orientation`,`acceleration_lon`,`acceleration_lat`,`acceleration_vert`,`acceleration_yaw`,\
			`speed`,`heading`,`steering_wheel_angle`,`wheel_brakes`,`brake_control`,\
			`throttle_control`,`wheel_throttles`,`width`,`length`,`height`,\
			`light`,`endurance`,`pos_confid_id`,`ele_confid_id`,`transmission_id`,\
			`speed_cfd_id`,`heading_cfd_id`,`steer_cfd_id`,`brake_padel_id`,`traction_id`,\
			`abs_id`,`scs_id`,`brake_boost_id`,`aux_brakes_id`,`throttle_padel_id`,\
			`vehicle_type_id`,`fuel_type_id`,`drive_modedrive_status_id`,`emergency_status_id`,`wiper_id`,\
			`outof_control_id`,`cam_data_id`)VALUES");

			bsmSqls += bsmSql;
		}

		sprintf(bsmSql, "(%lld,%d,'%s','%s',%lld,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d %d,\
        %d,%d,%d,%d,%d,%d,%lld),",
			bsmId, splitId, obuId.c_str(), plateNo.c_str(), bsmTimestamp,
			bsmLat, bsmLon, bsmEle, semiMajor, semiMinor,
			orientation, bsmAccelerationLon, bsmAccelerationLat, bsmAccelerationVert, bsmAccelerationYaw,
			bsmSpeed, bsmHeading, steeringWheelAngle, wheelBrakes, brakeControl,
			throttleControl, wheelThrottles, bsmVehicleSizeWidth, bsmVehicleSizeLength, bsmVehicleSizeHeight,
			bsmLight, bsmEndurance, bsmPosConfidId, bsmEleConfidId, transmission,
			bsmSpeedCfd, bsmHeadingCfd, bsmSteerCfd, brakePadel, traction,
			abs, scs, brakeBoost, auxBrakes, throttlePadel,
			bsmVehicleType, bsmFuelType, driveModedriveStatus, emergencyStatus, bsmWiper,
			bsmOutofControl, camId);

		bsmSqls += bsmSql;
	}

	if (bsmSqls.length() > 0)
	{
		bsmSqls[bsmSqls.length() - 1] = ';';
	}

	if (bsmSqls.length() > 10)
	{
		ret = db->excuteSql(bsmSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert Bsm Data Fail,SQL:{}", bsmSqls);
		}
	}

	// rscList
	string rscSqls = "";
	for (int i = 0; i < camData.rsclist_size(); i++)
	{
		char rscSql[40960];
		long long rsclistId = g_sf->nextId();
		int rscLat = 0;
		int rscLon = 0;
		int rscEle = 0;
		string vehId = "";
		int driveSuggestionSuggestion = 0;
		int timeOffset = 0;
		int driveSuggestionUpstreamNodeRegionId = 0;
		int driveSuggestionUpstreamNodeId = 0;
		int driveSuggestionDownstreamNodeRegionId = 0;
		int driveSuggestionDownstreamNodeId = 0;
		int driveSuggestionReferenceLanes = 0;
		int driveSuggestionPathRadius = 0;
		int vehicleCoordinationInfo = 0;
		int laneCoordinatioUpstreamNodeRegionId = 0;
		int laneCoordinatioUpstreamNodeId = 0;
		int laneCoordinatioDownstreamNodeRegionId = 0;
		int laneCoordinatioDownstreamNodeId = 0;
		int laneCoordinatioReferenceLanes = 0;
		int vehCoordinateInfo = 0;
		long long tBegin = 0;
		long long tEnd = 0;
		int laneCoordinatioPathRadius = 0;
		int recommendedSpeed = 0;
		int recommendedBehavior = 0;
		int laneCoordinatesInfo = 0;
		string description = "";
		if (camData.rsclist(i).has_pos())
		{
			rscLat = camData.rsclist(i).pos().lat();
			rscLon = camData.rsclist(i).pos().lon();
			rscEle = camData.rsclist(i).pos().ele();
		}
		if (camData.rsclist(i).has_coordinates())
		{
			vehId = camData.rsclist(i).coordinates().vehid();
			if (camData.rsclist(i).coordinates().has_drivesuggestion())
			{
				if (camData.rsclist(i).coordinates().drivesuggestion().has_suggestion())
				{
					driveSuggestionSuggestion = camData.rsclist(i).coordinates().drivesuggestion().suggestion().drivebehavior();
				}
				timeOffset = camData.rsclist(i).coordinates().drivesuggestion().timeoffset();
				if (camData.rsclist(i).coordinates().drivesuggestion().has_relatedlink())
				{
					if (camData.rsclist(i).coordinates().drivesuggestion().relatedlink().has_upstreamnodeid())
					{
						driveSuggestionUpstreamNodeRegionId = camData.rsclist(i).coordinates().drivesuggestion().relatedlink().upstreamnodeid().region();
						driveSuggestionUpstreamNodeId = camData.rsclist(i).coordinates().drivesuggestion().relatedlink().upstreamnodeid().nodeid();
					}
					if (camData.rsclist(i).coordinates().drivesuggestion().relatedlink().has_downstreamnodeid())
					{
						driveSuggestionDownstreamNodeRegionId = camData.rsclist(i).coordinates().drivesuggestion().relatedlink().downstreamnodeid().region();
						driveSuggestionDownstreamNodeId = camData.rsclist(i).coordinates().drivesuggestion().relatedlink().downstreamnodeid().nodeid();
					}
					if (camData.rsclist(i).coordinates().drivesuggestion().relatedlink().has_referencelanes())
					{
						driveSuggestionReferenceLanes = camData.rsclist(i).coordinates().drivesuggestion().relatedlink().referencelanes().referencelanes();
					}
				}
				if (camData.rsclist(i).coordinates().drivesuggestion().has_relatedpath())
				{
					string rscDriveSugPosSqls = "";
					driveSuggestionPathRadius = camData.rsclist(i).coordinates().drivesuggestion().relatedpath().pathradius();
					for (int j = 0; j < camData.rsclist(i).coordinates().drivesuggestion().relatedpath().activepath_size(); j++)
					{
						char rscDriveSugPosSql[4096];
						long long rscDriveSugPosId = g_sf->nextId();

						if (j == 0)
						{
							sprintf(rscDriveSugPosSql, "INSERT INTO `rsc_data_drive_suggestion_position_3_d`(`id`,`split_id`,`lat`,`lon`,`ele`,`rsc_data_id`)VALUES");
							rscDriveSugPosSqls += rscDriveSugPosSql;
						}

						sprintf(rscDriveSugPosSql, "(%lld,%d,%d,%d,%d,%lld),",
							rscDriveSugPosId, splitId, camData.rsclist(i).coordinates().drivesuggestion().relatedpath().activepath(j).lat(),
							camData.rsclist(i).coordinates().drivesuggestion().relatedpath().activepath(j).lon(), camData.rsclist(i).coordinates().drivesuggestion().relatedpath().activepath(j).ele(), rsclistId);

						rscDriveSugPosSqls += rscDriveSugPosSql;
					}

					if (rscDriveSugPosSqls.length() > 0)
					{
						rscDriveSugPosSqls[rscDriveSugPosSqls.length() - 1] = ';';
					}

					if (rscDriveSugPosSqls.length() > 10)
					{
						ret = db->excuteSql(rscDriveSugPosSqls);
						if (ret < 0)
						{
							ERRORLOG("Insert RscDriveSugPos Data Fail,SQL:{}", rscDriveSugPosSqls);
						}
					}
				}
			}
			if (camData.rsclist(i).coordinates().has_pathguidance())
			{
				string rscPathPlanningSqls = "";
				for (int k = 0; k < camData.rsclist(i).coordinates().pathguidance().pathplanning_size(); k++)
				{
					char rscPathPlanningSql[10240];
					long long rscPathPlanningId = g_sf->nextId();
					int rscPathPlanningLat = 0;
					int rscPathPlanningLon = 0;
					int rscPathPlanningEle = 0;
					int rscposConfidPosCfd = 0;
					int rscposConfidEleCfd = 0;
					int rscAcceSetLat = 0;
					int rscAcceSetYaw = 0;
					int rscAcceSetLon = 0;
					int rscAcceSetVert = 0;
					int rscAcceSetLatCfd = 0;
					int rscAcceSetYawCfd = 0;
					int rscAcceSetLonCfd = 0;
					int rscAcceSetVertCfd = 0;
					int pathPlanningUpstreamNodeRegionId = 0;
					int pathPlanningUpstreamNodeId = 0;
					int pathPlanningDownstreamNodeRegionId = 0;
					int pathPlanningDownstreamNodeId = 0;
					int pathPlanningReferenceLanes = 0;
					if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).has_pos())
					{
						rscPathPlanningLat = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).pos().lat();
						rscPathPlanningLon = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).pos().lon();
						rscPathPlanningEle = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).pos().ele();
					}
					if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).has_posconfid())
					{
						rscposConfidPosCfd = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posconfid().posconfid();
						rscposConfidEleCfd = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posconfid().eleconfid();
					}
					if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).has_acceleration())
					{
						rscAcceSetLat = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).acceleration().lat();
						rscAcceSetLon = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).acceleration().lon();
						rscAcceSetVert = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).acceleration().vert();
						rscAcceSetYaw = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).acceleration().yaw();
					}
					if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).has_accelerationconfid())
					{
						rscAcceSetLatCfd = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).accelerationconfid().lataccelconfid();
						rscAcceSetLonCfd = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).accelerationconfid().lonaccelconfid();
						rscAcceSetVertCfd = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).accelerationconfid().verticalaccelconfid();
						rscAcceSetYawCfd = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).accelerationconfid().yawrateconfid();
					}
					if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).has_posinmap())
					{
						if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().has_upstreamnodeid())
						{
							pathPlanningUpstreamNodeRegionId = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().upstreamnodeid().region();
							pathPlanningUpstreamNodeId = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().upstreamnodeid().nodeid();
						}
						if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().has_downstreamnodeid())
						{
							pathPlanningDownstreamNodeRegionId = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().downstreamnodeid().region();
							pathPlanningDownstreamNodeId = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().downstreamnodeid().nodeid();
						}
						if (camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().has_referencelanes())
						{
							pathPlanningReferenceLanes = camData.rsclist(i).coordinates().pathguidance().pathplanning(k).posinmap().referencelanes().referencelanes();
						}
					}

					if (k == 0)
					{
						sprintf(rscPathPlanningSql, "INSERT INTO `rsc_data_path_planning_point`(`id`,`split_id`,`lat`,`lon`,`ele`,\
						`upstream_node_region_id`,`upstream_node_id`,`downstream_node_region_id`,`downstream_node_id`,`reference_lanes`,\
						`speed`,`heading`,`acceleration_lon`,`acceleration_lat`,`acceleration_vert`,\
						`acceleration_yaw`,`estimated_time`,`pos_confid_id`,`ele_confid_id`,`speed_confid_id`,\
						`heading_confid_id`,`lon_accel_confid_id`,`lat_accel_confid_id`,`vertical_accel_confid_id`,`yaw_rate_confid_id`,\
						`time_confidence_id`,`rsc_data_id`)VALUES");

						rscPathPlanningSqls += rscPathPlanningSql;
					}

					sprintf(rscPathPlanningSql, "(%lld,%d,%d,%d,%d,\
					%d,%d,%d,%d,%d,\
					%d,%d,%d,%d,%d,\
					%d,%d,%d,%d,%d,\
					%d,%d,%d,%d,%d,%d,%lld),",
						rscPathPlanningId, splitId, rscPathPlanningLat, rscPathPlanningLon, rscPathPlanningEle,
						pathPlanningUpstreamNodeRegionId, pathPlanningUpstreamNodeId, pathPlanningDownstreamNodeRegionId, pathPlanningDownstreamNodeId, pathPlanningReferenceLanes,
						camData.rsclist(i).coordinates().pathguidance().pathplanning(k).speed(), camData.rsclist(i).coordinates().pathguidance().pathplanning(k).heading(), rscAcceSetLon, rscAcceSetLat, rscAcceSetVert,
						rscAcceSetYaw, camData.rsclist(i).coordinates().pathguidance().pathplanning(k).estimatedtime(), rscposConfidPosCfd, rscposConfidEleCfd, camData.rsclist(i).coordinates().pathguidance().pathplanning(k).speedconfid(),
						camData.rsclist(i).coordinates().pathguidance().pathplanning(k).headingconfid(), rscAcceSetLonCfd, rscAcceSetLatCfd, rscAcceSetVertCfd, rscAcceSetYawCfd,
						camData.rsclist(i).coordinates().pathguidance().pathplanning(k).timeconfidence(), rsclistId);

					rscPathPlanningSqls += rscPathPlanningSql;
				}

				if (rscPathPlanningSqls.length() > 0)
				{
					rscPathPlanningSqls[rscPathPlanningSqls.length() - 1] = ';';
				}

				if (rscPathPlanningSqls.length() > 10)
				{
					ret = db->excuteSql(rscPathPlanningSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert RscPathPlanning Data Fail,SQL:{}", rscPathPlanningSqls);
					}
				}
			}
			if (camData.rsclist(i).coordinates().has_info())
			{
				vehCoordinateInfo = camData.rsclist(i).coordinates().info().coordinationinfo();
			}
		}
		if (camData.rsclist(i).has_lanecoordinates())
		{
			if (camData.rsclist(i).lanecoordinates().has_targetlane())
			{
				if (camData.rsclist(i).lanecoordinates().targetlane().has_upstreamnodeid())
				{
					laneCoordinatioUpstreamNodeRegionId = camData.rsclist(i).lanecoordinates().targetlane().upstreamnodeid().region();
					laneCoordinatioUpstreamNodeId = camData.rsclist(i).lanecoordinates().targetlane().upstreamnodeid().nodeid();
				}
				if (camData.rsclist(i).lanecoordinates().targetlane().has_downstreamnodeid())
				{
					laneCoordinatioDownstreamNodeRegionId = camData.rsclist(i).lanecoordinates().targetlane().downstreamnodeid().region();
					laneCoordinatioDownstreamNodeId = camData.rsclist(i).lanecoordinates().targetlane().downstreamnodeid().nodeid();
				}
				if (camData.rsclist(i).lanecoordinates().targetlane().has_referencelanes())
				{
					laneCoordinatioReferenceLanes = camData.rsclist(i).lanecoordinates().targetlane().referencelanes().referencelanes();
				}
			}
			if (camData.rsclist(i).lanecoordinates().has_relatedpath())
			{
				string rscLaneCoorPosSqls = "";
				laneCoordinatioPathRadius = camData.rsclist(i).lanecoordinates().relatedpath().pathradius();
				for (int m = 0; m < camData.rsclist(i).lanecoordinates().relatedpath().activepath_size(); m++)
				{
					char rscLaneCoorPosSql[10240];
					long long rscLaneCoorPosId = g_sf->nextId();

					if (m == 0)
					{
						sprintf(rscLaneCoorPosSql, "INSERT INTO `rsc_data_lane_coordination_position_3_d`(`id`,`split_id`,`lat`,`lon`,`ele`,`rsc_data_id`)VALUES");
						rscLaneCoorPosSqls += rscLaneCoorPosSql;
					}

					sprintf(rscLaneCoorPosSql, "(%lld,%d,%d,%d,%d,%lld),",
						rscLaneCoorPosId, splitId, camData.rsclist(i).lanecoordinates().relatedpath().activepath(m).lat(),
						camData.rsclist(i).lanecoordinates().relatedpath().activepath(m).lon(), camData.rsclist(i).lanecoordinates().relatedpath().activepath(m).ele(), rsclistId);

					rscLaneCoorPosSqls += rscLaneCoorPosSql;
				}

				if (rscLaneCoorPosSqls.length() > 0)
				{
					rscLaneCoorPosSqls[rscLaneCoorPosSqls.length() - 1] = ';';
				}

				if (rscLaneCoorPosSqls.length() > 10)
				{
					ret = db->excuteSql(rscLaneCoorPosSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert RscLaneCoorPos Data Fail,SQL:{}", rscLaneCoorPosSqls);
					}
				}
			}
			tBegin = camData.rsclist(i).lanecoordinates().tbegin();
			tEnd = camData.rsclist(i).lanecoordinates().tend();
			recommendedSpeed = camData.rsclist(i).lanecoordinates().recommendedspeed();
			description = camData.rsclist(i).lanecoordinates().description();
			if (camData.rsclist(i).lanecoordinates().has_recommendedbehavior())
			{
				recommendedBehavior = camData.rsclist(i).lanecoordinates().recommendedbehavior().drivebehavior();
			}
			if (camData.rsclist(i).lanecoordinates().has_info())
			{
				laneCoordinatesInfo = camData.rsclist(i).lanecoordinates().info().coordinationinfo();
			}
		}

		if (i == 0)
		{
			sprintf(rscSql, "INSERT INTO `rsc_data`(`id`,`split_id`,`msg_cnt`,`rsu_id`,`timestamp`,\
			`lat`,`lon`,`ele`,`veh_id`,`drive_suggestion_suggestion`,\
			`time_offset`,`drive_suggestion_upstream_node_region_id`,`drive_suggestion_upstream_node_id`,`drive_suggestion_downstream_node_region_id`,`drive_suggestion_downstream_node_id`,\
			`drive_suggestion_reference_lanes`,`drive_suggestion_path_radius`,`coordinates_info`,`lane_coordinatio_upstream_node_region_id`,`lane_coordinatio_upstream_node_id`,\
			`lane_coordinatio_downstream_node_region_id`,`lane_coordinatio_downstream_node_id`,`lane_coordinatio_reference_lanes`,`path_radius`,`t_begin`,\
			`t_end`,`recommended_speed`,`recommended_behavior`,`lane_coordinates_info`,`description`,\
			`cam_data_id`)VALUES");
			rscSqls += rscSql;
		}

		sprintf(rscSql, "(%lld,%d,%d,'%s',%lld,\
		%d,%d,%d,'%s',%d,\
		%d,%d,%d,%d,%d,\
		%d,%d,%d,%d,%d,\
		%d,%d,%d,%d,%lld,\
		%lld,%d,%d,%d,'%s',%lld),",
			rsclistId, splitId, camData.rsclist(i).msgcnt(), camData.rsclist(i).rsuid().c_str(), camData.rsclist(i).timestamp(),
			rscLat, rscLon, rscEle, vehId.c_str(), driveSuggestionSuggestion,
			timeOffset, driveSuggestionUpstreamNodeRegionId, driveSuggestionUpstreamNodeId, driveSuggestionDownstreamNodeRegionId, driveSuggestionDownstreamNodeId,
			driveSuggestionReferenceLanes, driveSuggestionPathRadius, vehicleCoordinationInfo, laneCoordinatioUpstreamNodeRegionId, laneCoordinatioUpstreamNodeId,
			laneCoordinatioDownstreamNodeRegionId, laneCoordinatioDownstreamNodeId, laneCoordinatioReferenceLanes, laneCoordinatioPathRadius, tBegin,
			tEnd, recommendedSpeed, recommendedBehavior, laneCoordinatesInfo, description.c_str(), camId);

		rscSqls += rscSql;
	}

	if (rscSqls.length() > 0)
	{
		rscSqls[rscSqls.length() - 1] = ';';
	}

	if (rscSqls.length() > 10)
	{
		ret = db->excuteSql(rscSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert Rsc Data Fail,SQL:{}", rscSqls);
		}
	}

	// vir_data -Table motified
	string virSqls = "";
	for (int i = 0; i < camData.virlist_size(); i++)
	{
		char virSql[10240];
		long long virId = g_sf->nextId();
		int virMsgCnt = camData.virlist(i).msgcnt();
		string virVehicleId = camData.virlist(i).vehicleid();
		long long virTimestamp = camData.virlist(i).timestamp();
		/* pos Position3D 拆 */
		int virLat = 0;
		int virLon = 0;
		int virEle = 0;
		if (camData.virlist(i).has_pos())
		{
			virLat = camData.virlist(i).pos().lat();
			virLon = camData.virlist(i).pos().lon();
			virEle = camData.virlist(i).pos().ele();
		}
		/* intAndReq IarData 拆*/
		int currentBehavior = 0;
		/* currentPos PathPlanningPoint 拆 地图中的当前位置 */
		int virSpeed = 0;
		int virHeading = 0;
		int virSpeedConfid = 0;
		int virHeadingConfid = 0;
		int virEstimatedTime = 0;
		int virTimeConfidence = 0;
		/* pos Position3D 标志标线位置*/
		int currentPosLat = 0;
		int currentPosLon = 0;
		int currentPosEle = 0;
		/* posConfid PositionConfidenceSet 拆*/
		int virPosConfid = 0;
		int virEleConfid = 0;
		/* acceleration AccelerationSet4Way 拆 四轴加速度 */
		int virAcclerationLon = 0;
		int virAcclerationLat = 0;
		int virAcclerationVert = 0;
		int virAcclerationYaw = 0;
		/* accelerationConfid AccelerationConfidence 拆*/
		int virLonAccelConfid = 0;
		int virLatAccelConfid = 0;
		int virVertAccelConfid = 0;
		int virYawAccelConfid = 0;
		/* posInMap ReferenceLink 拆  相关的车道和链接位置 */
		int virReferenceLanes = 0;
		/* upstreamNodeId NodeReferenceId 拆*/
		int virUpstreamNodeRegionId = 0;
		int virUpstreamNodeId = 0;
		/* downstreamNodeId NodeReferenceId 拆*/
		int virDownstreamNodeRegionId = 0;
		int virDownstreamNodeId = 0;
		if (camData.virlist(i).has_intandreq())
		{
			currentBehavior = camData.virlist(i).intandreq().currentbehavior().drivebehavior();
			if (camData.virlist(i).intandreq().has_currentpos())
			{
				virSpeed = camData.virlist(i).intandreq().currentpos().speed();
				virHeading = camData.virlist(i).intandreq().currentpos().heading();
				virSpeedConfid = camData.virlist(i).intandreq().currentpos().speedconfid();
				virHeadingConfid = camData.virlist(i).intandreq().currentpos().headingconfid();
				virEstimatedTime = camData.virlist(i).intandreq().currentpos().estimatedtime();
				virTimeConfidence = camData.virlist(i).intandreq().currentpos().timeconfidence();
				if (camData.virlist(i).intandreq().currentpos().has_pos())
				{
					currentPosLat = camData.virlist(i).intandreq().currentpos().pos().lat();
					currentPosLon = camData.virlist(i).intandreq().currentpos().pos().lon();
					currentPosEle = camData.virlist(i).intandreq().currentpos().pos().ele();
				}
				if (camData.virlist(i).intandreq().currentpos().has_posconfid())
				{
					virPosConfid = camData.virlist(i).intandreq().currentpos().posconfid().posconfid();
					virEleConfid = camData.virlist(i).intandreq().currentpos().posconfid().eleconfid();
				}
				if (camData.virlist(i).intandreq().currentpos().has_acceleration())
				{
					virAcclerationLon = camData.virlist(i).intandreq().currentpos().acceleration().lon();
					virAcclerationLat = camData.virlist(i).intandreq().currentpos().acceleration().lat();
					virAcclerationVert = camData.virlist(i).intandreq().currentpos().acceleration().vert();
					virAcclerationYaw = camData.virlist(i).intandreq().currentpos().acceleration().yaw();
				}
				if (camData.virlist(i).intandreq().currentpos().has_accelerationconfid())
				{
					virLonAccelConfid = camData.virlist(i).intandreq().currentpos().accelerationconfid().lonaccelconfid();
					virLatAccelConfid = camData.virlist(i).intandreq().currentpos().accelerationconfid().lataccelconfid();
					virVertAccelConfid = camData.virlist(i).intandreq().currentpos().accelerationconfid().verticalaccelconfid();
					virYawAccelConfid = camData.virlist(i).intandreq().currentpos().accelerationconfid().yawrateconfid();
				}
				if (camData.virlist(i).intandreq().currentpos().has_posinmap())
				{
					if (camData.virlist(i).intandreq().currentpos().posinmap().has_referencelanes())
					{
						virReferenceLanes = camData.virlist(i).intandreq().currentpos().posinmap().referencelanes().referencelanes();
					}
					if (camData.virlist(i).intandreq().currentpos().posinmap().has_upstreamnodeid())
					{
						virUpstreamNodeRegionId = camData.virlist(i).intandreq().currentpos().posinmap().upstreamnodeid().region();
						virUpstreamNodeId = camData.virlist(i).intandreq().currentpos().posinmap().upstreamnodeid().nodeid();
					}
					if (camData.virlist(i).intandreq().currentpos().posinmap().has_downstreamnodeid())
					{
						virDownstreamNodeRegionId = camData.virlist(i).intandreq().currentpos().posinmap().downstreamnodeid().region();
						virDownstreamNodeId = camData.virlist(i).intandreq().currentpos().posinmap().downstreamnodeid().nodeid();
					}
				}
			}
		}

		if (i == 0)
		{
			sprintf(virSql, "INSERT INTO `vir_data`\
			(`id`,`split_id`,`msg_cnt`,`vehicle_id`,`timestamp`,\
			`lat`,`lon`,`ele`,`current_pos_lat`,`current_pos_lon`,\
			`current_pos_ele`,`upstream_node_region_id`,`upstream_node_id`,`downstream_node_region_id`,`downstream_node_id`,\
			`reference_lanes`,`speed`,`heading`,`acceleration_lon`,`acceleration_lat`,\
			`acceleration_vert`,`acceleration_yaw`,`estimated_time`,`current_behavior`,`pos_confid_id`,\
			`ele_confid_id`,`speed_confid_id`,`heading_confid_id`,`lon_accel_confid_id`,`lat_accel_confid_id`,\
			`vertical_accel_confid_id`,`yaw_rate_confid_id`,`time_confidence_id`,`cam_data_id`)VALUES");

			virSqls += virSql;
		}

		sprintf(virSql, "(%lld,%d,%d,'%s',%lld,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%d,%d,\
        %d,%d,%d,%lld),",
			virId, splitId, virMsgCnt, virVehicleId.c_str(), virTimestamp,
			virLat, virLon, virEle, currentPosLat, currentPosLon,
			currentPosEle, virUpstreamNodeRegionId, virUpstreamNodeId, virDownstreamNodeRegionId, virDownstreamNodeId,
			virReferenceLanes, virSpeed, virHeading, virAcclerationLon, virAcclerationLat,
			virAcclerationVert, virAcclerationYaw, virEstimatedTime, currentBehavior, virPosConfid,
			virEleConfid, virSpeedConfid, virHeadingConfid, virLonAccelConfid, virAcclerationLat,
			virVertAccelConfid, virYawAccelConfid, virTimeConfidence, camId);

		virSqls += virSql;

		/* pathPlanning  PathPlanning VirDataPathPlanningPoint 子集 一对多 共享的实时路径规划,按时间顺序列出 */
		// vir_data_path_planning_point -Table
		string virPathSqls = "";
		for (int j = 0; j < camData.virlist(i).intandreq().pathplanning().pathplanning_size(); j++)
		{
			char virPathSql[10240];
			long long virPathId = g_sf->nextId();
			int virPathSpeed = camData.virlist(i).intandreq().pathplanning().pathplanning(j).speed();
			int virPathHeading = camData.virlist(i).intandreq().pathplanning().pathplanning(j).heading();
			int virPathSpeedConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).speedconfid();
			int virPathHeadingConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).headingconfid();
			int virPathEstimatedTime = camData.virlist(i).intandreq().pathplanning().pathplanning(j).estimatedtime();
			int virPathTimeConfidence = camData.virlist(i).intandreq().pathplanning().pathplanning(j).timeconfidence();
			/* pos Position3D 标志标线位置*/
			int virPathPosLat = 0;
			int virPathPosLon = 0;
			int virPathPosEle = 0;
			/* posConfid PositionConfidenceSet 拆*/
			int virPathPosConfid = 0;
			int virPathEleConfid = 0;
			/* acceleration AccelerationSet4Way 拆 四轴加速度 */
			int virPathAcclerationLon = 0;
			int virPathAcclerationLat = 0;
			int virPathAcclerationVert = 0;
			int virPathAcclerationYaw = 0;
			/* accelerationConfid AccelerationConfidence 拆*/
			int virPathLonAccelConfid = 0;
			int virPathLatAccelConfid = 0;
			int virPathVertAccelConfid = 0;
			int virPathYawAccelConfid = 0;
			/* posInMap ReferenceLink 拆  相关的车道和链接位置 */
			int virPathReferenceLanes = 0;
			/* upstreamNodeId NodeReferenceId 拆*/
			int virPathUpstreamNodeRegionId = 0;
			int virPathUpstreamNodeId = 0;
			/* downstreamNodeId NodeReferenceId 拆*/
			int virPathDownstreamNodeRegionId = 0;
			int virPathDownstreamNodeId = 0;

			if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).has_pos())
			{
				virPathPosLat = camData.virlist(i).intandreq().pathplanning().pathplanning(j).pos().lat();
				virPathPosLon = camData.virlist(i).intandreq().pathplanning().pathplanning(j).pos().lon();
				virPathPosEle = camData.virlist(i).intandreq().pathplanning().pathplanning(j).pos().ele();
			}
			if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).has_posconfid())
			{
				virPathPosConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).posconfid().posconfid();
				virPathEleConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).posconfid().eleconfid();
			}
			if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).has_acceleration())
			{
				virPathAcclerationLon = camData.virlist(i).intandreq().pathplanning().pathplanning(j).acceleration().lon();
				virPathAcclerationLat = camData.virlist(i).intandreq().pathplanning().pathplanning(j).acceleration().lat();
				virPathAcclerationVert = camData.virlist(i).intandreq().pathplanning().pathplanning(j).acceleration().vert();
				virPathAcclerationYaw = camData.virlist(i).intandreq().pathplanning().pathplanning(j).acceleration().yaw();
			}
			if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).has_accelerationconfid())
			{
				virPathLonAccelConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).accelerationconfid().lonaccelconfid();
				virPathLatAccelConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).accelerationconfid().lataccelconfid();
				virPathVertAccelConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).accelerationconfid().verticalaccelconfid();
				virPathYawAccelConfid = camData.virlist(i).intandreq().pathplanning().pathplanning(j).accelerationconfid().yawrateconfid();
			}
			if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).has_posinmap())
			{
				if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().has_referencelanes())
				{
					virPathReferenceLanes = camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().referencelanes().referencelanes();
				}
				if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().has_upstreamnodeid())
				{
					virPathUpstreamNodeRegionId = camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().upstreamnodeid().region();
					virPathUpstreamNodeId = camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().upstreamnodeid().nodeid();
				}
				if (camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().has_downstreamnodeid())
				{
					virPathDownstreamNodeRegionId = camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().downstreamnodeid().region();
					virPathDownstreamNodeId = camData.virlist(i).intandreq().pathplanning().pathplanning(j).posinmap().downstreamnodeid().nodeid();
				}
			}

			if (j == 0)
			{
				sprintf(virPathSql, "INSERT INTO `vir_data_path_planning_point`\
				(`id`,`split_id`,`lat`,`lon`,`ele`,\
				`upstream_node_region_id`,`upstream_node_id`,`downstream_node_region_id`,`downstream_node_id`,`reference_lanes`,\
				`speed`,`heading`,`acceleration_lon`,`acceleration_lat`,`acceleration_vert`,\
				`acceleration_yaw`,`estimated_time`,`pos_confid_id`,`ele_confid_id`,`speed_confid_id`,\
				`heading_confid_id`,`lon_accel_confid_id`,`lat_accel_confid_id`,`vertical_accel_confid_id`,`yaw_rate_confid_id`,\
				`time_confidence_id`,`vir_data_id`)VALUES");
				virPathSqls += virPathSql;
			}

			sprintf(virPathSql, "(%lld,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,\
        		%d,%d,%d,%d,%d,%d,%lld),",
				virPathId, splitId, virPathPosLat, virPathPosLon, virPathPosEle,
				virPathUpstreamNodeRegionId, virPathUpstreamNodeId, virPathDownstreamNodeRegionId, virPathDownstreamNodeId, virPathReferenceLanes,
				virPathSpeed, virPathHeading, virPathAcclerationLon, virPathAcclerationLat, virPathAcclerationVert,
				virPathAcclerationYaw, virPathEstimatedTime, virPathPosConfid, virPathEleConfid, virPathSpeedConfid,
				virPathHeadingConfid, virPathLonAccelConfid, virPathLatAccelConfid, virPathVertAccelConfid, virPathYawAccelConfid,
				virPathTimeConfidence, virId);

			virPathSqls += virPathSql;
		}

		if (virPathSqls.length() > 0)
		{
			virPathSqls[virPathSqls.length() - 1] = ';';
		}

		if (virPathSqls.length() > 10)
		{
			ret = db->excuteSql(virPathSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert VirPath Data Fail,SQL:{}", virPathSqls);
			}
		}

		/* reqs DriveRequest 子集  一对多 请求序列*/
		// drive_request -Tbale
		string virReqsSqls = "";
		for (int j = 0; j < camData.virlist(i).intandreq().reqs_size(); j++)
		{
			char virReqsSql[10240];
			long long virReqsId = g_sf->nextId();
			int reqId = camData.virlist(i).intandreq().reqs(j).reqid();
			int reqStatus = camData.virlist(i).intandreq().reqs(j).status();
			string reqPriority = camData.virlist(i).intandreq().reqs(j).reqpriority();
			string targetVeh = camData.virlist(i).intandreq().reqs(j).targetveh();
			string targetRsu = camData.virlist(i).intandreq().reqs(j).targetrsu();
			int reqLifeTime = camData.virlist(i).intandreq().reqs(j).lifetime();
			/* info ReqInfo 拆 */
			/* laneChange ReqLaneChange 拆 车道变更请求*/
			/* upStreamNode NodeReferenceId 拆 */
			int laneChangeUpStreamNodeRegionId = 0;
			int laneChangeUpStreamNodeId = 0;
			/* downStreamNode NodeReferenceId 拆 */
			int laneChangeDownStreamNodeRegionId = 0;
			int laneChangeDownStreamNodeId = 0;
			/**目标路段id*/
			int laneChangeTargetLane = 0;
			/* clearTheWay ReqClearTheWay 拆 道路清空请求 */
			/* upStreamNode NodeReferenceId 拆 */
			int clearTheWayUpStreamNodeRegionId = 0;
			int clearTheWayUpStreamNodeId = 0;
			/* downStreamNode NodeReferenceId 拆 */
			int clearTheWayDownStreamNodeRegionId = 0;
			int clearTheWayDownStreamNodeId = 0;
			/**目标路段id*/
			int clearTheWayTargetLane = 0;
			/* signalPriority ReqSignalPriority 拆 信号优先请求*/
			/* intersectionId NodeReferenceId 拆 */
			int intersectionNodeRegionId = 0;
			int intersectionNodeId = 0;
			/* requiredMove MovementStatInfo  运动信息 拆*/
			/* remoteIntersection NodeReferenceId 拆 下游路口编号*/
			int movementStatInfoNodeRegionId = 0;
			int movementStatInfoNodeId = 0;
			/* turnDirection ManeuverDesc 枚举  转向信息 */
			int movementStatTurnDirection = 0;
			/**时间偏移*/
			int estimatedArrivalTime = 0;
			/**到达路口的距离,单位0.1m*/
			int distance2Intersection = 0;
			/* sensorSharing ReqSensorSharing 感知信息共享请求 展 */
			/* detectorArea DriveRequestReferencePath 子集 一对多 请求的感知区域的相关路径列表*/
			/* parking ReqParkingArea 拆 场站入场请求*/
			/* vehicleType  VehicleTypeDesc 枚举 车辆类型分类 */
			int reqParkingVehicleType = 0;
			/**ParkingRequest 来自车或交通站的停车区请求*/
			int reqParkingRequest = 0;
			/* parkingType 二进制 停车位类型 */
			int reqParkingType = 0;
			/**预期停车位id*/
			int expectedParkingSlotId = 0;
			if (camData.virlist(i).intandreq().reqs(j).has_info())
			{
				if (camData.virlist(i).intandreq().reqs(j).info().has_lanechange())
				{
					if (camData.virlist(i).intandreq().reqs(j).info().lanechange().has_upstreamnode())
					{
						laneChangeUpStreamNodeRegionId = camData.virlist(i).intandreq().reqs(j).info().lanechange().upstreamnode().region();
						laneChangeUpStreamNodeId = camData.virlist(i).intandreq().reqs(j).info().lanechange().upstreamnode().nodeid();
					}
					if (camData.virlist(i).intandreq().reqs(j).info().lanechange().has_downstreamnode())
					{
						laneChangeDownStreamNodeRegionId = camData.virlist(i).intandreq().reqs(j).info().lanechange().downstreamnode().region();
						laneChangeDownStreamNodeRegionId = camData.virlist(i).intandreq().reqs(j).info().lanechange().downstreamnode().nodeid();
					}
					laneChangeTargetLane = camData.virlist(i).intandreq().reqs(j).info().lanechange().targetlane();
				}
				if (camData.virlist(i).intandreq().reqs(j).info().has_cleartheway())
				{
					if (camData.virlist(i).intandreq().reqs(j).info().cleartheway().has_upstreamnode())
					{
						clearTheWayUpStreamNodeRegionId = camData.virlist(i).intandreq().reqs(j).info().cleartheway().upstreamnode().region();
						clearTheWayUpStreamNodeId = camData.virlist(i).intandreq().reqs(j).info().cleartheway().upstreamnode().nodeid();
					}
					if (camData.virlist(i).intandreq().reqs(j).info().cleartheway().has_downstreamnode())
					{
						clearTheWayDownStreamNodeRegionId = camData.virlist(i).intandreq().reqs(j).info().cleartheway().downstreamnode().region();
						clearTheWayDownStreamNodeId = camData.virlist(i).intandreq().reqs(j).info().cleartheway().downstreamnode().nodeid();
					}
					clearTheWayTargetLane = camData.virlist(i).intandreq().reqs(j).info().cleartheway().targetlane();
				}
				if (camData.virlist(i).intandreq().reqs(j).info().has_signalpriority())
				{
					estimatedArrivalTime = camData.virlist(i).intandreq().reqs(j).info().signalpriority().estimatedarrivaltime();
					distance2Intersection = camData.virlist(i).intandreq().reqs(j).info().signalpriority().distance2intersection();
					if (camData.virlist(i).intandreq().reqs(j).info().signalpriority().has_intersectionid())
					{
						intersectionNodeRegionId = camData.virlist(i).intandreq().reqs(j).info().signalpriority().intersectionid().region();
						intersectionNodeId = camData.virlist(i).intandreq().reqs(j).info().signalpriority().intersectionid().nodeid();
					}
					if (camData.virlist(i).intandreq().reqs(j).info().signalpriority().has_requiredmove())
					{
						if (camData.virlist(i).intandreq().reqs(j).info().signalpriority().requiredmove().has_remoteintersection())
						{
							movementStatInfoNodeRegionId = camData.virlist(i).intandreq().reqs(j).info().signalpriority().requiredmove().remoteintersection().region();
							movementStatInfoNodeId = camData.virlist(i).intandreq().reqs(j).info().signalpriority().requiredmove().remoteintersection().nodeid();
						}
						movementStatTurnDirection = camData.virlist(i).intandreq().reqs(j).info().signalpriority().requiredmove().turndirection();
					}
				}
				if (camData.virlist(i).intandreq().reqs(j).info().has_parking())
				{
					reqParkingVehicleType = camData.virlist(i).intandreq().reqs(j).info().parking().vehicletype();
					if (camData.virlist(i).intandreq().reqs(j).info().parking().has_req())
					{
						reqParkingRequest = camData.virlist(i).intandreq().reqs(j).info().parking().req().req();
					}
					if (camData.virlist(i).intandreq().reqs(j).info().parking().has_parkingtype())
					{
						reqParkingType = camData.virlist(i).intandreq().reqs(j).info().parking().parkingtype().parkingtype();
					}
					expectedParkingSlotId = camData.virlist(i).intandreq().reqs(j).info().parking().expectedparkingslotid();
				}
			}

			if (j == 0)
			{
				sprintf(virReqsSql, "INSERT INTO `drive_request`\
				(`id`,`split_id`,`req_id`,`req_priority`,`target_veh`,\
				`targer_rsu`,`lane_change_up_stream_node_region_id`,`lane_change_up_stream_node_id`,`lane_change_down_stream_node_region_id`,`lane_change_down_stream_node_id`,\
				`lane_change_target_lane`,`clear_the_way_up_stream_node_region_id`,`clear_the_way_up_stream_node_id`,`clear_the_way_down_stream_node_region_id`,`clear_the_way_down_stream_node_id`,\
				`clear_the_way_target_lane`,`intersection_node_region_id`,`intersection_node_id`,`movement_stat_info_node_region_id`,`movement_stat_info_node_id`,\
				`estimated_arrival_time`,`distance_2_intersection`,`req`,`parking_type`,`expected_parking_slot_id`,\
				`life_time`,`status_id`,`vehicle_type_id`,`turn_direction_id`,`vir_data_id`)VALUES");
				virReqsSqls += virReqsSql;
			}

			sprintf(virReqsSql, "(%lld,%d,%d,'%s','%s',\
            '%s',%d,%d,%d,%d,\
            %d,%d,%d,%d,%d,\
            %d,%d,%d,%d,%d,\
            %d,%d,%d,%d,%d,\
            %d,%d,%d,%d,%lld),",
				virReqsId, splitId, reqId, reqPriority.c_str(), targetVeh.c_str(),
				targetRsu.c_str(), laneChangeUpStreamNodeRegionId, laneChangeUpStreamNodeId, laneChangeDownStreamNodeRegionId, laneChangeDownStreamNodeId,
				laneChangeTargetLane, clearTheWayUpStreamNodeRegionId, clearTheWayUpStreamNodeId, clearTheWayDownStreamNodeRegionId, clearTheWayDownStreamNodeId,
				clearTheWayTargetLane, intersectionNodeRegionId, intersectionNodeId, movementStatInfoNodeRegionId, movementStatInfoNodeId,
				estimatedArrivalTime, distance2Intersection, reqParkingRequest, reqParkingType, expectedParkingSlotId,
				reqLifeTime, reqStatus, reqParkingVehicleType, movementStatTurnDirection, virId);

			virReqsSqls += virReqsSql;

			/* detectorArea DriveRequestReferencePath 子集 一对多 请求的感知区域的相关路径列表*/
			// drive_request_reference_path
			string driveReqRefSqls = "";
			for (int k = 0; k < camData.virlist(i).intandreq().reqs(j).info().sensorsharing().detectorarea_size(); k++)
			{
				char driveReqRefSql[10240];
				long long driveReqRefId = g_sf->nextId();
				int driveReqRefPathRadius = camData.virlist(i).intandreq().reqs(j).info().sensorsharing().detectorarea(k).pathradius();

				if (k == 0)
				{
					sprintf(driveReqRefSql, "INSERT INTO `drive_request_reference_path`\
					(`id`,`split_id`,`path_radius`,`drive_request_id`)VALUES");
					driveReqRefSqls += driveReqRefSql;
				}

				sprintf(driveReqRefSql, "(%lld,%d,%d,%lld),",
					driveReqRefId, splitId, driveReqRefPathRadius, virReqsId);

				driveReqRefSqls += driveReqRefSql;

				/*activePath DriveRequestReferencePathPosition3D 子集 一对多 */
				// drive_request_reference_path_position_3_d
				string driveReqRefPosSqls = "";
				for (int m = 0; i < camData.virlist(i).intandreq().reqs(j).info().sensorsharing().detectorarea_size(); i++)
				{
					char driveReqRefPosSql[10240];
					long long driveReqRefPosId = g_sf->nextId();
					int driveReqRefPosLon = 0;
					int driveReqRefPosLat = 0;
					int driveReqRefPosEle = 0;
					if (camData.virlist(i).intandreq().reqs(j).info().sensorsharing().detectorarea(k).activepath_size())
					{
						driveReqRefPosLon = camData.virlist(i).intandreq().reqs(j).info().sensorsharing().detectorarea(k).activepath(m).lon();
						driveReqRefPosLat = camData.virlist(i).intandreq().reqs(j).info().sensorsharing().detectorarea(k).activepath(m).lat();
						driveReqRefPosEle = camData.virlist(i).intandreq().reqs(j).info().sensorsharing().detectorarea(k).activepath(m).ele();
					}

					if (m == 0)
					{
						sprintf(driveReqRefPosSql, "INSERT INTO `drive_request_reference_path_position_3_d`\
						(`id`,`split_id`,`lon`,`lat`,`ele`,`drive_request_reference_path_id`)VALUES");
						driveReqRefPosSqls += driveReqRefPosSql;
					}

					sprintf(driveReqRefPosSql, "(%lld,%d,%d,%d,%d,%lld),",
						driveReqRefPosId, splitId, driveReqRefPosLon, driveReqRefPosLat, driveReqRefPosEle, driveReqRefId);

					driveReqRefPosSqls += driveReqRefPosSql;
				}

				if (driveReqRefPosSqls.length() > 0)
				{
					driveReqRefPosSqls[driveReqRefPosSqls.length() - 1] = ';';
				}

				if (driveReqRefPosSqls.length() > 10)
				{
					ret = db->excuteSql(driveReqRefPosSqls);
					if (ret < 0)
					{
						ERRORLOG("Insert DriveReqRefPos Data Fail,SQL:{}", driveReqRefPosSqls);
					}
				}
			}

			if (driveReqRefSqls.length() > 0)
			{
				driveReqRefSqls[driveReqRefSqls.length() - 1] = ';';
			}

			if (driveReqRefSqls.length() > 10)
			{
				ret = db->excuteSql(driveReqRefSqls);
				if (ret < 0)
				{
					ERRORLOG("Insert DriveReqRef Data Fail,SQL:{}", driveReqRefSqls);
				}
			}
		}

		if (virReqsSqls.length() > 0)
		{
			virReqsSqls[virReqsSqls.length() - 1] = ';';
		}

		if (virReqsSqls.length() > 10)
		{
			ret = db->excuteSql(virReqsSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert VirReqs Data Fail,SQL:{}", virReqsSqls);
			}
		}
	}

	if (virSqls.length() > 0)
	{
		virSqls[virSqls.length() - 1] = ';';
	}

	if (virSqls.length() > 10)
	{
		ret = db->excuteSql(virSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert Vir Data Fail,SQL:{}", virSqls);
		}
	}
}

void DataInsert::cameraInsert(MultiPathDatas cameraData, DbBase* db, SnowFlake* g_sf)
{
	int ret = 1;
	srand((unsigned)time(NULL));
	int splitId = rand() % 10000;
	char multiPathSql[4096];
	long long multiPathId = g_sf->nextId();
	int ms1 = cameraData.sendtime() % 1000 * 1000;
	auto timePtrSend = gettm(cameraData.sendtime());
	char sendTime[25] = { 0 };
	sprintf(sendTime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrSend->tm_year + 1900,
		timePtrSend->tm_mon + 1, timePtrSend->tm_mday, timePtrSend->tm_hour,
		timePtrSend->tm_min, timePtrSend->tm_sec, ms1);
	sprintf(multiPathSql, "INSERT INTO `multi_path_datas` (`id`,`split_id`,`send_time`)VALUES(%lld,%d,'%s');",
		multiPathId, splitId, sendTime);

	ret = db->excuteSql(multiPathSql);
	if (ret < 0)
	{
		ERRORLOG("Insert multiPath Data Fail,SQL:{}", multiPathSql);
	}

	// cameraPathList
	string cameraPathListSqls = "";
	for (int i = 0; i < cameraData.camerapathlist_size(); i++)
	{
		char cameraPathListSql[40960];
		long long cameraPathListId = g_sf->nextId();
		int ms2 = cameraData.camerapathlist(i).processtime() % 1000 * 1000;
		int ms3 = cameraData.camerapathlist(i).commrcvtime() % 1000 * 1000;
		int ms4 = cameraData.camerapathlist(i).datatime() % 1000 * 1000;
		auto timePtrPro = gettm(cameraData.camerapathlist(i).processtime());
		char processtime[25] = { 0 };
		sprintf(processtime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrPro->tm_year + 1900,
			timePtrPro->tm_mon + 1, timePtrPro->tm_mday, timePtrPro->tm_hour,
			timePtrPro->tm_min, timePtrPro->tm_sec, ms2);
		auto timePtrComm = gettm(cameraData.camerapathlist(i).commrcvtime());
		char commtime[25] = { 0 };
		sprintf(commtime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrComm->tm_year + 1900,
			timePtrComm->tm_mon + 1, timePtrComm->tm_mday, timePtrComm->tm_hour,
			timePtrComm->tm_min, timePtrComm->tm_sec, ms3);
		auto timePtrData = gettm(cameraData.camerapathlist(i).datatime());
		char datatime[25] = { 0 };
		sprintf(datatime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrData->tm_year + 1900,
			timePtrData->tm_mon + 1, timePtrData->tm_mday, timePtrData->tm_hour,
			timePtrData->tm_min, timePtrData->tm_sec, ms4);
		if (i == 0)
		{
			sprintf(cameraPathListSql, "INSERT INTO `camera_path_list`(`id`,`split_id`,`data_time`,`device_id`,`cap_type`,\
			`obj_count`,`process_time`,`comm_rcv_time`,`multi_path_datas_id`)VALUE");
			cameraPathListSqls += cameraPathListSql;
		}
		sprintf(cameraPathListSql, "(%lld,%d,%lld,'%s',%d,%d,'%s','%s',%lld),",
			cameraPathListId, splitId, cameraData.camerapathlist(i).datatime(), cameraData.camerapathlist(i).deviceid().c_str(), cameraData.camerapathlist(i).captype(),
			cameraData.camerapathlist(i).objcount(), processtime, commtime, multiPathId);

		cameraPathListSqls += cameraPathListSql;


		// cameraPath
		string cameraPathSqls = "";
		for (int j = 0; j < cameraData.camerapathlist(i).camerapathlist_size(); j++)
		{
			char cameraPathSql[40960];
			long long cameraPathId = g_sf->nextId();
			double longitude = 0;
			double latitude = 0;
			double x = 0;
			double y = 0;
			int objOri = 0;
			int objWidth = 0;
			int objLength = 0;
			int objHeight = 0;
			int speedX = 0;
			int speedY = 0;
			int speedZ = 0;
			int speed = 0;
			long deviceType = 200;
			string deviceId = cameraData.camerapathlist(i).deviceid(); // cameraData.camerapathlist(i).deviceid().c_str()
			// int plateNo = stoi(cameraData.camerapathlist(i).camerapathlist(j).plateno()); // cameraData.camerapathlist(i).camerapathlist(j).plateno().c_str()
			string vehColor = cameraData.camerapathlist(i).camerapathlist(j).vehcolor(); // cameraData.camerapathlist(i).camerapathlist(j).vehcolor().c_str()
			// allSql = "";
			if (cameraData.camerapathlist(i).camerapathlist(j).has_globalspaceinfo())
			{
				if (cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().has_position_gnss())
				{
					longitude = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().position_gnss().longitude();
					latitude = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().position_gnss().latitude();
				}
				if (cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().has_position_utm())
				{
					x = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().position_utm().x();
					y = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().position_utm().y();
				}
				objOri = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().objori();
				objWidth = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().objwidth();
				objLength = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().objlength();
				objHeight = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().objheight();
				speedX = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().speedx();
				speedY = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().speedy();
				speedZ = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().speedz();
				speed = cameraData.camerapathlist(i).camerapathlist(j).globalspaceinfo().speed();
			}
			if (j == 0)
			{
				sprintf(cameraPathSql, "INSERT INTO `camera_path`(`id`,`split_id`,`device_id`,`obj_id`,`lane_no`,\
				`plate_color`,`plate_no`,`veh_type`,`veh_color`,`obj_x`,`obj_y`,`obj_dist_x`,`obj_dist_y`,`obj_bottom_mid_x`,`obj_bottom_mid_y`,\
				`longitude`,`latitude`,`x`,`y`,`obj_ori`,`obj_width`,`obj_length`,`obj_height`,`speed_x`,`speed_y`,\
				`speed_z`,`speed`,`obj_cf`,`obj_kind`,`obj_img_top`,`obj_img_left`,`obj_img_right`,`obj_img_bottom`,`device_type_id`,`camera_path_list_id`,`process_time`,`comm_rcv_time`,`data_time`)VALUES");
				cameraPathSqls += cameraPathSql;
			}
			sprintf(cameraPathSql, "(%lld,%d,'%s',%d,%d,\
			%d,'%s',%d,'%s',%d,%d,%d,%d,%d,%d,\
			%lf,%lf,%lf,%lf,%d,%d,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,%d,%d,%d,%ld,%lld,'%s','%s','%s'),",
				cameraPathId, splitId, deviceId.c_str(), cameraData.camerapathlist(i).camerapathlist(j).objid(), cameraData.camerapathlist(i).camerapathlist(j).laneno(),
				cameraData.camerapathlist(i).camerapathlist(j).platecolor(), cameraData.camerapathlist(i).camerapathlist(j).plateno().c_str(), cameraData.camerapathlist(i).camerapathlist(j).vehtype(), vehColor.c_str(), cameraData.camerapathlist(i).camerapathlist(j).objx(),
				cameraData.camerapathlist(i).camerapathlist(j).objy(), cameraData.camerapathlist(i).camerapathlist(j).objdistx(), cameraData.camerapathlist(i).camerapathlist(j).objdisty(), cameraData.camerapathlist(i).camerapathlist(j).objbottommidx(), cameraData.camerapathlist(i).camerapathlist(j).objbottommidy(),
				longitude, latitude, x, y, objOri,
				objWidth, objLength, objHeight, speedX, speedY,
				speedZ, speed, cameraData.camerapathlist(i).camerapathlist(j).objcf(), cameraData.camerapathlist(i).camerapathlist(j).objkind(), cameraData.camerapathlist(i).camerapathlist(j).objimgtop(),
				cameraData.camerapathlist(i).camerapathlist(j).objimgleft(), cameraData.camerapathlist(i).camerapathlist(j).objimgright(), cameraData.camerapathlist(i).camerapathlist(j).objimgbottom(), deviceType, cameraPathListId, processtime, commtime, datatime);
			cameraPathSqls += cameraPathSql;
			// LLOG(INFO)<<" cameraPathSqls: "<<cameraPathSqls;
		}
		if (cameraPathSqls.length() > 0)
		{
			cameraPathSqls[cameraPathSqls.length() - 1] = ';';
		}

		if (cameraPathSqls.length() > 10)
		{
			ret = db->excuteSql(cameraPathSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert cameraPath Data Fail,SQL:{}", cameraPathSqls);
			}
		}
	}
	if (cameraPathListSqls.length() > 0)
	{
		cameraPathListSqls[cameraPathListSqls.length() - 1] = ';';
	}

	if (cameraPathListSqls.length() > 10)
	{
		ret = db->excuteSql(cameraPathListSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert cameraPathList Data Fail,SQL:{}", cameraPathListSqls);
		}
	}
}

void DataInsert::radarInsert(MultiPathDatas radarData, DbBase* db, SnowFlake* g_sf)
{
	int ret = 1;
	srand((unsigned)time(NULL));
	int splitId = rand() % 10000;
	char multiPathSql[4096];
	long long multiPathId = g_sf->nextId();
	int ms1 = radarData.sendtime() % 1000 * 1000;
	auto timePtrSend = gettm(radarData.sendtime());
	char sendTime[25] = { 0 };
	sprintf(sendTime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrSend->tm_year + 1900,
		timePtrSend->tm_mon + 1, timePtrSend->tm_mday, timePtrSend->tm_hour,
		timePtrSend->tm_min, timePtrSend->tm_sec, ms1);
	sprintf(multiPathSql, "INSERT INTO `multi_path_datas` (`id`,`split_id`,`send_time`)VALUES(%lld,%d,'%s');",
		multiPathId, splitId, sendTime);

	ret = db->excuteSql(multiPathSql);
	if (ret < 0)
	{
		ERRORLOG("Insert multiPath Data Fail,SQL:{}", multiPathSql);
	}

	// RadarDevPathList
	string radarDevPathListSqls = "";
	for (int i = 0; i < radarData.lidarpathlist_size(); i++)
	{
		char radarDevPathListSql[40960];
		long long radarDevPathListId = g_sf->nextId();

		int ms2 = radarData.lidarpathlist(i).datatime() % 1000 * 1000;
		int ms3 = radarData.lidarpathlist(i).processtime() % 1000 * 1000;
		int ms4 = radarData.lidarpathlist(i).commrcvtime() % 1000 * 1000;
		auto timePtrData = gettm(radarData.lidarpathlist(i).datatime());
		char datatime[25] = { 0 };
		sprintf(datatime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrData->tm_year + 1900,
			timePtrData->tm_mon + 1, timePtrData->tm_mday, timePtrData->tm_hour,
			timePtrData->tm_min, timePtrData->tm_sec, ms2);

		auto timePtrPro = gettm(radarData.lidarpathlist(i).processtime());
		char processtime[25] = { 0 };
		sprintf(processtime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrPro->tm_year + 1900,
			timePtrPro->tm_mon + 1, timePtrPro->tm_mday, timePtrPro->tm_hour,
			timePtrPro->tm_min, timePtrPro->tm_sec, ms3);

		auto timePtrComm = gettm(radarData.lidarpathlist(i).commrcvtime());
		char commtime[25] = { 0 };
		sprintf(commtime, "%d-%02d-%02d %02d:%02d:%02d.%06d", timePtrComm->tm_year + 1900,
			timePtrComm->tm_mon + 1, timePtrComm->tm_mday, timePtrComm->tm_hour,
			timePtrComm->tm_min, timePtrComm->tm_sec, ms4);

		if (i == 0)
		{
			sprintf(radarDevPathListSql, "INSERT INTO `millimeter_wave_radar_path_list`(`id`,`split_id`,`data_time`,`device_id`,`area_no`,\
			`process_time`,`comm_rcv_time`,`multi_path_datas_id`)VALUES");
			radarDevPathListSqls += radarDevPathListSql;
		}

		sprintf(radarDevPathListSql, "(%lld,%d,'%s','%s',%d,'%s','%s',%lld),",
			radarDevPathListId, splitId, datatime, radarData.lidarpathlist(i).deviceid().c_str(), radarData.lidarpathlist(i).areano(),
			processtime, commtime, multiPathId);

		radarDevPathListSqls += radarDevPathListSql;

		// RadarPath
		string radarPathSqls = "";
		for (int j = 0; j < radarData.lidarpathlist(i).radarpathlist_size(); j++)
		{
			long long radarpathId = g_sf->nextId();
			char radarPathSql[10960];
			int deviceType = 203;

			if (j == 0)
			{
				sprintf(radarPathSql, "INSERT INTO `millimeter_wave_radar_path`(`id`,`split_id`,`device_id`,`obj_id`,`longitude`,\
				`lattitude`,`elevation`,`obj_x`,`obj_y`,`obj_z`,\
				`obj_enu_x`,`obj_enu_y`,`obj_enu_z`,`speed`,`speed_x`,\
				`speed_y`,`speed_z`,`speed_enu_x`,`speed_enu_y`,`speed_enu_z`,\
				`obj_ori`,`speed_heading`,`obj_width`,`obj_length`,`obj_height`,\
				`aclr`,`aclr_angle`,`aclr_x`,`aclr_y`,`aclr_z`,\
				`aclr_enu_x`,`aclr_enu_y`,`aclr_enu_z`,`obj_state`,`obj_cf`,\
				`obj_type`,`obj_kind`,`obj_dist`,`device_type_id`,`millimeter_wave_radar_path_list_id`,`process_time`,`comm_rcv_time`,`data_time`)VALUES");
				radarPathSqls += radarPathSql;
			}

			sprintf(radarPathSql, "(%lld,%d,'%s',%d,%lf,\
			%lf,%d,%f,%f,%f,\
			%f,%f,%f,%d,%d,\
			%d,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,\
			%d,%d,%d,%d,%d,\
			%d,%d,%f,%d,%lld,'%s','%s','%s'),",
				radarpathId, splitId, radarData.lidarpathlist(i).deviceid().c_str(), radarData.lidarpathlist(i).radarpathlist(j).objid(), radarData.lidarpathlist(i).radarpathlist(j).longitude(),
				radarData.lidarpathlist(i).radarpathlist(j).lattitude(), radarData.lidarpathlist(i).radarpathlist(j).elevation(), radarData.lidarpathlist(i).radarpathlist(j).objx(), radarData.lidarpathlist(i).radarpathlist(j).objy(), radarData.lidarpathlist(i).radarpathlist(j).objz(),
				radarData.lidarpathlist(i).radarpathlist(j).obj_enu_x(), radarData.lidarpathlist(i).radarpathlist(j).obj_enu_y(), radarData.lidarpathlist(i).radarpathlist(j).obj_enu_z(), radarData.lidarpathlist(i).radarpathlist(j).speed(), radarData.lidarpathlist(i).radarpathlist(j).speedx(),
				radarData.lidarpathlist(i).radarpathlist(j).speedy(), radarData.lidarpathlist(i).radarpathlist(j).speedz(), radarData.lidarpathlist(i).radarpathlist(j).speed_enu_x(), radarData.lidarpathlist(i).radarpathlist(j).speed_enu_y(), radarData.lidarpathlist(i).radarpathlist(j).speed_enu_z(),
				radarData.lidarpathlist(i).radarpathlist(j).objori(), radarData.lidarpathlist(i).radarpathlist(j).speedheading(), radarData.lidarpathlist(i).radarpathlist(j).objwidth(), radarData.lidarpathlist(i).radarpathlist(j).objlength(), radarData.lidarpathlist(i).radarpathlist(j).objheight(),
				radarData.lidarpathlist(i).radarpathlist(j).aclr(), radarData.lidarpathlist(i).radarpathlist(j).aclrangle(), radarData.lidarpathlist(i).radarpathlist(j).aclr_x(), radarData.lidarpathlist(i).radarpathlist(j).aclr_y(), radarData.lidarpathlist(i).radarpathlist(j).aclr_z(),
				radarData.lidarpathlist(i).radarpathlist(j).aclr_enu_x(), radarData.lidarpathlist(i).radarpathlist(j).aclr_enu_y(), radarData.lidarpathlist(i).radarpathlist(j).aclr_enu_z(), radarData.lidarpathlist(i).radarpathlist(j).objstate(), radarData.lidarpathlist(i).radarpathlist(j).objcf(),
				radarData.lidarpathlist(i).radarpathlist(j).objtype(), radarData.lidarpathlist(i).radarpathlist(j).objkind(), radarData.lidarpathlist(i).radarpathlist(j).objdist(), deviceType, radarDevPathListId, processtime, commtime, datatime);

			radarPathSqls += radarPathSql;
		}
		if (radarPathSqls.length() > 0)
		{
			radarPathSqls[radarPathSqls.length() - 1] = ';';
		}

		if (radarPathSqls.length() > 10)
		{
			ret = db->excuteSql(radarPathSqls);
			if (ret < 0)
			{
				ERRORLOG("Insert radarPath Data Fail,SQL:{}", radarPathSqls);
			}
		}
	}
	if (radarDevPathListSqls.length() > 0)
	{
		radarDevPathListSqls[radarDevPathListSqls.length() - 1] = ';';
	}

	if (radarDevPathListSqls.length() > 10)
	{
		ret = db->excuteSql(radarDevPathListSqls);
		if (ret < 0)
		{
			ERRORLOG("Insert radarDevPathList Data Fail,SQL:{}", radarDevPathListSqls);
		}
	}
}

map<string,string> *DataMap::deviceCodeMap(std::map<string, string>& devMap, DbBase* db)
{
	try
	{
		soci::rowset<soci::row> rows = db->select("select device_id,short_device_id from device_id_map");
		for (const soci::row& row : rows)
		{
			string devId = "";
			string sDevId = "";
			if (row.get_indicator(0) != soci::i_null)
			{
				devId = row.get<string>(0);
			}
			if (row.get_indicator(1) != soci::i_null)
			{
				sDevId = row.get<string>(1);
			}
			devMap[devId] = sDevId;
		}
	}
	catch (const exception& e)
	{
		ERRORLOG("Device Map Build Failed:{}", e.what());
	}
	return &devMap;

}

map<string, string>* DataMap::nodeMatchMap(std::map<string, string>& nodeMap, DbBase* db)
{
	try
	{
		soci::rowset<soci::row> rows = db->select("select short_device_id,match_node_id from device_id_map");
		for (const soci::row& row : rows)
		{	
			string sDevId = "";
			string nodeId = "";
			if (row.get_indicator(0) != soci::i_null)
			{
				sDevId = row.get<string>(0);
			}
			if (row.get_indicator(1) != soci::i_null)
			{
				nodeId = row.get<string>(1);
			}
			nodeMap[sDevId] = nodeId;
		}
	}
	catch (const exception& e)
	{
		ERRORLOG("Node Match Map Build Failed:{}", e.what());
	}
	return &nodeMap;
}