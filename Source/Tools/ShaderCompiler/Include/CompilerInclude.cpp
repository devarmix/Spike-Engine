#include <CompilerInclude.h>
#include <iostream>

bool ShaderCompiler::ValidateBinary(std::ifstream& stream) {

	const char MAGIC_ID[4] = { 'S', 'C', 'B', 'F' };

	char magicID[4] = {};
	stream.read((char*)&magicID, sizeof(char) * 4);
	return memcmp(magicID, MAGIC_ID, sizeof(char) * 4) == 0;
}

namespace ShaderCompiler {

	void SerializeStringVector(const std::vector<std::string>& vector, std::ofstream& stream) {

		uint8_t size = (uint8_t)vector.size();
		stream.write((char*)&size, sizeof(uint8_t));

		for (auto& s : vector) {

			uint8_t strSize = (uint8_t)s.size();
			stream.write((char*)&strSize, sizeof(uint8_t));
			stream.write(s.c_str(), sizeof(char) * s.size());
		}
	}

	void DeserializeStringVector(std::vector<std::string>& out, std::ifstream& stream) {

		uint8_t size = 0;
		stream.read((char*)&size, sizeof(uint8_t));

		for (int i = 0; i < size; i++) {

			uint8_t strSize = 0;
			stream.read((char*)&strSize, sizeof(uint8_t));

			std::string str(strSize, 0);
			stream.read(&str[0], sizeof(char) * strSize);
			out.push_back(str);
		}
	}

	void SerializeByteVector(const std::vector<char>& vector, std::ostream& stream) {

		size_t size = vector.size();
		stream.write((char*)&size, sizeof(size_t));
		stream.write(vector.data(), size);
	}

	void DeserializeByteVector(std::vector<char>& out, std::ifstream& stream) {

		size_t size = 0;
		stream.read((char*)&size, sizeof(size_t));

		out.resize(size);
		stream.read(out.data(), size);
	}

	void BinaryShader::Serialize(std::ofstream& stream) const {

		stream.write((char*)&Hash, sizeof(size_t));
		SerializeStringVector(MaterialData.PScalar, stream);
		SerializeStringVector(MaterialData.PUint, stream);
		SerializeStringVector(MaterialData.PVec2, stream);
		SerializeStringVector(MaterialData.PVec4, stream);
		SerializeStringVector(MaterialData.PTexture, stream);
		{
			size_t size = ShaderData.Bindings.size();
			stream.write((char*)&size, sizeof(size_t));

			for (auto& b : ShaderData.Bindings) {

				stream.write((char*)&b.Type, sizeof(uint8_t));
				stream.write((char*)&b.Binding, sizeof(uint32_t));
				stream.write((char*)&b.Set, sizeof(uint32_t));
				stream.write((char*)&b.Count, sizeof(uint32_t));
			}
		}
		stream.write((char*)&ShaderData.PushDataSize, sizeof(uint32_t));
		stream.write((char*)&ShaderData.IsMaterialShader, sizeof(bool));
		SerializeByteVector(VertexRange, stream);
		SerializeByteVector(PixelRange, stream);
		SerializeByteVector(ComputeRange, stream);
	}

	void BinaryShader::Deserialize(std::ifstream& stream) {

		stream.read((char*)&Hash, sizeof(size_t));
		DeserializeStringVector(MaterialData.PScalar, stream);
		DeserializeStringVector(MaterialData.PUint, stream);
		DeserializeStringVector(MaterialData.PVec2, stream);
		DeserializeStringVector(MaterialData.PVec4, stream);
		DeserializeStringVector(MaterialData.PTexture, stream);
		{
			size_t size = 0;
			stream.read((char*)&size, sizeof(size_t));

			ShaderData.Bindings.resize(size);
			for (int i = 0; i < size; i++) {

				ShaderMetadata::ShaderBinding& b = ShaderData.Bindings[i];

				stream.read((char*)&b.Type, sizeof(uint8_t));
				stream.read((char*)&b.Binding, sizeof(uint32_t));
				stream.read((char*)&b.Set, sizeof(uint32_t));
				stream.read((char*)&b.Count, sizeof(uint32_t));
			}
		}
		stream.read((char*)&ShaderData.PushDataSize, sizeof(uint32_t));
		stream.read((char*)&ShaderData.IsMaterialShader, sizeof(bool));
		DeserializeByteVector(VertexRange, stream);
		DeserializeByteVector(PixelRange, stream);
		DeserializeByteVector(ComputeRange, stream);
	}
}