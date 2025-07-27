#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace utility
{
    namespace string {
        void inline split(const std::string& s,
            std::vector<std::string>& sv,
            const char delim = ' ') {
            sv.clear();
            std::istringstream iss(s);
            std::string temp;

            while (std::getline(iss, temp, delim)) {
                sv.emplace_back(std::move(temp));
            }

            return;
        }

        /*
         函数说明：对字符串中所有指定的子串进行替换
         参数：
        string resource_str            //源字符串
        string sub_str                //被替换子串
        string new_str                //替换子串
        返回值: string
         */
        std::string inline subreplace(std::string resource_str, std::string sub_str, std::string new_str)
        {
            std::string dst_str = resource_str;
            std::string::size_type pos = 0;
            while ((pos = dst_str.find(sub_str)) != std::string::npos)   //替换所有指定子串
            {
                dst_str.replace(pos, sub_str.length(), new_str);
            }
            return dst_str;
        }

        // 函数定义：查找并替换字符串中的第一个匹配的子字符串
        /*
         参数：
        string original            //源字符串
        string target                //被替换子串
        string replacement                //替换子串
        返回值: string
        */
        std::string inline replaceFirstString(const std::string& original, const std::string& target, const std::string& replacement) {
            std::string result = original;
            size_t pos = result.find(target);

            // 如果找到了匹配的子字符串，则进行替换一次后，退出
            if (pos != std::string::npos) {
                result.replace(pos, target.length(), replacement);
            }

            return result;
        }


        std::string inline cutlaststr(std::string resource_str, std::string cut_str)
        {
            std::string dst_str = resource_str;
            size_t pos = dst_str.find_last_of(cut_str);
            if (pos != std::string::npos) {
                dst_str = dst_str.substr(0, pos);  // 提取子字符串
            }
            else {
                std::cout << "Invalid input string." << std::endl;;
            }
            return dst_str;
        }

        //获取第一个分割字串左边串
        std::string inline cutFisrtLeftStr(std::string resource_str, std::string cut_str)
        {
            std::string dst_str = resource_str;
            size_t pos = dst_str.find_first_of(cut_str);
            if (pos != std::string::npos) {
                dst_str = dst_str.substr(0, pos);  // 提取子字符串
            }
            else {
                std::cout << "Invalid input string." << std::endl;;
            }
            return dst_str;
        }
        //获取最后一个分割字串右边串
        std::string inline cutLastRightStr(std::string resource_str, std::string cut_str)
        {
            std::string dst_str = resource_str;
            size_t pos = dst_str.find_last_of(cut_str);
            if (pos != std::string::npos) {
                dst_str = dst_str.substr(pos, resource_str.size());  // 提取子字符串
            }
            else {
                std::cout << "Invalid input string." << std::endl;;
            }
            return dst_str;
        }
        //获取最后一个分割字串左边串
        std::string inline cutLastLeftStr(std::string resource_str, std::string cut_str)
        {
            std::string dst_str = resource_str;
            size_t pos = dst_str.find_last_of(cut_str);
            if (pos != std::string::npos) {
                dst_str = dst_str.substr(0,pos);  // 提取子字符串
            }
            else {
                std::cout << "Invalid input string." << std::endl;;
            }
            return dst_str;
        }
        //获取第一个分割字串右边串
        std::string inline cutFisrtRightstr(std::string resource_str, std::string cut_str)
        {
            std::string dst_str = resource_str;
            size_t pos = dst_str.find_first_of(cut_str);
            if (pos != std::string::npos) {
                dst_str = dst_str.substr(pos, resource_str.size());  // 提取子字符串
            }
            else {
                std::cout << "Invalid input string." << std::endl;;
            }
            return dst_str;
        }

        std::string inline trim(std::string& s)
        {
            if (s.empty())
            {
                return s;
            }
            s.erase(0, s.find_first_not_of(" "));
            s.erase(s.find_last_not_of(" ") + 1);
            return s;
        }
        
        std::string inline formatPath(std::string& path) {
            if (path[0] == '/') {
                return "." + path;
            }
            else {
                return path;
            }
        }

    }
}

//int main() {
//    std::string s("abc:def:ghi");
//    std::vector<std::string> sv;
//
//    split(s, sv, ':');
//
//    for (const auto& s : sv) {
//        std::cout << s << std::endl;
//    }
//
//    return 0;
//}