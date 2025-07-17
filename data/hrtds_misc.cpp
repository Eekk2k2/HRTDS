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