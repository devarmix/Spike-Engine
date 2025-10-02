#pragma once

#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <string>

namespace Spike {

	class BinaryReadStream {
	public:
		BinaryReadStream(const std::filesystem::path& path) : m_Stream(path, std::ios::binary) {}
		~BinaryReadStream() { m_Stream.close(); }

		void ReadRaw(void* out, size_t size) { m_Stream.read((char*)out, size); }
		bool IsOpen() const { return m_Stream.is_open(); }

		template<typename T>
		friend BinaryReadStream& operator>>(BinaryReadStream& stream, T& t) {
			stream.ReadRaw((void*)&t, sizeof(T));
			return stream;
		}

		friend BinaryReadStream& operator>>(BinaryReadStream& stream, std::string& str) {

			size_t size = 0;
			stream >> size;

			str.resize(size);
			stream.ReadRaw(str.data(), size);

			return stream;
		}

		template<typename T>
		friend BinaryReadStream& operator>>(BinaryReadStream& stream, std::vector<T>& v) {

			size_t size = 0;
			stream >> size;

			v.resize(size);
			if constexpr (std::is_trivial<T>()) {
				stream.ReadRaw(v.data(), sizeof(T) * size);
			}
			else {
				for (size_t i = 0; i < size; i++) {
					stream >> v[i];
				}
			}
			return stream;
		}

		template<typename T, typename U, typename H = std::hash<T>>
		friend BinaryReadStream& operator>>(BinaryReadStream& stream, std::unordered_map<T, U, H>& m) {

			size_t size = 0;
			stream >> size;

			m.reserve(size);
			for (size_t i = 0; i < size; i++) {

				T k{};
				stream >> k;
				stream >> m[k];
			}
			return stream;
		}

	private:
		std::ifstream m_Stream;
	};

	class BinaryWriteStream {
	public:
		BinaryWriteStream(const std::filesystem::path& path) : m_Stream(path, std::ios::trunc | std::ios::binary | std::ios::out) {}
		~BinaryWriteStream() { m_Stream.close(); }

		void WriteRaw(const void* obj, size_t size) { m_Stream.write((const char*)obj, size); }
		bool IsOpen() const { return m_Stream.is_open(); }

		template<typename T>
		friend BinaryWriteStream& operator<<(BinaryWriteStream& stream, const T& t) {
			stream.WriteRaw((void*)&t, sizeof(T));
			return stream;
		}

		friend BinaryWriteStream& operator<<(BinaryWriteStream& stream, const std::string& str) {
			stream << str.size();
			stream.WriteRaw(str.data(), str.size());

			return stream;
		}

		template<typename T>
		friend BinaryWriteStream& operator<<(BinaryWriteStream& stream, const std::vector<T>& v) {

			stream << v.size();
			if constexpr (std::is_trivial<T>()) {
				stream.WriteRaw(v.data(), sizeof(T) * v.size());
			}
			else {
				for (auto& t : v) {
					stream << t;
				}
			}
			return stream;
		}

		template<typename T, typename U, typename H = std::hash<T>>
		friend BinaryWriteStream& operator<<(BinaryWriteStream& stream, const std::unordered_map<T, U, H>& m) {

			stream << m.size();
			for (auto& [k, v] : m) {
				stream << k;
				stream << v;
			}
			return stream;
		}

	private:
		std::ofstream m_Stream;
	};
}