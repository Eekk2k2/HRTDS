
# HRTDS C++ Library

A lightweight C++ static library for parsing and mapping hierarchical, strongly-typed data using a custom Human-Readable Typed Data Serialization (HRTDS) format.

## Key Features

-   **Custom Data Layout**: Define schemas (`struct` blocks) and data fields with typed identifiers like `string`, `bool`, `int`, `float`, nested structures, and arrays.
    
-   **Memory-mapped struct files**: `StructFile<T>` wraps Windows file mapping APIs to map a C++ struct directly into a file for fast storage and retrieval.
    
-   **Zero-dependency**: Uses only the C++ standard library and Windows API — no external dependencies.
    
-   **Strong typing**: Access parsed values with templated getters (`Get<T>(...)`, `Get_Vector<T>()`).
    

## Installation

1. You can either download the `.lib` along with its header file from the releases page, or build the library yourself from source. 
2. In addition, you can also import the files in your project and include them in-build.

## Usage

1.  **Define a schema and data** in an `.hrtds` file (syntax is provided further down):
    
    ```text
	${															
		&struct& Version : {									
			&int& versionNumber;								
			&bool& betaRelease;									
			&int& releaseDate;									
			&string& author;									
		};														
																
		&int& age : 32;											
		&float& temperature : 16.5f;							
		&bool& developer : true;								
		&string& description : \"This is a description.\";		
																
		&int& currentVersionIndex : 2;							
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

			hrtds::HRTDS hrtdsFile(content);

			// Access values
			int age = hrtdsFile["age"].Get<int>();
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
    
3.  **Additionally: Memory-map a struct**:
    
    ```cpp
    // Define a struct containing only trivially copyable fields
    struct MyData { int x; float y; char name[32]; };
    
    // Then create a new StructFile<T> with a filename
    hrtds::StructFile<MyData> mappedFile("data.bin");
    
    // Writing is the same as modifying the struct
    mappedFile.data->x = 42;
    mappedFile.data->y = 38;
    
    mappedFile.Flush(); // Ensures data is written
    
    // Same with reading
    char[32] name;
    memcpy(&name, mappedFile.data->name, 32);
    
    ```

## Docs

### Terminology

|Name| Description |
|--|--|
| Field | Any set of a `value`, `field name` and `identifier` components |
| Field name | The name of a given field, used for addressing different fields within a `structure` |
| Structure | A data schema with a defined layout of `identifiers` |
| Half-field | Just like a `field` but without the `value`. Used in structures to define the layout. |
| Identifier | Identifies which type of field we are working with |
| Type | A data type, such as an `int`, `float`, `string` or `bool` |

### Syntax

**1. Defining the boundaries**
Start off by defining the the region of a file which will be parsed. This is done through the `hrtds::utils::HRTDS_BEGIN_HARD` and `hrtds::utils::HRTDS_END_HARD` substrings. 

> In the standard implementation these look like `${` and `}$`, respectively, but can through a custom one be whatever. (This also applies to any other substring)

\
*example.hrtds*:
```text
${
	...
}$
```
\
\
**2. Creating a basic field**
A field consists of three main parts. It needs an identifier in the front, a name in the middle, and at the end a value. 
|Field Component|Syntax|Definition|
|---------------|------|----------|
|Identifier|`&<identifier>&`|Wrap a string with the `utils::HRTDS_IDENTIFIER` (which is the `&`)
|Identifier|`...& <name>`| The name always comes after the identifier and before the value.
|Value|`: <value>;`| Wrap the value with a leading `utils::HRTDS_ASSIGNMENT` (`:`, or the 'colon') and a trailing `utils::HRTDS_TERMINATOR` (`;`, or a 'semi colon'). To find more about values head to *# 3. Delving into values*.
**Example field:** `&string& Author : "Eekk2k2";` 
\
*example.hrtds*:

```text
${
	&int& age : 32;
	&float& temperature : 16.5f;
	&bool& developer : true;
	&string& description : "This is a description.";
}$
```
\
\
**3. Delving into values**
The library includes a few built-in value identifiers, called "types". These are any identifier linked directly with a data-type.
|Value Type| Syntax | Definition |
|--|--|--|
|`&int&`|`25`|Non-decimal number, the value must fit all requirements for `std::stoi`|
|`&float&`|`25`, `25.0` or `.5`|Any number, the value must fit all requirements for `std::stof`|
|`&bool&`|`true` or `1`|If the value is equal to `true` or `1` then its evaluated to `true`, if not; `false`|
|`&string&`|`"Hello World"`|Any value wrapped in `utils::HRTDS_QUOTE` (`"`, quotation mark), including semantic|
You can also define your own data schemas. To do so, create a field using the `struct` identifier, and leave the value with a `utils::HRTDS_BEGIN_SOFT` (`{`, opening brace) and a `utils::HRTDS_END_SOFT` (`}`, closing brace). The syntax for this follows a C-like structure:
```
&struct& <name> : {
	
};
```
To define the layout, populate the braces with half-fields. Half-fields are generally similar to regular fields, just lacks the `: value` component. An example of this would look something like: `&int& Age;`. Populated, the struct should look like this:
```
&struct& <name> : {
	&<identifier>& <name1>;
	&<identifier>& <name2>;
	&<identifier>& <name3>;
};
```
In order to use this structure in a field you first set the identifier to the name of the structure, then you modify the value to be a tuple, where each of the values in the tuple is chronologically aligned with the schema:   
 ```
&struct& Version : {
	&int& versionNumber;
	&bool& betaRelease;
	&int& releaseDate;
	&string& author;
};

&Version& version : (3, false, 17534, "Eekk2k2");
```
The final thing remaining are arrays. These are 1:1 syntactically similar to any C-like language's array, with the values separated by a `utils::HRTDS_SEPARATOR` (`,` or better known: the comma), and then wrapped with a leading `utils::HRTDS_BEGIN_ARRAY` (`[`, opening square bracket) and a trailing `utils::HRTDS_END_ARRAY` (`]`, closing square bracket). 
```
&int[]& integers : [1, 2, 3, 4, 5];
```
> Also remember to append a set of both BEG_ARR and END_ARR (`[]`) at the end of an identifier (`&...[]&`. This is to inform the parser about a coming array.

\
*example.hrtds*:
```text
${
	&struct& Version : {
		&int& versionNumber;
		&bool& betaRelease;
		&int& releaseDate;
		&string& author;
	};

	&int& age : 32;
	&float& temperature : 16.5f;
	&bool& developer : true;
	&string& description : "This is a description.";
	
	&int& currentVersionIndex : 2;
	&Version[]& versions : [
		(1, true, 	17534, "Eekk2k2"),
		(2, false, 	17512, "Eekk2k2"),
		(3, false, 	34509, "Eekk2k2"),				
	];
}$
```
## API Reference

### `hrtds::HRTDS`

-   `HRTDS(const std::string &content)`: Construct and parse HRTDS content string.
    
-   `void Parse(const std::string &content)`: Parse or re-parse content.
    
-   `HRTDS_VALUE& operator[](const std::string &key)`: Access a field by name.
    

### `HRTDS_VALUE`

-   `template<typename T> T Get(size_t index = 0)`: Retrieve a typed value.
    
-   `template<typename T> std::vector<T> Get_Vector()`: Retrieve an array of typed values.
    
-   `HRTDS_VALUE& operator[](size_t index)`: Index into array of structures.
    
-   `HRTDS_VALUE& operator[](const std::string &subKey)`: Access nested struct field.
    

### `hrtds::StructFile<T>`

-   `StructFile(const std::string &path)`: Open or create a memory-mapped file of size `sizeof(T)`.
    
-   `void Flush()`: Flush changes to disk.
    
-   `~StructFile()`: Unmaps and closes file handles.
    
## Future Features

1. Right now the library only supports deserialzing the data from a content string, in the future you will also be able to serialize the data.

## Contributing

1.  This library is private and intended for personal use only. Contributions, forks, or modifications are not permitted. All rights reserved. Usage is only allowed with explicit permission from the owner.
    

## License

See `LICENSE` for details.

----------
