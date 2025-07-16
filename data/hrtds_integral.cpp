#include "hrtds_integral.h"

#include "..\hrtds_config.h"

#include <type_traits>
#include <limits>
#include <cstdint>

// T is the target type
// U is the source type
template <typename T, typename U>
constexpr T saturate_cast(U value) {
	static_assert(std::is_integral_v<T> && std::is_integral_v<U>, "Only supports integer types");

	if constexpr (std::is_signed_v<U> == std::is_signed_v<T>) {
		if (value > static_cast<U>(std::numeric_limits<T>::max())) return std::numeric_limits<T>::max();
		if (value < static_cast<U>(std::numeric_limits<T>::min())) return std::numeric_limits<T>::min();
		return static_cast<T>(value);
	}
	else if constexpr (std::is_signed_v<U>) { // signed -> unsigned
		if (value < 0) return 0;
		if (static_cast<std::make_unsigned_t<U>>(value) > std::numeric_limits<T>::max()) return std::numeric_limits<T>::max();
		return static_cast<T>(value);
	}
	else { // unsigned -> signed
		if (value > static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max())) return std::numeric_limits<T>::max();
		return static_cast<T>(value);
	}
}

void* hrtds::data::StaticConverter<int8_t>::FromString(const std::string& input) 
{
	int8_t* output = new int8_t();
	*output = saturate_cast<int8_t>(std::stoi(input));

	return output;
}

std::string hrtds::data::StaticConverter<int8_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const int8_t*>(data));
}

void hrtds::data::StaticConverter<int8_t>::Destroy(void* data) {
	delete reinterpret_cast<int8_t*>(data);
}

void* hrtds::data::StaticConverter<int16_t>::FromString(const std::string& input)
{
	int16_t* output = new int16_t();
	*output = saturate_cast<int16_t>(std::stoi(input));

	return output;
}

std::string hrtds::data::StaticConverter<int16_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const int16_t*>(data));
}

void hrtds::data::StaticConverter<int16_t>::Destroy(void* data) {
	delete reinterpret_cast<int16_t*>(data);
}

void* hrtds::data::StaticConverter<int32_t>::FromString(const std::string& input)
{
	int32_t* output = new int32_t();
	*output = saturate_cast<int32_t>(std::stol(input));

	return output;
}

std::string hrtds::data::StaticConverter<int32_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const int32_t*>(data));
}

void hrtds::data::StaticConverter<int32_t>::Destroy(void* data) {
	delete reinterpret_cast<int32_t*>(data);
}

void* hrtds::data::StaticConverter<int64_t>::FromString(const std::string& input)
{
	int64_t* output = new int64_t();
	*output = saturate_cast<int64_t>(std::stoll(input));

	return output;
}

std::string hrtds::data::StaticConverter<int64_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const int64_t*>(data));
}

void hrtds::data::StaticConverter<int64_t>::Destroy(void* data) {
	delete reinterpret_cast<int64_t*>(data);
}

void* hrtds::data::StaticConverter<uint8_t>::FromString(const std::string& input)
{
	uint8_t* output = new uint8_t();
	*output = saturate_cast<uint8_t>(std::stoul(input));

	return output;
}

std::string hrtds::data::StaticConverter<uint8_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const uint8_t*>(data));
}

void hrtds::data::StaticConverter<uint8_t>::Destroy(void* data) {
	delete reinterpret_cast<uint8_t*>(data);
}

void* hrtds::data::StaticConverter<uint16_t>::FromString(const std::string& input)
{
	uint16_t* output = new uint16_t();
	*output = saturate_cast<uint16_t>(std::stoul(input));

	return output;
}

std::string hrtds::data::StaticConverter<uint16_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const uint16_t*>(data));
}

void hrtds::data::StaticConverter<uint16_t>::Destroy(void* data) {
	delete reinterpret_cast<uint16_t*>(data);
}

void* hrtds::data::StaticConverter<uint32_t>::FromString(const std::string& input)
{
	uint32_t* output = new uint32_t();
	*output = saturate_cast<uint32_t>(std::stoul(input));

	return output;
}

std::string hrtds::data::StaticConverter<uint32_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const uint32_t*>(data));
}

void hrtds::data::StaticConverter<uint32_t>::Destroy(void* data) {
	delete reinterpret_cast<uint32_t*>(data);
}

void* hrtds::data::StaticConverter<uint64_t>::FromString(const std::string& input)
{
	uint64_t* output = new uint64_t();
	*output = saturate_cast<uint64_t>(std::stoull(input));

	return output;
}

std::string hrtds::data::StaticConverter<uint64_t>::ToString(const void* data) {
	return std::to_string(*reinterpret_cast<const uint64_t*>(data));
}

void hrtds::data::StaticConverter<uint64_t>::Destroy(void* data) {
	delete reinterpret_cast<uint64_t*>(data);
}

//std::vector<std::byte> hrtds::data::StaticConverter<int8_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(int8_t));
//	int8_t data = saturate_cast<int8_t>(std::stoi(input));
//
//	memcpy(buffer.data(), &data, sizeof(int8_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<int8_t>::ToString(const std::vector<std::byte>& input) {
//	int8_t data = hrtds::data::StaticConverter<int8_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<int8_t>::FromType(const int8_t& input) {
//	std::vector<std::byte> buffer(sizeof(int8_t));
//
//	memcpy(buffer.data(), &input, sizeof(int8_t));
//	return buffer;
//}
//
//int8_t hrtds::data::StaticConverter<int8_t>::ToType(const std::vector<std::byte>& input) {
//	int8_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(int8_t));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<int16_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(int16_t));
//	int16_t data = saturate_cast<int16_t>(std::stoi(input));
//
//	memcpy(buffer.data(), &data, sizeof(int16_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<int16_t>::ToString(const std::vector<std::byte>& input) {
//	int16_t data = hrtds::data::StaticConverter<int16_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<int16_t>::FromType(const int16_t& input) {
//	std::vector<std::byte> buffer(sizeof(int16_t));
//
//	memcpy(buffer.data(), &input, sizeof(int16_t));
//	return buffer;
//}
//
//int16_t hrtds::data::StaticConverter<int16_t>::ToType(const std::vector<std::byte>& input) {
//	int16_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(int16_t));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<int32_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(int32_t));
//	int32_t data = saturate_cast<int32_t>(std::stol(input));
//
//	memcpy(buffer.data(), &data, sizeof(int32_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<int32_t>::ToString(const std::vector<std::byte>& input) {
//	int32_t data = hrtds::data::StaticConverter<int32_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<int32_t>::FromType(const int32_t& input) {
//	std::vector<std::byte> buffer(sizeof(int32_t));
//
//	memcpy(buffer.data(), &input, sizeof(int32_t));
//	return buffer;
//}
//
//int32_t hrtds::data::StaticConverter<int32_t>::ToType(const std::vector<std::byte>& input) {
//	int32_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(int32_t));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<int64_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(int64_t));
//	int64_t data = saturate_cast<int64_t>(std::stoll(input));
//
//	memcpy(buffer.data(), &data, sizeof(int64_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<int64_t>::ToString(const std::vector<std::byte>& input) {
//	int64_t data = hrtds::data::StaticConverter<int64_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<int64_t>::FromType(const int64_t& input) {
//	std::vector<std::byte> buffer(sizeof(int64_t));
//
//	memcpy(buffer.data(), &input, sizeof(int64_t));
//	return buffer;
//}
//
//int64_t hrtds::data::StaticConverter<int64_t>::ToType(const std::vector<std::byte>& input) {
//	int64_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(int64_t));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint8_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(uint8_t));
//	uint8_t data = saturate_cast<uint8_t>(std::stoul(input));
//
//	memcpy(buffer.data(), &data, sizeof(uint8_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<uint8_t>::ToString(const std::vector<std::byte>& input) {
//	uint8_t data = hrtds::data::StaticConverter<uint8_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint8_t>::FromType(const uint8_t& input) {
//	std::vector<std::byte> buffer(sizeof(uint8_t));
//
//	memcpy(buffer.data(), &input, sizeof(uint8_t));
//	return buffer;
//}
//
//uint8_t hrtds::data::StaticConverter<uint8_t>::ToType(const std::vector<std::byte>& input) {
//	uint8_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(uint8_t));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint16_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(uint16_t));
//	uint16_t data = saturate_cast<uint16_t>(std::stoul(input));
//
//	memcpy(buffer.data(), &data, sizeof(uint16_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<uint16_t>::ToString(const std::vector<std::byte>& input) {
//	uint16_t data = hrtds::data::StaticConverter<uint16_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint16_t>::FromType(const uint16_t& input) {
//	std::vector<std::byte> buffer(sizeof(uint16_t));
//
//	memcpy(buffer.data(), &input, sizeof(uint16_t));
//	return buffer;
//}
//
//uint16_t hrtds::data::StaticConverter<uint16_t>::ToType(const std::vector<std::byte>& input) {
//	uint16_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(uint16_t));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint32_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(uint32_t));
//	uint32_t data = saturate_cast<uint32_t>(std::stoul(input));
//
//	memcpy(buffer.data(), &data, sizeof(uint32_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<uint32_t>::ToString(const std::vector<std::byte>& input) {
//	uint32_t data = hrtds::data::StaticConverter<uint32_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint32_t>::FromType(const uint32_t& input) {
//	std::vector<std::byte> buffer(sizeof(uint32_t));
//
//	memcpy(buffer.data(), &input, sizeof(uint32_t));
//	return buffer;
//}
//
//uint32_t hrtds::data::StaticConverter<uint32_t>::ToType(const std::vector<std::byte>& input) {
//	uint32_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(uint32_t));
//	return buffer;
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint64_t>::FromString(const std::string& input) {
//	std::vector<std::byte> buffer(sizeof(uint64_t));
//	uint64_t data = saturate_cast<uint64_t>(std::stoull(input));
//
//	memcpy(buffer.data(), &data, sizeof(uint64_t));
//	return buffer;
//}
//
//std::string hrtds::data::StaticConverter<uint64_t>::ToString(const std::vector<std::byte>& input) {
//	uint64_t data = hrtds::data::StaticConverter<uint64_t>::ToType(input);
//	return std::to_string(data);
//}
//
//std::vector<std::byte> hrtds::data::StaticConverter<uint64_t>::FromType(const uint64_t& input) {
//	std::vector<std::byte> buffer(sizeof(uint64_t));
//
//	memcpy(buffer.data(), &input, sizeof(uint64_t));
//	return buffer;
//}
//
//uint64_t hrtds::data::StaticConverter<uint64_t>::ToType(const std::vector<std::byte>& input) {
//	uint64_t buffer = 0;
//
//	memcpy(&buffer, input.data(), sizeof(uint64_t));
//	return buffer;
//}