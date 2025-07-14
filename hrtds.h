#pragma once
#include ".\data\hrtds_data.h"
#include ".\data\hrtds_misc.h"
#include ".\data\hrtds_decimal.h"
#include ".\data\hrtds_integral.h"

namespace hrtds {
	namespace utils {
		// https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
		void Trim(std::string& s);

		std::vector<size_t> RetrieveSameLevelSeparators(const std::string& content);
	};	

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

	namespace tokenizer {
		enum class TokenType {
			IDENTIFIER,		// &...&
			DEFINING,		// &...:
			DECLARING,		// &...,
			VALUE,			// :...;
		};

		enum class ValueType {
			SCOPE,	// {...}
			ARRAY,	// [...]
			TUPLE,	// (...)
			DATA	// ...
		};

		class Token {
		public:
			Token() = default;
			Token(TokenType type) : tokenType(type) {}
			Token(TokenType type, std::string data) : tokenType(type), data(data) {}
			~Token() = default;

			void SetTokenType(TokenType type);
			TokenType GetTokenType() const;

			void SetValueType(ValueType type);
			ValueType GetValueType() const;

			void AddChild(Token token);
			std::vector<Token>& GetChildren();

			void SetData(const std::string& data);
			std::string GetData() const;
		private:
			TokenType tokenType = TokenType::IDENTIFIER;
			ValueType valueType = ValueType::SCOPE;

			// Data
			std::vector<Token> children;
			std::string data;
		};

		class Tokenizer {
		public:
			static std::vector<Token> Tokenize(const std::string& content, const std::vector<std::string>& stringBank);
		private:
			static void TokenizeScope(Token& parent, const std::vector<std::string>& stringBank);
			static void TokenizeArray(Token& parent, const std::vector<std::string>& stringBank);
			static void TokenizeTuple(Token& parent, const std::vector<std::string>& stringBank);
			static void TokenizeData(Token& dataToken, const std::vector<std::string>& stringBank);
		};
	};

	// Whether we are a
	//	* (..., ..., ...) or
	//	* ... 
	enum class IdentifierType {
		TUPLE,
		BUILTIN
	};

	// Used in Identifier::Determine() before defined
	class HRTDS;

	//         &int& Age : 32;
	//  this:---^^^
	class Identifier {
	public:
		Identifier() = default;
		Identifier(bool valid) : valid(valid) {};
		Identifier(IdentifierType type, std::string name) : identifierType(type), name(name), valid(true) {};
		Identifier(Identifier&& other) noexcept;
		Identifier(const Identifier& other);
		~Identifier() = default;

		Identifier& operator=(Identifier&& other) noexcept;
		Identifier& operator=(const Identifier& other);

		void SetIdentifierType(IdentifierType type);
		IdentifierType GetIdentifierType() const;

		void SetIdentifierName(const std::string& name);
		std::string GetIdentifierName() const;

		void SetArray(bool array);
		bool isArray() const;

		void SetValid(bool valid);
		bool isValid() const;

		static Identifier Determine(std::string identifierString, const HRTDS& hrtds);
	private:
		IdentifierType identifierType = IdentifierType::BUILTIN;
		std::string name = "";
		bool array = false;
		
		bool valid = false;
	};

	//           &int& Age : 32;
	//  as one:---^^^--^^^
	struct LayoutElement {
		Identifier identifier;
		std::string name;
	};

	// these:-----------------------|
	//								|
	//	&struct& Version : {		|
	//		&float& Date,		<---|
	//		&int[]& Version,	<---|
	//		&string& Download	<---|
	//	};
	class StructureLayout { 
	public:
		StructureLayout() = default;
		StructureLayout(StructureLayout&& other) noexcept;
		StructureLayout(const StructureLayout& other);
		~StructureLayout() = default;

		StructureLayout& operator=(StructureLayout&& other) noexcept;
		StructureLayout& operator=(const StructureLayout& other);

		void AddLayoutElement(LayoutElement element);
		LayoutElement* GetLayoutElement(size_t index);
		LayoutElement* operator[](size_t index);

		std::vector<LayoutElement>& GetLayoutElements();
		const std::vector<LayoutElement>& GetLayoutElements() const;

		static StructureLayout Parse(tokenizer::Token& valueToken, const HRTDS& hrtds);
	private:
		std::vector<LayoutElement> layout;
	};

	class Value {
	public:
		Value() = default;
		Value(Value&& other) noexcept;
		Value(const Value& other) = delete;
		~Value() = default;

		Value& operator=(Value&& other) noexcept;
		Value& operator=(const Value& other) = delete;

		Value& operator[](size_t index);
		Value& operator[](const std::string& name);

		void SetIdentifier(Identifier identifier);
		const Identifier& GetIdentifier() const;

		std::vector<Value>& GetChildren();
		const std::vector<Value>& GetChildren() const;

		void SetLayout(StructureLayout layout);
		const StructureLayout& GetLayout() const;

		template<typename T>
		T Get();

		const std::vector<std::byte>& Get() const;

		template<typename T>
		void Set(T data);

		void Set(std::vector<std::byte> data);

		static hrtds::Value Parse(Identifier& identifier, tokenizer::Token& valueToken, const HRTDS& hrtds);
		static std::string Compose(const Value& value, int level);
	private:
		// The identity of this value
		Identifier identifier;

		// For storing raw data
		std::vector<std::byte> data;

		// For storing a tuple or array
		std::vector<Value> children;

		StructureLayout layout;
		std::unordered_map<std::string, int> fieldMap;
	};

	template<typename T>
	inline T Value::Get()
	{
		return data::StaticConverter<T>::ToType(this->data);
	}

	template<typename T>
	inline void Value::Set(T data) {
		this->children.clear();
		this->data = data::StaticConverter<T>::FromType(data);
	}

	inline void Value::Set(std::vector<std::byte> data) {
		this->children.clear();
		this->data = data;
	}

	// The main class, this is the root of the file structure
	//     
	//	    <root>
	//		HRTDS 
	//		  |
	//		  |
	//	"HRTDS_STRUCUTRE" (map linking name with value)
	//		  |
	//		  |		   <name>		   <value>
	//		  \-------["value1"]-----> Value
	//		  |
	//		  |		   <name>		   <value>
	//		  \-------["value2"]-----> Value
	//		  |
	//		  |		   <name>		   <value>
	//		  \-------["value3"]-----> Value
	//     

	class HRTDS
	{
	public:
		HRTDS() = default;
		HRTDS(HRTDS&& other) noexcept;
		HRTDS(const HRTDS& other) = delete;
		~HRTDS() = default;

		HRTDS& operator=(HRTDS&& other) noexcept = delete;
		HRTDS& operator=(const HRTDS& other) = delete;

		void DeclareStructure(const std::string& name, StructureLayout layout);
		StructureLayout* RetrieveStructureDeclaration(const std::string& name);
		const std::unordered_map<std::string, StructureLayout>& GetDeclaredStructures() const;
		const std::vector<std::string>& GetStructureOrder() const;

		void DefineField(const std::string& name, Value value);
		Value* RetrieveFieldDefinition(const std::string& name);
		Value& operator[](const std::string& name);
		const std::unordered_map<std::string, Value>& GetFields() const;
		const std::vector<std::string>& GetFieldOrder() const;

		static void Parse(HRTDS& hrtds, std::string content);
		static std::string Compose(const HRTDS& hrtds);
	private:
		// Association associates "this" with "these"
		// 
		// these:-----------------------|
		//								|
		// this:------------|			|
		//			\_______/			|
		//	&struct& Version : {		|
		//		&float& Date;		<---|
		//		&int& Version;		<---|
		//		&string& Download;	<---|
		//	};
		std::unordered_map<std::string, StructureLayout> declaredStructures;
		std::vector<std::string> structureOrder;

		std::unordered_map<std::string, Value> fields;
		std::vector<std::string> fieldOrder;
	};
};