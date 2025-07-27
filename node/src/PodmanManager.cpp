// PodmanManager.cpp

#include "PodmanManager.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/Process.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <sstream>
#include <fstream>
#include "../utils/log/BaseLog.h" // 假设您有一个日志类

PodmanManager::PodmanManager(const std::string& baseUrl) : baseUrl(baseUrl) {
    INFOLOG("PodmanManager initialized with base URL: {}", baseUrl);
}

std::string PodmanManager::jsonToString(const json& jsonObj) {
    std::ostringstream oss;
    Poco::JSON::Stringifier::stringify(jsonObj, oss, 4);
    return oss.str();
}

json PodmanManager::stringToJson(const std::string& jsonString) {
    try {
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(jsonString);
        return result.extract<json>();
    }
    catch (const Poco::JSON::JSONException& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error converting to json: " + std::string(e.what()));
    }
}

std::string PodmanManager::listContainers(bool all) {
    DEBUGLOG("Listing containers. All: {}", all);
    std::string endpoint = "/libpod/containers/json?all=" + std::string(all ? "true" : "false");
    return executeRequest(endpoint, "GET");
}

std::string PodmanManager::createContainer(const json& config) {
    INFOLOG("Creating container");
    return executeRequest("/libpod/containers/create", "POST", jsonToString(config));
}

std::string PodmanManager::startContainer(const std::string& containerId) {
    INFOLOG("Starting container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId + "/start", "POST");
}

std::string PodmanManager::stopContainer(const std::string& containerId) {
    INFOLOG("Stopping container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId + "/stop", "POST");
}

std::string PodmanManager::removeContainer(const std::string& containerId) {
    INFOLOG("Removing container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId, "DELETE");
}

std::string PodmanManager::inspectContainer(const std::string& containerId) {
    DEBUGLOG("Inspecting container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId + "/json", "GET");
}

std::string PodmanManager::restartContainer(const std::string& containerId) {
    INFOLOG("Restarting container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId + "/restart", "POST");
}

std::string PodmanManager::pauseContainer(const std::string& containerId) {
    INFOLOG("Pausing container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId + "/pause", "POST");
}

std::string PodmanManager::unpauseContainer(const std::string& containerId) {
    INFOLOG("Unpausing container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId + "/unpause", "POST");
}

std::string PodmanManager::execInContainer(const std::string& containerId, const json& execConfig) {
    INFOLOG("Executing command in container: {}", containerId);
    return executeRequest("/libpod/containers/" + containerId + "/exec", "POST", jsonToString(execConfig));
}

std::string PodmanManager::listImages() {
    DEBUGLOG("Listing images");
    return executeRequest("/libpod/images/json", "GET");
}

std::string PodmanManager::pullImage(const std::string& imageName, const std::string& tag, const std::string& registry) {
    INFOLOG("Pulling image: {}:{} from {}", imageName, tag, registry);
    std::string fullImageName = registry + "/" + imageName + ":" + tag;
    json body = new Poco::JSON::Object;
    body->set("reference", fullImageName);
    return executeRequest("/libpod/images/pull", "POST", jsonToString(body));
}

std::string PodmanManager::removeImage(const std::string& imageId) {
    INFOLOG("Removing image: {}", imageId);
    return executeRequest("/libpod/images/" + imageId, "DELETE");
}

std::string PodmanManager::inspectImage(const std::string& imageId) {
    DEBUGLOG("Inspecting image: {}", imageId);
    return executeRequest("/libpod/images/" + imageId + "/json", "GET");
}

std::string PodmanManager::tagImage(const std::string& imageId, const std::string& newTag) {
    INFOLOG("Tagging image: {} with new tag: {}", imageId, newTag);
    json body = new Poco::JSON::Object;
    body->set("tag", newTag);
    return executeRequest("/libpod/images/" + imageId + "/tag", "POST", jsonToString(body));
}

std::string PodmanManager::listNetworks() {
    DEBUGLOG("Listing networks");
    return executeRequest("/libpod/networks", "GET");
}

std::string PodmanManager::createNetwork(const json& config) {
    INFOLOG("Creating network");
    return executeRequest("/libpod/networks/create", "POST", jsonToString(config));
}

std::string PodmanManager::removeNetwork(const std::string& networkId) {
    INFOLOG("Removing network: {}", networkId);
    return executeRequest("/libpod/networks/" + networkId, "DELETE");
}

std::string PodmanManager::inspectNetwork(const std::string& networkId) {
    DEBUGLOG("Inspecting network: {}", networkId);
    return executeRequest("/libpod/networks/" + networkId, "GET");
}

std::string PodmanManager::listVolumes() {
    DEBUGLOG("Listing volumes");
    return executeRequest("/libpod/volumes", "GET");
}

std::string PodmanManager::createVolume(const json& config) {
    INFOLOG("Creating volume");
    return executeRequest("/libpod/volumes/create", "POST", jsonToString(config));
}

std::string PodmanManager::removeVolume(const std::string& volumeName) {
    INFOLOG("Removing volume: {}", volumeName);
    return executeRequest("/libpod/volumes/" + volumeName, "DELETE");
}

std::string PodmanManager::inspectVolume(const std::string& volumeName) {
    DEBUGLOG("Inspecting volume: {}", volumeName);
    return executeRequest("/libpod/volumes/" + volumeName, "GET");
}

std::string PodmanManager::listStacks() {
    INFOLOG("Listing stacks");
    return executePodmanCompose({ "ps", "--format", "json" });
}

std::string PodmanManager::deployStack(const std::string& stackName, const std::string& composeFile) {
    INFOLOG("Deploying stack: {}", stackName);
    writeComposeFile(stackName, composeFile);
    return executePodmanCompose({ "up", "-d" }, stackName);
}

std::string PodmanManager::removeStack(const std::string& stackName) {
    INFOLOG("Removing stack: {}", stackName);
    auto result = executePodmanCompose({ "down" }, stackName);
    removeComposeFile(stackName);
    return result;
}

std::string PodmanManager::inspectStack(const std::string& stackName) {
    DEBUGLOG("Inspecting stack: {}", stackName);
    return executePodmanCompose({ "ps", "--format", "json" }, stackName);
}

std::string PodmanManager::updateStack(const std::string& stackName, const std::string& composeFile) {
    INFOLOG("Updating stack: {}", stackName);
    writeComposeFile(stackName, composeFile);
    return executePodmanCompose({ "up", "-d" }, stackName);
}

std::string PodmanManager::startStack(const std::string& stackName) {
    INFOLOG("Starting stack: {}", stackName);
    return executePodmanCompose({ "start" }, stackName);
}

std::string PodmanManager::stopStack(const std::string& stackName) {
    INFOLOG("Stopping stack: {}", stackName);
    return executePodmanCompose({ "stop" }, stackName);
}

std::string PodmanManager::restartStack(const std::string& stackName) {
    INFOLOG("Restarting stack: {}", stackName);
    return executePodmanCompose({ "restart" }, stackName);
}

std::string PodmanManager::pauseStack(const std::string& stackName) {
    INFOLOG("Pausing stack: {}", stackName);
    return executePodmanCompose({ "pause" }, stackName);
}

std::string PodmanManager::unpauseStack(const std::string& stackName) {
    INFOLOG("Unpausing stack: {}", stackName);
    return executePodmanCompose({ "unpause" }, stackName);
}

std::string PodmanManager::scaleStack(const std::string& stackName, const json& serviceScaleConfig) {
    INFOLOG("Scaling stack: {}", stackName);
    std::vector<std::string> scaleArgs = { "scale" };
    for (const auto& pair : *serviceScaleConfig) {
        scaleArgs.push_back(pair.first + "=" + pair.second.toString());
    }
    return executePodmanCompose(scaleArgs, stackName);
}

std::string PodmanManager::getStackLogs(const std::string& stackName) {
    DEBUGLOG("Getting logs for stack: {}", stackName);
    return executePodmanCompose({ "logs" }, stackName);
}

std::string PodmanManager::listRegistries() {
    DEBUGLOG("Listing registries");
    return executeRequest("/libpod/registries", "GET");
}

std::string PodmanManager::addRegistry(const std::string& url) {
    INFOLOG("Adding registry: {}", url);
    json body = new Poco::JSON::Object;
    body->set("url", url);
    return executeRequest("/libpod/registries", "POST", jsonToString(body));
}

std::string PodmanManager::removeRegistry(const std::string& url) {
    INFOLOG("Removing registry: {}", url);
    return executeRequest("/libpod/registries/" + url, "DELETE");
}

std::string PodmanManager::loginRegistry(const std::string& url, const std::string& username, const std::string& password) {
    INFOLOG("Logging into registry: {}", url);
    json body = new Poco::JSON::Object;
    body->set("url", url);
    body->set("username", username);
    body->set("password", password);
    return executeRequest("/libpod/auth", "POST", jsonToString(body));
}

std::string PodmanManager::executeRequest(const std::string& endpoint, const std::string& method, const std::string& body) {
    DEBUGLOG("Executing request. Endpoint: {}, Method: {}", endpoint, method);
    Poco::URI uri(baseUrl + endpoint);
    Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
    Poco::Net::HTTPRequest request(method, uri.getPathAndQuery(), Poco::Net::HTTPMessage::HTTP_1_1);

    if (!body.empty()) {
        request.setContentType("application/json");
        request.setContentLength(body.length());
    }

    std::ostream& os = session.sendRequest(request);
    if (!body.empty()) {
        os << body;
    }

    Poco::Net::HTTPResponse response;
    std::istream& rs = session.receiveResponse(response);

    std::string responseBody;
    Poco::StreamCopier::copyToString(rs, responseBody);

    DEBUGLOG("Response status: {}", response.getStatus());
    DEBUGLOG("Response body: {}", responseBody);

    return responseBody;
}

std::string PodmanManager::executePodmanCompose(const std::vector<std::string>& args, const std::string& stackName) {
    DEBUGLOG("Executing podman-compose command. Stack: {}", stackName);
    std::vector<std::string> command = { "podman-compose" };

    if (!stackName.empty()) {
        command.push_back("-f");
        command.push_back(getComposeFilePath(stackName));
    }

    command.insert(command.end(), args.begin(), args.end());

    Poco::Pipe outPipe;
    Poco::Pipe errPipe;
    Poco::ProcessHandle ph = Poco::Process::launch("podman-compose", command, 0, &outPipe, &errPipe);

    Poco::PipeInputStream istr(outPipe);
    std::string output;
    Poco::StreamCopier::copyToString(istr, output);

    Poco::PipeInputStream errStr(errPipe);
    std::string error;
    Poco::StreamCopier::copyToString(errStr, error);

    int exitCode = ph.wait();

    if (exitCode != 0) {
        ERRORLOG("podman-compose command failed. Exit code: {}", exitCode);
        ERRORLOG("Error output: {}", error);
        throw std::runtime_error("podman-compose command failed: " + error);
    }

    DEBUGLOG("podman-compose command output: {}", output);
    return output;
}

std::string PodmanManager::getComposeFilePath(const std::string& stackName) {
    return "/path/to/compose/files/" + stackName + ".yml";
}

void PodmanManager::writeComposeFile(const std::string& stackName, const std::string& composeFile) {
    INFOLOG("Writing compose file for stack: {}", stackName);
    std::string filePath = getComposeFilePath(stackName);
    std::ofstream file(filePath);
    if (!file.is_open()) {
        ERRORLOG("Failed to open file for writing: {}", filePath);
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    file << composeFile;
    file.close();
    DEBUGLOG("Compose file written successfully: {}", filePath);
}

void PodmanManager::removeComposeFile(const std::string& stackName) {
    INFOLOG("Removing compose file for stack: {}", stackName);
    std::string filePath = getComposeFilePath(stackName);
    if (std::remove(filePath.c_str()) != 0) {
        ERRORLOG("Failed to remove file: {}", filePath);
        throw std::runtime_error("Failed to remove file: " + filePath);
    }
    DEBUGLOG("Compose file removed successfully: {}", filePath);
}