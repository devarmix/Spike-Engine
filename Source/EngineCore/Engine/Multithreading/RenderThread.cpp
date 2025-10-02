#include <Engine/Multithreading/RenderThread.h>

namespace Spike {

	void RenderThread::Terminate() {
		PushTask([this]() { m_ShouldTerminate = true; });
		Process();
	}

	void RenderThread::WaitTillDone() {
		m_WaitSemaphore.acquire();
	}

	void RenderThread::Process() {
		m_RTQueue = std::move(m_Queue);
		m_Queue.clear();

		m_BlockSemaphore.release();
	}

	void RenderThread::RenderThreadLoop() {
		while (true) {
			m_BlockSemaphore.acquire();

			for (auto& task : m_RTQueue) {
				task();
			}

			// signal end of processing
			m_WaitSemaphore.release();
			if (m_ShouldTerminate) break;
		}
	}
}