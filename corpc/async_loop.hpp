#ifndef CORPC_ASYNC_LOOP_HPP
#define CORPC_ASYNC_LOOP_HPP

#include "corpc/epoll_loop.hpp"
#include "corpc/timer_loop.hpp"
#include <thread>

namespace corpc
{
struct AsyncLoop
{
	TimerLoop tloop;
	EpollLoop eloop;

	operator TimerLoop&()
	{
		return tloop;
	}

	operator EpollLoop&()
	{
		return eloop;
	}

	void run()
	{
		while (true)
		{
			auto timeout = tloop.run();
			if (!eloop.empty())
			{
				eloop.run(timeout);
			}
			else if (timeout.has_value())
			{
				std::this_thread::sleep_for(timeout.value());
			}
			else
			{
				break;
			}
		}
	}
};
} // namespace corpc

#endif
