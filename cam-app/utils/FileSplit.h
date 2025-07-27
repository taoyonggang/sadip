#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING 1
#include <iostream>
#include <fstream>
//#include <filesystem>
#include <string>
#include <cstring>
#include <cstdio>
#include <experimental/filesystem>

#include "../utils/log/BaseLog.h"
#include "../../utils/split.h"


//using namespace std::experimental::filesystem;

//using namespace std;
namespace fs = std::experimental::filesystem;

typedef unsigned long long LL;


namespace utility
{
    namespace file {

        //static unsigned int SPLIT_SIZE = 60;   //不要超过64k,容易失败

        struct FileInfo {
            std::string filePathName;
            bool isDir;
            long long fileSize;
        };


        static long long GetFileLength(const std::string& filepath)
        {
            long long len = -1;
            std::ifstream in;
            in.open(filepath, std::ios_base::in);
            //in.open(filepath, std::ios::binary);
            if (in.is_open())
            {
                in.seekg(0, std::ios::end);
                len = in.tellg();
                in.close();
            }
            else
            {
                ERRORLOG("{} open failed!", filepath);
            }

            return len;
        }

        static long long GetLastUpdatetime(const std::string& filepath)
        {
            auto now = std::chrono::system_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

            long long updateTime = milliseconds;
            auto ftime = fs::last_write_time(filepath);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            updateTime = std::chrono::system_clock::to_time_t(sctp);

            return updateTime;
        }

        static void mkdir(std::string path)
        {
            //string s = "mkdir -p " + path.substr(2);
             //system(s.c_str());
            //ACE_OS::mkdir(path.c_str());
            fs::create_directories(path);

        }

        static void cls()
        {
            system("cls");
        }

        static bool IsDir(const char* path)
        {
            return fs::is_directory(fs::path(path));
        }

        //blockSize -> KB
        static void split(const std::string& filepath, long long  blockSize, const std::string& tmppath)
        {
            long long  ori_size = GetFileLength(filepath);
            blockSize *= 1024; //KB -> B
            if (ori_size == -1)
            {
                ERRORLOG("get file length failed.");
                return;
            }
            std::ifstream in;
            in.open(filepath, std::ios_base::in | std::ios_base::binary);
            if (!in.is_open())
            {
                ERRORLOG("ifstream open failed");
                return;
            }

            char buff[1024];
            memset(buff, 0, 1024);
            long long read_count = 0;
            long long cur_count = 0;
            int i;
            for (i = 0; read_count < ori_size; ++i)
            {
                long long thisTimesRead = 0;
                std::string divfile = tmppath + std::to_string(i);
                INFOLOG("spliting file: {} ..", divfile);
                std::ofstream out;
                out.open(divfile, std::ios_base::out | std::ios_base::binary);
                if (!out.is_open())
                {
                    ERRORLOG("open {} failed.", divfile);
                    std::string error = std::strerror(errno);
                    // 可以根据需要处理文件打开失败的情况，例如输出错误信息
                    ERRORLOG("Failed to open file: {}", error);
                    break;
                }
                while (thisTimesRead < blockSize && read_count < ori_size)
                {
                    memset(buff, 0, 1024);
                    in.read(buff, 1024);
                    cur_count = in.gcount();
                    read_count += cur_count;
                    thisTimesRead += cur_count;
                    out.write(buff, cur_count);
                }
                out.close();
            }

            in.close();
            INFOLOG("Split {} files, original file size: {} Bytes.", i, ori_size);
        }

        static int merge(std::string dir,int blockNum)
        {
            if (!IsDir(dir.c_str()))
            {
                ERRORLOG("error path: {}", dir);
                return -1;
            }
            std::string outFile = dir + "/merge.out";
            std::ofstream out;
            out.open(outFile, std::ios_base::binary | std::ios_base::out);
            if (!out.is_open())
            {
                ERRORLOG("can not create merge.out in {}", dir);
                return -1;
            }
            dir += "/";
            char buffer[1024];
            LL read_size = 0;
            int i = 0;
            while (1)
            {
                if (i == blockNum) {
                    out.close();
                    break;
                }
                LL curlen = GetFileLength(dir + std::to_string(i));
                LL thisTimesRead = 0;
                std::ifstream in;
                in.open(dir + std::to_string(i), std::ios_base::binary | std::ios_base::in);
                 
                if (!in.is_open()) {
                     out.close();
                     return i;
                }

                INFOLOG("writing {} file..", i);
                while (thisTimesRead < curlen)
                {
                    memset(buffer, 0, 1024);
                    in.read(buffer, 1024);
                    size_t cur_count = in.gcount();
                    thisTimesRead += cur_count;
                    read_size += cur_count;
                    out.write(buffer, cur_count);
                }
                in.close();
                ++i;
            }
            out.close();
            if (read_size == 0) {
                ERRORLOG("Error: Can not find files to merge.");
                return -1;
            }
            else
                INFOLOG("merged {} files, totel {} Bytes.", i, read_size);

            return 0;
        }

        static void write_file(const  std::string& filename,
            const void* buff, unsigned long size)
        {
            std::ofstream output_stream(filename, std::ios::out | std::ios::trunc | std::ios::binary);

            // Check that the stream opened correctly
            if (output_stream.fail())
            {
                ERRORLOG("ERROR - Unable to open file {}", filename);
            }
            else
            {
                // write the diff to the file
                const char* buffer = (const char*)buff;
                output_stream.write(buffer, size);

                // check that the data was written to the file.
                if (output_stream.bad())
                {
                    ERRORLOG("ERROR - Unable to write ouput to the file {}", filename);
                }
                else
                {
                    // write was successful so set the version and increment write count
                    INFOLOG("File: write the file success. fileName: {}", filename);
                }

                // close the file.
                output_stream.close();
            }
        }

        static bool move(const std::string& src, const std::string& dst)
        {
            std::ifstream ifs(src, std::ios::binary);
            std::ofstream ofs(dst, std::ios::binary);
            if (!ifs.is_open()) {
                ERRORLOG("open src file fail: {}", src);
                return false;
            }
            ofs << ifs.rdbuf();
            ifs.close();
            ofs.close();
            if (0 != remove(src.c_str())) {
                ERRORLOG("remove src file fail: {}", src);
            }
            return true;
        }

        static void remove_all_files(const fs::path& dir) {
            if (fs::exists(dir) && fs::is_directory(dir)) {
                for (const auto& entry : fs::directory_iterator(dir)) {
                    if (fs::is_regular_file(entry.status())) {
                        fs::remove(entry.path());
                    }
                }
                fs::remove_all(dir);
            }
        }

        static bool del(const std::string& dir) {
            if (!fs::is_directory(dir)) {
                fs::remove(dir);
                return true;
            }
            return false;
        }

        static std::vector<FileInfo> ls_all_files(const fs::path& dir) {
            std::vector<FileInfo> fileInfos;
            if (fs::exists(dir) && fs::is_directory(dir)) {
                for (const auto& entry : fs::directory_iterator(dir)) {
                    FileInfo fileInfo;
                    fileInfo.filePathName = entry.path().string();
                    if (fs::is_regular_file(entry.status())) {
                        fileInfo.isDir = false;
                    }
                    else {
                        fileInfo.isDir = true;
                    }
                    fileInfos.push_back(fileInfo);
                }
            }
            return fileInfos;
        }

        static std::vector<FileInfo> getFilePathNames(std::string path) {
            std::vector<FileInfo> pathV;
            for (const auto& entry : fs::directory_iterator(path)) {
                FileInfo fi;
                fi.filePathName = entry.path().string();
                fi.isDir = IsDir(entry.path().string().c_str());
                fi.fileSize = -1;
                if (!fi.isDir) {

                    fi.fileSize = GetFileLength(fi.filePathName.c_str());
                }
                pathV.push_back(fi);
                //
            }
            return pathV;
        }


        //static void read(string filename,)

        //int main(int argc, char** argv)
        //{
        //    //cout << GetFileLength("1.jpg") << endl;
        //    //split("2.jpg", 128);
        //    cls();
        //    string curDir(argv[0]);
        //    curDir = curDir.substr(0, curDir.find_last_of('\\'));
        //    //merge(curDir);

        //    int opt;
        //    cout << "What do you want to do:" << endl
        //        << "1. Split file." << endl
        //        << "2. Merge file." << endl
        //        << "3. Exit." << endl
        //        << "Your option: " << endl;
        //    cin >> opt;
        //    string path;
        //    int blockSize = 0;
        //    cls();
        //    switch (opt)
        //    {
        //    case 1:
        //        cout << "Input the path of file you want to split and the size of each part (KB):" << endl;
        //        cin >> path >> blockSize;
        //        split(path, blockSize);
        //        break;
        //    case 2:
        //        cout << "Merging in current directory." << endl;
        //        merge(curDir);
        //        break;
        //    default:

        //        break;
        //    }
        //    cout << "Exit." << endl;
        //    return 0;
        //}
    }
}
