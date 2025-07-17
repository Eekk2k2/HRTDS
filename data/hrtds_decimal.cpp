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