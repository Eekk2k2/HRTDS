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
		void Trim(std::string& input)
		{
			input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](unsigned char chr) {
				return !std::isspace(chr);
				}));

			input.erase(std::find_if(input.rbegin(), input.rend(), [](unsigned char chr) {
				return !std::isspace(chr);
				}).base(), input.end());
		}
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
		+ utils::HRTDS_LIST_SEPARATOR
	);

	inline int EncodePair(char begin, char end) {
		return (static_cast<unsigned char>(begin) << 8) | static_cast<unsigned char>(end);
	}

	static std::unordered_map<int, HRTDS_TOKEN_TYPE> CharToTokenMap{
		// &...&
		{ EncodePair(utils::HRTDS_IDENTIFIER, utils::HRTDS_IDENTIFIER), HRTDS_TOKEN_TYPE::IDENTIFIER },
		// &...:
		{ EncodePair(utils::HRTDS_IDENTIFIER, utils::HRTDS_ASSIGNMENT), HRTDS_TOKEN_TYPE::NAME },

		// :...;
		{ EncodePair(utils::HRTDS_ASSIGNMENT, utils::HRTDS_TERMINATOR), HRTDS_TOKEN_TYPE::VALUE_SINGLE },

		// :...,
		{ EncodePair(utils::HRTDS_ASSIGNMENT, utils::HRTDS_LIST_SEPARATOR), HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_START },
		// ,...,
		{ EncodePair(utils::HRTDS_LIST_SEPARATOR, utils::HRTDS_LIST_SEPARATOR), HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT },
		// ,...;
		{ EncodePair(utils::HRTDS_LIST_SEPARATOR, utils::HRTDS_TERMINATOR), HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_END },

		// :{&
		{ EncodePair(utils::HRTDS_ASSIGNMENT, utils::HRTDS_IDENTIFIER), HRTDS_TOKEN_TYPE::STRUCT_BEGIN },
		// &...;
		{ EncodePair(utils::HRTDS_IDENTIFIER, utils::HRTDS_TERMINATOR), HRTDS_TOKEN_TYPE::STRUCT_FIELD_NAME },
		// ;};
		{ EncodePair(utils::HRTDS_TERMINATOR, utils::HRTDS_TERMINATOR), HRTDS_TOKEN_TYPE::STRUCT_END }
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
		) {
			// Update old data
			this->identifierPair = identifierPair;
			this->isArray = isArray;
			this->name = fieldName;

			// Clear old data
			this->data = std::vector<std::vector<std::byte>>();
			this->structures = std::vector<HRTDS_STRUCTURE>();

			// Parse new data
			if (this->isArray) {
				this->Parse_Array(tokens, valueStart, identifierPair, layoutByStructKeyMap);

				return;
			}

			if (this->identifierPair.type == HRTDS_STANDARD_IDENTIFIER_TYPES::IDENTIFIER_STRUCT) {
				this->Parse_Structure_Tuple(
					tokens, valueStart, identifierPair.structureKey, layoutByStructKeyMap, 0
				);

				return;
			}

			this->Parse_Value(tokens[valueStart].content, identifierPair.type, 0);
		}

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
		) {
			// Verify and retreive
			size_t valueTokenEnd = 0;
			for (size_t i = valueTokenStart; i < tokens.size(); i++)
			{
				HRTDS_TOKEN_TYPE currentTokenType = tokens[i].tokenType;
				switch (currentTokenType)
				{
				case hrtds::VALUE_LIST_ELEMENT_START:
				case hrtds::VALUE_LIST_ELEMENT: continue;

				case hrtds::VALUE_LIST_ELEMENT_END: {
					valueTokenEnd = i;

					// Break the for-loop
					i = tokens.size();
					break;
				}

				default: {
					std::cerr << "Failed to parse array as token[" << i << "] " <<
						", which is token '" << tokens[i].content << "' "
						<< "is not a valid list element." << std::endl;

					return;
				}
				}
			}

			if (valueTokenEnd == 0) {
				std::cerr << "Failed to find the last element of an array starting at token: " << valueTokenStart << std::endl;

				return;
			}

			// Set values
			bool isStructure = identifierPair.type == HRTDS_STANDARD_IDENTIFIER_TYPES::IDENTIFIER_STRUCT;
			for (size_t i = valueTokenStart; i < (valueTokenEnd + 1); i++)
			{
				if (isStructure) {
					this->Parse_Structure_Tuple(
						tokens,
						valueTokenStart,
						identifierPair.structureKey,
						layoutByStructKeyMap,
						i
					);

					continue;
				}

				std::string valueString = tokens[i].content;
				switch (tokens[i].tokenType)
				{
				case HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_START: {
					valueString.erase(valueString.begin());

					break;
				}
				case HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_END: {
					valueString.erase(valueString.end());

					break;
				}

				default: break;
				}

				this->Parse_Value(valueString, identifierPair.type, (i - valueTokenStart));
			}

			// Output
			valueTokenStart = valueTokenEnd;
		}

		// <value>
		void Parse_Value(std::string value, HRTDS_STANDARD_IDENTIFIER_TYPES valueType, size_t index) {
			if (index + 1 != this->data.size()) {
				this->data.resize(index + 1);
			}

			SetBytes[valueType](value, this->data[index]);
		}

		// Empty if this->type != struct
		std::vector<HRTDS_STRUCTURE> structures;

		// (<value1>, <value2>, <value3>)
		void Parse_Structure_Tuple(
			std::vector<HRTDS_TOKEN>& tokens, size_t& valueTokenStart,
			const HRTDS_STRUCTURE_KEY& structureKey, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap,
			size_t index
		) {
			// Verify and retreive
			size_t valueTokenEnd = 0;
			for (size_t i = valueTokenStart; i < tokens.size(); i++)
			{
				HRTDS_TOKEN_TYPE currentTokenType = tokens[i].tokenType;
				switch (currentTokenType)
				{
				case hrtds::VALUE_LIST_ELEMENT_START:
				case hrtds::VALUE_LIST_ELEMENT: continue;

				case hrtds::VALUE_LIST_ELEMENT_END: {
					valueTokenEnd = i;

					// Break the for-loop
					i = tokens.size();
					break;
				}

				default: {
					std::cerr << "Failed to parse tuple as token[" << i << "] " <<
						", which is token '" << tokens[i].content << "' "
						<< "is not a valid list element." << std::endl;

					return;
				}
				}
			}

			if (valueTokenEnd == 0) {
				std::cerr << "Failed to find the last element of an array starting at token: " << valueTokenStart << std::endl;

				return;
			}

			// Retrieve layout
			HRTDS_LAYOUT_BY_STRUCT_KEY_MAP::const_iterator layoutIt =
				layoutByStructKeyMap.find(structureKey);

			if (layoutIt == layoutByStructKeyMap.end()) {
				std::cerr << "Could not find structure '" << structureKey << "'" << std::endl;

				return;
			}

			HRTDS_LAYOUT layout = layoutIt->second;

			// Set data
			if (index + 1 != this->data.size()) {
				this->structures.resize(index + 1);
			}

			for (size_t i = valueTokenStart; i < (valueTokenEnd + 1); i++)
			{
				std::string valueString = tokens[i].content;
				switch (tokens[i].tokenType)
				{
				case HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_START: {
					valueString.erase(valueString.begin());

					break;
				}
				case HRTDS_TOKEN_TYPE::VALUE_LIST_ELEMENT_END: {
					valueString.erase(valueString.end());

					break;
				}

				default: break;
				}

				tokens[i].content = valueString;

				size_t localI = i - valueTokenStart;
				this->structures[index][layout[localI].fieldName] = HRTDS_VALUE(
					tokens,
					valueTokenStart,

					layout[localI].isArray,
					layout[localI].fieldName,
					layout[localI].identifierPair,
					layoutByStructKeyMap
				);
			}

			// Output
			valueTokenStart = valueTokenEnd;
		}
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
		HRTDS();
		HRTDS(const std::string& content);
		~HRTDS();

		HRTDS_VALUE& operator[](std::string key);

		void Parse(std::string content);

	private:
		HRTDS_IDENTIFIER_PAIR RetrieveIdentifier(const std::string& identifierString);

		const HRTDS_STANDARD_IDENTIFIER_BY_NAME_MAP standardIdentifierByNameMap{
		{ "string", IDENTIFIER_STRING },
		{ "bool", IDENTIFIER_BOOL },
		{ "int", IDENTIFIER_INT },
		{ "float", IDENTIFIER_FLOAT },
		};

		HRTDS_LAYOUT_BY_STRUCT_KEY_MAP layoutByStructKeyMap;
		void RegisterNewLayout(std::vector<HRTDS_TOKEN>& tokens, std::string fieldName, size_t& valueTokenStart);

		HRTDS_STRUCTURE structure;
	};

	inline HRTDS::HRTDS() { }

	inline HRTDS::HRTDS(const std::string& content)
	{
		this->Parse(content);
	}

	inline HRTDS::~HRTDS() { }

	inline HRTDS_VALUE& HRTDS::operator[](std::string key)
	{
		return this->structure[key];
	}

	inline void HRTDS::Parse(std::string content)
	{
		// Verify valid file
		size_t startFile = content.find(utils::HRTDS_BEGIN_HARD);
		if (startFile == content.npos) {
			std::cerr << "Start of file not found" << std::endl; // TODO : Errh
			return;
		}

		startFile += utils::HRTDS_BEGIN_HARD.size();

		size_t endFile = content.rfind(utils::HRTDS_END_HARD);
		if (endFile == content.npos) {
			std::cerr << "End of file not found" << std::endl; // TODO : Errh
			return;
		}

		// Trim all comments
		content = content.substr(startFile, (endFile - startFile));

		// Trim content
		utils::Trim(content);

		// Collect every string
		std::unordered_map<int, std::string> stringBank;
		for (size_t i = 0; i < content.size(); i++)
		{
			size_t quoteBegin = content.find(utils::HRTDS_QUOTE, i);
			if (quoteBegin == content.npos) {
				break; // There are no more strings
			}

			// Jump from here "... to here "...
			//                ^             ^
			quoteBegin += 1;
			i = quoteBegin;

			size_t quoteEnd = content.find(utils::HRTDS_QUOTE, i);
			if (quoteEnd == content.npos) {
				std::cerr << "Could not find end-quote" << std::endl;

				return;
			}

			i = quoteEnd;

			// Collect the string
			std::string collectedString = content.substr(quoteBegin, (quoteEnd - quoteBegin));
			size_t stringIndex = stringBank.size();
			stringBank[stringIndex] = collectedString;

			// Replace the string in the original
			content.replace(content.begin() + quoteBegin, content.begin() + quoteEnd, std::to_string(stringIndex));
		}

		// Remove all whitesapce (whitespace is only cared for inside strings
		content.erase(std::remove_if(content.begin(), content.end(), std::isspace), content.end());

		// Tokenize file
		std::vector<HRTDS_TOKEN> tokens;
		for (size_t i = 0; i < content.size(); i++)
		{
			size_t tokenEnd = content.find_first_of(HRTDS_NEW_TOKEN_STRING, i);
			tokenEnd = tokenEnd == content.npos ? content.size() : tokenEnd;

			std::string token = content.substr(i, tokenEnd - i);
			if (!token.empty()) {
				char beginChar = content[i - 1];
				char endChar = content[tokenEnd];

				std::unordered_map<int, HRTDS_TOKEN_TYPE>::iterator typeIt
					= CharToTokenMap.find(EncodePair(beginChar, endChar));

				if (typeIt == CharToTokenMap.end()) {
					std::cerr << "Could not specify token: '" << token << "' "
						<< "because the token-specifiers did not specify a type"
						<< " '" << beginChar << "', '" << endChar << "'" << std::endl;

					return;
				}

				// Check if for this <identifier>[]
				//         ^
				bool isArray = token[token.size() - 1] == utils::HRTDS_END_ARRAY;

				tokens.push_back({ typeIt->second, isArray, token });
			}

			i = tokenEnd;
		}

		// Repopulate every string
		for (size_t i = 0; i < tokens.size(); i++)
		{
			// Checks for this "<index>"
			//   ^
			if (tokens[i].content[0] == utils::HRTDS_QUOTE) {
				std::string keyString = tokens[i].content; // Is this "<index>"
				keyString.erase(keyString.begin()); // Remove this ---> "<index>"
				keyString.pop_back(); // Remove this  <index>" <----

				// Retrieve
				int keyIndex = std::stoi(keyString);
				std::string retrievedString = stringBank[keyIndex];

				// Repopulate
				tokens[i].content = retrievedString;
			}
		}

		// Now build the structure
		for (size_t i = 0; i < tokens.size(); i++)
		{
			// The token vector will always have this format:
			// [identifier][name][value1][value2][...][identifier][name][value1]...
			//
			// So this parser should always be able to count on the first two tokens after a value being
			// 1) An identifier token
			// 2) And a name token

			// Verify everything about the identifier token
			HRTDS_TOKEN identifierToken = tokens[i];
			if (identifierToken.tokenType != HRTDS_TOKEN_TYPE::IDENTIFIER) {
				std::cerr << "Unexpected token type '" << identifierToken.tokenType
					<< "' from token: " << identifierToken.content
					<< ". Expected type '" << HRTDS_TOKEN_TYPE::IDENTIFIER << std::endl;

				return;
			}

			std::string cleanIdentifier = identifierToken.content;
			if (identifierToken.isArray) {
				cleanIdentifier.pop_back(); cleanIdentifier.pop_back();
			}

			HRTDS_IDENTIFIER_PAIR identifierPair = this->RetrieveIdentifier(cleanIdentifier);

			if (cleanIdentifier != "struct" && identifierPair.type == IDENTIFIER_STRUCT && identifierPair.structureKey.empty()) {
				std::cerr << "Identifier '" << cleanIdentifier << "' not recognized" << std::endl;

				return;
			}

			// Next token
			i += 1;

			// Now everything about the name token
			HRTDS_TOKEN nameToken = tokens[i];
			if (nameToken.tokenType != HRTDS_TOKEN_TYPE::NAME) {
				std::cerr << "Unexpected token type '" << identifierToken.tokenType
					<< "' from token: " << identifierToken.content
					<< ". Expected type '" << HRTDS_TOKEN_TYPE::NAME << std::endl;

				return;
			}

			// Next token (if everything has gone as expected, this should be a value token)
			i += 1;

			if (identifierPair.type == IDENTIFIER_STRUCT && identifierPair.structureKey.empty()) {
				this->RegisterNewLayout(tokens, nameToken.content, i);

				continue;
			}

			// Verification happens inside the new HRTDS_VALUE(..)
			this->structure[nameToken.content] = HRTDS_VALUE(
				tokens, i, identifierToken.isArray, nameToken.content, identifierPair, this->layoutByStructKeyMap
			);

			// 'i' is modified by the functions above, and is now pointing to the last value token
			// the for-loop will increment i with one and we will be ready to do this all over again
		}
	}

	inline HRTDS_IDENTIFIER_PAIR HRTDS::RetrieveIdentifier(const std::string& identifierString) {
		HRTDS_STANDARD_IDENTIFIER_BY_NAME_MAP::const_iterator typeNameIt =
			this->standardIdentifierByNameMap.find(identifierString);

		if (typeNameIt != this->standardIdentifierByNameMap.end()) {
			return HRTDS_IDENTIFIER_PAIR{
			typeNameIt->second,
			""
			};
		}

		HRTDS_LAYOUT_BY_STRUCT_KEY_MAP::const_iterator structNameIt =
			this->layoutByStructKeyMap.find(identifierString);

		if (structNameIt != this->layoutByStructKeyMap.end()) {
			return HRTDS_IDENTIFIER_PAIR{
			HRTDS_STANDARD_IDENTIFIER_TYPES::IDENTIFIER_STRUCT,
			structNameIt->first
			};
		}

		// TODO : Error handling
		return HRTDS_IDENTIFIER_PAIR{
		HRTDS_STANDARD_IDENTIFIER_TYPES::IDENTIFIER_STRUCT,
		"" // Error-designator
		};
	}

	void HRTDS::RegisterNewLayout(std::vector<HRTDS_TOKEN>& tokens, std::string fieldName, size_t& valueTokenStart) {
		// The layout tokens should be layed out (heh) like this
		// [structure_begin][identifier][struct_field_name][...][structure_end]

		// Verify the first token, it should be a struct begin one
		HRTDS_TOKEN structBeginToken = tokens[valueTokenStart];
		// ... TODO

		// Move on to the next token
		valueTokenStart += 1;

		// Loop until struct_end is found
		HRTDS_LAYOUT layout = HRTDS_LAYOUT();

		size_t structEndTokenIndex = 0;
		for (size_t i = valueTokenStart; i < tokens.size(); i++)
		{
			if (tokens[i].tokenType == HRTDS_TOKEN_TYPE::STRUCT_END) {
				structEndTokenIndex = i;

				break;
			}

			// Verify that token [i] is an identifier
			HRTDS_TOKEN identifierToken = tokens[i];
			if (identifierToken.tokenType != HRTDS_TOKEN_TYPE::IDENTIFIER) {
				std::cerr << "Unexpected token type '" << identifierToken.tokenType
					<< "' from token: " << identifierToken.content
					<< ". Expected type '" << HRTDS_TOKEN_TYPE::IDENTIFIER << std::endl;

				return;
			}

			std::string cleanIdentifier = identifierToken.content;
			if (identifierToken.isArray) {
				cleanIdentifier.pop_back(); cleanIdentifier.pop_back();
			}

			HRTDS_IDENTIFIER_PAIR identifierPair = this->RetrieveIdentifier(cleanIdentifier);

			if (identifierPair.type == IDENTIFIER_STRUCT && identifierPair.structureKey.empty()) {
				std::cerr << "Identifier '" << cleanIdentifier << "' not recognized" << std::endl;

				return;
			}

			// Move on to the next token
			i += 1;

			// Verify that token [i+1] is an struct_field_name
			HRTDS_TOKEN structFieldNameToken = tokens[i];
			if (structFieldNameToken.tokenType != HRTDS_TOKEN_TYPE::STRUCT_FIELD_NAME) {
				std::cerr << "Unexpected token type '" << identifierToken.tokenType
					<< "' from token: " << identifierToken.content
					<< ". Expected type '" << HRTDS_TOKEN_TYPE::STRUCT_FIELD_NAME << std::endl;

				return;
			}

			// If successful append to layout object
			HRTDS_LAYOUT_ELEMENT layoutElement;
			layoutElement.fieldName = structFieldNameToken.content;
			layoutElement.identifierPair = identifierPair;
			layoutElement.isArray = identifierToken.isArray;

			layout.push_back(layoutElement);
		}

		// If end was found, publish the object to layoutbystructname
		if (structEndTokenIndex == 0) {
			std::cerr << "Could not find end token for the struct '" << fieldName << std::endl;

			return;
		}

		this->layoutByStructKeyMap[fieldName] = layout;

		// Output
		valueTokenStart = structEndTokenIndex;
	}
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

//bool hrtds::utils::isQuotationMarkValid(const std::string& content, bool inQuotes, size_t pos) {
// if (inQuotes == true) {
// return true;
// }

// size_t closingQuote = content.find(HRTDS_QUOTE, pos + 1);
// bool validClosingQuote = closingQuote != content.npos;

// return validClosingQuote;
//}

//std::vector<std::string> hrtds::utils::DecomposeList(const std::string& listString, char begin, char end, char separator)
//{
// size_t listStart = listString.find(begin, 0);
// if (listStart == listString.npos) {
// std::cerr << "Could not decompose list '" << listString << "' " <<
// "because the beginning char wasnt found '" << begin << "'" << std::endl;

// return {};
// }

// size_t listEnd = listString.rfind(end);
// if (listEnd == listString.npos) {
// std::cerr << "Could not decompose list '" << listString << "' " <<
// "because ending char wasnt found '" << end << "'" << std::endl;

// return {};
// }

// std::vector<std::string> list;
// std::stringstream currentWord;
// bool inQuotes = false;

// for (size_t cursor = listStart + 1; cursor < listEnd; cursor++)
// {
// char currentChar = listString[cursor];

// if (currentChar == HRTDS_QUOTE) {
// bool validQuote = isQuotationMarkValid(listString, inQuotes, cursor);
// if (!validQuote) {
// std::cerr << "Invalid quote at: " << cursor << std::endl;

// return {};
// }

// inQuotes = !inQuotes;
// currentWord << currentChar;
// }
// else if (!inQuotes && (currentChar == end|| currentChar == separator)) {
// std::string listElement = currentWord.str();
// Trim(listElement);

// list.push_back(listElement);

// // Clear
// currentWord.str("");
// currentWord.clear();
// }
// else { currentWord << currentChar; }
// }

// if (!currentWord.str().empty()) {
// std::string listElement = currentWord.str();
// Trim(listElement);

// list.push_back(listElement);

// // Clear
// currentWord.str("");
// currentWord.clear();
// }

// return list;
//}

// (<value1>, <value2>, <value3>)
//void Parse_Structure_Tuple(
// const std::string& tuple,
// const HRTDS_STRUCTURE_KEY& structureKey,
// const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap,
// size_t index
//) {
//// Retrieve layout
//HRTDS_LAYOUT_BY_STRUCT_KEY_MAP::const_iterator layoutIt =
// layoutByStructKeyMap.find(structureKey);

//if (layoutIt == layoutByStructKeyMap.end()) {
// return; // TODO : Error handling
//}

//HRTDS_LAYOUT layout = layoutIt->second;

//// Decompose the tuple
//std::vector<std::string> decomposedTuple = utils::DecomposeList(
// tuple, utils::HRTDS_BEGIN_TUPLE, utils::HRTDS_END_TUPLE, utils::HRTDS_LIST_SEPARATOR
//);

//// Verify congruence
//if (layout.size() != decomposedTuple.size()) {
// return; // TODO : Error handling
//}

//// Set data
//if (index == 0) {
// this->structures.clear();
//}

//this->structures.push_back(HRTDS_STRUCTURE());
//for (size_t i = 0; i < decomposedTuple.size(); i++)
//{
// this->structures[index][layout[i].fieldName] = HRTDS_VALUE();
// this->structures[index][layout[i].fieldName].Set(
// decomposedTuple[i],
// layout[i].isArray,
// layout[i].identifierPair,
// layoutByStructKeyMap
// );
//}
//}

//bool ValidateWhitespace(const std::string& content, size_t cursor, char leading, char trailing);

//std::string ParseIdentifierString(const std::string& content, size_t& cursor);
//HRTDS_CHARACTER_VALIDITY ValidateIdentifierCharacter(char character, const std::string& content, size_t cursor);



//std::string ParseNameString(const std::string& content, size_t& cursor);
//HRTDS_CHARACTER_VALIDITY ValidateNameCharacter(char character, const std::string& content, size_t cursor);

//std::string ParseValueString(const std::string& content, size_t& cursor);
//HRTDS_CHARACTER_VALIDITY ValidateValueCharacter(char character, const std::string& content, bool& inQuotes, size_t cursor);


//inline bool HRTDS::ValidateWhitespace(const std::string& content, size_t cursor, char leading, char trailing)
//{
// // Leading spaces '...&       <name>' (allowed)
// //                     ^-----^
// int i = 1;
// bool nonSpaceCharacterFound = false;
// char characterToTest = content[cursor - i];
// while (!nonSpaceCharacterFound) {
// if (characterToTest == utils::HRTDS_WHITESPACE_SPACE) {
// i++;
// characterToTest = content[cursor - i];
// }
// else {
// nonSpaceCharacterFound = true;
// }
// }

// if (characterToTest == leading) {
// return true;
// }

// // Trailing spaces '<name>       :...' (allowed)
// //                        ^-----^
// i = 1;
// nonSpaceCharacterFound = false;
// characterToTest = content[cursor + i];
// while (!nonSpaceCharacterFound) {
// if (characterToTest == utils::HRTDS_WHITESPACE_SPACE) {
// i++;
// characterToTest = content[cursor + i];
// }
// else {
// nonSpaceCharacterFound = true;
// }
// }

// if (characterToTest == trailing) {
// return true;
// }

// return false;
//}

//inline std::string HRTDS::ParseIdentifierString(const std::string& content, size_t& cursor)
//{
// size_t end = content.find(utils::HRTDS_IDENTIFIER, cursor);
// if (end == content.npos) {
// Print_TerminatingIdentifierNotFound(cursor);
// return "";
// }

// // Loop from after the first identifier until the second in "&type&"
// std::stringstream identifierStringStream;
// char current = content[cursor];
// while (current != utils::HRTDS_IDENTIFIER && cursor < end) {
// // Validate
// HRTDS_CHARACTER_VALIDITY validity = this->ValidateIdentifierCharacter(current, content, cursor);
// if (validity == HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK) {
// return "";
// }
//
// // Add the character
// if (validity == HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT) {
// identifierStringStream << current;
// }

// // Move on to the next character
// cursor++;
// current = content[cursor];
// }

// std::string identifierString = identifierStringStream.str();
// return identifierString;
//}

//inline HRTDS_CHARACTER_VALIDITY HRTDS::ValidateIdentifierCharacter(char character, const std::string& content, size_t cursor)
//{
// switch (character)
// {
// case utils::HRTDS_WHITESPACE_SPACE:
// case utils::HRTDS_WHITESPACE_NEWLINE: {
// // We allow spaces if they are either leading or trailing
// if (this->ValidateWhitespace(content, cursor, utils::HRTDS_IDENTIFIER, utils::HRTDS_IDENTIFIER)) {
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_IGNORE;
// }

// Print_WhitespaceNotAllowed(cursor);
// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// case utils::HRTDS_ASSIGNMENT:
// case utils::HRTDS_BEGIN_SOFT:
// case utils::HRTDS_END_SOFT:
// case utils::HRTDS_QUOTE:
// case utils::HRTDS_TERMINATOR: {
// Print_UnexpectedCharacter(character, cursor);

// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// case utils::HRTDS_BEGIN_ARRAY: {
// // If it looks like this &....[]&
// //                   from here^
// if (
// content[cursor + 1] == utils::HRTDS_END_ARRAY &&
// content[cursor + 2] == utils::HRTDS_IDENTIFIER
// ) {
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }
//
// Print_UnexpectedCharacter(character, cursor);
// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// case utils::HRTDS_END_ARRAY: {
// // If it looks like this &....[]&
// //                    from here^
// if (
// content[cursor - 1] == utils::HRTDS_BEGIN_ARRAY &&
// content[cursor + 1] == utils::HRTDS_IDENTIFIER
// ) {
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }
//
// Print_UnexpectedCharacter(character, cursor);
// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// default: return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }
//}

/*inline std::string HRTDS::ParseNameString(const std::string& content, size_t& cursor)
{
size_t end = content.find(utils::HRTDS_ASSIGNMENT, cursor);
if (end == content.npos) {
Print_AssignmentOperatorNotFound(cursor);
return "";
}

std::stringstream nameStringStream;
char current = content[cursor];
while (current != utils::HRTDS_ASSIGNMENT && cursor < end) {
HRTDS_CHARACTER_VALIDITY validity = this->ValidateNameCharacter(current, content, cursor);
if (validity == HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK) {
return "";
}

if (validity == HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT) {
nameStringStream << current;
}

cursor++;
current = content[cursor];
}

std::string nameString = nameStringStream.str();
return nameString;
}*/

//inline HRTDS_CHARACTER_VALIDITY HRTDS::ValidateNameCharacter(char character, const std::string& content, size_t cursor)
//{
// switch (character)
// {
// case utils::HRTDS_WHITESPACE_SPACE:
// case utils::HRTDS_WHITESPACE_NEWLINE: {
// // We allow spaces if they are either leading or trailing
// if (this->ValidateWhitespace(content, cursor, utils::HRTDS_IDENTIFIER, utils::HRTDS_ASSIGNMENT)) {
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_IGNORE;
// }

// Print_WhitespaceNotAllowed(cursor);
// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// case utils::HRTDS_IDENTIFIER:
// case utils::HRTDS_BEGIN_SOFT:
// case utils::HRTDS_END_SOFT:
// case utils::HRTDS_QUOTE:
// case utils::HRTDS_BEGIN_ARRAY:
// case utils::HRTDS_END_ARRAY:
// case utils::HRTDS_TERMINATOR: {
// Print_UnexpectedCharacter(character, cursor);

// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// default: return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }
//}

/*inline std::string HRTDS::ParseValueString(const std::string& content, size_t& cursor)
{
size_t end = content.find(utils::HRTDS_TERMINATOR, cursor);
if (end == content.npos) {
Print_TerminatorNotFound(cursor);
return "";
}

std::stringstream valueStringStream;
char current = content[cursor];
bool inQuotes = false;
while (current != utils::HRTDS_TERMINATOR && cursor < end) {
HRTDS_CHARACTER_VALIDITY validity = this->ValidateValueCharacter(current, content, inQuotes, cursor);
if (validity == HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK) {
return "";
}

if (validity == HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT) {
valueStringStream << current;
}

cursor++;
current = content[cursor];
}

std::string valueString = valueStringStream.str();
return valueStringStream.str();
}*/

//inline HRTDS_CHARACTER_VALIDITY HRTDS::ValidateValueCharacter(char character, const std::string& content, bool& inQuotes, size_t cursor)
//{
// switch (character)
// {
// case utils::HRTDS_QUOTE: {
// bool validQuote = utils::isQuotationMarkValid(content, inQuotes, cursor);
// if (validQuote) {
// Print_ClosingQuoteNotFound(cursor);

// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// inQuotes = true;
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }

// case utils::HRTDS_WHITESPACE_SPACE:
// case utils::HRTDS_WHITESPACE_NEWLINE: {
// if (inQuotes == true) {
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }

// // We allow spaces if they are either leading or trailing
// if (this->ValidateWhitespace(content, cursor, utils::HRTDS_ASSIGNMENT, utils::HRTDS_TERMINATOR)) {
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_IGNORE;
// }

// Print_WhitespaceNotAllowed(cursor);
// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// case utils::HRTDS_BEGIN_SOFT:
// case utils::HRTDS_END_SOFT:
// case utils::HRTDS_IDENTIFIER:
// case utils::HRTDS_ASSIGNMENT:
// case utils::HRTDS_TERMINATOR: {
// if (inQuotes) {
// return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }

// Print_UnexpectedCharacter(character, cursor);

// return HRTDS_CHARACTER_VALIDITY::VALIDITIY_BREAK;
// }

// default: return HRTDS_CHARACTER_VALIDITY::VALIDITY_INSERT;
// }
//}