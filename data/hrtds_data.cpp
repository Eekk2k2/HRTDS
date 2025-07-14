#include ".\hrtds_data.h"

void hrtds::data::DynamicConverter::Register(const std::string& key, FromStringFunction fromFunc, ToStringFunction toFunc)
{
	DynamicConverter::FromString[key] = fromFunc;
	DynamicConverter::ToString[key] = toFunc;
}