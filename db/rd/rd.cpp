#include "../../utils/pch.h"
#include "rd.h"
#include "../../utils/log/BaseLog.h"

Rd::Rd(int dbType, string dbhost, string dbuser, string dbpwd, string dbname, int dbport, int maxConn) :
    dbType_(dbType),
    dbhost_(std::move(dbhost)),
    dbuser_(std::move(dbuser)),
    dbpwd_(std::move(dbpwd)),
    dbname_(std::move(dbname)),
    dbport_(dbport),
    maxConn_(maxConn)
{
    std::memset(constr_, 0, sizeof(constr_));
}

Rd::~Rd() {
    try {
        uInit();  // 确保清理所有连接
        if (checkThread_.joinable()) {
            stopCheck_ = true;
            checkThread_.join();
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Exception in destructor: {}", e.what());
    }
}

int Rd::init() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);

        // 初始化连接池
        pool_ = std::make_unique<connection_pool>(maxConn_);
        sqls_ = std::make_unique<session>(*pool_);

        // 构建连接字符串
        snprintf(constr_, sizeof(constr_),
            "dbname=%s user=%s password=%s host=%s port=%d charset=gbk",
            dbname_.c_str(), dbuser_.c_str(), dbpwd_.c_str(),
            dbhost_.c_str(), dbport_);

        // 初始化连接池中的所有连接
        if (dbType_ == 1) {  // MySQL
            for (size_t i = 0; i < maxConn_; ++i) {
                session& sql = pool_->at(i);
                sql.open(*soci::factory_mysql(), constr_);
            }
        }

        // 启动检查线程
        INFOLOG("Starting MySQL connection check thread...");
        checkThread_ = std::thread([this] { this->startCheck(); });

        return 0;
    }
    catch (const std::exception& e) {
        ERRORLOG("Failed to initialize database: {}", e.what());
        return -1;
    }
}

int Rd::uInit() {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        stopCheck();

        if (dbType_ == 1) {  // MySQL
            if (pool_) {
                for (size_t i = 0; i < maxConn_; ++i) {
                    try {
                        session& sql = pool_->at(i);
                        sql.close();
                    }
                    catch (const std::exception& e) {
                        ERRORLOG("Error closing connection {}: {}", i, e.what());
                    }
                }
            }
        }

        pool_.reset();
        sqls_.reset();

        return 0;
    }
    catch (const std::exception& e) {
        ERRORLOG("Error in uInit: {}", e.what());
        return -1;
    }
}

void Rd::startCheck() {
    while (!stopCheck_) {
        INFOLOG("MySQL connections check thread running...");
        std::this_thread::sleep_for(std::chrono::seconds(CHECK_INTERVAL_SECONDS));
        checkConnections();
    }
}

void Rd::stopCheck() {
    stopCheck_ = true;
}

void Rd::checkConnections(bool needReconnect) {
    if (!waitForOperation(isUsing_)) {
        ERRORLOG("Timeout waiting for database operations to complete");
        return;
    }

    ConnectionGuard guard(isCheck_);

    try {
        if (dbType_ == 1) {  // MySQL
            for (size_t i = 0; i < maxConn_; i++) {
                std::lock_guard<std::mutex> lock(mutex_);
                session& sql = pool_->at(i);

                if (sql.is_connected()) {
                    DEBUGLOG("MySQL connection {} is connected", i);
                    continue;
                }

                DEBUGLOG("MySQL connection {} is lost", i);
                if (needReconnect) {
                    try {
                        INFOLOG("Reconnecting MySQL connection {}", i);
                        sql.reconnect();
                    }
                    catch (const std::exception& e) {
                        ERRORLOG("Failed to reconnect connection {}: {}", i, e.what());
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Error in checkConnections: {}", e.what());
    }
}

bool Rd::waitForOperation(std::atomic<bool>& flag, int maxAttempts) {
    int attempts = 0;
    while (flag.load(std::memory_order_relaxed)) {
        if (++attempts > maxAttempts) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));
    }
    return true;
}

void Rd::handleDatabaseError(const soci::soci_error& e, const std::string& sql) {
    if (e.get_error_message().find("lost connection") != std::string::npos) {
        ERRORLOG("Connection lost: {} SQL: {}", e.what(), sql);
        checkConnections(true);
    }
    ERRORLOG("Database error: {} SQL: {}", e.what(), sql);
}

int Rd::excuteSql(string sql) {
    if (!waitForOperation(isCheck_)) {
        ERRORLOG("Timeout waiting for database check to complete");
        return -1;
    }

    ConnectionGuard guard(isUsing_);

    try {
        if (sql.empty()) {
            return -1;
        }

        session sqls(*pool_);
        char firstChar = sql[0];
        soci::statement stmt = (sqls.prepare << sql);
        stmt.execute();

        const long long affectedRows = stmt.get_affected_rows();
        if (affectedRows > 0 || firstChar == 't' || firstChar == 'T') {
            return 1;
        }

        ERRORLOG("Execute failed, SQL: {}", sql);
        return -1;
    }
    catch (const soci::soci_error& e) {
        handleDatabaseError(e, sql);
        return -1;
    }
    catch (const std::exception& e) {
        ERRORLOG("Unexpected error: {} SQL: {}", e.what(), sql);
        return -1;
    }
}

soci::rowset<soci::row> Rd::select(string sql) {
    if (!waitForOperation(isCheck_)) {
        throw soci::soci_error("Timeout waiting for database check to complete");
    }

    ConnectionGuard guard(isUsing_);

    try {
        session sqls(*pool_);
        return sqls.prepare << sql;
    }
    catch (const soci::soci_error& e) {
        handleDatabaseError(e, sql);
        throw;
    }
    catch (const std::exception& e) {
        ERRORLOG("Unexpected error in select: {} SQL: {}", e.what(), sql);
        throw;
    }
}