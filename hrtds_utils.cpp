#include "hrtds_utils.h"

#include ".\hrtds_config.h"

void hrtds::utils::Trim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char chr) {
		return !std::isspace(chr);
		}));

	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char chr) {
		return !std::isspace(chr);
		}).base(), s.end());
}

std::vector<size_t> hrtds::utils::RetrieveSameLevelSeparators(const std::string& content)
{
	// Find every list separator on the same level
	std::vector<size_t> listSeparators;
	int level = 0;
	for (size_t i = 0; i < content.size(); i++)
	{
		char current = content[i];
		switch (current)
		{
		case config::Glyph::BEGIN_SCOPE:
		case config::Glyph::BEGIN_ARRAY:
		case config::Glyph::BEGIN_TUPLE: {
			level++; break;
		}

		case config::Glyph::END_SCOPE:
		case config::Glyph::END_ARRAY:
		case config::Glyph::END_TUPLE: {
			level--; break;
		}

		case config::Glyph::LIST_SEPARATOR: {
			if (level == 0) {
				listSeparators.push_back(i);
			}

			break;
		}

		default: break;
		}
	}

	return listSeparators;
}