#include ".\hrtds_data.h"

void hrtds::data::DynamicConverter::Register(const std::string& key, FromStringFunction fromFunc, ToStringFunction toFunc, DestroyFunction destroyFunc)
{
	DynamicConverter::FromString[key] = fromFunc;
	DynamicConverter::ToString[key] = toFunc;
	DynamicConverter::Destroy[key] = destroyFunc;
}
