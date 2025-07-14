#include ".\hrtds_misc.h"

std::vector<std::byte> hrtds::data::StaticConverter<std::string>::FromString(const std::string& input) {
	std::vector<std::byte> buffer(input.size());
	const char* data = input.data();

	memcpy(buffer.data(), data, buffer.size());
	return buffer;
}

std::string hrtds::data::StaticConverter<std::string>::ToString(const std::vector<std::byte>& input) {
	std::string data = StaticConverter<std::string>::ToType(input);
	return std::string()
		+ config::Glyph::QUOTE
		+ data
		+ config::Glyph::QUOTE;
}

std::vector<std::byte> hrtds::data::StaticConverter<std::string>::FromType(const std::string& input) {
	return StaticConverter<std::string>::FromString(input);
}

std::string hrtds::data::StaticConverter<std::string>::ToType(const std::vector<std::byte>& input) {
	std::string data = std::string();
	data.resize(input.size());

	memcpy(data.data(), input.data(), data.size());
	return data;
}

std::vector<std::byte> hrtds::data::StaticConverter<bool>::FromString(const std::string& input) {
	std::vector<std::byte> buffer(sizeof(bool));
	bool data = (input == "true" || input == "1");

	memcpy(buffer.data(), &data, sizeof(bool));
	return buffer;
}

std::string hrtds::data::StaticConverter<bool>::ToString(const std::vector<std::byte>& input) {
	bool data = StaticConverter<bool>::ToType(input);
	std::string dataStr = data ? "true" : "false";
	return dataStr;
}

std::vector<std::byte> hrtds::data::StaticConverter<bool>::FromType(const bool& input) {
	std::vector<std::byte> buffer(sizeof(bool));

	memcpy(buffer.data(), &input, sizeof(bool));
	return buffer;
}

bool hrtds::data::StaticConverter<bool>::ToType(const std::vector<std::byte>& input) {
	bool buffer = false;

	memcpy(&buffer, input.data(), sizeof(bool));
	return buffer;
}