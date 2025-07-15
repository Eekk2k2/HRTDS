# HRTDS (C++20)
A modern and lightweight format for hierarchical, strongly-typed data.
## Key Features
* **Custom Data Layouts**: Define schemas (`struct` blocks) and data fields with typed identifiers like `string`, `bool`, `intN`, `float`, nested structures, and arrays.

* **Extensible Type System**: Add parsing support for any custom C++ type.

* **Zero-dependency**: This library uses only the C++ standard library — no external dependencies.

*  **Strong Typing**: Access parsed values with templated getters (`Get<T>(...)`).

## Installation
1. Download the latest release files.
2. Extract the file to any location you prefer
3. Move the files from the `include/` folder over into *your* projects include folder.
>The hrtds.h file has to exist in your root include folder. (You should only need to write: #include <hrtds.h>)
4.  Link the compiled library 
	- Either build the files in `build/src/`, or
	- Use the prebuilt `.lib` from `library/`, or 
	- Include the files in `build/src/` in your project.
5. Include by writing `#include <hrtds.h>` and start using the library. If you need some help I encourage you to check out the *# Docs*. 

## Example Usage
*example.hrtds:*
```hrtds
${
	&struct& Element : {
		&string& id
	};
	
	&struct& Window : {
		&int32_[]& position,
		&int32_[]& size,
		&string& title,
		&Element[]& elements
	};
	
	&Window[]& windows : [
		(
			[0, 0],
			[1920, 1080],
			"HRTDS Example Application",
			[
				("ae0568a2-a855-4d0e-bfa8-0482c0addefb"),
				("7a996740-a992-4c89-9390-286436288381"),
				("fda8ca15-da9f-4013-b364-b88f7cd28707"),
				("24052f5c-abf5-434d-b0e7-9bbf6b7988f6")
			]
		),
		(
			[960, 540],
			[400, 200],
			"Developed by Eekk2k2",
			[
				("d5e9e2d6-9621-4a85-a6c2-424c77a3a202"),
				("5f31a91a-6960-445c-9a2a-57e3e5da1402"),
				("83720bb2-3cce-4535-8fb3-6e6d97f8c579")
			]
		),
	];
}$
```
*main.cpp*: 
```cpp
#include <hrtds.h>

// include other headers...

struct Vector2i {
	int x, y;
}

int main() {
	// Load from file or recieve from network
	hrtds::HRTDS file = hrtds::HRTDS();
	std::string content = ...;
	
	hrtds::HRTDS::Parse(file, content);

	// Access
	hrtds::Value& windowsArray = file["windows"];
	for (int i = 0; i < windowsArray.size(); i++) {
		hrtds::Value& currentWindow = windowsArray[i];
		Vector2i position = { 
			currentWindow["position"][0].Get<int32_t>(), 
			currentWindow["position"][1].Get<int32_t>() 
		};
		
		std::string title = currentWindow["title"].Get<std::string>();
		
		// ...
	}
	
	// Store
	std::string composed = hrtds::HRTDS::Compose(file);
	// <write to file/send over network>
	
	return 0;
}
```
## Docs

### Syntax / Tutorial

**1. Setting the boundaries**
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
| Value				| `..: <value>;`		| Wrap the value with a leading `Glyph::ASSIGNMENT` (a  'colon', `:`) and a trailing `Glyph::TERMINATOR` (a 'semi colon', `;`). To find more about values head to *# Docs > 3. Delving into values*. |

Example field: `&string& Author : "Eekk2k2";` 

\
*example.hrtds*:

```text
${
	&uint8_& age : 32;
	&float& temperature : 16.5;
	&bool& developer : true;
	&string& description : "This is a description.";
}$
```

\
**3. Delving Into Values**
The library includes a few value identifiers which map to a C++ type.  These value identifiers are referred to as "types" in hrtds too, and if you want to add support for your own types read *# Docs > Extras - Add Custom Type Support*

In the table below you see the built-in types.
| Value Type	| Syntax 		| Definition 																			|
|---------------|---------------------- |---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `&(u)intN_&`	| `25`			| Non-decimal number, the value must fit all requirements for `std::stoi`/`std::stol`/`std::stoll`/`std::stoul`/`std::stoull` (depending on integer bit count). The "-N" represents any power of 2 up until and including 64.|

> Integers are the only built-in type with a trailing underscore. This is because it was too difficult to make out the bit count without it: &int8& vs &int8_&. 

|  |  | |
|-|-|-|
| `&float&`	| `25`, `25.0` or `.5`	| Any number, the value must fit all requirements for `std::stof`												|
| `&double&`	| `25`, `25.0` or `.5`	| Any number, the value must fit all requirements for `std::stod`. Has higher precision than float.								|
| `&bool&`	| `true` or `1`		| If the value is equal to `true` or `1` then its evaluated to `true`, if not, `false`. It is still recommended to write `false` for a falsy value.		|
| `&string&`	| `"Hello World"`	| Any value wrapped in `Glyph::QUOTE`s (`"`, quotation mark).													|

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


### Extras - Add custom type support
A step beyond the in-file data schemas, lies the extensibility of the type system. 

\
**The system**
HRTDS handles types through three functions found in the templated `StaticConverter<T>` struct: 
```cpp
// This function is used for converting from the input value 
// string to it's C++ counterpart. It is a part of the parsing stage.
void* StaticConverter<T>::FromString(const std::string&);
```
```cpp
// This one is for converting a type back into it's string form. It
// is a part of the composing stage.
std::string& StaticConverter<T>::FromString(const void*);
```
```cpp
// Deletes the data in the pointer specified by type T
void StaticConverter<T>::Destroy(void*);
```
> The data returned from the `StaticConverter<T>::FromString(const std::string&)` function will live on the heap and therefore it's lifetime needs to be managed - don't worry though as it is handled by `hrtds::Value`'s destructor (`hrtds::Value::~Value()`), which calls `StaticConverter<T>::Destroy(..)` on its data.

The association between the identifier string found in the `.hrtds` file and the static C++ type happens in the `DynamicConverter` struct's maps:
```cpp
typedef void*(*FromStringFunction)(const std::string&);

// Map associating the identifier name (&<this part>&) to its 
// StaticConverter<T>::FromString() function.
static inline std::unordered_map<std::string, FromStringFunction> FromString{};
```
```cpp
typedef std::string(*ToStringFunction)(const void*);

// Associates the identifier name to its corresponding
// StaticConverter<T>::ToString(..) function.
static inline std::unordered_map<std::string, ToStringFunction> ToString{};
```
```cpp
typedef void(*DestroyFunction)(void* data);

// You get the gist
static inline std::unordered_map<std::string, DestroyFunction> Destroy{};
```
 So all that happens when a new identifier is found is a lookup and a function call, before setting the `hrtds::Value::data` pointer to the result. When you retrieve the data using the `T hrtds::Value::Get<T>()` the only thing which happens is a `reinterpret_cast<T*>(this->data)` and a succeeding dereference before returning.
 > This isn't typesafe and it puts a lot of trust in the user to not mess up. Ideally there would be some checks verifying that data both isn't `nullptr` nor the wrong type. 
 > I am planning to add this next time around, but as of right now my time is up. 

\
**Adding Support  - The Header File**

In the same file where your type lies, either include `hrtds.h` or `hrtds_data.h` (the latter doesn't come included with your `includes/` folder, but you can find it in `build/src/data/`).
```cpp
#include <hrtds_data,h> // lighter include than hrtds.h

class MyType {
	...
};
```

Then specialize the `hrtds::data::StaticConverter<T>` struct with your type. There are two ways to achieve this, depending on the level of control you wish. The simplest way is to utilize the `HRTDS_DATA_STATIC_CONVERTER(Type, alias)` macro. All you need to do is to plug in your type's name and your requested alias (what it will be named in-file). 
```cpp
#include <hrtds_data,h>

class MyType {
	...
};

HRTDS_DATA_STATIC_CONVERTER(MyType, "mytype");
```
> Note that the alias will override any existing override, but this behavior is unpredictable so use an original alias.

\
**Adding Support - The Source File**
### API Reference 
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


###  Language Support

This library was developed and tested using **C++20** with **MSVC 2022 (x64)** on **Windows 11**. However, since it has no platform-specific dependencies, it should compile and run on any platform that supports modern C++ (C++20 or later).

## Contributing

### Supporting ❣️ 

## Licence
See `LICENSE` for details.
