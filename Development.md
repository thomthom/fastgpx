# Development

## Python

### Import Order

The typical organization of Python imports follows a structured convention to improve readability and maintainability of the code. This organization often adheres to the PEP 8 style guide, which is widely adopted in the Python community. Here is the recommended order and format:

Python imports are generally organized into three main sections, each separated by a blank line:

1. Standard Library Imports: These are modules that are part of Python's standard library, such as `os`, `sys`, `datetime`, etc.

2. Third-Party Imports: These are external libraries that are not part of the standard library, such as `numpy`, `requests`, etc.

3. Local Application or Project-Specific Imports: These are your own modules that are part of the project.

```py
# Standard library imports
import os
import sys
from datetime import datetime

# Third-party imports
import requests
import numpy as np

# Local application imports
from my_project.module import my_function
from . import another_module
```

## C++

### Include order

The organization of C++ includes follows certain conventions that are similar in spirit to Python import conventions. Well-structured includes can improve readability, reduce compile times, and minimize dependencies. Here are the typical guidelines and best practices for organizing C++ includes:

C++ includes are generally organized in a specific order, often grouped and separated by blank lines. The typical order is as follows:

1. Header File for the Current Implementation File (if applicable)
2. Standard Library Headers (e.g., `<iostream>`, `<vector>`)
3. Third-Party Library Headers (e.g., `boost`, or other external dependencies)
4. Project-Specific Headers (e.g., your own modules or classes)

This organization helps to ensure that your file includes what it needs directly, and minimizes the chance of accidentally relying on transitive includes from other files.

```cpp
// Current implementation file's corresponding header
#include "my_class.h"

// Standard library headers
#include <iostream>
#include <vector>
#include <string>

// Third-party library headers
#include <boost/algorithm/string.hpp>

// Project-specific headers
#include "utils.h"
#include "data_processing.h"
```

### Reformat all C++ sources

```sh
cd src\cpp
for /R %f in (*.cpp *.hpp) do "C:\Program Files\LLVM\bin\clang-format.exe" -i "%f"
```
