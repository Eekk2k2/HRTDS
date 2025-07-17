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
			static inline void* FromString(const std::string& input) {
				static_assert(dependent_false<T>::value, "Converter<T>::FromString(const std::string&) is not implemented for type T.");
			}

			static inline std::string ToString(const void* data) {
				static_assert(dependent_false<T>::value, "Converter<T>::ToString(const std::string&) is not implemented for type T.");
			}

			static inline void Destroy(void* data) {
				static_assert(dependent_false<T>::value, "Converter<T>::Destroy(void*) is not implemented for type T.");
			}
		private:
			static inline bool _reg;
		};


#define HRTDS_DATA_STATIC_CONVERTER(Type, alias)						\
    template<>															\
    struct hrtds::data::StaticConverter<Type> {							\
        static void* FromString(const std::string&);					\
        static std::string ToString(const void*);								\
        static void Destroy(void*);										\
    private:															\
        static inline bool _reg = []{									\
            DynamicConverter::Register(									\
                alias,													\
                &StaticConverter<Type>::FromString,						\
                &StaticConverter<Type>::ToString,						\
                &StaticConverter<Type>::Destroy							\
            );															\
																		\
            return true;												\
        }();															\
    };																	\

		typedef void*(*FromStringFunction)(const std::string&);
		typedef std::string(*ToStringFunction)(const void*);
		typedef void(*DestroyFunction)(void*);
		struct DynamicConverter {
			static inline std::unordered_map<std::string, FromStringFunction> FromString{};
			static inline std::unordered_map<std::string, ToStringFunction> ToString{};
			static inline std::unordered_map<std::string, DestroyFunction> Destroy{};
			static void Register(const std::string& key, FromStringFunction fromFunc, ToStringFunction toFunc, DestroyFunction destroyFunc);
		};

	};
};