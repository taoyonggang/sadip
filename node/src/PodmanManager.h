// PodmanManager.h

#pragma once

#include <string>
#include <vector>
#include <Poco/JSON/Object.h>

using json = Poco::JSON::Object::Ptr;

class PodmanManager {
public:
    PodmanManager(const std::string& baseUrl);

    // Container operations
    std::string listContainers(bool all = false);
    std::string createContainer(const json& config);
    std::string startContainer(const std::string& containerId);
    std::string stopContainer(const std::string& containerId);
    std::string removeContainer(const std::string& containerId);
    std::string inspectContainer(const std::string& containerId);
    std::string restartContainer(const std::string& containerId);
    std::string pauseContainer(const std::string& containerId);
    std::string unpauseContainer(const std::string& containerId);
    std::string execInContainer(const std::string& containerId, const json& execConfig);

    // Image operations
    std::string listImages();
    std::string pullImage(const std::string& imageName, const std::string& tag, const std::string& registry);
    std::string removeImage(const std::string& imageId);
    std::string inspectImage(const std::string& imageId);
    std::string tagImage(const std::string& imageId, const std::string& newTag);

    // Network operations
    std::string listNetworks();
    std::string createNetwork(const json& config);
    std::string removeNetwork(const std::string& networkId);
    std::string inspectNetwork(const std::string& networkId);

    // Volume operations
    std::string listVolumes();
    std::string createVolume(const json& config);
    std::string removeVolume(const std::string& volumeName);
    std::string inspectVolume(const std::string& volumeName);

    // Stack operations
    std::string listStacks();
    std::string deployStack(const std::string& stackName, const std::string& composeFile);
    std::string removeStack(const std::string& stackName);
    std::string inspectStack(const std::string& stackName);
    std::string updateStack(const std::string& stackName, const std::string& composeFile);
    std::string startStack(const std::string& stackName);
    std::string stopStack(const std::string& stackName);
    std::string restartStack(const std::string& stackName);
    std::string pauseStack(const std::string& stackName);
    std::string unpauseStack(const std::string& stackName);
    std::string scaleStack(const std::string& stackName, const json& serviceScaleConfig);
    std::string getStackLogs(const std::string& stackName);

    // Registry operations
    std::string listRegistries();
    std::string addRegistry(const std::string& url);
    std::string removeRegistry(const std::string& url);
    std::string loginRegistry(const std::string& url, const std::string& username, const std::string& password);

private:
    std::string baseUrl = "http://127.0.0.1:8888";

    std::string executeRequest(const std::string& endpoint, const std::string& method, const std::string& body = "");
    std::string executePodmanCompose(const std::vector<std::string>& args, const std::string& stackName = "");
    std::string getComposeFilePath(const std::string& stackName);
    void writeComposeFile(const std::string& stackName, const std::string& composeFile);
    void removeComposeFile(const std::string& stackName);

public:
    static std::string jsonToString(const json& jsonObj);
    static json stringToJson(const std::string& jsonString);
};