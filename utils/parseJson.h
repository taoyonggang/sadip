#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

// 解析 JSON 文件并生成 map
std::map<std::string, std::vector<std::string>> parseJsonToMap(const std::string& filename) {
    // 打开 JSON 文件
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    // 使用 RapidJSON 解析 JSON 文件
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    // 检查 JSON 是否有效
    if (doc.HasParseError()) {
        throw std::runtime_error("Error parsing JSON file.");
    }

    // 初始化 map
    std::map<std::string, std::vector<std::string>> groupMap;

    // 遍历 "groups" 数组
    if (doc.HasMember("groups") && doc["groups"].IsArray()) {
        const rapidjson::Value& groups = doc["groups"];
        for (const auto& group : groups.GetArray()) {
            // 获取 group_name
            if (group.HasMember("group_name") && group["group_name"].IsString()) {
                std::string groupName = group["group_name"].GetString();

                // 获取 group_member 并转换为 vector<string>
                std::vector<std::string> memberList;
                if (group.HasMember("group_member") && group["group_member"].IsArray()) {
                    for (const auto& member : group["group_member"].GetArray()) {
                        if (member.IsString()) {
                            memberList.push_back(member.GetString());
                        }
                    }
                }

                // 将 group_name 和 group_member 存入 map
                groupMap[groupName] = memberList;
            }
        }
    }

    return groupMap;
}


// 函数定义：从 map 中提取某些键对应的值，并追加到目标 vector 中
void getGroupVec(
    const std::map<std::string, std::vector<std::string>>& inputMap, // 输入的 map
    const std::vector<std::string>& keys,                           // 要提取的键列表
    std::vector<std::string>& outputVector                          // 输出的目标 vector
) {
    for (const auto& key : keys) { // 遍历所有需要的键
        auto it = inputMap.find(key);
        if (it != inputMap.end()) { // 如果键存在
            outputVector.insert(outputVector.end(), it->second.begin(), it->second.end());
        }
    }
}

