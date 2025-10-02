#pragma once

#include <thread>
#include <functional>
#include <semaphore>
#include <mutex>

namespace Spike {

	class RenderThread {
	public:
		RenderThread() { m_Thread = std::thread(&RenderThread::RenderThreadLoop, this); }
		~RenderThread() {}

		void Join() { m_Thread.join(); }
		void Terminate();

		using Func = std::function<void()>;
		void PushTask(Func&& func) { m_Queue.push_back(std::move(func)); }

		void WaitTillDone();
		void Process();

		std::thread::id GetID() const { return m_Thread.get_id(); }

	private:
		void RenderThreadLoop();

	private:
		std::thread m_Thread;
		std::vector<Func> m_Queue;
		std::vector<Func> m_RTQueue;
		//std::mutex m_Mutex;

		std::binary_semaphore m_BlockSemaphore{ 0 };
		std::binary_semaphore m_WaitSemaphore{ 1 };
		//std::binary_semaphore m_TerminateSemaphore{ 0 };
		bool m_ShouldTerminate = false;
	};
}