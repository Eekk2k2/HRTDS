#pragma once
#include <string>

namespace hrtds {
	namespace config {
		struct IdenifierLiterals {
			static inline const std::string STRUCT_IDENTIFIER = "struct";
		};

		struct GlyphLiterals {
			static inline const std::string BEGIN_FILE_SCOPE = "${";
			static inline const std::string END_FILE_SCOPE = "}$";
		};

		struct Glyph {
			static constexpr char BEGIN_SCOPE = '{';
			static constexpr char BEGIN_TUPLE = '(';
			static constexpr char BEGIN_ARRAY = '[';
			static constexpr char END_SCOPE = '}';
			static constexpr char END_ARRAY = ']';
			static constexpr char END_TUPLE = ')';

			static constexpr char IDENTIFIER = '&';
			static constexpr char ASSIGNMENT = ':';
			static constexpr char TERMINATOR = ';';

			static constexpr char QUOTE = '\"';
			static constexpr char LIST_SEPARATOR = ',';

			static constexpr char WHITESPACE_SPACE = ' ';
			static constexpr char WHITESPACE_NEWLINE = '\n';
			static constexpr char WHITESPACE_TAB = '\t';
		};
	};
}