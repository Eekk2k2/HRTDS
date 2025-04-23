#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <bit>
#include <sstream>
#include <string>
#include <algorithm>

namespace hrtds {
	namespace utils {
		const std::string HRTDS_BEGIN_HARD = "${";
		const std::string HRTDS_END_HARD = "}$";
		const char HRTDS_BEGIN_SOFT = '{';
		const char HRTDS_END_SOFT = '}';

		const char HRTDS_IDENTIFIER = '&';
		const char HRTDS_ASSIGNMENT = ':';
		const char HRTDS_BEGIN_ARRAY = '[';
		const char HRTDS_END_ARRAY = ']';
		const char HRTDS_TERMINATOR = ';';
		const char HRTDS_QUOTE = '\"';
		const char HRTDS_BEGIN_TUPLE = '(';
		const char HRTDS_END_TUPLE = ')';
		const char HRTDS_LIST_SEPARATOR = ',';

		const char HRTDS_WHITESPACE_SPACE = ' ';
		const char HRTDS_WHITESPACE_NEWLINE = '\n';

		// https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
		void Trim(std::string& input);
	};

	template <typename T>
	class StructFile {
	public:
		StructFile(std::string path);
		StructFile(const StructFile& other) = delete;
		~StructFile();

		StructFile& operator=(const StructFile& other) = delete;

		void Flush();

		T* data;
	private:
		HANDLE hFile, hFileMapping;
	};

	template<typename T>
	inline StructFile<T>::StructFile(std::string path)
	{
		std::wstring wpath = std::wstring(path.begin(), path.end());
		LPCTSTR wpathcstr = wpath.c_str();
		this->hFile = CreateFile(
			wpathcstr,
			GENERIC_READ | GENERIC_WRITE,
			0, NULL,
			OPEN_ALWAYS,
			0, 0
		);

		if (this->hFile == INVALID_HANDLE_VALUE) {
			std::cerr << "Failed to create file handle: " << GetLastError() << std::endl;

			return;
		}

		this->hFileMapping = CreateFileMapping(
			this->hFile,
			NULL, PAGE_READWRITE, 0,
			sizeof(T), NULL
		);

		if (this->hFileMapping == nullptr) {
			std::cerr << "Failed to create file mapping: " << GetLastError() << std::endl;

			return;
		}

		this->data = reinterpret_cast<T*>(MapViewOfFile(
			this->hFileMapping,
			FILE_MAP_ALL_ACCESS,
			0, 0, sizeof(T)
		));

		if (!this->data) {
			std::cout << "Could not create a file mapping view" << std::endl;

			return;
		}
	}

	template<typename T>
	inline StructFile<T>::~StructFile()
	{
		this->Flush();

		UnmapViewOfFile(this->data);

		if (this->hFileMapping) {
			CloseHandle(this->hFileMapping);
		}

		if (this->hFile != INVALID_HANDLE_VALUE) {
			CloseHandle(this->hFile);
		}
	}

	template<typename T>
	inline void StructFile<T>::Flush()
	{
		FlushViewOfFile(this->data, sizeof(T));
		FlushFileBuffers(this->hFile);
	}

	static void Print_BeginHardNotFound() {
		std::cerr << "Could not find the hard begin '"
			<< utils::HRTDS_BEGIN_HARD << "'." << std::endl;
	}

	static void Print_EndHardNotFound(size_t pos) {
		std::cerr << "Could not find the hard end '"
			<< utils::HRTDS_END_HARD << "' from " << pos << std::endl;
	}

	static void Print_TerminatingIdentifierNotFound(size_t pos) {
		std::cerr << "Terminating identifier '" << utils::HRTDS_IDENTIFIER
			<< "' not found from position: " << pos << std::endl;
	}

	static void Print_AssignmentOperatorNotFound(size_t pos) {
		std::cerr << "Assignment operator '" << utils::HRTDS_ASSIGNMENT
			<< "' not found from position: " << pos << std::endl;
	}

	static void Print_TerminatorNotFound(size_t pos) {
		std::cerr << "The terminator '" << utils::HRTDS_TERMINATOR
			<< "' could not be found from: " << pos << std::endl;
	}

	static void Print_WhitespaceNotAllowed(size_t pos) {
		std::cerr << "Whitespace is not allowed here: " << pos << std::endl;
	}

	static void Print_UnexpectedCharacter(char character, size_t pos) {
		std::cerr << "Unexpected character '" << character << "' "
			<< "at position: " << pos << std::endl;
	}

	static void Print_IdentifierNotFound(std::string identifier, size_t pos) {
		std::cerr << "Could not find identifier: '" << identifier
			<< "' which is located at: " << pos << std::endl;
	}

	static void Print_ClosingQuoteNotFound(size_t pos) {
		std::cerr << "Could not find the closing quote '" << utils::HRTDS_QUOTE << "' "
			<< "for the quote at: " << pos << std::endl;
	}

	static void Print_TupleBeginNotFound(std::string tuple) {
		std::cerr << "Could not find the tuple begin char '" << utils::HRTDS_BEGIN_TUPLE
			<< "' in tuple: '" << tuple << "'" << std::endl;
	}

	static void Print_TupleEndNotFoundFromStart(std::string tuple, size_t start) {
		std::cerr << "Could not find the end tuple char '" << utils::HRTDS_END_TUPLE
			<< "' in tuple: '" << tuple << "' from pos: " << start << std::endl;
	}

	static void Print_StructIdentifierNotFound(std::string structIdentifierString) {
		std::cerr << "Could not find the structure identifier '" << structIdentifierString
			<< "' " << std::endl;
	}

	enum HRTDS_CHARACTER_VALIDITY : int {
		VALIDITY_IGNORE,
		VALIDITIY_BREAK,
		VALIDITY_INSERT
	};

	enum HRTDS_STANDARD_IDENTIFIER_TYPES {
		// References a structure of data types
		IDENTIFIER_STRUCT,
		IDENTIFIER_STRUCT_DEFINITION,

		// References data types
		IDENTIFIER_STRING,
		IDENTIFIER_BOOL,
		IDENTIFIER_INT,
		IDENTIFIER_FLOAT
	};

	// The text name of the different types, used
	// in the association map
	typedef std::string
		HRTDS_STANDARD_IDENTIFIER_NAME;

	typedef std::unordered_map<HRTDS_STANDARD_IDENTIFIER_NAME, HRTDS_STANDARD_IDENTIFIER_TYPES>
		HRTDS_STANDARD_IDENTIFIER_BY_NAME_MAP;

	// &struct& Version : { ...
	// this:----^^^^^^^
	typedef std::string
		HRTDS_STRUCTURE_KEY;

	typedef struct HRTDS_IDENTIFIER_PAIR {
		HRTDS_STANDARD_IDENTIFIER_TYPES type;

		// Only used when type == STRUCT
		HRTDS_STRUCTURE_KEY structureKey;
	} HRTDS_IDENTIFIER_PAIR;

	//           &int& Age : 32;
	//  this:----------^^^
	typedef std::string
		HRTDS_FIELD_NAME;

	//           &int& Age : 32;
	//  this:-----^^^--^^^
	typedef struct HRTDS_LAYOUT_ELEMENT {
		HRTDS_IDENTIFIER_PAIR identifierPair; // &int&
		bool isArray;
		HRTDS_FIELD_NAME fieldName; // Age
	} HRTDS_LAYOUT_ELEMENT;


	// these:-----------------------|
	// |
	// &struct& Version : { |
	// &float& Date; <---|
	// &int[]& Version; <---|
	// &string& Download; <---|
	// };
	typedef std::vector<HRTDS_LAYOUT_ELEMENT>
		HRTDS_LAYOUT;

	// Association associates "this" with "these"
	//
	// these:-----------------------|
	// |
	// this:------------| |
	// \_______/ |
	// &struct& Version : { |
	// &float& Date; <---|
	// &int& Version; <---|
	// &string& Download; <---|
	// };
	typedef std::unordered_map<HRTDS_STRUCTURE_KEY, HRTDS_LAYOUT>
		HRTDS_LAYOUT_BY_STRUCT_KEY_MAP;

	class HRTDS_VALUE; // Forward-declaration
	// Used in code to represent this struct, with its data filled in:
	//
	// &struct& Version : {
	// &float& Date;
	// &int& Version;
	// &string& Download;
	// };
	typedef std::unordered_map<HRTDS_STRUCTURE_KEY, HRTDS_VALUE>
		HRTDS_STRUCTURE;

	/* Functions used in HRTDS_VALUE::Parse_Value(...) */
	static void SetBytes_String(std::string inputString, std::vector<std::byte>& bytes);
	static void SetBytes_Bool(std::string inputString, std::vector<std::byte>& bytes);
	static void SetBytes_Int(std::string inputString, std::vector<std::byte>& bytes);
	static void SetBytes_Float(std::string inputString, std::vector<std::byte>& bytes);

	typedef void(*SetBytesFunc)(std::string, std::vector<std::byte>&);
	static std::unordered_map<HRTDS_STANDARD_IDENTIFIER_TYPES, SetBytesFunc> SetBytes{
	{ IDENTIFIER_STRING, &SetBytes_String },
	{ IDENTIFIER_BOOL, &SetBytes_Bool },
	{ IDENTIFIER_INT, &SetBytes_Int },
	{ IDENTIFIER_FLOAT, &SetBytes_Float }
	};

	void hrtds::SetBytes_String(std::string inputString, std::vector<std::byte>& bytes)
	{
		// Prepare buffer
		bytes.clear();
		bytes.resize(inputString.size());

		// Retrieve data
		char* data = inputString.data();

		// Publish data
		memcpy(bytes.data(), data, bytes.size());
	}

	void hrtds::SetBytes_Bool(std::string inputString, std::vector<std::byte>& bytes)
	{
		// Prepare buffer
		bytes.clear();
		bytes.resize(sizeof(bool));

		// Retrieve data
		bool data = (inputString == "true" || inputString == "1");
		// Publish data
		memcpy(bytes.data(), &data, bytes.size());
	}

	void hrtds::SetBytes_Int(std::string inputString, std::vector<std::byte>& bytes)
	{
		// Prepare buffer
		bytes.clear();
		bytes.resize(sizeof(int));

		// Retrieve data
		int data = std::stoi(inputString);

		// Publish
		memcpy(bytes.data(), &data, bytes.size());
	}

	void hrtds::SetBytes_Float(std::string inputString, std::vector<std::byte>& bytes)
	{
		// Prepare buffer
		bytes.clear();
		bytes.resize(sizeof(float));

		// Retrieve data
		float data = std::stof(inputString);

		// Publish
		memcpy(bytes.data(), &data, bytes.size());
	}

	enum HRTDS_TOKEN_TYPE {
		IDENTIFIER,
		NAME,

		VALUE_SINGLE,
		VALUE_LIST_ELEMENT_START,
		VALUE_LIST_ELEMENT,
		VALUE_LIST_ELEMENT_END,

		STRUCT_BEGIN,
		STRUCT_FIELD_NAME,
		STRUCT_END
	};

	typedef struct HRTDS_TOKEN {
		HRTDS_TOKEN_TYPE tokenType;
		bool isArray;

		std::string content;
	} HRTDS_TOKEN;

	static std::string HRTDS_NEW_TOKEN_STRING(
		std::string(" ")
		+ std::string("\t")
		+ std::string("\r")
		+ std::string("\n")
		+ utils::HRTDS_IDENTIFIER
		+ utils::HRTDS_ASSIGNMENT
		+ utils::HRTDS_TERMINATOR
		//+ utils::HRTDS_LIST_SEPARATOR
	);

	inline int EncodePair(char begin, char end) {
		return (static_cast<unsigned char>(begin) << 8) | static_cast<unsigned char>(end);
	}

	static std::unordered_map<int, HRTDS_TOKEN_TYPE> CharToTokenMap{
		// &...&
		{ EncodePair(utils::HRTDS_IDENTIFIER, utils::HRTDS_IDENTIFIER), HRTDS_TOKEN_TYPE::IDENTIFIER	},
		// &...:
		{ EncodePair(utils::HRTDS_IDENTIFIER, utils::HRTDS_ASSIGNMENT), HRTDS_TOKEN_TYPE::NAME			},

		// :...;
		{ EncodePair(utils::HRTDS_ASSIGNMENT, utils::HRTDS_TERMINATOR), HRTDS_TOKEN_TYPE::VALUE_SINGLE	},

		// :...,
		{ EncodePair(utils::HRTDS_ASSIGNMENT,		utils::HRTDS_LIST_SEPARATOR ),	HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_START	},
		// ,...,
		{ EncodePair(utils::HRTDS_LIST_SEPARATOR,	utils::HRTDS_LIST_SEPARATOR	),	HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT		},
		// ,...;
		{ EncodePair(utils::HRTDS_LIST_SEPARATOR,	utils::HRTDS_TERMINATOR		),	HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_END	},

		// :{&
		{ EncodePair(utils::HRTDS_ASSIGNMENT, utils::HRTDS_IDENTIFIER),	HRTDS_TOKEN_TYPE::STRUCT_BEGIN		},
		// &...;
		{ EncodePair(utils::HRTDS_IDENTIFIER, utils::HRTDS_TERMINATOR),	HRTDS_TOKEN_TYPE::STRUCT_FIELD_NAME	},
		// ;};
		{ EncodePair(utils::HRTDS_TERMINATOR, utils::HRTDS_TERMINATOR),	HRTDS_TOKEN_TYPE::STRUCT_END		}
	};

	class HRTDS_VALUE {
	public:
		HRTDS_VALUE() = default;
		HRTDS_VALUE(
			std::vector<HRTDS_TOKEN>& tokens,
			size_t& valueTokenStart, bool isArray, HRTDS_FIELD_NAME fieldName,
			HRTDS_IDENTIFIER_PAIR identifierPair, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap
		) {
			this->Set(tokens, valueTokenStart, isArray, fieldName, identifierPair, layoutByStructKeyMap);
		}

		~HRTDS_VALUE() = default;

		HRTDS_STRUCTURE& operator[](size_t index) {
			return this->structures[0];
		}

		HRTDS_VALUE& operator[](std::string key) {
			return this->structures[0][key];
		}

		void Set(
			std::vector<HRTDS_TOKEN>& tokens, size_t& valueStart,
			bool isArray, HRTDS_FIELD_NAME fieldName,
			HRTDS_IDENTIFIER_PAIR identifierPair, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap
		);

		template <typename T>
		T Get(size_t index) {
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

			// Verify correct type
			// ...

			// Verify if array
			// ...

			// Return data
			T value = T();
			memcpy(&value, this->data[index].data(), this->data[index].size());

			return value;
		}

		template <>
		std::string Get<std::string>(size_t index) {
			std::string value = std::string();
			value.resize(this->data[index].size());
			memcpy(value.data(), this->data[index].data(), this->data[index].size());

			return value;
		}

		template <typename T>
		T Get() {
			return this->Get<T>(0);
		}

		template <typename T>
		std::vector<T> Get_Vector() {
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

			// Verify correct type
			// ...

			// Verify if array
			// ...

			// Return data
			std::vector<T> values;
			values.resize(this->data.size());

			for (size_t i = 0; i < this->data.size(); i++)
			{
				values[i] = this->Get<T>(i);
			}

			return values;
		}

	private:
		HRTDS_FIELD_NAME name;
		HRTDS_IDENTIFIER_PAIR identifierPair;

		/* One of these are the value */
		bool isArray;

		// Filled if this->type != struct
		std::vector<std::vector<std::byte>> data;

		// [<value1>, <value2>, <value3>]
		void Parse_Array(
			std::vector<HRTDS_TOKEN>& tokens, size_t& valueTokenStart,
			const HRTDS_IDENTIFIER_PAIR& identifierPair, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap
		);

		// <value>
		void Parse_Value(std::string value, HRTDS_STANDARD_IDENTIFIER_TYPES valueType, size_t index);

		// Empty if this->type != struct
		std::vector<HRTDS_STRUCTURE> structures;

		// (<value1>, <value2>, <value3>)
		void Parse_Structure_Tuple(
			std::vector<HRTDS_TOKEN>& tokens, size_t& valueTokenStart,
			const HRTDS_STRUCTURE_KEY& structureKey, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap,
			size_t index
		);
	};

	// The main class, this is the root of the file structure
	//    
	//    <root>
	// HRTDS
	//  |
	//  |
	// "HRTDS_STRUCUTRE" (map linking name with value)
	//  |
	//  |   <name>   <value>
	//  \-------["value1"]-----> HRTDS_VALUE
	//  |
	//  |   <name>   <value>
	//  \-------["value2"]-----> HRTDS_VALUE
	//  |
	//  |   <name>   <value>
	//  \-------["value3"]-----> HRTDS_VALUE
	//    

	class HRTDS
	{
	public:
		HRTDS() = default;
		HRTDS(const std::string& content);
		~HRTDS() = default;

		HRTDS_VALUE& operator[](std::string key);

		void Parse(std::string content);

	private:
		std::vector<HRTDS_TOKEN> TokensizeArray(HRTDS_TOKEN arrayToken);
		std::vector<HRTDS_TOKEN> TokenizeTuple(HRTDS_TOKEN tupleToken);

		HRTDS_IDENTIFIER_PAIR RetrieveIdentifier(const std::string& identifierString);

		const HRTDS_STANDARD_IDENTIFIER_BY_NAME_MAP standardIdentifierByNameMap 
		{
			{ "string", IDENTIFIER_STRING	},
			{ "bool",	IDENTIFIER_BOOL		},
			{ "int",	IDENTIFIER_INT		},
			{ "float",	IDENTIFIER_FLOAT	},
		};

		HRTDS_LAYOUT_BY_STRUCT_KEY_MAP layoutByStructKeyMap;
		void RegisterNewLayout(std::vector<HRTDS_TOKEN>& tokens, std::string fieldName, size_t& valueTokenStart);

		HRTDS_STRUCTURE structure;
	};
};

/*

-------------------------------------------------------------
Comments go here, this is an example comment for
the <filename>.human.hrtds file.

${
&struct& Version : {
&float& Date;
&int& Version;
&string& Download;
};

&string[]& Authors : ["Jenkins", "Joe"];
&int& Age : 32;

&int& Version : 4;
&bool& Developer : false;
&float& Released : 84683093;

&Version[]& Versions : [
(4029347892, 4, "https://..."),
(2374985345, 3, "https://..."),
(8323457998, 1, "https://..."),
(3453049583, 8, "https://...")
];
}$

Comments can also go here if you want, all of this
will be ignored in the compilation and parsing.

-------------------------------------------------------------

//void Parse(const std::string& content);


*/