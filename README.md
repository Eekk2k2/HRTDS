# HRTDS C++ Library

A static library written in C++ for parsing and mapping hierarchical, runtime-typed data structures from a custom Human-Readable Typed Data Serialization (HRTDS) format.

## Key Features

-   **Custom Data Layout**: Define schemas (`struct` blocks) and data fields with typed identifiers for `string`, `bool`, `int`, `float`, nested structures, and arrays.
    
-   **Memory-mapped struct files**: `StructFile<T>` wraps Windows file mapping APIs to map a C++ struct directly into a file for fast storage and retrieval.
    
-   **Zero-dependency**: Uses only the C++ standard library and Windows API—no external dependencies.
    
-   **Strong typing**: Access parsed values with templated getters (`Get<T>(...)`, `Get_Vector<T>()`).
    

## Installation

1. You can either download the `.lib` along with its header file from the releases page, or build the library yourself from source. 
2. In addition, you can also import the files in your project and include them in-build.

## Usage

1.  **Define a schema and data** in an `.hrtds` file:
    
    ```text
    ${
      &struct& Version : {
        &float& Date;
        &int& Version;
        &string& Download;
      };
    
      &string[]& Authors : ["Jenkins", "Joe"];
      &int& Age : 32;
      &bool& Developer : false;
    
      &Version[]& Versions : [
        (4029347892, 4, "https://..."),
        (2374985345, 3, "https://..."),
        (8323457998, 1, "https://..."),
        (3453049583, 8, "https://...")
      ];
    }$
    
    ```
    
2.  **Load and parse**:
    
    ```cpp
    #include "HRTDS.h"
    
    int main() { 
      std::string content; // Populated from a file or network traffic
    
      hrtds::HRTDS doc(content);
    
      // Access scalar values
      int age = doc["Age"].Get<int>();
      bool isDev = doc["Developer"].Get<bool>();
    
      // Access array
      std::vector<std::string> authors = doc["Authors"].Get_Vector<std::string>();
    
      // Access nested structure
      hrtds::HRTDS_STRUCTURE versionStruct = doc["Versions"][1];
      
      // Retrieve fields from the structure
      float date = versionStruct["Date"].Get<float>();
      int verNum = versionStruct["Version"].Get<int>();
      std::string downloadUrl = versionStruct["Download"].Get<std::string>();
    
      std::cout << "First Author: " << authors[0] << std::endl;
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
    char[32] name = mappedFile.data->name;
    
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
