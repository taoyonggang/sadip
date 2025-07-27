#ifndef _FILE_WATCHER_H_
#define _FILE_WATCHER_H_
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/FileStream.h>
#include <Poco/Zip/Decompress.h>
#include <Poco/Zip/Compress.h>
#include <Poco/Timestamp.h>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "../../utils/aws/s3/S3Singleton.h"
#include "../utils/log/BaseLog.h"
#include "../utils/split.h"


/**
 * @brief A class for watching files in a directory and detecting changes.
 *
 */

using namespace Poco;
using namespace Poco::Zip;

class FileWatcher {
public:
	FileWatcher(const std::string& directory, const std::string& bucket, const std::string& nodeid, int sleep, bool keepFile,bool needZip,bool watchSubDir)
		: m_directory(directory), m_stop(true), m_bucket(bucket), m_nodeid(nodeid), m_keepFile(keepFile), m_needZip(needZip), m_watchSubDir(watchSubDir){
		scanDirectory(m_files1);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		scanDirectory(m_files2);
	}

	~FileWatcher() {
		stop();
	}

	void start() {
		INFOLOG("File: s3 watcher:{} start", m_directory);
		m_stop = false;
		if (m_needZip) {
			Poco::File zipDir(m_directory+"/zip/");
			zipDir.createDirectories();
		}
		m_thread = std::thread(std::bind(&FileWatcher::watch, this));
	}

	void stop() {
		INFOLOG("File: s3 watcher:{} stop", m_directory);
		//if (m_thread.joinable()) 
		{
			m_stop = true;
			//m_thread.join();
		}
	}

    void scanDirectoryRecursive(std::map<std::string, Poco::Timestamp>& files, const std::string& directory) {
        try {
            INFOLOG("File: s3 watcher scanDirectoryRecursive:{} files", directory);
            Poco::File dir(directory);
            if (dir.exists() && dir.isDirectory() && !isZipDirectory(directory)) {
				INFOLOG("s3 found watcher dir:{}", directory);
				//if (!isZipDirectory(directory)) //没找到zip子目录，则监控
				{
					Poco::DirectoryIterator end;
					for (Poco::DirectoryIterator it(dir); it != end; ++it) {
						if (it->isFile()) {
							files[it->path()] = it->getLastModified();
						}
						else if (it->isDirectory() && m_watchSubDir) {							
							if (!isZipDirectory(directory)) {
								INFOLOG("s3 found sub dir:{},need add", it->path());
								scanDirectoryRecursive(files, it->path());
							}
							
						}
					}
				}
            }
        } catch (const exception& e) {
            ERRORLOG("dir not exist or not directory,error:{}",e.what());
        }
    }

	bool isZipDirectory(const std::string& path) {
		return path.find("/zip") != std::string::npos;
	}

    void scanDirectory(std::map<std::string, Poco::Timestamp>& files) {
        scanDirectoryRecursive(files, m_directory);
    }



	//std::map<std::string, Poco::Timestamp> getChangedFiles() {
	//	INFOLOG("File: s3 watcher:{} get change files", m_directory);
	//	std::map<std::string, Poco::Timestamp> changedFiles;
	//	Poco::File dir(m_directory);
	//	Poco::DirectoryIterator end;
	//	for (Poco::DirectoryIterator it(dir); it != end; ++it) {
	//		auto found = m_files1.find(it->path());
	//		if (found == m_files1.end() || found->second != it->getSize()) {
	//			// 新增的文件或文件大小改变
	//			changedFiles[it->path()] = it->getSize();
	//		}
	//		m_files1.erase(it->path());
	//	}
	//	// 删除的文件
	//	for (const auto& file : m_files1) {
	//		changedFiles[file.first] = 0;
	//	}
	//	m_files1.swap(changedFiles);
	//	return m_files1;
	//}


	void uploadToS3(const std::pair<const std::string, Poco::Timestamp>& file)
	{
		    if (m_needZip)
            //if (false)
            {
				string zipDir = utility::string::cutLastLeftStr(file.first,"/") + "/zip";
				if (!isDirectory(zipDir)) {
					createDirectory(zipDir);
			}				
			Poco::Path path(file.first);
			string zipFile = zipDir + "/" + path.getFileName() + ".zip";
			if (this->zipFile(file.first, zipFile, m_keepFile)) {
				string zipS3Key = m_nodeid  + zipFile;
				zipS3Key = utility::string::subreplace(zipS3Key, "//", "/");
				zipS3Key = utility::string::subreplace(zipS3Key, "/./", "/");
				INFOLOG("File: begin uplaod file {} to s3:{} success", zipS3Key, zipFile);
				if (m_s3->s3_helper_->uploadfile(m_bucket, zipS3Key, zipFile)){
					INFOLOG("File: {} zip and upload to s3:{} success", zipFile, zipS3Key);
				}
				else {
					ERRORLOG("File: {} zip and upload to s3:{} failed", zipFile, zipS3Key);
				}
			}
			else {
				ERRORLOG("File: {} zip failed,can't upload to s3", file.first);
			}

		}
		else {
			string s3Key = m_nodeid  + file.first;
			s3Key = utility::string::subreplace(s3Key, "//", "/");
			s3Key = utility::string::subreplace(s3Key, "/./", "/");
			if(m_s3->s3_helper_->uploadfile(m_bucket, s3Key, file.first)){
				INFOLOG("File: {} upload to s3:{} success", file.first, s3Key);
			}
			else {
				ERRORLOG("File: {} upload to s3:{} failed", file.first, s3Key);
			}
		};
	}

	void scanContinuously() {
		std::map<std::string, Poco::Timestamp> files3;
		scanDirectory(files3);
		for (const auto& file : files3) {
			if (m_files1.count(file.first) && m_files2.count(file.first) && m_files1[file.first] != m_files2[file.first] && m_files2[file.first] == file.second) {
				INFOLOG("File: {} changed between the first and second scans but did not change in the third scan", file.first);
				uploadToS3(file);
			};

			if (!m_files1.count(file.first) && m_files2.count(file.first) && m_files2[file.first] == file.second) {
				INFOLOG("File: {} added between the first and second scans but did not change in the third scan", file.first);
				//m_s3->s3_helper_->uploadfile(m_bucket, m_nodeid + "/" + file.first, file.first);
				uploadToS3(file);
			}
		}
		m_files1 = m_files2;
		m_files2 = files3;
	}


	bool zipFile(string inputDirPath, string outputFilePath, bool keepFile = true) {

		INFOLOG("File: s3 watcher zip file:{} to:{}", inputDirPath, outputFilePath);

		//Creating ofstream object and name of our zip file is going to be same as argv[1]
		ofstream out(outputFilePath, ios::binary);

		//Creating compress object with seekableOut as true for local file
		Compress c(out, true);

		Poco::File f(inputDirPath);
		if (f.exists())
		{
			Path p(f.path());

			if (f.isDirectory())
			{
				ERRORLOG("{} Not a file. ", inputDirPath);
				return false;
			}
			else if (f.isFile())
			{				
				c.addFile(p, p.getFileName());
				INFOLOG("File added succesfully as {}", f.path());
			}
		}
		//Closing compress object
		c.close();
		out.close();

		if (!keepFile) {
			INFOLOG("File remove local file:{}", inputDirPath);
			Poco::File file(inputDirPath);
			if (file.exists()) {
				file.remove();
			}
		}

		return true;
	}

	bool isDirectory(const std::string& path) {
		Poco::File file(path);
		return file.exists() && file.isDirectory();
	}

	bool createDirectory(const std::string& path) {
		Poco::File file(path);
		if (!file.exists()) {
			file.createDirectories();
		}
		return true;
	}

public:
	void watch() {
		while (!m_stop) {
			scanContinuously();
			std::this_thread::sleep_for(std::chrono::seconds(m_sleep));
		}
	}
	std::string m_bucket;
	std::string m_nodeid;
	std::string m_directory;
	std::map<std::string, Poco::Timestamp> m_files1;
	std::map<std::string, Poco::Timestamp> m_files2;
	std::thread m_thread;
	bool m_stop;
	bool m_keepFile;
	int  m_sleep = 10;
	bool m_needZip = 1;
	bool m_watchSubDir = 0;

	S3Singleton* m_s3 = S3Singleton::getInstance();
};


class FileWatcherFactory {
public:
	static std::mutex mtx;
	static std::vector<FileWatcher*> fileWatchers;

	static FileWatcher* createFileWatcher(const std::string& directory, const std::string& bucket, const std::string& nodeid, int sleep, bool keepFile, bool needZip,bool watchSubDir) {
		std::lock_guard<std::mutex> lock(FileWatcherFactory::mtx);
		INFOLOG("File s3 fileWatchers size:{}", FileWatcherFactory::fileWatchers.size());
		if (findFileWatcher(directory) == nullptr){
			INFOLOG("File s3 watcher local file:{} is't exist", directory);
			FileWatcher* fileWatcher = new FileWatcher(directory, bucket, nodeid, sleep, keepFile, needZip, watchSubDir);
			fileWatcher->start();
			FileWatcherFactory::fileWatchers.push_back(std::move(fileWatcher));
			return fileWatcher;
		}
		else {
			INFOLOG("File s3 watcher local file:{} is exist,not add", directory);
			return nullptr;
		}

	}

	static void deleteFileWatcher(FileWatcher* fileWatcher) {
		std::lock_guard<std::mutex> lock(FileWatcherFactory::mtx);
		auto it = std::find(fileWatchers.begin(), fileWatchers.end(), fileWatcher);
		if (it != FileWatcherFactory::fileWatchers.end()) {
			fileWatcher->stop();
			FileWatcherFactory::fileWatchers.erase(it);
			delete fileWatcher;
		}
	}

	static void updateFileWatcher(FileWatcher* fileWatcher, const std::string& directory, const std::string& bucket, const std::string& nodeid, int sleep, bool keepFile) {
		fileWatcher->stop();
		fileWatcher->m_directory = directory;
		fileWatcher->m_bucket = bucket;
		fileWatcher->m_nodeid = nodeid;
		fileWatcher->m_sleep = sleep;
		fileWatcher->m_keepFile = keepFile;
		fileWatcher->start();
	}

	static FileWatcher* findFileWatcher(const std::string& directory) {
		//std::lock_guard<std::mutex> lock(FileWatcherFactory::mtx);
		for (auto fileWatcher : FileWatcherFactory::fileWatchers) {
			if (fileWatcher->m_directory == directory) {
				INFOLOG("File s3 watcher local file:{} is exist", directory);
				return fileWatcher;
			}
		}
		INFOLOG("File s3 watcher local file:{} is't exist", directory);
		return nullptr;
	}

	static void startFileWatcher(FileWatcher* fileWatcher) {
		fileWatcher->start();
	}

	static void stopFileWatcher(FileWatcher* fileWatcher) {
		fileWatcher->stop();
	}

	static void deleteFileWatcherByDirectory(const std::string& directory) {
		std::lock_guard<std::mutex> lock(FileWatcherFactory::mtx);
		FileWatcher* fileWatcher = findFileWatcher(directory);
		if (fileWatcher != nullptr) {
			INFOLOG("File s3 watcher local file:{} removed", directory);
			deleteFileWatcher(fileWatcher);
		}
	}

	static std::string listAllWatchers() {
		std::lock_guard<std::mutex> lock(FileWatcherFactory::mtx);
		rapidjson::Document doc;
		doc.SetObject();

		rapidjson::Value watchers(rapidjson::kArrayType);
		for (auto fileWatcher : FileWatcherFactory::fileWatchers) {
			rapidjson::Value watcher(rapidjson::kObjectType);
			watcher.AddMember("directory", rapidjson::Value(fileWatcher->m_directory.c_str(), doc.GetAllocator()), doc.GetAllocator());
			watcher.AddMember("bucket", rapidjson::Value(fileWatcher->m_bucket.c_str(), doc.GetAllocator()), doc.GetAllocator());
			watcher.AddMember("nodeid", rapidjson::Value(fileWatcher->m_nodeid.c_str(), doc.GetAllocator()), doc.GetAllocator());
			watcher.AddMember("sleep", fileWatcher->m_sleep, doc.GetAllocator());
			watcher.AddMember("keepFile", fileWatcher->m_keepFile, doc.GetAllocator());
			watchers.PushBack(watcher, doc.GetAllocator());
		}

		doc.AddMember("watchers", watchers, doc.GetAllocator());

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		return buffer.GetString();
	}

};



#endif
