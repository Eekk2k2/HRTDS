#include "HRTDS.h"

void hrtds::utils::Trim(std::string& input)
{
	input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](unsigned char chr) {
		return !std::isspace(chr);
	}));

	input.erase(std::find_if(input.rbegin(), input.rend(), [](unsigned char chr) {
		return !std::isspace(chr);
	}).base(), input.end());
}

void hrtds::HRTDS_VALUE::Set(std::vector<HRTDS_TOKEN>& tokens, size_t& valueStart, bool isArray, HRTDS_FIELD_NAME fieldName, HRTDS_IDENTIFIER_PAIR identifierPair, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap)
{
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

void hrtds::HRTDS_VALUE::Parse_Array(std::vector<HRTDS_TOKEN>& tokens, size_t& valueTokenStart, const HRTDS_IDENTIFIER_PAIR& identifierPair, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap) 
{
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

void hrtds::HRTDS_VALUE::Parse_Value(std::string value, HRTDS_STANDARD_IDENTIFIER_TYPES valueType, size_t index)
{
	if (index + 1 != this->data.size()) {
		this->data.resize(index + 1);
	}

	SetBytes[valueType](value, this->data[index]);
}

void hrtds::HRTDS_VALUE::Parse_Structure_Tuple(std::vector<HRTDS_TOKEN>& tokens, size_t& valueTokenStart, const HRTDS_STRUCTURE_KEY& structureKey, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap, size_t index) 
{
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

hrtds::HRTDS::HRTDS(const std::string& content)
{
	this->Parse(content);
}

hrtds::HRTDS_VALUE& hrtds::HRTDS::operator[](std::string key)
{
	return this->structure[key];
}

void hrtds::HRTDS::Parse(std::string content)
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

hrtds::HRTDS_IDENTIFIER_PAIR hrtds::HRTDS::RetrieveIdentifier(const std::string& identifierString) {
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

void hrtds::HRTDS::RegisterNewLayout(std::vector<HRTDS_TOKEN>& tokens, std::string fieldName, size_t& valueTokenStart) {
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