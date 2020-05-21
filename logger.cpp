#include <stdexcept>
#include <algorithm>

#include "logger.h"
#include "file.h"


Logger* g_defaultLogger = &Logger::Instance();


// Logger
INSTANCE_IMP(Logger, exeName());


#ifdef _WIN32
#pragma warning(disable:4996)
#define CLEAR_COLOR 7
static const WORD LOG_CONST_TABLE[][3] = {
		{0x97, 0x09 , 'T'},	// 蓝底灰字，黑底蓝字，Windows Console 默认黑底
		{0xA7, 0x0A , 'D'}, // 绿底灰字，黑底绿字
		{0xB7, 0x0B , 'I'}, // 天蓝底灰字，黑底天蓝字
		{0xE7, 0x0E , 'W'}, // 黄底灰字，黑底黄字
		{0xC7, 0x0C , 'E'}  // 红底灰字，黑底红字
}; 

bool SetConsoleColor(WORD Color) {
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (handle == 0)
		return false;

	BOOL ret = SetConsoleTextAttribute(handle, Color);
	return(ret == TRUE);
}
#else
#define CLEAR_COLOR "\033[0m"
static const char *LOG_CONST_TABLE[][3] = {
		{"\033[44;37m", "\033[34m", "T"},
		{"\033[42;37m", "\033[32m", "D"},
		{"\033[46;37m", "\033[36m", "I"},
		{"\033[43;37m", "\033[33m", "W"},
		{"\033[41;37m", "\033[31m", "E"} };
#endif


Logger::~Logger() {
	_writer.reset();
	{
		LogContextCapturer(*this, LInfo, __FILE__, __FUNCTION__, __LINE__);
	}
	_channels.clear();
}


// Add a LogChannel for Logger. 
void 
Logger::addChannel(const std::shared_ptr<LogChannel> &channel) {
	_channels[channel->name()] = channel;
}


// Del a LogChannel from Logger.
void 
Logger::delChannel(const std::string &name) {
	_channels.erase(name);
}

// Get a LogChannel from Logger.
std::shared_ptr<LogChannel> 
Logger::getChannel(const std::string &name) {
	auto it = _channels.find(name);
	if (it == _channels.end())
		return nullptr;
	return it->second;
}

// Set Logger 's level.
void 
Logger::setLevel(LogLevel level) {
	for (auto &channel : _channels) {
		channel.second->setLevel(level);
	}
}

// Write Log.
void 
Logger::write(const LogContextPtr &ctx) {
	if (_writer) {
		_writer->write(ctx);
	}
	else {
		writeChannels(ctx);
	}
}

// private function
// only for friend class AsyncLogWriter
void 
Logger::writeChannels(const LogContextPtr &ctx) {
	for (auto &channel : _channels) {
		channel.second->write(*this, ctx);
	}
}

//LogContext

static inline const char *getFileName(const char *file) {
	auto pos = strrchr(file, '/');
#ifdef _WIN32
	if (!pos) {
		pos = strrchr(file, '\\');
	}
#endif
	return pos ? pos + 1 : file;
}

static inline const char *getFunctionName(const char *func) {
#ifndef _WIN32
	return func;
#else
	auto pos = strrchr(func, ':');
	return pos ? pos + 1 : func;
#endif
}

LogContext::LogContext(
	LogLevel level, 
	const char *file, 
	const char *function, 
	int line) 
	:
	_level(level),
	_line(line),
	_file(getFileName(file)),
	_function(getFunctionName(function)) {
	gettimeofday(&_tv, NULL);
}

// LogContextCapturer
LogContextCapturer::LogContextCapturer(
	Logger &logger,
	LogLevel level,
	const char *file,
	const char *function,
	int line
) : _ctx(new LogContext(level, file, function, line)), 
	_logger(logger) {
}

LogContextCapturer::LogContextCapturer(const LogContextCapturer &that) :
	_ctx(that._ctx),
	_logger(that._logger) {
	const_cast<LogContextPtr&>(that._ctx).reset();
}

LogContextCapturer::~LogContextCapturer() {
	*this << std::endl;
}

/*
*Summary: print Log when f is std::endl.
*Parameters:
*		f: std::endl.
*Return : *this
*/
LogContextCapturer&
LogContextCapturer::operator<<(std::ostream &(*f)(std::ostream &)) {
	if (!_ctx) {
		return *this;
	}
	_logger.write(_ctx);
	_ctx.reset();
	return *this;
}

//AsyncLogWriter
AsyncLogWriter::AsyncLogWriter(Logger &logger) :
	_exit(false),
	_logger(logger) {
	_thread = std::make_shared<std::thread>([this]() {this->run(); });
}

AsyncLogWriter::~AsyncLogWriter() {
	_exit = true;
	_sem.post();
	_thread->join();
	flushAll();
}

void
AsyncLogWriter::run() {
	while (!_exit) {
		_sem.wait();
		flushAll();
	}
}

void 
AsyncLogWriter::flushAll() {
	std::list<LogContextPtr> tmp;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		tmp.swap(_pending);
	}
	/*
	tmp.for_each([&](const LogContextPtr &ctx) {
		_logger.writeChannels(ctx);
	});
	*/
	std::for_each(
		tmp.begin(), 
		tmp.end(), 
		[&](const LogContextPtr &ctx) {
			_logger.writeChannels(ctx); 
		});
}

void
AsyncLogWriter::write(const LogContextPtr &ctx) {
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_pending.emplace_back(ctx);
	}
	_sem.post();
}

//LogChannel
LogChannel::LogChannel(const std::string &name, LogLevel level, bool enableDeatil) :
	_name(name),
	_level(level), 
	_enableDetail(enableDeatil){
}

LogChannel::~LogChannel() {
}

std::string
LogChannel::printTime(const timeval &tv) {
	time_t second = tv.tv_sec;
	struct tm *tm = localtime(&second);
	char buff[128];
	snprintf(buff, sizeof(buff), "%d-%02d-%02d %02d:%02d:%02d.%03d",
			1900 + tm->tm_year,
			1 + tm->tm_mon,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			(int) (tv.tv_usec / 1000)
			);
	return buff;

}

void
LogChannel::format(
	const Logger &logger,
	std::ostream &ost,
	const LogContextPtr &ctx,
	bool enableColor,
	bool enableDetail
) {
	if (!enableDetail && ctx->str().empty()) {
		return;
	}

	if (enableColor) {
#ifdef _WIN32
		SetConsoleColor(LOG_CONST_TABLE[ctx->_level][1]);
#else
		ost << LOG_CONST_TABLE[ctx->level][1];
#endif // _WIN32
	}

#ifdef _WIN32
	ost << printTime(ctx->_tv) << " " << (char)LOG_CONST_TABLE[ctx->_level][2] << " ";
#else
	ost << printTime(ctx->_tv) << " " << LOG_CONST_TABLE[ctx->_level][2] << " ";
#endif // _WIN32

	if (enableDetail) {
#ifdef _WIN32
		ost << logger.getName() << "[" << GetCurrentProcessId();
#else
		ost << logger.getName() << "[" << getpid();
#endif // _WIN32
		ost << "] " << ctx->_file << ":" << ctx->_line << " " << ctx->_function << " | ";
	}

	ost << ctx->str();

	if (enableColor) {
#ifdef _WIN32
		SetConsoleColor(CLEAR_COLOR);
#else
		ost << CLEAR_COLOR;
#endif // _WIN32
	}

	ost << std::endl;
}


//ConsoleChannel
ConsoleChannel::ConsoleChannel(const std::string &name, LogLevel level, bool enableDetail) :
	LogChannel(name, level, enableDetail) {
}

ConsoleChannel::~ConsoleChannel() {
}

void
ConsoleChannel::write(const Logger &logger, const LogContextPtr &ctx) {
	if (_level > ctx->_level) {
		return;
	}
	// enableColor = true
	format(logger, std::cout, ctx, true, _enableDetail);
}

//FileChannelBase
FileChannelBase::FileChannelBase(
	const std::string &name,
	const std::string &path,
	LogLevel level
) : LogChannel(name, level), _path(path) {
}

FileChannelBase::~FileChannelBase() {
	close();
}

void
FileChannelBase::write(const Logger &logger, const LogContextPtr &ctx) {
	if (_level > ctx->_level) {
		return;
	}
	if (!_fstream.is_open()) {
		open();
	}
	// enableColor = false
	format(logger, _fstream, ctx, false, _enableDetail);
}

bool
FileChannelBase::setPath(const std::string &path) {
	_path = path;
	return open();
}

bool
FileChannelBase::open() {
	if (_path.empty()) {
		throw std::runtime_error("Log file path must be set.");
	}
	_fstream.close();

#ifndef _WIN32
	File::create_path(_path.c_str(), S_IRWXO | S_IRWXG | S_IRWXU);
#else
	File::create_path(_path.c_str(), 0);
#endif // !_WIN32

	_fstream.open(_path.c_str(), std::ios::out | std::ios::app);

	if (!_fstream.is_open()) {
		return false;
	}
	return true;
}

void
FileChannelBase::close() {
	_fstream.close();
}

//FileChannel;

static const auto s_second_per_day = 24 * 60 * 60;

FileChannel::FileChannel(
	const std::string &name,
	const std::string &dir,
	LogLevel level) :
	FileChannelBase(name, "", level) {
	_dir = dir;
	if (_dir.back() != '/') {
		_dir.append("/");
	}
}

FileChannel::~FileChannel() {

}

int64_t
FileChannel::getDay(time_t second) {
	return second / s_second_per_day;
}

/*

*/
static std::string
getLogFilePath(const std::string &dir, uint64_t day) {
	time_t second = s_second_per_day * day;
	struct tm *tm = localtime(&second);
	char buf[32];
	snprintf(buf, 
			sizeof(buf), 
			"%d-%02d-%02d.log", 
			1900 + tm->tm_year, 
			1 + tm->tm_mon, 
			tm->tm_mday);

	return dir + buf;
}

void
FileChannel::write(const Logger &logger, const LogContextPtr &ctx) {
	auto day = getDay(ctx->_tv.tv_sec);
	if (day != _lastDay) {
		_lastDay = day;
		auto logFilePath = getLogFilePath(_dir, day);
		_logFileMap.emplace(day, logFilePath);
		_canWrite = setPath(logFilePath);
		if (!_canWrite) {
			ErrorL << "Failed to open log file: " << _path;
		}
		clean();
	}
	if (_canWrite) {
		FileChannelBase::write(logger, ctx);
	}
}

void 
FileChannel::clean() {
	auto today = getDay(time(NULL));
	for (auto it = _logFileMap.begin(); it != _logFileMap.end();) {
		if (today < it->first + _logMaxDay) {
			break;
		}
		
		File::delete_file(it->second.data());

		it = _logFileMap.erase(it);
	}
}

void 
FileChannel::setMaxDay(int maxDay) {
	_logMaxDay = maxDay > 1 ? maxDay : 1;
}