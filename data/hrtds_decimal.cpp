#include ".\hrtds_decimal.h"

std::vector<std::byte> hrtds::data::StaticConverter<float>::FromString(const std::string& input) {
	std::vector<std::byte> buffer(sizeof(float));
	float data = std::stof(input);

	memcpy(buffer.data(), &data, sizeof(float));
	return buffer;
}

std::string hrtds::data::StaticConverter<float>::ToString(const std::vector<std::byte>& input) {
	float data = StaticConverter<float>::ToType(input);
	return std::to_string(data);
}

std::vector<std::byte> hrtds::data::StaticConverter<float>::FromType(const float& input) {
	std::vector<std::byte> buffer(sizeof(float));

	memcpy(buffer.data(), &input, sizeof(float));
	return buffer;
}

float hrtds::data::StaticConverter<float>::ToType(const std::vector<std::byte>& input) {
	float buffer = 0.0f;

	memcpy(&buffer, input.data(), sizeof(float));
	return buffer;
}

std::vector<std::byte> hrtds::data::StaticConverter<double>::FromString(const std::string& input) {
	std::vector<std::byte> buffer(sizeof(double));
	double data = std::stod(input);

	memcpy(buffer.data(), &data, sizeof(double));
	return buffer;
}

std::string hrtds::data::StaticConverter<double>::ToString(const std::vector<std::byte>& input) {
	double data = StaticConverter<double>::ToType(input);
	return std::to_string(data);
}

std::vector<std::byte> hrtds::data::StaticConverter<double>::FromType(const double& input) {
	std::vector<std::byte> buffer(sizeof(double));

	memcpy(buffer.data(), &input, sizeof(double));
	return buffer;
}

double hrtds::data::StaticConverter<double>::ToType(const std::vector<std::byte>& input) {
	double buffer = 0.0;

	memcpy(&buffer, input.data(), sizeof(double));
	return buffer;
}