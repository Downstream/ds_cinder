#pragma once
#ifndef DS_UTIL_STRINGUTIL_H_
#define DS_UTIL_STRINGUTIL_H_

#include <exception>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <cinder/Vector.h>
#include <cinder/Rect.h>

namespace ds {
// Format conversions
std::wstring		wstr_from_utf8(const std::string&);		
std::string			utf8_from_wstr(const std::wstring&);	

// If you have some ANSI text, iso 8859-1
std::wstring		iso_8859_1_string_to_wstring(const std::string& input);
std::string			iso_8859_1_wstring_to_string(const std::wstring& input);

// Number conversions
template <typename T>
bool wstring_to_value(const std::wstring& str, T& ans){
	std::wistringstream		ss(str);
	T						v;
	ss >> v;
	if (ss.eof() && !ss.fail()) { 
		ans = v;
		return true;
	}
	return false;
}

template <typename T>
bool string_to_value(const std::string& str, T& ans){
	std::istringstream		ss(str);
	T						v;
	ss >> v;
	if (ss.eof() && !ss.fail()) { 
		ans = v;
		return true;
	}
	return false;
}

template <typename T>
std::string value_to_string(T number){
	std::ostringstream ss;
	ss << number;
	return ss.str();
}

template <typename T>
std::wstring value_to_wstring(T number){
	std::wostringstream ss;
	ss << number;
	return ss.str();
}

// extra convenience, so you can use inline
const float string_to_float(const std::string& str);
const float wstring_to_float(const std::wstring& str);
const int string_to_int(const std::string& str);
const int wstring_to_int(const std::wstring& str);
const double string_to_double(const std::string& str);
const double wstring_to_double(const std::wstring& str);

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6) {
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return out.str();
}

std::vector<std::string> split( const std::string &str, const std::string &delimiters, bool dropEmpty = false );
std::vector<std::wstring> split( const std::wstring &str, const std::wstring &delimiters, bool dropEmpty = false );

struct Token
{
  Token(int pos = 0, int size = 0)
	: pos(pos)
	, size(size) {}
  int pos;
  int size;
};

std::vector<std::string> partition( const std::string &str, const std::string &partitioner );
std::vector<std::string> partition(const std::string &str, const std::vector<std::string> &partitioners);
void partition( const std::wstring &str, const std::wstring &partitioner, const Token &token, std::vector<Token> &partitions );
std::vector<std::wstring> partition(const std::wstring &str, const std::vector<std::wstring> &partitioners);

int find_count( const std::string &str, const std::string &token );
int find_count( const std::wstring &str, const std::wstring &token );

void replace( std::string &str, const std::string &oldToken, const std::string &newToken );
void replace( std::wstring &str, const std::wstring &oldToken, const std::wstring &newToken );

void loadFileIntoString( const std::string &filename, std::string &destination );
void loadFileIntoString( const std::wstring &filename, std::wstring &destination );
// Same as above but send each line to the function
void loadFileIntoStringByLine( const std::string &filename, const std::function<void(const std::string& line)>& );
void loadFileIntoStringByLine( const std::wstring &filename, const std::function<void(const std::wstring& line)>& );

void saveStringToFile( const std::string &filename, const std::string &src );
void saveStringToFile( const std::wstring &filename, const std::wstring &src );

// Tokenize the input, passing each token to the supplied function
void tokenize(const std::string& input, const char delim, const std::function<void(const std::string&)>&);
void tokenize(const std::string& input, const std::function<void(const std::string&)>&);

void to_lowercase(std::string& str);
void to_lowercase(std::wstring& str);
void to_uppercase(std::string& str);
void to_uppercase(std::wstring& str);

/// Parses a string into a 3d vector. Example: size="400, 400, 0" the space after the comma is required to read the second and third token.
/// Defaults parameters to 0 if they don't exist.
ci::vec3 parseVector(const std::string &s);

/// Parses a string into a rectangle. Example: size="400, 400, 0, 0", where it's "L, T, W, H" the space after the comma is required to read the second and third token.
/// Defaults parameters to 0 if they don't exist.
ci::Rectf parseRect(const std::string &s);

/// The inverse of parseRect. For an input of ci::Rectf(100.0f, 100.0f, 2220.0f, 1180.0f) returns "100.0, 100.0, 1920.0, 1080.0"
///														X1		Y1		X2		 Y2				   L	  T		 W		 H
std::string unparseRect(const ci::Rectf& v);

/// The inverse of parseVector. For an input of ci::vec3(123.0f, 0.0f, 987.6f) returns "123.0, 0.0, 987.6"
std::string unparseVector(const ci::vec3& v);

/// The inverse of parseVector. For an input of ci::vec2f(123.0f, 0.0f) returns "123.0, 0.0"
std::string unparseVector(const ci::vec2& v);

/// Parse true/false from a string. 
bool parseBoolean(const std::string &s);

/// The inverse of parseBoolean
std::string unparseBoolean(const bool b);

std::vector<std::pair<int, std::string>> extractPairs(const std::string& value, const std::string& leftDelim, const std::string& rightDelim);

} // namespace ds

#endif // DS_UTIL_STRINGUTIL_H_
