#include "hrtds_misc.h"

#include "..\hrtds_config.h"

void* hrtds::data::StaticConverter<std::string>::FromString(const std::string& input) 
{
	std::string* output = new std::string();
	*output = input;

	return output;
}

std::string hrtds::data::StaticConverter<std::string>::ToString(const void* data) {
	const std::string* typedData = reinterpret_cast<const std::string*>(data);
	
	return std::string()
		+ config::Glyph::QUOTE
		+ *typedData
		+ config::Glyph::QUOTE;
}

void hrtds::data::StaticConverter<std::string>::Destroy(void* data) {
	delete reinterpret_cast<std::string*>(data);
}

void* hrtds::data::StaticConverter<bool>::FromString(const std::string& input)
{
	bool* output = new bool();
	*output = (input == "true" || input == "1") ? true : false;

	return reinterpret_cast<void*>(output);
}

std::string hrtds::data::StaticConverter<bool>::ToString(const void* data) {
	const bool* typedData = reinterpret_cast<const bool*>(data);
	std::string output = *typedData ? "true" : "false";

	return output;
}

void hrtds::data::StaticConverter<bool>::Destroy(void* data) {
	delete reinterpret_cast<bool*>(data);
}

//std::vector<std::byte> hrtds::data::StaticConverter<std::string>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(input.size());
//	const char* data = input.data();
//
//	memcpy(buffer.data(), data, buffer.size());
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<std::string>::ToString(const std::vector<std::byte>& input) {
//	std::string data = StaticConverter<std::string>::ToType(input);
//	return std::string()
//		+ config::Glyph::QUOTE
//		+ data
//		+ config::Glyph::QUOTE;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<std::string>::FromType(const std::string& input) {
//	return StaticConverter<std::string>::FromString(input);
//}
//
//std::string hrtds::data::StaticConverter<std::string>::ToType(const std::vector<std::byte>& input) {
//	std::string data = std::string();
//	data.resize(input.size());
//
//	memcpy(data.data(), input.data(), data.size());
//	return data;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<bool>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(bool));
//	bool data = (input == "true" || input == "1");
//
//	memcpy(buffer.data(), &data, sizeof(bool));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<bool>::ToString(const std::vector<std::byte>& input) {
//	bool data = StaticConverter<bool>::ToType(input);
//	std::string dataStr = data ? "true" : "false";
//	return dataStr;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<bool>::FromType(const bool& input) {
//	std::vector<std::byte> buffer(sizeof(bool));
//
//	memcpy(buffer.data(), &input, sizeof(bool));
//	return buffer;
//}
//
//bool hrtds::data::StaticConverter<bool>::ToType(const std::vector<std::byte>& input) {
//	bool buffer = false;
//
//	memcpy(&buffer, input.data(), sizeof(bool));
//	return buffer;
//}