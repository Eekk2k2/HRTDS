

# HRTDS C++20 Library

A modern and lightweight C++ static library for parsing and mapping hierarchical, strongly-typed data using a custom format named Human-Readable Typed Data Serialization (HRTDS).

## Key Features

-   **Custom Data Layout**: Define schemas (`struct` blocks) and data fields with typed identifiers like `string`, `bool`, `intN`, `float`, nested structures, and arrays.

- **Extensible Type System (Coming soon)**:  Add parsing support for any C++ type by specializing `StaticConverter<T>`.
    
-   **Zero-dependency**: Uses only the C++ standard library — no external dependencies.
    
-   **Strong typing**: Access parsed values with templated getters (`Get<T>(...)`).
    

## Installation

1. You can either download the `.lib` along with its header file from the releases page, or build the library yourself from source. 
2. In addition, you can also import the files in your project and include them in-build.

## Usage

1.  **Define structure layouts and value fields** in an `.hrtds` file (example is provided further down):
    
    ```text
	${															
		&struct& Version : {									
			&int64& versionNumber,								
			&bool& betaRelease,									
			&int64& releaseDate,									
			&string& author,									
		};														
																
		&int8& age : 32;											
		&float& temperature : 16.5;							
		&bool& developer : true;								
		&string& description : \"This is a description.\";		
																
		&int16& currentVersionIndex : 2;							
		&Version[]& versions : [								
			(1, true, 	17534, \"Eekk2k2\"),					
			(2, false, 	17512, \"Eekk2k2\"),					
			(3, false, 	34509, \"Eekk2k2\"),					
		];														
	}$    
    ```
    
2.  **Load and parse**:
    
    ```cpp
    #include "HRTDS.h"
    
    int main() {
		// ...
		
		std::string content; // Populated from a file or network traffic

		hrtds::HRTDS hrtdsFile = hrtds::Hrtds();
		hrtds::HRTDS::Parse(hrtdsFile, content);

		// Access values
		int8_t age = hrtdsFile["age"].Get<int8_t>();
		float temp = hrtdsFile["temperature"].Get<float>();
		bool developer = hrtdsFile["developer"].Get<bool>();
		std::string description = hrtdsFile["description"].Get<std::string>();

		// Retrieve value from array (and a struct)
		int currentVersionIndex = hrtdsFile["currentVersionIndex"].Get<int>();

		hrtds::HRTDS_VALUE& currentVersionStruct = hrtdsFile["versions"][currentVersionIndex];
		int currentVersionReleaseDate = currentVersionStruct["releaseDate"].Get<int>();
		std::string currentVersionAuthor = currentVersionStruct["author"].Get<std::string>();
		
		// Use the data
		// ...
    }
    
    ```




## Docs

### Syntax

**1. Setting boundaries**
Begin by defining the the file scope to indicate which region will be parsed. This is done through the `hrtds::config::Glyph::BEGIN_FILE_SCOPE` and `hrtds::config::Glyph::BEGIN_FILE_SCOPE` glyph literals. 

> In the standard implementation these look like `${` and `}$`, respectively. However  in a custom one be whatever. (This also applies to any other glyph used in parsing, such as `&`, `;`, `[`, etc)

\
*example.hrtds*:
```text
You can write comments before...

${
	...
}$

... and after the file scope - they just won't persist through any parsing.
```

\
**2. Creating a basic field**
A field consists of three main parts. It needs an identifier at the front, a name in the middle, and a value at the end. 
| Token			| Syntax			| Definition																	|
|-------------------|-------------------|-------------------------------------------------------------------------------|
| Identifier		| `&<identifier>&..`	| Wrap a string with the `Glyph::IDENTIFIER` (which is the `&`) 			|
| Name				| `..& <name> :.. `		| The name always comes after the identifier and before the value. 				|
| Value				| `..: <value>;`		| Wrap the value with a leading `Glyph::ASSIGNMENT` (a  'colon', `:`) and a trailing `Glyph::TERMINATOR` (a 'semi colon', `;`). To find more about values head to *# 3. Delving into values*. |

**Example field:** `&string& Author : "Eekk2k2";` 
\
*example.hrtds*:

```text
${
	&int8& age : 32;
	&float& temperature : 16.5;
	&bool& developer : true;
	&string& description : "This is a description.";
}$
```

\
**3. Delving Into values**
The library includes a few built-in value identifiers, called "types".  In the future, you will be able to add support for your own types.

| Value Type	| Syntax 				| Definition 																			|
|---------------|---------------------- |---------------------------------------------------------------------------------------|
| `&(u)intN&`		| `25`					| Non-decimal number, the value must fit all requirements for `std::stoi`/`std::stol`/`std::stoll`/`std::stoul`/`std::stoull` (depending on integer bit count)				|
| `&float&`		| `25`, `25.0` or `.5`	| Any number, the value must fit all requirements for `std::stof`						|
| `&double&`		| `25`, `25.0` or `.5`	| Any number, the value must fit all requirements for `std::stod`. Has higher precision than float.						|
| `&bool&`		| `true` or `1`			| If the value is equal to `true` or `1` then its evaluated to `true`, if not, `false`. It is still recommended to write `false` for a falsy value.	|
| `&string&`	| `"Hello World"`		| Any value wrapped in `Glyph::QUOTE` (`"`, quotation mark)	|

You can also define your own data layouts. To do so, create a field using the `struct` identifier, and leave the value with a `Glyph::BEGIN_SCOPE` (`{`, opening brace) and a `Glyph::END_SCOPE` (`}`, closing brace). The syntax for this follows a C-like structure:
```
&struct& myStruct : {
	
};
```
To define the layout, populate the braces with declaring fields. Which are fields without the value component and which are separated by a `Glyph::LIST_SEPARATOR` (a 'comma', `,`), rather than a `Glyph::TERMINATOR`. An example of this would be:
```
&struct& myStruct : {
	&type& <name1>,
	&type& <name2>,
	&type& <name3>
};
```
In order to use this structure in a value you first set the identifier to the name of the structure:
```
&struct& Asset : {
     &int8& type,
     &string& author,
     &int16[]& version,
     &string& path
};

&Asset& myAsset : ... ;
```
Then you populate the value with a tuple, where each of the values in the tuple is aligned chronologically with its field in the schema:   
```
&struct& Asset : {
     &int8& type,
     &string& author,
     &int16[]& version,
     &string& path
};

&Asset& myAsset : (2, "Eekk2k2", [1, 0, 0], ".\Assets\myAsset.asset") ;
```
As you can see in the example above, I am using arrays. These are syntactically similar to any C-like language's array, with the values separated by a `Glyph::LIST_SEPARATOR` (`,` or better known as the comma), and then wrapped with a leading `Glyph::BEGIN_ARRAY` (`[`, opening square bracket) and a trailing `Glyph::END_ARRAY` (`]`, closing square bracket).
```
&uint8[]& integers : [1, 2, 3, 4, 5];
```
> Also remember to append a set of both BEGIN_ARRAY and END_ARRAY (`[]`) at the end of any identifier (`&...[]&`. This is to inform the parser about a coming array.

\
*example.hrtds*:
```text
${
	&struct& Asset : {
	    &int8& type,
	    &string& author,
	    &int16[]& version,
	    &string& path
	};

	&Asset[]& assets : [
		(1, "Eekk2k2", 	[1, 0, 1], "<path>"),
		(2, "Eekk2k2", 	[1, 0, 1], "<path>"),
		(2, "Eekk2k2", 	[1, 0, 1], "<path>")				
	];
}$
```

## API Reference

### `hrtds::HRTDS`

-   `static void Parse(HRTDS& hrtds, std::string content)`: Populates a HRTDS object from a parsed content string.
- `static std::string Compose(const HRTDS& hrtds)`: Composes a HRTDS object into a content string.
    
-   `HRTDS_VALUE& operator[](const std::string &key)`: Access a field by name.

> The `HRTDS` class has other member functions, but these are not meant for the end user to interact with. Functions such as - but not limited to - `DefineField(...)`, `DeclareStructure(...)`, `RetrieveStructureDeclaration(...)` are primarily there for the parser. Although I won't come after you if you do choose to use them. 

### `hrtds::Value`

-   `template<typename T> T Get()`: Retrieves the stored bytes of Value to type T. Currently no type verification.

 - `const std::vector<std::byte>& Get()`: Retrieves the stored bytes of Value. 

- `template<typename T> T Set(T data)`: Sets the bytes of Value from data of type T. No type verification.  
    
- `Set(std::vector<std::byte> data)`: Sets the bytes of Value from data.

    
-   `Value& operator[](size_t index)`: Return child value of value array. Use `Value::Get()` to retrieve data.
    
-   `Value& operator[](const std::string &name)`: Access field of structure layout.
   
   > Similar to how the `HRTDS` class works, the `Value` class has other member functions than the ones shown above, but these are also not meant for the end user to interact with. Functions such as - but not limited to - `SetIdentifier(...)`, and `SetLayout(...)`  are primarily there for the parser. Although I won't come after you if you do choose to use them. 

## Language Support

This library was developed and tested using **C++20** with **MSVC 2022 (x64)** on **Windows 11**. However, since it has no platform-specific dependencies, it should compile and run on any platform that supports modern C++ (C++20 or later).

## License

See `LICENSE` for details.

----------
