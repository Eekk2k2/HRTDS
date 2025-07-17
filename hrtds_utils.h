#pragma once
#include <string>
#include <vector>

namespace hrtds {
	namespace utils {
		void Trim(std::string& s); // https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
		std::vector<size_t> RetrieveSameLevelSeparators(const std::string& content);
	};
}