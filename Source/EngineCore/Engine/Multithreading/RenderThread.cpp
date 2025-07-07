#include <Engine/Multithreading/RenderThread.h>

namespace Spike {

	void RenderThread::Terminate() {

		// create termination task
		PushTask([this]() { m_ShouldTerminate = true; });
		Process();
	}

	void RenderThread::PushTask(Func&& func) {

		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_Queue.push_back(std::forward<Func>(func));
	}

	void RenderThread::WaitTillDone() {

		m_WaitSemaphore.acquire();
	}

	void RenderThread::Process() {

		m_BlockSemaphore.release();
	}

	void RenderThread::RenderThreadLoop() {

		while (true) {

			m_BlockSemaphore.acquire();

			std::vector<Func> tasks;
			{
				std::lock_guard<std::mutex> lock(m_QueueMutex);
				tasks = m_Queue;
				m_Queue.clear();
			}

			for (Func& task : tasks) {

				task();
			}

			// signal end of processing
			m_WaitSemaphore.release();
			if (m_ShouldTerminate) break;
		}
	}
}