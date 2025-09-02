# RowDB

RowDB is a lightweight, interactive C++ database system for managing personal data tables in the console. It supports creating, editing, viewing, saving, and loading tables with a simple command-driven interface.

## Features
- Interactive command-line interface
- Create tables with custom columns
- Edit individual cells using references (e.g., A5)
- View tables in ASCII format with column letters and row numbers
- Save and load tables from files (.odt format)
- Select and switch between multiple tables
- List all loaded tables
- Windows console title set to RowDB

## Getting Started

### Compilation
To compile RowDB, use:
```
g++ -std=c++11 -o app app.cpp
```

### Usage
Double-click `app.exe` or run from the terminal:
```
./app
```

#### Interactive Commands
- `-c, --create <table> [columns...]`  Create a new table
- `-e, --edit <cellRef> <value>`       Edit a cell (e.g., A5)
- `-v, --view`                         View current table
- `-s, --select <table>`               Select a table
- `-l, --load <file>`                  Load a table from file
- `-sv, --save <file>`                 Save current table to file
- `--list`                             List all loaded tables
- `help`                               Show help message
- `version`                            Show version information
- `exit`                               Quit the application

### Table Format
Saved tables use the `.odt` (Open Data Table) format, which is a simple, unencrypted text file.

## Example
```
RowDB 1.0.0
RowDB >> -c contacts Name Phone Email
Table 'contacts' created successfully.
RowDB/contacts >> -e A1 John Doe
Cell A1 updated to: John Doe
RowDB/contacts >> -v
+----------+-----------+-------+
| Name     | Phone     | Email |
+----------+-----------+-------+
| John Doe |           |       |
+----------+-----------+-------+
```

## License
This project is open source and free to use, licensed to [GPL-v3.0](LICENSE.txt)
