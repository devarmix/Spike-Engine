#pragma once

#include <memory>
#include <stack>
#include <bitset>
#include <mutex>
#include <vector>

namespace Spike {

	template<typename T>
	void SwapDelete(std::vector<T>& v, uint32_t idx) {
		if (v.size() > 0) {
			v[idx] = std::move(v.back());
		}
		v.pop_back();
	}

	struct IndexQueue {
		IndexQueue() : NextIndex(0) {}

		uint32_t Grab() {
			uint32_t index = NextIndex;

			if (Queue.size() > 0) {
				index = Queue.back();
				Queue.pop_back();
			}
			else {
				NextIndex++;
			}
			return index;
		}

		void Release(uint32_t index) { Queue.push_back(index); }

	private:
		uint32_t NextIndex;
		std::vector<uint32_t> Queue;
	};

	template<typename T>
	class DenseBuffer {
	public:
		DenseBuffer()
			: m_Nodes(nullptr), m_FreeHead(INVALID_IDX), m_Tail(INVALID_IDX), m_NextNode(0), m_Size(0), m_Ptr(nullptr) {}
		DenseBuffer(T* ptr, uint32_t maxSize)
			: m_Nodes(new Node[maxSize]), m_FreeHead(INVALID_IDX), m_Tail(INVALID_IDX), m_NextNode(0), m_Size(0), m_Ptr(ptr) {}
		DenseBuffer(const DenseBuffer& copy) = delete;

		DenseBuffer(DenseBuffer&& move) noexcept {
			m_Nodes = move.m_Nodes;
			m_Ptr = move.m_Ptr;
			m_FreeHead = move.m_FreeHead;
			m_NextNode = move.m_NextNode;
			m_Size = move.m_Size;
			m_Tail = move.m_Tail;
		}
		~DenseBuffer() { if (m_Nodes) delete[] m_Nodes; }

		void Make(T* ptr, uint32_t maxSize) {
			if (m_Nodes) delete[] m_Nodes;

			m_Nodes = new Node[maxSize];
			m_Ptr = ptr;
			m_FreeHead = INVALID_IDX;
			m_Tail = INVALID_IDX;
			m_NextNode = 0;
			m_Size = 0;
		}

		uint32_t Push(T&& element) {
			uint32_t offset = 0;
			uint32_t nIndex = PushInternal(offset);

			m_Ptr[offset] = std::move(element);
			return nIndex;
		}

		uint32_t Push(const T& element) {
			uint32_t offset = 0;
			uint32_t nIndex = PushInternal(offset);

			m_Ptr[offset] = element;
			return nIndex;
		}

		void Pop(uint32_t idx) {
			if (m_Size > 0 && idx < m_Size) {

				Node& elNode = m_Nodes[idx];
				Node& tailNode = m_Nodes[m_Tail];

				m_Ptr[elNode.MemOffset] = std::move(m_Ptr[tailNode.MemOffset]);
				m_Tail = tailNode.Prev;

				tailNode.Prev = elNode.Prev;
				tailNode.MemOffset = elNode.MemOffset;
				elNode.NextFree = m_FreeHead;
				m_FreeHead = idx;
				m_Size--;
			}
		}

		template<typename U>
		void ForEach(U&& iterator) {
			uint32_t nodeIdx = m_Tail;

			for (uint32_t i = 0; i < m_Size; i++) {
				// chech if node is not free (valid)
				Node& n = m_Nodes[nodeIdx];
				if (n.NextFree != INVALID_IDX) {
					iterator(m_Ptr[n.MemOffset], nodeIdx);
					nodeIdx = n.Prev;
				}
			}
		}

		uint32_t IndexToRawOffset(uint32_t idx) {
			return m_Nodes[idx].MemOffset;
		}

		uint32_t Size() const { return m_Size; }
		T& operator[](uint32_t idx) { return m_Ptr[m_Nodes[idx].MemOffset]; }

	private:
		uint32_t PushInternal(uint32_t& offset) {
			uint32_t nIndex = m_NextNode;

			if (m_FreeHead != INVALID_IDX) {
				nIndex = m_FreeHead;
				m_FreeHead = m_Nodes[m_FreeHead].NextFree;
			}
			else {
				m_NextNode++;
			}

			Node& elNode = m_Nodes[nIndex];
			elNode.Prev = m_Tail;
			elNode.NextFree = INVALID_IDX;
			elNode.MemOffset = m_Size;

			m_Tail = nIndex;
			m_Size++;
			offset = elNode.MemOffset;
			return nIndex;
		}

	private:
		struct Node {
			uint32_t Prev;
			uint32_t NextFree;
			uint32_t MemOffset;
		};

		Node* m_Nodes;
		uint32_t m_FreeHead;
		uint32_t m_NextNode;
		uint32_t m_Tail;

		uint32_t m_Size;
		T* m_Ptr;

		inline static constexpr uint32_t INVALID_IDX = UINT32_MAX;
	};

	// not thread safe implementation of ref counted pointer
	class RefCounted {
	public:
		virtual ~RefCounted() = default;

		virtual void AddRef() const = 0;
		virtual void Release() const = 0;

		uint32_t GetRefCount() const { return m_Counter; }
	protected:
		mutable uint32_t m_Counter{ 0 };
	};

	// for RefCounted derived classes
	template<typename T>
	class Ref {
	public:
		Ref() : m_Ptr(nullptr) {}
		Ref(T* ptr) {
			m_Ptr = ptr;
			if (m_Ptr) {
				m_Ptr->AddRef();
			}
		}

		Ref(const Ref& copy) {
			m_Ptr = copy.m_Ptr;
			if (m_Ptr) {
				m_Ptr->AddRef();
			}
		}

		template<typename CopyT>
		explicit Ref(const Ref<CopyT>& copy) {
			m_Ptr = static_cast<T*>(copy.Get());
			if (m_Ptr) {
				m_Ptr->AddRef();
			}
		}

		template<typename CopyT>
		Ref<CopyT> As() const {
			return Ref<CopyT>(*this);
		}

		Ref(Ref&& move) noexcept {
			m_Ptr = move.m_Ptr;
			move.m_Ptr = nullptr;
		}

		template<typename MoveT>
		explicit Ref(Ref<MoveT>&& move) {
			m_Ptr = static_cast<T*>(move.Get());
			move.m_Ptr = nullptr;
		}

		~Ref() {
			if (m_Ptr) {
				m_Ptr->Release();
			}
		}

		Ref& operator=(T* ptr) {
			T* old = m_Ptr;
			m_Ptr = ptr;

			if (m_Ptr) {
				m_Ptr->AddRef();
			}
			if (old) {
				old->Release();
			}

			return *this;
		}

		Ref& operator=(const Ref& other) {
			T* old = m_Ptr;
			m_Ptr = other.m_Ptr;

			if (m_Ptr) {
				m_Ptr->AddRef();
			}
			if (old) {
				old->Release();
			}

			return *this;
		}

		template<typename CopyT>
		Ref& operator=(const Ref<CopyT>& other) {
			T* old = m_Ptr;
			m_Ptr = (T*)other.Get();

			if (m_Ptr) {
				m_Ptr->AddRef();
			}
			if (old) {
				old->Release();
			}

			return *this;
		}


		Ref& operator=(Ref&& move) {
			if (this != &move) {

				T* old = m_Ptr;
				m_Ptr = move.m_Ptr;
				move.m_Ptr = nullptr;

				if (old) {
					old->Release();
				}
			}

			return *this;
		}

		T* Get() const { return m_Ptr; }

		T* operator->() const {
			return m_Ptr;
		}

		operator T* () const {
			return m_Ptr;
		}

		bool operator==(const Ref<T>& other) const {
			return m_Ptr == other.m_Ptr;
		}

		bool operator==(T* other) const {
			return m_Ptr == other;
		}

		bool Valid() { return m_Ptr != nullptr; }

	private:
		mutable T* m_Ptr;

		template<typename OtherT>
		friend class Ref;
	};

	template<typename T, typename... Args>
	Ref<T> CreateRef(Args&&... args) {
		return Ref<T>(new T(std::forward<Args>(args)...));
	}

	template<typename T>
	class ThreadSafeQueue {
	public:
		ThreadSafeQueue() {}
		~ThreadSafeQueue() { m_Queue.clear(); }

		void Push(T&& element) {
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Queue.push_back(std::move(element));
		}

		void Push(const T& element) {
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Queue.push_back(element);
		}

		void PopAll(std::vector<T>& other) {
			std::lock_guard<std::mutex> lock(m_Mutex);
			other = std::move(m_Queue);
			m_Queue.clear();
		}

	private:
		std::vector<T> m_Queue;
		std::mutex m_Mutex;
	};
}