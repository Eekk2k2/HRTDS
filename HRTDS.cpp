#include "HRTDS.h"

// https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
void hrtds::utils::Trim(std::string& input)
{
	input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](unsigned char chr) {
		return !std::isspace(chr);
	}));

	input.erase(std::find_if(input.rbegin(), input.rend(), [](unsigned char chr) {
		return !std::isspace(chr);
	}).base(), input.end());
}

size_t hrtds::utils::FindAtSameLevel(const std::string& input, char character, size_t from = 0)
{
	// Going down level means going into another list:
	//  
	//				[..., ..., (..., ...), ..., ...]
	//				 0		    \ 1        / 0
	//
	// In this library both tuples and arrays are considered lists

	// Finding the next character at the same level entails finding 
	// the next character where localLevel is equal to 0. We go out 
	// of scope if localLevel ever goes below zero.
	int localLevel = 0;
	for (size_t i = from; i < input.size(); i++)
	{
		char reference = input[i];

		// Going down '\'
		if (reference == HRTDS_BEGIN_ARRAY || reference == HRTDS_BEGIN_TUPLE) {
			localLevel++;
		}

		// Going up '/'
		else if (reference == HRTDS_END_ARRAY || reference == HRTDS_END_TUPLE) {
			localLevel--;

			// Out of scope (met the end)
			if (localLevel < 0) {
				return input.npos;
			}
		}

		// We found a matching character
		else if (reference == character) {
			if (localLevel != 0) {
				continue;
			}

			return i;
		}
	}

	// None found
	return input.npos;
}

hrtds::HRTDS_VALUE::HRTDS_VALUE(std::string content, HRTDS_IDENTIFIER_TYPE valueType)
{
	this->ParseValue(content, valueType);
}

hrtds::HRTDS_VALUE::HRTDS_VALUE(std::vector<HRTDS_TOKEN>& tokens, size_t& cursor, const HRTDS_IDENTIFIER_PAIR& arrayIdentifier, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap)
{
	this->ParseArray(tokens, cursor, arrayIdentifier, layoutByStructKeyMap);
}

hrtds::HRTDS_VALUE::HRTDS_VALUE(std::vector<HRTDS_TOKEN>& tokens, size_t& cursor, const HRTDS_STRUCTURE_KEY& structureKey, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap)
{
	this->ParseTuple(tokens, cursor, structureKey, layoutByStructKeyMap);
}

hrtds::HRTDS_VALUE::HRTDS_VALUE(std::vector<HRTDS_TOKEN>& tokens, size_t& cursor, const HRTDS_IDENTIFIER_PAIR& identifierPair, bool isArray, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap)
{
	this->Set(tokens, cursor, identifierPair, isArray, layoutByStructKeyMap);
}

hrtds::HRTDS_VALUE& hrtds::HRTDS_VALUE::operator[](size_t index)
{
	return this->array.at(index);
}

hrtds::HRTDS_VALUE& hrtds::HRTDS_VALUE::operator[](std::string key)
{
	return this->structure.at(key);
}

void hrtds::HRTDS_VALUE::Set(std::vector<HRTDS_TOKEN>& tokens, size_t& cursor, const HRTDS_IDENTIFIER_PAIR& identifierPair, bool isArray, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap)
{
	// Clear old data
	this->data = std::vector<std::byte>();
	this->structure = HRTDS_STRUCTURE();
	this->array = std::vector<HRTDS_VALUE>();

	// Set new data
	if (isArray) {
		this->ParseArray(tokens, cursor, identifierPair, layoutByStructKeyMap); return;
	}

	bool isStructure = identifierPair.type == IDENTIFIER_STRUCT;
	if (isStructure) {
		this->ParseTuple(tokens, cursor, identifierPair.structureKey, layoutByStructKeyMap); return;
	}

	this->ParseValue(tokens[cursor].content, identifierPair.type);
}

void hrtds::HRTDS_VALUE::ParseValue(std::string content, HRTDS_IDENTIFIER_TYPE valueType)
{
	SetBytes[valueType](content, this->data);
}

void hrtds::HRTDS_VALUE::ParseArray(std::vector<HRTDS_TOKEN>& tokens, size_t& cursor, const HRTDS_IDENTIFIER_PAIR& arrayIdentifier, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap)
{
	// The first token of an array should always be VALUE_ARRAY_BEGIN
	size_t arrayTokenStart = cursor;
	HRTDS_TOKEN& arrayBeginToken = tokens[arrayTokenStart];
	if (arrayBeginToken.tokenType != HRTDS_TOKEN_TYPE::VALUE_ARRAY_BEGIN) {
		std::cerr << "Field with an array identifier '&" << tokens[arrayTokenStart - 2].content << "[]&'"
			<< " did not have an array value ': [...]'" << std::endl;

		return;
	}

	// Move to next token
	cursor++;

	// Now march through the tokens array until VALUE_ARRAY_END is found
	size_t arrayTokenEnd = 0;
	for (size_t i = cursor; i < tokens.size(); i++)
	{
		HRTDS_TOKEN_TYPE currentType = tokens[i].tokenType;

		// Break at found end
		if (currentType == HRTDS_TOKEN_TYPE::VALUE_ARRAY_END) {
			arrayTokenEnd = i;

			break;
		}

		// Check if its a nested array
		else if (currentType == HRTDS_TOKEN_TYPE::VALUE_ARRAY_BEGIN) {
			HRTDS_VALUE array = HRTDS_VALUE(tokens, i, arrayIdentifier, layoutByStructKeyMap);
			this->array.push_back(array);
		}

		// or if its a nested tuple
		else if (currentType == HRTDS_TOKEN_TYPE::VALUE_TUPLE_BEGIN) {
			HRTDS_VALUE structure = HRTDS_VALUE(tokens, i, arrayIdentifier.structureKey, layoutByStructKeyMap);
			this->array.push_back(structure);
		}

		// or maybe just a single value
		else if (currentType == HRTDS_TOKEN_TYPE::VALUE_ARRAY_ELEMENT) {
			HRTDS_VALUE arrayElement = HRTDS_VALUE(tokens[i].content, arrayIdentifier.type);
			this->array.push_back(arrayElement);
		}

		// if not any of these then theres something fishy
		else {
			std::cerr << "Unexpected token type " << static_cast<int>(currentType)
				<< " at index " << i << std::endl;
		}
	}

	// Check if we found an end
	if (arrayTokenEnd == 0) {
		std::cerr << "Failed to find the end of an array" << std::endl;

		return;
	}

	// Output
	cursor = arrayTokenEnd;
}

void hrtds::HRTDS_VALUE::ParseTuple(std::vector<HRTDS_TOKEN>& tokens, size_t& cursor, const HRTDS_STRUCTURE_KEY& structureKey, const HRTDS_LAYOUT_BY_STRUCT_KEY_MAP& layoutByStructKeyMap)
{
	// The first token of a tuple should always be VALUE_TUPLE_BEGIN
	size_t tupleTokenStart = cursor;
	HRTDS_TOKEN& tupleBeginToken = tokens[tupleTokenStart];
	if (tupleBeginToken.tokenType != HRTDS_TOKEN_TYPE::VALUE_TUPLE_BEGIN) {
		std::cerr << "Field with a tuple identifier '&" << tokens[tupleTokenStart - 2].content << "([])&'"
			<< " did not have a tuple value ': (...)'" << std::endl;

		return;
	}

	// Move to next token
	cursor++;

	// Retrieve the tuple's layout
	HRTDS_LAYOUT_BY_STRUCT_KEY_MAP::const_iterator layoutIt =
		layoutByStructKeyMap.find(structureKey);

	if (layoutIt == layoutByStructKeyMap.end()) {
		std::cerr << "Could not find structure '" << structureKey << "'" << std::endl;

		return;
	}

	HRTDS_LAYOUT layout = layoutIt->second;

	// Now march through the tokens array until VALUE_TUPLE_END is found
	size_t tupleTokenEnd = 0, j = 0;
	for (size_t i = cursor; i < tokens.size(); i++)
	{
		HRTDS_TOKEN_TYPE currentType = tokens[i].tokenType;

		// Break at found end
		if (currentType == HRTDS_TOKEN_TYPE::VALUE_TUPLE_END) {
			tupleTokenEnd = i;

			break;
		}

		// Check if its a nested array
		else if (currentType == HRTDS_TOKEN_TYPE::VALUE_ARRAY_BEGIN) {
			HRTDS_VALUE structure = HRTDS_VALUE(tokens, i, layout[j].identifierPair, layoutByStructKeyMap);
			this->structure[layout[j].fieldName] = structure;

			j++;
		}

		// or if its a nested tuple
		else if (currentType == HRTDS_TOKEN_TYPE::VALUE_TUPLE_BEGIN) {
			HRTDS_VALUE array = HRTDS_VALUE(tokens, i, layout[j].identifierPair.structureKey, layoutByStructKeyMap);
			this->structure[layout[j].fieldName] = array;

			j++;
		}

		// or maybe just a single value
		else if (currentType == HRTDS_TOKEN_TYPE::VALUE_TUPLE_ELEMENT) {
			HRTDS_VALUE tupleElement = HRTDS_VALUE(tokens[i].content, layout[j].identifierPair.type);
			this->structure[layout[j].fieldName] = tupleElement;

			j++;
		}

		// if not any of these then theres something fishy
		else {
			std::cerr << "Unexpected token type " << static_cast<int>(currentType)
				<< " at index " << i << std::endl;
		}
	}

	if (tupleTokenEnd == 0) {
		std::cerr << "Failed to find the end of a tuple" << std::endl;

		return;
	}

	// Output
	cursor = tupleTokenEnd;
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

		// Now the final quote has moved back a bit : "Eekk2k2" to "0"......
		//                                                    ^            ^
		// So we move the cursor back to the end quote "0"......
		//												 ^
		// Position of first quote + characters in the number + the last quote
		i = quoteBegin + std::to_string(stringIndex).size() + 1; 
	}

	// Remove all whitesapce (whitespace is only cared for inside strings
	content.erase(std::remove_if(content.begin(), content.end(), [](char c) { return std::isspace(static_cast<unsigned char>(c)); }), content.end());

	// Tokenize file (first pass)
	std::vector<HRTDS_TOKEN> tokens;
	for (size_t i = 0; i < content.size(); i++)
	{
		size_t tokenEnd = content.find_first_of(HRTDS_NEW_TOKEN_STRING, i);
		tokenEnd = tokenEnd == content.npos ? content.size() : tokenEnd;

		std::string token = content.substr(i, tokenEnd - i);
		if (token.empty()) {
			i = tokenEnd;

			continue;
		}

		char beginChar = content[i - 1];
		char endChar = content[tokenEnd];

		std::unordered_map<int, HRTDS_TOKEN_TYPE>::iterator typeIt
			= CharToTokenMap.find(EncodePair(beginChar, endChar));

		if (typeIt == CharToTokenMap.end()) {
			std::cerr << "Could not specify token: '" << beginChar << token << endChar << "' "
				<< "because the token-specifiers did not specify a type"
				<< " '" << beginChar << "', '" << endChar << "'" << std::endl;

			return;
		}

		// Check if for this <identifier>[]
		//								  ^
		bool isArray = token[token.size() - 1] == utils::HRTDS_END_ARRAY;

		// Special tokenization happens if we are an array or a tuple
		HRTDS_TOKEN parsedToken = { typeIt->second, isArray, token };

		if (parsedToken.tokenType == VALUE_SINGLE) {
			std::vector<HRTDS_TOKEN> tokenizedValue = this->TokenizeValue(parsedToken);
			tokens.insert(tokens.end(), tokenizedValue.begin(), tokenizedValue.end());
		}
		else { tokens.push_back(parsedToken); }

		i = tokenEnd;
	}

	// Repopulate every string
	for (size_t i = 0; i < tokens.size(); i++)
	{
		// Checks for this "<index>"
		//				   ^
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

		if (identifierPair.type == IDENTIFIER_STRUCT && identifierPair.structureKey == "") {
			return; // Error message printed in RetrieveIdentifier
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

		if (identifierPair.type == IDENTIFIER_STRUCT_DEFINITION) {
			this->RegisterNewLayout(tokens, nameToken.content, i);

			continue;
		}

		this->structure[nameToken.content] = HRTDS_VALUE(tokens, i, identifierPair, identifierToken.isArray, layoutByStructKeyMap);

		// 'i' is modified by the functions above, and is now pointing to the last value token
		// the for-loop will increment i with one and we will be ready to do this all over again
	}
}

std::vector<hrtds::HRTDS_TOKEN> hrtds::HRTDS::TokenizeValue(HRTDS_TOKEN valueToken)
{
	std::string valueContent = valueToken.content;

	// Tokenize as array
	if (valueContent[0] == utils::HRTDS_BEGIN_ARRAY) {
		return this->TokenizeArray(valueToken.content);
	}

	// Tokenize as tuple
	if (valueContent[0] == utils::HRTDS_BEGIN_TUPLE) {
		return this->TokenizeTuple(valueToken.content);
	}
	
	return std::vector<HRTDS_TOKEN> { valueToken };
}

std::vector<hrtds::HRTDS_TOKEN> hrtds::HRTDS::TokenizeArray(std::string arrayString)
{
	std::vector<HRTDS_TOKEN> tokens;

	// Clean the input
	arrayString.erase(arrayString.begin());
	arrayString.pop_back();

	// Signifies the start of an array
	tokens.push_back({ HRTDS_TOKEN_TYPE::VALUE_ARRAY_BEGIN, false, "" }); 

	// Here we add all the elements
	for (size_t i = 0; i < arrayString.size(); i++)
	{
		size_t nextSeparator = utils::FindAtSameLevel(arrayString, utils::HRTDS_LIST_SEPARATOR, i);
		nextSeparator = nextSeparator != arrayString.npos ? nextSeparator : arrayString.size();
		
		std::string token = arrayString.substr(i, (nextSeparator - i));
		if (token.empty()) {
			i = nextSeparator;

			continue;
		}

		// The current element can be another array or a tuple  [...,[...],...] [...,(...),...]
		//													         ^               ^
		if (token[0] == utils::HRTDS_BEGIN_ARRAY) {
			std::vector<HRTDS_TOKEN> tokenizedArray = this->TokenizeArray(token);
			tokens.insert(tokens.end(), tokenizedArray.begin(), tokenizedArray.end());
		}
		else if (token[0] == utils::HRTDS_BEGIN_TUPLE) {
			std::vector<HRTDS_TOKEN> tokenizedTuple = this->TokenizeTuple(token);
			tokens.insert(tokens.end(), tokenizedTuple.begin(), tokenizedTuple.end());
		}
		else {
			HRTDS_TOKEN arrayElementToken = { HRTDS_TOKEN_TYPE::VALUE_ARRAY_ELEMENT, false, token };
			tokens.push_back(arrayElementToken);
		}

		i = nextSeparator;
	}

	// Signifies the end of an array
	tokens.push_back({ HRTDS_TOKEN_TYPE::VALUE_ARRAY_END, false, "" });

	return tokens;
}

std::vector<hrtds::HRTDS_TOKEN> hrtds::HRTDS::TokenizeTuple(std::string tupleString)
{
	std::vector<HRTDS_TOKEN> tokens;

	// Clean the input (...) -> ...
	tupleString.erase(tupleString.begin());
	tupleString.pop_back();

	// Signifies the start of an array
	tokens.push_back({ HRTDS_TOKEN_TYPE::VALUE_TUPLE_BEGIN, false, "" });

	// Here we add all the elements
	for (size_t i = 0; i < tupleString.size(); i++)
	{
		size_t nextSeparator = utils::FindAtSameLevel(tupleString, utils::HRTDS_LIST_SEPARATOR, i);
		nextSeparator = nextSeparator != tupleString.npos ? nextSeparator : tupleString.size();

		std::string token = tupleString.substr(i, (nextSeparator - i));
		if (token.empty()) {
			i = nextSeparator;

			continue;
		}

		// The current element can be another tuple or an array  (...,(...),...) (...,[...],...)
		//													          ^               ^
		if (token[0] == utils::HRTDS_BEGIN_ARRAY) {
			std::vector<HRTDS_TOKEN> tokenizedArray = this->TokenizeArray(token);
			tokens.insert(tokens.end(), tokenizedArray.begin(), tokenizedArray.end());
		}
		else if (token[0] == utils::HRTDS_BEGIN_TUPLE) {
			std::vector<HRTDS_TOKEN> tokenizedTuple = this->TokenizeTuple(token);
			tokens.insert(tokens.end(), tokenizedTuple.begin(), tokenizedTuple.end());
		}
		else {
			HRTDS_TOKEN tupleElementToken = { HRTDS_TOKEN_TYPE::VALUE_TUPLE_ELEMENT, false, token };
			tokens.push_back(tupleElementToken);
		}

		i = nextSeparator;
	}

	// Signifies the end of an array
	tokens.push_back({ HRTDS_TOKEN_TYPE::VALUE_TUPLE_END, false, "" });

	return tokens;
}

hrtds::HRTDS_IDENTIFIER_PAIR hrtds::HRTDS::RetrieveIdentifier(const std::string& identifierString) {
	// First of all we check if its not a struct defining identifier
	if (identifierString == "struct") {
		return HRTDS_IDENTIFIER_PAIR { 
			HRTDS_IDENTIFIER_TYPE::IDENTIFIER_STRUCT_DEFINITION,
			""
		};
	}
	
	// Then we search for the core identifiers (those linked to a specific data type)
	HRTDS_STANDARD_IDENTIFIER_BY_NAME_MAP::const_iterator typeNameIt =
		this->standardIdentifierByNameMap.find(identifierString);

	if (typeNameIt != this->standardIdentifierByNameMap.end()) {
		return HRTDS_IDENTIFIER_PAIR{
			typeNameIt->second,
			""
		};
	}

	// Then we serach for the identifier in the custom structures list
	HRTDS_LAYOUT_BY_STRUCT_KEY_MAP::const_iterator structNameIt =
		this->layoutByStructKeyMap.find(identifierString);

	if (structNameIt != this->layoutByStructKeyMap.end()) {
		return HRTDS_IDENTIFIER_PAIR{
			HRTDS_IDENTIFIER_TYPE::IDENTIFIER_STRUCT,
			structNameIt->first
		};
	}

	// Here we did not find anything
	std::cerr << "Could not find identifier '" << identifierString << "'. "
		<< "If you meant to reference a user-defined structure, make sure its definition comes before the use" << std::endl;
	
	// Error-designator (IDENTIFIER_STRUCT with an emtpy StructureKey)
	return HRTDS_IDENTIFIER_PAIR{
		HRTDS_IDENTIFIER_TYPE::IDENTIFIER_STRUCT,
		"" 
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