/*
CS-300 Final Project
Steven Tepsic
Feb 19, 2026
The purpose of this code is to parse a CSV file fore data on specific courses and store the information into a vector for sorting and editing/displaying
*/

#include <iostream>
#include <time.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace std;

//Definie objects and Functions

//CVS Parsing Functions

namespace csv
{
    class Error : public std::runtime_error
    {

    public:
        Error(const std::string& msg) :
            std::runtime_error(std::string("CSVparser : ").append(msg))
        {
        }
    };

    class Row
    {
    public:
        Row(const std::vector<std::string>&);
        ~Row(void);

    public:
        unsigned int size(void) const;
        void push(const std::string&);
        bool set(const std::string&, const std::string&);

    private:
        const std::vector<std::string> _header;
        std::vector<std::string> _values;

    public:

        template<typename T>
        const T getValue(unsigned int pos) const
        {
            if (pos < _values.size())
            {
                T res;
                std::stringstream ss;
                ss << _values[pos];
                ss >> res;
                return res;
            }
            throw Error("can't return this value (doesn't exist)");
        }
        const std::string operator[](unsigned int) const;
        const std::string operator[](const std::string& valueName) const;
        friend std::ostream& operator<<(std::ostream& os, const Row& row);
        friend std::ofstream& operator<<(std::ofstream& os, const Row& row);
    };

    enum DataType {
        eFILE = 0,
        ePURE = 1
    };

    class Parser
    {

    public:
        Parser(const std::string&, const DataType& type = eFILE, char sep = ',');
        ~Parser(void);

    public:
        Row& getRow(unsigned int row) const;
        unsigned int rowCount(void) const;
        unsigned int columnCount(void) const;
        std::vector<std::string> getHeader(void) const;
        const std::string getHeaderElement(unsigned int pos) const;
        const std::string& getFileName(void) const;

    public:
        bool deleteRow(unsigned int row);
        bool addRow(unsigned int pos, const std::vector<std::string>&);
        void sync(void) const;

    protected:
        void parseHeader(void);
        void parseContent(void);

    private:
        std::string _file;
        const DataType _type;
        const char _sep;
        std::vector<std::string> _originalFile;
        std::vector<std::string> _header;
        std::vector<Row*> _content;

    public:
        Row& operator[](unsigned int row) const;
    };
}

namespace csv {

    Parser::Parser(const std::string& data, const DataType& type, char sep)
        : _type(type), _sep(sep)
    {
        std::string line;
        if (type == eFILE)
        {
            _file = data;
            std::ifstream ifile(_file.c_str());
            if (ifile.is_open())
            {
                while (ifile.good())
                {
                    getline(ifile, line);
                    if (line != "")
                        _originalFile.push_back(line);
                }
                ifile.close();

                if (_originalFile.size() == 0)
                    throw Error(std::string("No Data in ").append(_file));

                parseHeader();
                parseContent();
            }
            else
                throw Error(std::string("Failed to open ").append(_file));
        }
        else
        {
            std::istringstream stream(data);
            while (std::getline(stream, line))
                if (line != "")
                    _originalFile.push_back(line);
            if (_originalFile.size() == 0)
                throw Error(std::string("No Data in pure content"));

            parseHeader();
            parseContent();
        }
    }

    Parser::~Parser(void)
    {
        std::vector<Row*>::iterator it;

        for (it = _content.begin(); it != _content.end(); it++)
            delete* it;
    }

    void Parser::parseHeader(void)
    {
        // No real header row in this file; we expect 4 columns of data
        _header.assign(4, "");
    }

    void Parser::parseContent(void)
    {
        std::vector<std::string>::iterator it;

        it = _originalFile.begin();
        //it++; // skip header

        for (; it != _originalFile.end(); it++)
        {
            bool quoted = false;
            int tokenStart = 0;
            unsigned int i = 0;

            Row* row = new Row(_header);

            for (; i != it->length(); i++)
            {
                if (it->at(i) == '"')
                    quoted = ((quoted) ? (false) : (true));
                else if (it->at(i) == _sep && !quoted)
                {
                    row->push(it->substr(tokenStart, i - tokenStart));
                    tokenStart = i + 1;
                }
            }

            //end
            row->push(it->substr(tokenStart, it->length() - tokenStart));

            // if value(s) missing
            // Excel/CSV often omits trailing empty columns; pad them
            while (row->size() < _header.size()) {
                row->push("");
            }

            // If there are more values than headers, that's truly corrupted
            if (row->size() > _header.size()) {
                throw Error("corrupted data (too many columns)!");
            }

            _content.push_back(row);
        }
    }

    Row& Parser::getRow(unsigned int rowPosition) const
    {
        if (rowPosition < _content.size())
            return *(_content[rowPosition]);
        throw Error("can't return this row (doesn't exist)");
    }

    Row& Parser::operator[](unsigned int rowPosition) const
    {
        return Parser::getRow(rowPosition);
    }

    unsigned int Parser::rowCount(void) const
    {
        return _content.size();
    }

    unsigned int Parser::columnCount(void) const
    {
        return _header.size();
    }

    std::vector<std::string> Parser::getHeader(void) const
    {
        return _header;
    }

    const std::string Parser::getHeaderElement(unsigned int pos) const
    {
        if (pos >= _header.size())
            throw Error("can't return this header (doesn't exist)");
        return _header[pos];
    }

    bool Parser::deleteRow(unsigned int pos)
    {
        if (pos < _content.size())
        {
            delete* (_content.begin() + pos);
            _content.erase(_content.begin() + pos);
            return true;
        }
        return false;
    }

    bool Parser::addRow(unsigned int pos, const std::vector<std::string>& r)
    {
        Row* row = new Row(_header);

        for (auto it = r.begin(); it != r.end(); it++)
            row->push(*it);

        if (pos <= _content.size())
        {
            _content.insert(_content.begin() + pos, row);
            return true;
        }
        return false;
    }

    void Parser::sync(void) const
    {
        if (_type == DataType::eFILE)
        {
            std::ofstream f;
            f.open(_file, std::ios::out | std::ios::trunc);

            // header
            unsigned int i = 0;
            for (auto it = _header.begin(); it != _header.end(); it++)
            {
                f << *it;
                if (i < _header.size() - 1)
                    f << ",";
                else
                    f << std::endl;
                i++;
            }

            for (auto it = _content.begin(); it != _content.end(); it++)
                f << **it << std::endl;
            f.close();
        }
    }

    const std::string& Parser::getFileName(void) const
    {
        return _file;
    }

    /*
    ** ROW
    */

    Row::Row(const std::vector<std::string>& header)
        : _header(header) {
    }

    Row::~Row(void) {}

    unsigned int Row::size(void) const
    {
        return _values.size();
    }

    void Row::push(const std::string& value)
    {
        _values.push_back(value);
    }

    bool Row::set(const std::string& key, const std::string& value)
    {
        std::vector<std::string>::const_iterator it;
        int pos = 0;

        for (it = _header.begin(); it != _header.end(); it++)
        {
            if (key == *it)
            {
                _values[pos] = value;
                return true;
            }
            pos++;
        }
        return false;
    }

    const std::string Row::operator[](unsigned int valuePosition) const
    {
        if (valuePosition < _values.size())
            return _values[valuePosition];
        throw Error("can't return this value (doesn't exist)");
    }

    const std::string Row::operator[](const std::string& key) const
    {
        std::vector<std::string>::const_iterator it;
        int pos = 0;

        for (it = _header.begin(); it != _header.end(); it++)
        {
            if (key == *it)
                return _values[pos];
            pos++;
        }

        throw Error("can't return this value (doesn't exist)");
    }

    std::ostream& operator<<(std::ostream& os, const Row& row)
    {
        for (unsigned int i = 0; i != row._values.size(); i++)
            os << row._values[i] << " | ";

        return os;
    }

    std::ofstream& operator<<(std::ofstream& os, const Row& row)
    {
        for (unsigned int i = 0; i != row._values.size(); i++)
        {
            os << row._values[i];
            if (i < row._values.size() - 1)
                os << ",";
        }
        return os;
    }
}

//Define object for course information
struct Course {
	string courseNumber;
	string title;
	vector<string> prereqs;
};

//Create Vector to store each course object in
vector<Course> courses;

//Function to load file
vector<Course> loadCourses(string csvPath) {
    cout << "Loading CSV file " << csvPath << endl;

   

    try {
        // initialize the CSV Parser using the given path
        csv::Parser file = csv::Parser(csvPath);

        // loop to read rows of a CSV file
        for (int i = 0; i < file.rowCount(); i++) {

            // Create a data structure and add to the collection of bids
            Course course;
            course.courseNumber = file[i][0];
            course.title = file[i][1];
            if (!file[i][2].empty()) course.prereqs.push_back(file[i][2]);
            if (!file[i][3].empty()) course.prereqs.push_back(file[i][3]);

            //verify course has enough prereqs
            if (course.prereqs.size() >= 2) {
                // push this bid to the end
                courses.push_back(course);
            }
            else { 
                cout << course.courseNumber << " Does not have 2 or more prerequisites" << endl; 
            }
        }
    }
    catch (csv::Error& e) {
        std::cerr << e.what() << std::endl;
    }
    return courses;
}

void displayCourse(Course course) {
    cout << course.courseNumber << ": " << course.title << " | " << course.prereqs[0] << " | "
        << course.prereqs[1] << endl;
    return;
}

//Function for user to enter bid
Course getCourse() {
    Course course;

    cout << "Enter Id: ";
    cin.ignore();
    getline(cin, course.courseNumber);

    cout << "Enter title: ";
    getline(cin, course.title);

    string prereq;

    cout << "Enter first prereq: ";
    cin >> prereq;
    course.prereqs.push_back(prereq);


    cout << "Enter second prereqt: ";
    cin.ignore();
    cin >> prereq;
    course.prereqs.push_back(prereq);
    
    return course;
}

//function to seaqrch for courses
bool searchCourse(string key) {
    for (const auto& course : courses) {
        if (course.courseNumber == key) {
            displayCourse(course);
            return true;
        }
    }
    cout << "Course not found" << endl;
    return false;
}

//insertion sort function
void insertionSort(vector<Course>& courses) {
    for (size_t i = 1; i < courses.size(); ++i) {
        Course key = courses[i];   // the item to insert
        size_t j = i;

        // Move elements that are greater than key.courseNumber
        // one position ahead of their current position
        while (j > 0 && courses[j - 1].courseNumber > key.courseNumber) {
            courses[j] = courses[j - 1];
            --j;
        }

        courses[j] = key;
    }
}

/**
 * main() method
 */
int main(int argc, char* argv[]) {

    // process command line arguments
    string csvPath;
    switch (argc) {
    case 2:
        csvPath = argv[1];
        break;
    default:
        csvPath = "CS 300 ABCU_Advising_Program_Input.csv";
    }

    // Define a timer variable
    clock_t ticks;

    int choice = 0;
    while (choice != 9) {
        cout << "Menu:" << endl;
        cout << "  1. Load Coursess" << endl;
        cout << "  2. Display All Courses" << endl;
        cout << "  3. Search for courses" << endl;
        cout << "  4. Change file" << endl << "Current file: " << csvPath << endl;
        cout << "  9. Exit" << endl;
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {

        case 1:
            // Initialize a timer variable before loading courses
            ticks = clock();

            // Complete the method call to load the courses
            courses = loadCourses(csvPath);
            insertionSort(courses);

            cout << courses.size() << " courses read and sorted" << endl;

            // Calculate elapsed time and display result
            ticks = clock() - ticks; // current clock ticks minus starting clock ticks
            cout << "time: " << ticks << " clock ticks" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

            break;

        case 2:
            // Loop and display the bids read
            for (int i = 0; i < courses.size(); ++i) {
                displayCourse(courses[i]);
            }
            cout << endl;

            break;

        case 3: {

            // Initialize a timer variable before loading bids
            ticks = clock();

            //search for course
            string key;
            cout << "Enter course number:" << endl;
            cin >> key;
            searchCourse(key);

            // Calculate elapsed time and display result
            ticks = clock() - ticks; // current clock ticks minus starting clock ticks
            cout << "time: " << ticks << " clock ticks" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

            cout << endl;

            break;
        }

        case 4:

            // Initialize a timer variable before loading bids
            ticks = clock();

            //change file name
            cout << "Enter new file name:" << endl;
            cin >> csvPath;

            // Calculate elapsed time and display result
            ticks = clock() - ticks; // current clock ticks minus starting clock ticks
            cout << "time: " << ticks << " clock ticks" << endl;
            cout << "time: " << ticks * 1.0 / CLOCKS_PER_SEC << " seconds" << endl;

            cout << endl;

            break;

        }
    }

    cout << "Good bye." << endl;

    return 0;
}



