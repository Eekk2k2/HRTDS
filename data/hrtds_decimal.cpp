#include "hrtds_decimal.h"

#include "..\hrtds_config.h"

void* hrtds::data::StaticConverter<float>::FromString(const std::string& input) 
{
	float* output = new float();
	*output = std::stof(input);

	return output;
}

std::string hrtds::data::StaticConverter<float>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const float*>(data));
}

void hrtds::data::StaticConverter<float>::Destroy(void* data) {
	delete reinterpret_cast<float*>(data);
}

void* hrtds::data::StaticConverter<double>::FromString(const std::string& input) 
{
	double* output = new double();
	*output = std::stod(input);

	return output;
}

std::string hrtds::data::StaticConverter<double>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const double*>(data));
}

void hrtds::data::StaticConverter<double>::Destroy(void* data) {
	delete reinterpret_cast<double*>(data);
}

//std::vector<std::byte> hrtds::data::StaticConverter<float>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(float));
//	float data = std::stof(input);
//
//	memcpy(buffer.data(), &data, sizeof(float));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<float>::ToString(const std::vector<std::byte>& input) {
//	float data = StaticConverter<float>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<float>::FromType(const float& input) {
//	std::vector<std::byte> buffer(sizeof(float));
//
//	memcpy(buffer.data(), &input, sizeof(float));
//	return buffer;
//}
//
//float hrtds::data::StaticConverter<float>::ToType(const std::vector<std::byte>& input) {
//	float buffer = 0.0f;
//
//	memcpy(&buffer, input.data(), sizeof(float));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<double>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(double));
//	double data = std::stod(input);
//
//	memcpy(buffer.data(), &data, sizeof(double));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<double>::ToString(const std::vector<std::byte>& input) {
//	double data = StaticConverter<double>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<double>::FromType(const double& input) {
//	std::vector<std::byte> buffer(sizeof(double));
//
//	memcpy(buffer.data(), &input, sizeof(double));
//	return buffer;
//}
//
//double hrtds::data::StaticConverter<double>::ToType(const std::vector<std::byte>& input) {
//	double buffer = 0.0;
//
//	memcpy(&buffer, input.data(), sizeof(double));
//	return buffer;
//}