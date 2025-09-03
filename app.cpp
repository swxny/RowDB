#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <functional>

#define VERSION "1.0.0"
#define SOFTWARE_NAME "RowDB"

// Utility functions
std::vector<std::string> split(const std::string &s, char delimiter);
std::string trim(const std::string &s);
std::string toLower(const std::string &s);
bool isNumber(const std::string &s);
bool fileExists(const std::string &filename);

// Cell class representing a single cell in the table
class Cell {
private:
    std::string value;
public:
    Cell() : value("") {}
    Cell(const std::string &val) : value(val) {}
    
    std::string getValue() const { return value; }
    void setValue(const std::string &val) { value = val; }
    
    friend std::ostream& operator<<(std::ostream& os, const Cell& cell) {
        os << cell.value;
        return os;
    }
};

// Column class representing a column in the table
class Column {
private:
    std::string name;
    std::vector<Cell> cells;
public:
    Column() : name("") {} // Default constructor
    Column(const std::string &colName) : name(colName) {}
    
    std::string getName() const { return name; }
    void setName(const std::string &colName) { name = colName; }
    
    size_t size() const { return cells.size(); }
    
    Cell& operator[](size_t index) {
        if (index >= cells.size()) {
            cells.resize(index + 1);
        }
        return cells[index];
    }
    
    const Cell& operator[](size_t index) const {
        static Cell emptyCell;
        if (index < cells.size()) {
            return cells[index];
        }
        return emptyCell;
    }
    
    void addCell(const std::string &value) {
        cells.emplace_back(value);
    }
    
    void insertCell(size_t index, const std::string &value) {
        if (index >= cells.size()) {
            cells.resize(index + 1);
        }
        cells[index] = Cell(value);
    }
    
    void removeCell(size_t index) {
        if (index < cells.size()) {
            cells.erase(cells.begin() + index);
        }
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Column& col) {
        os << col.name << ": ";
        for (const auto& cell : col.cells) {
            os << cell.getValue() << " ";
        }
        return os;
    }
};

// Table class representing a complete table
class Table {
private:
    std::string name;
    std::map<std::string, Column> columns;
    std::vector<std::string> columnOrder;
    
public:
    Table() : name("") {} // Default constructor
    Table(const std::string &tableName) : name(tableName) {}
    
    std::string getName() const { return name; }
    
    void addColumn(const std::string &colName) {
        if (columns.find(colName) == columns.end()) {
            columns[colName] = Column(colName);
            columnOrder.push_back(colName);
        }
    }
    
    void removeColumn(const std::string &colName) {
        auto it = columns.find(colName);
        if (it != columns.end()) {
            columns.erase(it);
            columnOrder.erase(std::remove(columnOrder.begin(), columnOrder.end(), colName), columnOrder.end());
        }
    }
    
    Column& getColumn(const std::string &colName) {
        return columns[colName];
    }
    
    const Column& getColumn(const std::string &colName) const {
        static Column emptyColumn("");
        auto it = columns.find(colName);
        if (it != columns.end()) {
            return it->second;
        }
        return emptyColumn;
    }
    
    std::vector<std::string> getColumnNames() const {
        return columnOrder;
    }
    
    size_t getRowCount() const {
        if (columns.empty()) return 0;
        return columns.begin()->second.size();
    }
    
    Cell& getCell(const std::string &colName, size_t rowIndex) {
        return columns[colName][rowIndex];
    }
    
    const Cell& getCell(const std::string &colName, size_t rowIndex) const {
        static Cell emptyCell;
        auto it = columns.find(colName);
        if (it != columns.end()) {
            return it->second[rowIndex];
        }
        return emptyCell;
    }
    
    void setCell(const std::string &colName, size_t rowIndex, const std::string &value) {
        columns[colName][rowIndex].setValue(value);
    }
    
    void addRow(const std::vector<std::string> &values) {
        if (values.size() != columnOrder.size()) {
            throw std::runtime_error("Number of values doesn't match number of columns");
        }
        
        for (size_t i = 0; i < columnOrder.size(); i++) {
            columns[columnOrder[i]].addCell(values[i]);
        }
    }
    
    void saveToFile(const std::string &filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }
        
        file << "TABLE:" << name << "\n";
        file << "COLUMNS:";
        for (size_t i = 0; i < columnOrder.size(); i++) {
            if (i > 0) file << ",";
            file << columnOrder[i];
        }
        file << "\n";
        
        size_t rowCount = getRowCount();
        file << "ROWS:" << rowCount << "\n";
        file << "DATA:\n";
        
        for (size_t i = 0; i < rowCount; i++) {
            for (size_t j = 0; j < columnOrder.size(); j++) {
                if (j > 0) file << ",";
                file << columns[columnOrder[j]][i].getValue();
            }
            file << "\n";
        }
        
        file.close();
    }
    
    static Table loadFromFile(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        std::string line;
        
        // Read table name
        std::getline(file, line);
        if (line.substr(0, 6) != "TABLE:") {
            throw std::runtime_error("Invalid file format: missing TABLE header");
        }
        std::string tableName = line.substr(6);
        
        // Read columns
        std::getline(file, line);
        if (line.substr(0, 8) != "COLUMNS:") {
            throw std::runtime_error("Invalid file format: missing COLUMNS header");
        }
        std::string columnsStr = line.substr(8);
        std::vector<std::string> colNames = split(columnsStr, ',');
        
        Table table(tableName);
        for (const auto& colName : colNames) {
            table.addColumn(colName);
        }
        
        // Read row count
        std::getline(file, line);
        if (line.substr(0, 5) != "ROWS:") {
            throw std::runtime_error("Invalid file format: missing ROWS header");
        }
        size_t rowCount = std::stoul(line.substr(5));
        
        // Skip DATA line
        std::getline(file, line);
        
        // Read data
        for (size_t i = 0; i < rowCount; i++) {
            std::getline(file, line);
            std::vector<std::string> values = split(line, ',');
            
            if (values.size() != colNames.size()) {
                throw std::runtime_error("incorrect syntax in row " + std::to_string(i));
            }
            
            for (size_t j = 0; j < colNames.size(); j++) {
                table.setCell(colNames[j], i, values[j]);
            }
        }
        
        file.close();
        return table;
    }
    
    void displayASCII() const {
        if (columns.empty()) {
            std::cout << "Table is empty." << std::endl;
            return;
        }
        // Calculate column widths
        std::vector<size_t> colWidths;
        for (const auto& colName : columnOrder) {
            colWidths.push_back(colName.length());
        }
        size_t rowCount = getRowCount();
        for (size_t i = 0; i < rowCount; i++) {
            for (size_t j = 0; j < columnOrder.size(); j++) {
                const Cell& cell = getCell(columnOrder[j], i);
                if (cell.getValue().length() > colWidths[j]) {
                    colWidths[j] = cell.getValue().length();
                }
            }
        }
        // Add extra width for line numbers
        size_t lineNumWidth = std::to_string(rowCount).length();
        // Print header
        std::cout << "+" << std::string(lineNumWidth + 2, '-') << "+";
        for (size_t width : colWidths) {
            std::cout << std::string(width + 2, '-') << "+";
        }
        std::cout << std::endl;
        std::cout << "| " << std::setw(lineNumWidth) << std::left << "#" << " |";
        for (size_t j = 0; j < columnOrder.size(); j++) {
            std::cout << " " << std::setw(colWidths[j]) << std::left << columnOrder[j] << " |";
        }
        std::cout << std::endl;
        std::cout << "+" << std::string(lineNumWidth + 2, '-') << "+";
        for (size_t width : colWidths) {
            std::cout << std::string(width + 2, '-') << "+";
        }
        std::cout << std::endl;
        // Print rows with line numbers
        for (size_t i = 0; i < rowCount; i++) {
            std::cout << "| " << std::setw(lineNumWidth) << std::left << (i + 1) << " |";
            for (size_t j = 0; j < columnOrder.size(); j++) {
                const Cell& cell = getCell(columnOrder[j], i);
                std::cout << " " << std::setw(colWidths[j]) << std::left << cell.getValue() << " |";
            }
            std::cout << std::endl;
        }
        std::cout << "+" << std::string(lineNumWidth + 2, '-') << "+";
        for (size_t width : colWidths) {
            std::cout << std::string(width + 2, '-') << "+";
        }
        std::cout << std::endl;
    }
};

// DatabaseManager class to handle multiple tables and commands
class DatabaseManager {
private:
    std::map<std::string, Table> tables;
    Table* currentTable = nullptr;
    
public:
    void createTable(const std::string &tableName, const std::vector<std::string> &columns) {
        if (tables.find(tableName) != tables.end()) {
            throw std::runtime_error("Table already exists: " + tableName);
        }
        
        Table newTable(tableName);
        for (const auto& col : columns) {
            newTable.addColumn(col);
        }
        
        tables[tableName] = newTable;
        currentTable = &tables[tableName];
        std::cout << "Table '" << tableName << "' created successfully." << std::endl;
    }
    
    void loadTable(const std::string &filename) {
    std::string actualFilename = filename;
    
    // Check if file exists with .odt extension if not specified
    if (!fileExists(filename)) {
        if (!fileExists(filename + ".odt")) {
            throw std::runtime_error("Cannot open file: " + filename + 
                                   " (also tried: " + filename + ".odt)");
        }
        actualFilename = filename + ".odt";
    }
    
    Table table = Table::loadFromFile(actualFilename);
    tables[table.getName()] = table;
    currentTable = &tables[table.getName()];
    std::cout << "Table '" << table.getName() << "' loaded successfully from '" 
              << actualFilename << "'." << std::endl;
    }
    
    void saveTable(const std::string &filename) {
        if (!currentTable) {
            throw std::runtime_error("No table selected");
        }
        
        currentTable->saveToFile(filename);
        std::cout << "Table saved to '" << filename << "' successfully." << std::endl;
    }
    
    void selectTable(const std::string &tableName) {
        auto it = tables.find(tableName);
        if (it == tables.end()) {
            throw std::runtime_error("Table not found: " + tableName);
        }
        
        currentTable = &it->second;
        std::cout << "Selected table: " << tableName << std::endl;
    }
    
    void displayCurrentTable() {
        if (!currentTable) {
            throw std::runtime_error("No table selected");
        }
        
        currentTable->displayASCII();
    }
    
    void editCell(const std::string &cellRef, const std::string &newValue) {
        if (!currentTable) {
            throw std::runtime_error("No table selected");
        }
        // Parse cell reference (e.g., "Name5" or "A5")
        size_t i = 0;
        while (i < cellRef.length() && !isdigit(cellRef[i])) ++i;
        if (i == 0 || i == cellRef.length()) {
            throw std::runtime_error("Invalid cell reference: " + cellRef);
        }
        std::string colName = cellRef.substr(0, i);
        std::string rowStr = cellRef.substr(i);
        if (!isNumber(rowStr)) {
            throw std::runtime_error("Invalid row number: " + rowStr);
        }
        size_t rowIndex = std::stoul(rowStr) - 1; // Convert to 0-based index
        // Check if column exists
        auto colNames = currentTable->getColumnNames();
        if (std::find(colNames.begin(), colNames.end(), colName) == colNames.end()) {
            throw std::runtime_error("Column not found: " + colName);
        }
        // Automatically expand rows if needed
        while (rowIndex >= currentTable->getRowCount()) {
            std::vector<std::string> emptyRow(colNames.size(), "");
            currentTable->addRow(emptyRow);
        }
        currentTable->setCell(colName, rowIndex, newValue);
        std::cout << "Cell " << cellRef << " updated to: " << newValue << std::endl;
    }
    
    void addRow(const std::vector<std::string> &values) {
        if (!currentTable) {
            throw std::runtime_error("No table selected");
        }
        
        currentTable->addRow(values);
        std::cout << "Row added successfully." << std::endl;
    }
    
    void listTables() {
        if (tables.empty()) {
            std::cout << "No tables loaded." << std::endl;
            return;
        }
        
        std::cout << "Available tables:" << std::endl;
        for (const auto& pair : tables) {
            std::cout << "  " << pair.first << std::endl;
        }
    }
    
    bool hasCurrentTable() const {
        return currentTable != nullptr;
    }
    
    std::string getCurrentTableName() const {
        return currentTable ? currentTable->getName() : "";
    }
};

// Utility function implementations
std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    size_t end = s.find_last_not_of(" \t\n\r\f\v");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string toLower(const std::string &s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool isNumber(const std::string &s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

bool fileExists(const std::string &filename) {
    std::ifstream file(filename);
    return file.good();
}

// Main function and command processing
void showHelp() {
    std::cout << SOFTWARE_NAME << " - Personal Data Table Manager" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << SOFTWARE_NAME << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -c, --create <table> [columns...]  Create a new table" << std::endl;
    std::cout << "  -e, --edit <cellRef> <value>       Edit a cell (e.g., A5)" << std::endl;
    std::cout << "  -v, --view                         View current table" << std::endl;
    std::cout << "  -s, --select <table>               Select a table" << std::endl;
    std::cout << "  -l, --load <file>                  Load a table from file" << std::endl;
    std::cout << "  -sv, --save <file>                 Save current table to file" << std::endl;
    std::cout << "  --list                             List all loaded tables" << std::endl;
    std::cout << "  --help                             Show this help message" << std::endl;
    std::cout << "  --version                          Show version information" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported Formats:" << std::endl;
    std::cout << "  .odt - Open Data Table (unencrypted)" << std::endl;
}

void showVersion() {
    std::cout << SOFTWARE_NAME << " version " << VERSION << std::endl;
}

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleTitleA(SOFTWARE_NAME);
#endif
    DatabaseManager dbManager;
    
    // If no arguments, start interactive mode
    if (argc == 1) {
        std::cout << SOFTWARE_NAME << " " << VERSION << std::endl;
        std::cout << "Type 'help' for commands or 'exit' to quit." << std::endl;
        
        std::string input;
        while (true) {
            if (dbManager.hasCurrentTable()) {
                std::cout << SOFTWARE_NAME << "/" << dbManager.getCurrentTableName() << " >> ";
            } else {
                std::cout << SOFTWARE_NAME << " >> ";
            }
            
            std::getline(std::cin, input);
            if (input.empty()) continue;
            
            std::vector<std::string> args = split(input, ' ');
            std::string command = toLower(args[0]);
            
            if (command == "exit" || command == "quit") {
                break;
            } else if (command == "help") {
                showHelp();
            } else if (command == "version") {
                showVersion();
            } else if (command == "-c" || command == "--create") {
                if (args.size() < 3) {
                    std::cout << "Error: Table name and at least one column required." << std::endl;
                    continue;
                }
                
                std::string tableName = args[1];
                std::vector<std::string> columns(args.begin() + 2, args.end());
                dbManager.createTable(tableName, columns);
            } else if (command == "-e" || command == "--edit") {
                if (args.size() < 3) {
                    std::cout << "Error: Cell reference and value required." << std::endl;
                    continue;
                }
                
                std::string cellRef = args[1];
                std::string value = args[2];
                for (size_t i = 3; i < args.size(); i++) {
                    value += " " + args[i];
                }
                
                try {
                    dbManager.editCell(cellRef, value);
                } catch (const std::exception &e) {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            } else if (command == "-v" || command == "--view") {
                try {
                    dbManager.displayCurrentTable();
                } catch (const std::exception &e) {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            } else if (command == "-s" || command == "--select") {
                if (args.size() < 2) {
                    std::cout << "Error: Table name required." << std::endl;
                    continue;
                }
                
                std::string tableName = args[1];
                try {
                    dbManager.selectTable(tableName);
                } catch (const std::exception &e) {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            } else if (command == "-l" || command == "--load") {
                if (args.size() < 2) {
                    std::cout << "Error: Filename required." << std::endl;
                    continue;
                }
                std::string filename = args[1];
                try {
                    dbManager.loadTable(filename);
                } catch (const std::exception &e) {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            } else if (command == "-sv" || command == "--save") {
                if (args.size() < 2) {
                    std::cout << "Error: Filename required." << std::endl;
                    continue;
                }
                std::string filename = args[1];
                try {
                    dbManager.saveTable(filename);
                } catch (const std::exception &e) {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            } else if (command == "--list") {
                dbManager.listTables();
            } else {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands." << std::endl;
            }
        }
    } else {
        // Process command line arguments
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }
        
        std::string command = toLower(args[0]);
        
        if (command == "--help") {
            showHelp();
        } else if (command == "--version") {
            showVersion();
        } else {
            std::cout << "For interactive mode, run without arguments." << std::endl;
            std::cout << "Use --help for more information." << std::endl;
        }
    }
    
    return 0;
}