#include "hrtds.h"

#include ".\hrtds_config.h"

#include <algorithm>
#include <stdexcept>

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

void hrtds::tokenizer::Token::SetTokenType(TokenType type)
{
	this->tokenType = type;
}

hrtds::tokenizer::TokenType hrtds::tokenizer::Token::GetTokenType() const
{
	return this->tokenType;
}

void hrtds::tokenizer::Token::SetValueType(ValueType type)
{
	this->valueType = type;
}

hrtds::tokenizer::ValueType hrtds::tokenizer::Token::GetValueType() const
{
	return this->valueType;
}

void hrtds::tokenizer::Token::AddChild(Token token)
{
	this->children.push_back(token);
}

std::vector<hrtds::tokenizer::Token>& hrtds::tokenizer::Token::GetChildren()
{
	return this->children;
}

void hrtds::tokenizer::Token::SetData(const std::string& data)
{
	this->data = data;
}

std::string hrtds::tokenizer::Token::GetData() const
{
	return this->data;
}

std::vector<hrtds::tokenizer::Token> hrtds::tokenizer::Tokenizer::Tokenize(const std::string& content, const std::vector<std::string>& stringBank)
{
	// Tokenize file (first pass)
	std::vector<Token> tokens;
	for (size_t i = 0; i < content.size(); i++)
	{
		// The layout will be [Identifier][Defining][Value] for the 
		// first pass, it's in the second we tokennize the [Value] tokens
		
		// [Identifier] (&...&)
		Token identifierToken = Token(TokenType::IDENTIFIER);
		size_t identifierBegin = content.find(config::Glyph::IDENTIFIER, i);
		size_t identifierEnd = content.find(config::Glyph::IDENTIFIER, identifierBegin + 1);

		if (identifierBegin == content.npos || identifierEnd == content.npos) {
			throw std::runtime_error("In order to declare the identifier of a field you need to wrap it in two '" + std::to_string(config::Glyph::IDENTIFIER) + "'");
		}

		identifierBegin++;
		std::string identifierString = content.substr(identifierBegin, (identifierEnd - identifierBegin));
		identifierToken.SetData(identifierString);

		// [Defining] (&...:)
		Token definingToken = Token(TokenType::DEFINING);
		size_t definingBegin = identifierEnd;
		size_t definingEnd = content.find(config::Glyph::ASSIGNMENT, definingBegin + 1);

		definingBegin++;
		if (definingBegin == content.npos || definingEnd == content.npos) {
			throw std::runtime_error("In order to declare the name of a field you need to wrap it in a '" + std::to_string(config::Glyph::IDENTIFIER) + "' and '" + std::to_string(config::Glyph::ASSIGNMENT) + "'");
		}

		std::string definingString = content.substr(definingBegin, (definingEnd - definingBegin));
		definingToken.SetData(definingString);

		// [Value] (:...;)
		Token valueToken = Token(TokenType::VALUE);
		size_t valueBegin = definingEnd;
		size_t valueEnd = content.find(config::Glyph::TERMINATOR, valueBegin + 1);

		valueBegin++;
		if (valueBegin == content.npos || valueEnd == content.npos) {
			throw std::runtime_error("In order to declare the name of a field you need to wrap it in a '" + std::to_string(config::Glyph::IDENTIFIER) + "' and '" + std::to_string(config::Glyph::ASSIGNMENT) + "'");
		}

		std::string valueString = content.substr(valueBegin, (valueEnd - valueBegin));
		valueToken.SetData(valueString);

		// Add tokens
		tokens.reserve(tokens.size() + 3);
		tokens.push_back(identifierToken);
		tokens.push_back(definingToken);
		tokens.push_back(valueToken);

		// Move cursor
		i = valueEnd;
	}

	// Tokenize file (second pass)
	for (size_t i = 0; i < tokens.size(); i++)
	{
		Token& token = tokens.at(i);
		if (token.GetTokenType() != TokenType::VALUE) {
			continue;
		}

		switch (token.GetData()[0])
		{
			case config::Glyph::BEGIN_SCOPE: { 
				Tokenizer::TokenizeScope(token, stringBank); break;
			}
			case config::Glyph::BEGIN_ARRAY: {
				Tokenizer::TokenizeArray(token, stringBank); break;
			}
			case config::Glyph::BEGIN_TUPLE: {
				Tokenizer::TokenizeTuple(token, stringBank); break;
			}

			default: {
				Tokenizer::TokenizeData(token, stringBank); continue;
			}
		}		
	}

	return tokens;
}

void hrtds::tokenizer::Tokenizer::TokenizeScope(Token& parent, const std::vector<std::string>& stringBank)
{
	std::string content = parent.GetData(); // { <content> }
	content.erase(content.begin()); // { <content> }
	content.pop_back(); // <content> } 
	// resulting in <content>

	// Configure the parent token
	parent.SetTokenType(TokenType::VALUE);
	parent.SetValueType(ValueType::SCOPE);

	// Find every list separator on the same level
	std::vector<size_t> listSeparators = utils::RetrieveSameLevelSeparators(content);
	listSeparators.push_back(content.size()); // Includes the last list element

	// Tokenize (first pass)
	std::vector<Token>& children = parent.GetChildren();
	size_t cursor = 0;
	for (size_t i = 0; i < listSeparators.size(); i++)
	{
		// Locate the current list element
		size_t listSeparator = listSeparators[i];
		std::string listElement = content.substr(cursor, (listSeparator - cursor));
		
		// For a struct scope we expect the token layout to 
		// be [Identifier][Declaring].

		// [Identifier] (&...&)
		Token identifierToken = Token(TokenType::IDENTIFIER);
		size_t identifierBegin = listElement.find(config::Glyph::IDENTIFIER);
		size_t identifierEnd = listElement.find(config::Glyph::IDENTIFIER, identifierBegin + 1);

		if (identifierBegin == listElement.npos || identifierEnd == listElement.npos) {
			throw std::runtime_error("In order to declare the identifier of a field you need to wrap it in two '" + std::to_string(config::Glyph::IDENTIFIER) + "'");
		}

		identifierBegin++;
		std::string identifierString = listElement.substr(identifierBegin, (identifierEnd - identifierBegin));
		identifierToken.SetData(identifierString);
		
		// [Declaring] (&...,)
		Token declaringToken = Token(TokenType::DECLARING);
		size_t declaringBegin = identifierEnd;
		size_t declaringEnd = listElement.size();
		if (declaringBegin == listElement.npos || declaringEnd == listElement.npos) {
			throw std::runtime_error("In order to declare a declaring field name inside a struct scope, you need to wrap it in a '" + std::to_string(config::Glyph::IDENTIFIER) + "' and '" + std::to_string(config::Glyph::LIST_SEPARATOR) + "'");
		}

		declaringBegin++;
		std::string declaringString = listElement.substr(declaringBegin, (declaringEnd - declaringBegin));
		declaringToken.SetData(declaringString);

		// Add tokens
		children.reserve(children.size() + 2);
		children.push_back(identifierToken);
		children.push_back(declaringToken);

		// Next element
		cursor = listSeparator + 1;
	}
}

void hrtds::tokenizer::Tokenizer::TokenizeArray(Token& parent, const std::vector<std::string>& stringBank)
{
	std::string content = parent.GetData(); // { <content> }
	content.erase(content.begin()); // { <content> }
	content.pop_back(); // <content> } 
	// resulting in <content>

	// Configure the parent token
	parent.SetTokenType(TokenType::VALUE);
	parent.SetValueType(ValueType::ARRAY);

	// Find every list separator on the same level
	std::vector<size_t> listSeparators = utils::RetrieveSameLevelSeparators(content);
	listSeparators.push_back(content.size()); // Includes the last list element

	// Tokenize
	std::vector<Token>& children = parent.GetChildren();
	size_t cursor = 0;
	for (size_t i = 0; i < listSeparators.size(); i++)
	{
		// Locate the current list element
		size_t listSeparator = listSeparators[i];
		std::string listElement = content.substr(cursor, (listSeparator - cursor));
		
		// Create a simple value token from the listElement
		Token listElementToken = Token(TokenType::VALUE);
		listElementToken.SetData(listElement);

		// Run a second pass over the token (it may be another array or tuple)
		switch (listElementToken.GetData()[0])
		{
			case config::Glyph::BEGIN_SCOPE: {
				throw std::runtime_error("You cannot define a scope inside an array. (Scope-begin-marker found as array element)");
			}
			case config::Glyph::BEGIN_ARRAY: {
				Tokenizer::TokenizeArray(listElementToken, stringBank); break;
			}
			case config::Glyph::BEGIN_TUPLE: {
				Tokenizer::TokenizeTuple(listElementToken, stringBank); break;
			}
			default: {
				Tokenizer::TokenizeData(listElementToken, stringBank); break;
			}
		}

		// Add token to children
		children.push_back(listElementToken);
		
		// Next element
		cursor = listSeparator + 1;
	}
}

void hrtds::tokenizer::Tokenizer::TokenizeTuple(Token& parent, const std::vector<std::string>& stringBank)
{
	std::string content = parent.GetData(); // { <content> }
	content.erase(content.begin()); // { <content> }
	content.pop_back(); // <content> } 
	// resulting in <content>

	// Configure the parent token
	parent.SetTokenType(TokenType::VALUE);
	parent.SetValueType(ValueType::TUPLE);

	// Find every list separator on the same level
	std::vector<size_t> listSeparators = utils::RetrieveSameLevelSeparators(content);
	listSeparators.push_back(content.size()); // Includes the last list element

	// Tokenize
	std::vector<Token>& children = parent.GetChildren();
	size_t cursor = 0;
	for (size_t i = 0; i < listSeparators.size(); i++)
	{
		size_t listSeparator = listSeparators[i];
		std::string listElement = content.substr(cursor, (listSeparator - cursor));
		// Create a simple value token from the listElement
		Token listElementToken = Token(TokenType::VALUE);
		listElementToken.SetData(listElement);

		// Run a second pass over the token (it may be another array or tuple)
		switch (listElementToken.GetData()[0])
		{
			case config::Glyph::BEGIN_SCOPE: {
				throw std::runtime_error("You cannot define a scope inside an array. (Scope-begin-marker found as array element)");
			}
			case config::Glyph::BEGIN_ARRAY: {
				Tokenizer::TokenizeArray(listElementToken, stringBank); break;
			}
			case config::Glyph::BEGIN_TUPLE: {
				Tokenizer::TokenizeTuple(listElementToken, stringBank); break;
			}
			default: {
				Tokenizer::TokenizeData(listElementToken, stringBank); break;
			}
		}

		// Add token to children
		children.push_back(listElementToken);
		
		// Next element
		cursor = listSeparator + 1;
	}
}

void hrtds::tokenizer::Tokenizer::TokenizeData(Token& dataToken, const std::vector<std::string>& stringBank)
{
	// Configure token
	dataToken.SetTokenType(TokenType::VALUE);
	dataToken.SetValueType(ValueType::DATA);

	// Repopulate if string
	std::string data = dataToken.GetData();
	if (data[0] == config::Glyph::QUOTE) {
		std::string indexString = data; // "<index>"
		indexString.erase(indexString.begin());	// Removes this ---> "<index>"
		indexString.pop_back(); // Removes this <index>" <---
		// resuling in <index>

		// Claim string
		int index = std::stoi(indexString);
		std::string claimedString = stringBank[index];

		// Repopulate
		dataToken.SetData(claimedString);
	}
}

hrtds::Identifier::Identifier(Identifier&& other) noexcept
	: identifierType(other.identifierType)
	, name(std::move(other.name))
	, array(other.array)
	, valid(other.valid)
{}

hrtds::Identifier::Identifier(const Identifier& other)
	: identifierType(other.identifierType)
	, name(other.name)
	, array(other.array)
	, valid(other.valid)
{}

hrtds::Identifier& hrtds::Identifier::operator=(Identifier&& other) noexcept
{
	if (this == &other) {
		return *this;
	}

	this->identifierType = other.identifierType;
	this->name = std::move(other.name);
	this->array = other.array;
	this->valid = other.valid;

	return *this;
}

hrtds::Identifier& hrtds::Identifier::operator=(const Identifier& other)
{
	if (this == &other) {
		return *this;
	}

	this->identifierType = other.identifierType;
	this->name = other.name;
	this->array = other.array;
	this->valid = other.valid;

	return *this;
}

void hrtds::Identifier::SetIdentifierType(IdentifierType type)
{
	this->identifierType = type;
}

hrtds::IdentifierType hrtds::Identifier::GetIdentifierType() const
{
	return this->identifierType;
}

void hrtds::Identifier::SetIdentifierName(const std::string& name)
{
	this->name = name;
}

std::string hrtds::Identifier::GetIdentifierName() const
{
	return this->name;
}

void hrtds::Identifier::SetArray(bool array)
{
	this->array = array;
}

bool hrtds::Identifier::isArray() const
{
	return this->array;
}

void hrtds::Identifier::SetValid(bool valid)
{
	this->valid = valid;
}

bool hrtds::Identifier::isValid() const
{
	return this->valid;
}

hrtds::Identifier hrtds::Identifier::Determine(std::string identifierString, const HRTDS& hrtds)
{
	Identifier identifier;

	// Check if we are an array
	char lastCharacter = identifierString[identifierString.size() - 1];
	if (lastCharacter == config::Glyph::END_ARRAY) {
		identifierString.pop_back();
		identifierString.pop_back();
		identifier.SetArray(true);
	}

	identifier.SetIdentifierName(identifierString);

	// Determine if we are built-in or structure
	if (data::DynamicConverter::FromString.count(identifierString)) {
		identifier.SetIdentifierType(IdentifierType::BUILTIN);
		identifier.SetValid(true);
		return identifier;
	}

	const std::unordered_map<std::string, StructureLayout>& declaredStructures = hrtds.GetDeclaredStructures();
	auto it = declaredStructures.find(identifierString);
	if (it != declaredStructures.end()) {
		identifier.SetIdentifierType(IdentifierType::TUPLE);
		identifier.SetValid(true);
		return identifier;
	}

	// If not we do not exist
	return Identifier(false);
}

hrtds::StructureLayout::StructureLayout(StructureLayout&& other) noexcept
	: layout(std::move(other.layout))
{}

hrtds::StructureLayout::StructureLayout(const StructureLayout& other)
	: layout(other.layout)
{}

hrtds::StructureLayout& hrtds::StructureLayout::operator=(StructureLayout&& other) noexcept
{
	if (this == &other) {
		return *this;
	}

	this->layout = std::move(other.layout);

	return *this;
}

hrtds::StructureLayout& hrtds::StructureLayout::operator=(const StructureLayout& other)
{
	if (this == &other) {
		return *this;
	}

	this->layout = other.layout;

	return *this;
}

void hrtds::StructureLayout::AddLayoutElement(LayoutElement element)
{
	this->layout.push_back(element);
}

hrtds::LayoutElement* hrtds::StructureLayout::GetLayoutElement(size_t index)
{
	return index < this->layout.size() ? &this->layout[index] : nullptr;
}

hrtds::LayoutElement* hrtds::StructureLayout::operator[](size_t index)
{
	return this->GetLayoutElement(index);
}

std::vector<hrtds::LayoutElement>& hrtds::StructureLayout::GetLayoutElements()
{
	return this->layout;
}

const std::vector<hrtds::LayoutElement>& hrtds::StructureLayout::GetLayoutElements() const
{
	return this->layout;
}

hrtds::StructureLayout hrtds::StructureLayout::Parse(tokenizer::Token& valueToken, const HRTDS& hrtds)
{
	StructureLayout layout;

	std::vector<tokenizer::Token>& children = valueToken.GetChildren();
	for (size_t i = 0; i < children.size(); i++)
	{
		// When parsing a valueToken for a structure layout we expect the 
		// children to be [IDENTIFIER][DECLARING] repeating. We also work 
		// from the identifier.

		tokenizer::Token& identifierToken = children[i];
		if (identifierToken.GetTokenType() != tokenizer::TokenType::IDENTIFIER) {
			continue;
		}

		tokenizer::Token& declaringToken = children[i + 1];
		if (declaringToken.GetTokenType() != tokenizer::TokenType::DECLARING) {
			throw std::runtime_error("In a field of a structure declaration, an identifying token has to be preceeded by a declaring one. (Expected tokentype DECLARING, but found " + std::to_string(static_cast<int>(declaringToken.GetTokenType())) + ")");
		}

		Identifier identifier = Identifier::Determine(identifierToken.GetData(), hrtds);
		if (!identifier.isValid()) {
			throw std::runtime_error("Unrecognized identifier: '" + identifierToken.GetData() + "'. If you meant to use a custom struct make sure the name matches and the it's declarations exists before the use of it.");
		}

		layout.AddLayoutElement({ identifier, declaringToken.GetData() });
	}

	return layout;
}

hrtds::Value::Value(Value&& other) noexcept
	: identifier(std::move(other.identifier))
	, data(std::move(other.data))
	, children(std::move(other.children))
	, layout(std::move(other.layout))
	, fieldMap(std::move(other.fieldMap))
{}

hrtds::Value::~Value()
{
	if (this->identifier.GetIdentifierType() != IdentifierType::TUPLE &&
		!this->identifier.isArray()	&&
		!this->identifier.GetIdentifierName().empty()
	) {
		std::string identifierName = this->identifier.GetIdentifierName();
		data::DynamicConverter::Destroy[identifierName](this->data);
	}
}

hrtds::Value& hrtds::Value::operator=(Value&& other) noexcept
{
	if (this == &other) {
		return *this;
	}

	this->identifier = std::move(other.identifier);
	this->data = std::move(other.data);
	this->children = std::move(other.children);
	this->layout = std::move(other.layout);
	this->fieldMap = std::move(other.fieldMap);

	return *this;
}

hrtds::Value& hrtds::Value::operator[](size_t index)
{
	return this->children[index];
}

hrtds::Value& hrtds::Value::operator[](const std::string& name)
{
	size_t childIndex = this->fieldMap.at(name);
	return this->children[childIndex];
}

void hrtds::Value::SetIdentifier(Identifier identifier)
{
	this->identifier = identifier;
}

const hrtds::Identifier& hrtds::Value::GetIdentifier() const
{
	return this->identifier;
}

std::vector<hrtds::Value>& hrtds::Value::GetChildren()
{
	return this->children;
}

const std::vector<hrtds::Value>& hrtds::Value::GetChildren() const
{
	return this->children;
}

size_t hrtds::Value::size()
{
	return this->children.size();
}

void hrtds::Value::SetLayout(StructureLayout layout)
{
	this->layout = layout;

	this->fieldMap.clear();
	for (size_t i = 0; i < layout.GetLayoutElements().size(); i++)
	{
		fieldMap[layout[i]->name] = static_cast<int>(i);
	}
}

const hrtds::StructureLayout& hrtds::Value::GetLayout() const
{
	return this->layout;
}

const void* hrtds::Value::Get() const
{
	return this->data;
}

void hrtds::Value::Set(void* data)
{
	this->children.clear();
	this->data = data;
}

hrtds::Value hrtds::Value::Parse(Identifier& identifier, tokenizer::Token& valueToken, const HRTDS& hrtds)
{
	Value value = Value();
	value.SetIdentifier(identifier);

	if (identifier.isArray()) {
		std::vector<tokenizer::Token>& tokenChildren = valueToken.GetChildren();
		size_t childAmount = tokenChildren.size();

		std::vector<Value>& valueChildren = value.GetChildren();
		valueChildren.clear();
		valueChildren.reserve(childAmount);

		for (size_t i = 0; i < childAmount; i++)
		{
			Identifier childIdentifier = Identifier::Determine(identifier.GetIdentifierName(), hrtds);
			valueChildren.emplace_back(std::move(hrtds::Value::Parse(childIdentifier, tokenChildren[i], hrtds)));
		}

		return value;
	}

	switch (identifier.GetIdentifierType())
	{
		case IdentifierType::TUPLE: {
			std::vector<tokenizer::Token>& tokenChildren = valueToken.GetChildren();
			size_t childAmount = tokenChildren.size();

			StructureLayout childLayout = hrtds.GetDeclaredStructures().at(identifier.GetIdentifierName());
			std::vector<LayoutElement>& childLayoutElements = childLayout.GetLayoutElements();
			size_t layoutAmount = childLayoutElements.size();

			if (childAmount != layoutAmount) {
				throw std::runtime_error("You need to match the amount of elements in tuple to the layout.");
			}
			
			std::vector<Value>& valueChildren = value.GetChildren();
			valueChildren.clear();
			valueChildren.reserve(childAmount);

			for (size_t i = 0; i < childAmount; i++)
			{
				valueChildren.emplace_back(std::move(hrtds::Value::Parse(childLayoutElements[i].identifier, tokenChildren[i], hrtds)));
			}
			
			value.SetLayout(childLayout);
			break;
		}
		case IdentifierType::BUILTIN: {
			void* data = data::DynamicConverter::FromString[identifier.GetIdentifierName()](
				valueToken.GetData()
			);

			value.Set(data);
			break;
		}
		default: break;
	}

	return value;
}

std::string hrtds::Value::Compose(const Value& value, int level)
{
	std::string composed;
	const Identifier& identifier = value.GetIdentifier();
	bool isList = identifier.isArray() || identifier.GetIdentifierType() == IdentifierType::TUPLE;
	if (!isList) {
		composed = data::DynamicConverter::ToString[identifier.GetIdentifierName()](value.Get());
		return composed;
	}

	bool expandAsArray = identifier.isArray() && identifier.GetIdentifierType() == IdentifierType::TUPLE;
	bool expandAsTuple = false;
	const std::vector<Value>& children = value.GetChildren();

	for (const Value& child : children)
	{
		const Identifier& childIdentifier = child.GetIdentifier();
		expandAsTuple = childIdentifier.isArray() || childIdentifier.GetIdentifierType() == IdentifierType::TUPLE;

		if (expandAsTuple) {
			break;
		}
	}

	bool expand = expandAsArray || expandAsTuple;
	const char BEGIN_CHAR = identifier.isArray() ? 
		config::Glyph::BEGIN_ARRAY : 
		config::Glyph::BEGIN_TUPLE;

	const char END_CHAR = identifier.isArray() ?
		config::Glyph::END_ARRAY :
		config::Glyph::END_TUPLE;

	composed += std::string()
		+ BEGIN_CHAR
		+ (expand ? std::string() 
				+ config::Glyph::WHITESPACE_NEWLINE 
		: "");
	
	for (size_t i = 0; i < children.size(); i++)
	{
		std::string separator = std::string()
			+ config::Glyph::LIST_SEPARATOR
			+ config::Glyph::WHITESPACE_SPACE
			+ (expand ? std::string() + config::Glyph::WHITESPACE_NEWLINE : "");

		std::string indentation = std::string()
			+ std::string(level + 1, config::Glyph::WHITESPACE_TAB);

		bool first = i == 0;
		composed += std::string()
			+ (!first ? separator : "")
			+ (expand ? indentation : "")
			+ hrtds::Value::Compose(children[i], level + 1);
	}

	composed += std::string()
		+ (expand ? std::string()
				+ config::Glyph::WHITESPACE_NEWLINE
				+ std::string(level, config::Glyph::WHITESPACE_TAB)
		: "")
		+ END_CHAR;

	return composed;
}

hrtds::HRTDS::HRTDS(HRTDS&& other) noexcept
	: declaredStructures(std::move(other.declaredStructures))
	, fields(std::move(other.fields))
{}

void hrtds::HRTDS::DeclareStructure(const std::string& name, StructureLayout layout)
{
	this->declaredStructures[name] = layout;
	this->structureOrder.push_back(name);
}

hrtds::StructureLayout* hrtds::HRTDS::RetrieveStructureDeclaration(const std::string& name)
{
	auto it = this->declaredStructures.find(name);
	return it != this->declaredStructures.end() ? &it->second : nullptr;
}

const std::unordered_map<std::string, hrtds::StructureLayout>& hrtds::HRTDS::GetDeclaredStructures() const
{
	return this->declaredStructures;
}

const std::vector<std::string>& hrtds::HRTDS::GetStructureOrder() const
{
	return this->structureOrder;
}

void hrtds::HRTDS::DefineField(const std::string& name, Value&& value)
{
	this->fields[name] = std::move(value);
	this->fieldOrder.push_back(name);
}

hrtds::Value* hrtds::HRTDS::RetrieveFieldDefinition(const std::string& name)
{
	auto it = this->fields.find(name);
	return it != this->fields.end() ? &it->second : nullptr;
}

hrtds::Value& hrtds::HRTDS::operator[](const std::string& name)
{
	return this->fields[name];
}

const std::unordered_map<std::string, hrtds::Value>& hrtds::HRTDS::GetFields() const
{
	return this->fields;
}

const std::vector<std::string>& hrtds::HRTDS::GetFieldOrder() const
{
	return this->fieldOrder;
}

void hrtds::HRTDS::Parse(HRTDS& hrtds, std::string content)
{
	// Prepare file
	size_t fileScopeBeginPos = content.find(config::GlyphLiterals::BEGIN_FILE_SCOPE);
	if (fileScopeBeginPos == content.npos) {
		throw std::runtime_error("The file needs to include a '" + config::GlyphLiterals::BEGIN_FILE_SCOPE + "' to mark the beginning of the file. (The file-begin-marker could not be found)");
	}
	fileScopeBeginPos += config::GlyphLiterals::BEGIN_FILE_SCOPE.size();

	size_t fileScopeEndPos = content.rfind(config::GlyphLiterals::END_FILE_SCOPE);
	if (fileScopeEndPos == content.npos) {
		throw std::runtime_error("The file needs to include a '" + config::GlyphLiterals::END_FILE_SCOPE + "' to mark the end of the file. (The file-end-marker could not be found)");
	}
	
	content = content.substr(fileScopeBeginPos, (fileScopeEndPos - fileScopeBeginPos));
	utils::Trim(content);

	// Collect every string
	std::vector<std::string> stringBank;
	for (size_t i = 0; i < content.size(); i++)
	{
		// Locate the string
		size_t quoteBegin = content.find(config::Glyph::QUOTE, i);
		if (quoteBegin == content.npos) break; // There are no more strings

		// Jump from here "... to here "...
		//                ^             ^
		quoteBegin += 1;

		size_t quoteEnd = content.find(config::Glyph::QUOTE, quoteBegin);
		if (quoteEnd == content.npos) {
			throw std::runtime_error("To define a string you need both an opening quotationmark and a closing one. (Could not find closing quotationmark)");
		}

		// Collect the string
		std::string collectedString = content.substr(quoteBegin, (quoteEnd - quoteBegin));
		stringBank.push_back(collectedString);

		// Replace the original string with its index
		std::string stringIndex = std::to_string(stringBank.size() - 1);
		content.replace(content.begin() + quoteBegin, content.begin() + quoteEnd, stringIndex);

		// Re-sync cursor (i) position
		// From "<abc>"___ then "n"______ to "n"______
		//            ^              ^         ^
		i = quoteBegin + stringIndex.size();
	}

	// Remove all whitesapce (we only want to preserve whitespace inside strings)
	content.erase(std::remove_if(content.begin(), content.end(), [](char c) { return std::isspace(static_cast<unsigned char>(c)); }), content.end());

	// Tokenize file and repopulate strings
	std::vector<tokenizer::Token> tokens = tokenizer::Tokenizer::Tokenize(content, stringBank);

	/* Build the HRTDS structure */
	// This consists of:
	//		1) Looping through each token
	//		2) Declare a struct
	//		3) ... or declare a defining field
	//		4) While also verifying the syntax

	for (size_t i = 0; i < tokens.size(); i++)
	{
		// We work from [IDENTIFIER] tokens
		tokenizer::Token& identifierToken = tokens[i];
		if (identifierToken.GetTokenType() != tokenizer::TokenType::IDENTIFIER) {
			continue;
		}

		// Verify that the two following tokens are [DEFINING] and [VALUE], respectively
		tokenizer::Token& definingToken = tokens[i + 1];
		if (definingToken.GetTokenType() != tokenizer::TokenType::DEFINING) {
			throw std::runtime_error("In a field, an identifying token has to be preceeded by a defining one. (Expected tokentype DEFINING, but found " + std::to_string(static_cast<int>(definingToken.GetTokenType())) + ")");
		}

		tokenizer::Token& valueToken = tokens[i + 2];
		if (valueToken.GetTokenType() != tokenizer::TokenType::VALUE) {
			throw std::runtime_error("In a field, a defining token has to be preceeded by a value one. (Expected tokentype VALUE, but found " + std::to_string(static_cast<int>(valueToken.GetTokenType())) + ")");
		}

		switch (valueToken.GetValueType())
		{
			case tokenizer::ValueType::ARRAY:
			case tokenizer::ValueType::TUPLE:
			case tokenizer::ValueType::DATA: {
				// Define a field
				std::string identifierString = identifierToken.GetData();
				Identifier identifier = Identifier::Determine(identifierToken.GetData(), hrtds);
				if (!identifier.isValid()) {
					throw std::runtime_error("Unrecognized identifier: '" + identifierString + "'. If you meant to use a custom struct make sure the name matches and the it's declarations exists before the use of it.");
				}

				hrtds.DefineField(definingToken.GetData(), hrtds::Value::Parse(identifier, valueToken, hrtds));
				break;
			}
			
			case tokenizer::ValueType::SCOPE: {
				// Define a structure
				hrtds.DeclareStructure(definingToken.GetData(), hrtds::StructureLayout::Parse(valueToken, hrtds));
				break;
			}

			default: break;
		}
	}
}

std::string hrtds::HRTDS::Compose(const HRTDS& hrtds)
{
	std::string composed = std::string()
		+ config::GlyphLiterals::BEGIN_FILE_SCOPE
		+ config::Glyph::WHITESPACE_NEWLINE;

	const std::unordered_map<std::string, StructureLayout>& declaredStructures
		= hrtds.GetDeclaredStructures();
	const std::vector<std::string> structureOrder = hrtds.GetStructureOrder();

	for (const std::string& structureName : structureOrder)
	{
		const auto& structure = declaredStructures.at(structureName);

		std::string structString = std::string()
			+ config::Glyph::WHITESPACE_TAB
			+ config::Glyph::IDENTIFIER	 
			+ config::IdenifierLiterals::STRUCT_IDENTIFIER
			+ config::Glyph::IDENTIFIER	 
			+ config::Glyph::WHITESPACE_SPACE
			+ structureName				 
			+ config::Glyph::WHITESPACE_SPACE
			+ config::Glyph::ASSIGNMENT	 
			+ config::Glyph::WHITESPACE_SPACE
			+ config::Glyph::BEGIN_SCOPE;

		const StructureLayout& layout = structure;
		const std::vector<LayoutElement>& elements = layout.GetLayoutElements();
		for (size_t i = 0; i < elements.size(); i++)
		{								 
			const Identifier& identifier = elements[i].identifier;
			const std::string& fieldName = elements[i].name;
										 
			std::string declaringFieldString = std::string()
				+ config::Glyph::WHITESPACE_NEWLINE
				+ config::Glyph::WHITESPACE_TAB
				+ config::Glyph::WHITESPACE_TAB
				+ config::Glyph::IDENTIFIER
				+ identifier.GetIdentifierName()
				+ (identifier.isArray() ? std::string() 
						+ config::Glyph::BEGIN_ARRAY
						+ config::Glyph::END_ARRAY 
				: "")					 
				+ config::Glyph::IDENTIFIER
				+ config::Glyph::WHITESPACE_SPACE
				+ fieldName				 
				+ (i != elements.size() - 1 ? std::string() 
						+ config::Glyph::LIST_SEPARATOR 
				: "");
										 
			structString += declaringFieldString;
		}								 
										 
		structString += std::string()	 
			+ config::Glyph::WHITESPACE_NEWLINE
			+ config::Glyph::WHITESPACE_TAB
			+ config::Glyph::END_SCOPE	 
			+ config::Glyph::TERMINATOR	 
			+ config::Glyph::WHITESPACE_NEWLINE
			+ config::Glyph::WHITESPACE_NEWLINE;
										 
		composed += structString;		 
	}									 
										 
	const std::unordered_map<std::string, Value>& fields = hrtds.GetFields();
	const std::vector<std::string>& fieldOrder = hrtds.GetFieldOrder();
	for (const std::string& fieldName : fieldOrder)
	{
		const Value& value = fields.at(fieldName);
		const Identifier& identifier = value.GetIdentifier();
		
		std::string fieldString = std::string()
			+ config::Glyph::WHITESPACE_TAB
			+ config::Glyph::IDENTIFIER
			+ identifier.GetIdentifierName()
			+ (identifier.isArray() ? std::string()
					+ config::Glyph::BEGIN_ARRAY
					+ config::Glyph::END_ARRAY
			: "")
			+ config::Glyph::IDENTIFIER
			+ config::Glyph::WHITESPACE_SPACE
			+ fieldName
			+ config::Glyph::WHITESPACE_SPACE
			+ config::Glyph::ASSIGNMENT
			+ config::Glyph::WHITESPACE_SPACE
			+ hrtds::Value::Compose(value, 1)
			+ config::Glyph::TERMINATOR
			+ config::Glyph::WHITESPACE_NEWLINE;

		composed += fieldString;
	}

	composed += std::string()
		+ config::Glyph::WHITESPACE_NEWLINE
		+ config::GlyphLiterals::END_FILE_SCOPE;

	return composed;
}