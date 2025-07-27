#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

// ���� JSON �ļ������� map
std::map<std::string, std::vector<std::string>> parseJsonToMap(const std::string& filename) {
    // �� JSON �ļ�
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    // ʹ�� RapidJSON ���� JSON �ļ�
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    // ��� JSON �Ƿ���Ч
    if (doc.HasParseError()) {
        throw std::runtime_error("Error parsing JSON file.");
    }

    // ��ʼ�� map
    std::map<std::string, std::vector<std::string>> groupMap;

    // ���� "groups" ����
    if (doc.HasMember("groups") && doc["groups"].IsArray()) {
        const rapidjson::Value& groups = doc["groups"];
        for (const auto& group : groups.GetArray()) {
            // ��ȡ group_name
            if (group.HasMember("group_name") && group["group_name"].IsString()) {
                std::string groupName = group["group_name"].GetString();

                // ��ȡ group_member ��ת��Ϊ vector<string>
                std::vector<std::string> memberList;
                if (group.HasMember("group_member") && group["group_member"].IsArray()) {
                    for (const auto& member : group["group_member"].GetArray()) {
                        if (member.IsString()) {
                            memberList.push_back(member.GetString());
                        }
                    }
                }

                // �� group_name �� group_member ���� map
                groupMap[groupName] = memberList;
            }
        }
    }

    return groupMap;
}


// �������壺�� map ����ȡĳЩ����Ӧ��ֵ����׷�ӵ�Ŀ�� vector ��
void getGroupVec(
    const std::map<std::string, std::vector<std::string>>& inputMap, // ����� map
    const std::vector<std::string>& keys,                           // Ҫ��ȡ�ļ��б�
    std::vector<std::string>& outputVector                          // �����Ŀ�� vector
) {
    for (const auto& key : keys) { // ����������Ҫ�ļ�
        auto it = inputMap.find(key);
        if (it != inputMap.end()) { // ���������
            outputVector.insert(outputVector.end(), it->second.begin(), it->second.end());
        }
    }
}

