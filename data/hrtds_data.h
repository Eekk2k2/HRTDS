#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace hrtds {
	namespace data {
		template<typename T>
		struct dependent_false : std::false_type {};

		template<typename T>
		struct StaticConverter {
			static inline std::vector<std::byte> FromString(const std::string& input) {
				static_assert(dependent_false<T>::value, "Converter<T>::FromString(const std::string&) is not implemented for this type T.");
			}

			static inline std::string ToString(const std::vector<std::byte>& input) {
				static_assert(dependent_false<T>::value, "Converter<T>::ToString(const std::vector<std::byte>&) is not implemented for this type T.");
			}

			static inline std::vector<std::byte> FromType(const T& input) {
				static_assert(dependent_false<T>::value, "Converter<T>::ToBytes(const T&) is not implemented for this type T.");
			}

			static inline T ToType(const std::vector<std::byte>& input) {
				static_assert(dependent_false<T>::value, "Converter<T>::ToType(const std::vector<std::byte>&) is not implemented for this type T.");
			}
		private:
			static inline bool _reg;
		};

#define HRTDS_DATA_STATIC_CONVERTER(Type, alias)						\
    template<>															\
    struct hrtds::data::StaticConverter<Type> {							\
        static std::vector<std::byte> FromString(const std::string&);	\
        static std::string ToString(const std::vector<std::byte>&);		\
																		\
        static std::vector<std::byte> FromType(const Type& input);		\
        static Type ToType(const std::vector<std::byte>& input);		\
    private:															\
        static inline bool _reg = []{									\
            DynamicConverter::Register(									\
                alias,													\
                &StaticConverter<Type>::FromString,						\
                &StaticConverter<Type>::ToString						\
            );															\
																		\
            return true;												\
        }();															\
    };																	\

		typedef std::vector<std::byte>(*FromStringFunction)(const std::string&);
		typedef std::string(*ToStringFunction)(const std::vector<std::byte>&);
		struct DynamicConverter {
			static inline std::unordered_map<std::string, FromStringFunction> FromString{};
			static inline std::unordered_map<std::string, ToStringFunction> ToString{};
			static void Register(const std::string& key, FromStringFunction fromFunc, ToStringFunction toFunc);
		};
	};
};

