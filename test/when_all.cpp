#include "corpc/when_all.hpp"
#include "corpc/future.hpp"
#include "corpc/timer_loop.hpp"
#include "corpc/when_any.hpp"
#include "elog/logger.h"

using namespace corpc;
using namespace std::chrono_literals;

TimerLoop loop;

Future<int> hello1()
{
	LOG_INFO << "hello1开始睡1秒";
	co_await sleep_for(loop, 1s);
	LOG_INFO << "hello1睡醒了";
	co_return 1;
}

Future<int> hello2()
{
	LOG_INFO << "hello2开始睡2秒";
	co_await sleep_for(loop, 2s);
	LOG_INFO << "hello2睡醒了";
	co_return 2;
}

Future<int> hello()
{
	LOG_INFO << "hello开始等待1和2";
	auto v = co_await when_any(hello1(), hello2());
	LOG_INFO << "hello看到 " << std::get<0>(v) << " 睡醒了的结果之和";
	co_return std::get<0>(v);
}

int main()
{
	auto t = hello();
	run_future(loop, t);
	LOG_INFO << "主函数中得到hello结果: " << t.coro.promise().result();
	return 0;
}