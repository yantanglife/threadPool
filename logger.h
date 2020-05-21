#pragma once

#include <time.h>
#include <stdio.h>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include "util.h"
#include "semaphore.h"

typedef enum {
    LTrace = 0,
    LDebug,
    LInfo,
    LWarn,
    LError
} LogLevel;

class LogWriter;
class LogChannel;
class LogContext;

typedef std::shared_ptr<LogContext> LogContextPtr;

// Logger
class Logger : public std::enable_shared_from_this<Logger>, public noncopyable {
public:
    typedef std::shared_ptr<Logger> Ptr;
    
    friend class AsyncLogWriter;

    static Logger &Instance();

    Logger(const std::string &loggerName) : _loggerName(loggerName) {};
    ~Logger();

    void addChannel(const std::shared_ptr<LogChannel> &channel);
    void delChannel(const std::string &cName);
    std::shared_ptr<LogChannel> getChannel(const std::string &cName);

    const std::string &getName() const {
        return _loggerName;
    };

    void setLevel(LogLevel level);

    void setWriter(const std::shared_ptr<LogWriter> &writer) {
        _writer = writer;
    };

    void write(const LogContextPtr &ctx);
private:
    void writeChannels(const LogContextPtr &ctx);
private:
    std::map<std::string, std::shared_ptr<LogChannel>> _channels;
    std::shared_ptr<LogWriter> _writer;
    std::string _loggerName;

};

class LogChannel : public noncopyable {
public:
    LogChannel(const std::string &name, LogLevel level = LTrace, bool enableDetail = true);
    virtual ~LogChannel();
    virtual void write(const Logger &logger, const LogContextPtr &ctx) = 0;
    
    const std::string &name() const { return _name; };
    void setLevel(LogLevel level) { _level = level; };

    // modifiable
    void setDetail(bool enableDetail) { _enableDetail = enableDetail; };

    static std::string printTime(const timeval &tv);
protected:
    virtual void format(
        const Logger &logger,
        std::ostream &ost,
        const LogContextPtr &ctx,
        bool enableColor = true,
        bool enableDetail = true
    );
    
protected:
    std::string _name;
    LogLevel _level;
    // As a FLAG whether to write Log's detail, 
    // for some situation that don't want detail. 
    bool _enableDetail;
};

class ConsoleChannel : public LogChannel {
public:
    ConsoleChannel(
        const std::string &name = "ConsoleChannel", 
        LogLevel level = LTrace, 
        bool enanleDetail = true
    );
    ~ConsoleChannel();

    void write(const Logger &logger, const LogContextPtr &ctx) override;
};

class FileChannelBase : public LogChannel {
public:
    FileChannelBase(
        const std::string &name = "FileChannelBase", 
        const std::string &path = exePath() + ".log", 
        LogLevel level = LTrace
    );
    ~FileChannelBase();

    void write(const Logger &logger, const LogContextPtr &ctx) override;
    bool setPath(const std::string &path);
    const std::string &path() const { return _path; };
protected:
    virtual bool open();
    virtual void close();
protected:
    std::ofstream _fstream;
    std::string _path;
};

class FileChannel : public FileChannelBase {
public:    
    FileChannel(
        const std::string &name = "FileChannel", 
        const std::string &dir = exeDir() + "log/",
        LogLevel level = LTrace
    );
    ~FileChannel() override;

    void write(const Logger &logger, const LogContextPtr &ctx) override;
    void setMaxDay(int maxDay);
private:
    int64_t getDay(time_t second);
    void clean();
private:
    bool _canWrite = false;
    std::string _dir;
    int64_t _lastDay = -1;
    std::map<uint64_t, std::string> _logFileMap;
    int _logMaxDay = 30;
};

// Log info.
class LogContext : public std::ostringstream {
public:
    LogContext(LogLevel level, const char *file, const char *function, int line);
    ~LogContext() = default;

    LogLevel _level;
    std::string _file;
    std::string _function;
    int _line;
    struct timeval _tv;
    
};

class LogContextCapturer {
public:
    typedef std::shared_ptr<LogContextCapturer> Ptr;

    LogContextCapturer(
        Logger &logger, 
        LogLevel level, 
        const char *file, 
        const char *function, 
        int line
    );
    LogContextCapturer(const LogContextCapturer &that);
    ~LogContextCapturer();

    LogContextCapturer& operator<<  (std::ostream &(*f)(std::ostream &));

    template<typename T>
    LogContextCapturer &operator<< (T &&data) {
        if (!_ctx) {
            return *this;
        }
        (*_ctx) << std::forward<T>(data);
        return *this;
    }

    void clear() { _ctx.reset(); };
private:
    LogContextPtr _ctx;
    Logger &_logger;
};

class LogWriter : public noncopyable {
public:
    LogWriter() {}
    virtual ~LogWriter() {}
    virtual void write(const LogContextPtr &ctx) = 0;
};

class AsyncLogWriter : public LogWriter {
public:
    AsyncLogWriter(Logger &logger = Logger::Instance());
    ~AsyncLogWriter();
private:
    void run();
    void flushAll();
    void write(const LogContextPtr &ctx) override;
private:
    bool _exit;
    std::shared_ptr<std::thread> _thread;
    std::list<LogContextPtr> _pending;
    std::mutex _mutex;
    Semaphore _sem;
    Logger &_logger;
};


extern Logger* g_defaultLogger;

#define TraceL LogContextCapturer(*g_defaultLogger, LTrace, __FILE__,__FUNCTION__, __LINE__)
#define DebugL LogContextCapturer(*g_defaultLogger,LDebug, __FILE__,__FUNCTION__, __LINE__)
#define InfoL LogContextCapturer(*g_defaultLogger,LInfo, __FILE__,__FUNCTION__, __LINE__)
#define WarnL LogContextCapturer(*g_defaultLogger,LWarn,__FILE__, __FUNCTION__, __LINE__)
#define ErrorL LogContextCapturer(*g_defaultLogger,LError,__FILE__, __FUNCTION__, __LINE__)
#define WriteL(level) LogContextCapturer(*g_defaultLogger,level,__FILE__, __FUNCTION__, __LINE__)


