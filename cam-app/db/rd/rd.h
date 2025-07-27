#pragma once 

#include "../Idb.h"
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>

class Rd : public Idb {
public:
    // 常量定义
    static constexpr int DEFAULT_MAX_CONN = 50;
    static constexpr int CHECK_INTERVAL_SECONDS = 300;
    static constexpr int WAIT_TIMEOUT_MS = 200;
    static constexpr int MAX_WAIT_ATTEMPTS = 1000;

    // 构造和析构
    Rd(int dbType,
        std::string dbhost,
        std::string dbuser,
        std::string dbpwd,
        std::string dbname,
        int dbport,
        int maxConn);
    ~Rd() override;

    // 禁用拷贝和移动
    Rd(const Rd&) = delete;
    Rd& operator=(const Rd&) = delete;
    Rd(Rd&&) = delete;
    Rd& operator=(Rd&&) = delete;

    // 接口实现
    int init() override;
    int uInit() override;
    void checkConnections(bool needReconnect = true) override;
    soci::rowset<soci::row> select(std::string sql) override;
    int excuteSql(std::string sql) override;

private:
    // 内部类用于RAII方式管理连接状态
    class ConnectionGuard {
        std::atomic<bool>& flag_;
    public:
        explicit ConnectionGuard(std::atomic<bool>& flag) : flag_(flag) {
            flag_ = true;
        }
        ~ConnectionGuard() {
            flag_ = false;
        }
    };

    // 内部方法
    void startCheck();
    void stopCheck();
    bool waitForOperation(std::atomic<bool>& flag, int maxAttempts = MAX_WAIT_ATTEMPTS);
    void handleDatabaseError(const soci::soci_error& e, const std::string& sql);

    // 数据库配置
    const int dbType_{ 1 };  // 0=sqlite,1=mysql,2=oracle,3=pg,4=td
    const int maxConn_;
    const std::string dbhost_;
    const std::string dbuser_;
    const std::string dbpwd_;
    const std::string dbname_;
    const int dbport_;

    // 连接池相关
    std::unique_ptr<soci::connection_pool> pool_;
    std::unique_ptr<soci::session> sqls_;
    char constr_[256];

    // 状态标志
    std::atomic<bool> isCheck_{ false };
    std::atomic<bool> isUsing_{ false };
    std::atomic<bool> stopCheck_{ false };

    // 同步和线程
    std::mutex mutex_;
    std::thread checkThread_;
};

// 可选：为连接池提供工厂方法
class DatabaseConnectionFactory {
public:
    static std::unique_ptr<Rd> createMySQLConnection(
        const std::string& host,
        const std::string& user,
        const std::string& pwd,
        const std::string& dbname,
        int port = 3306,
        int maxConn = Rd::DEFAULT_MAX_CONN
    ) {
        return std::make_unique<Rd>(1, host, user, pwd, dbname, port, maxConn);
    }
};