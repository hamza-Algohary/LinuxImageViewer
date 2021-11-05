
#ifndef TEXTEDITOR_H_
#define TEXTEDITOR_H_

#include <string>
#include <sstream>
#include <cmath>
#include <vector>

using namespace std;

/*
template <typename T>
string toString(T var);

string toString(long double var);
string toString(int var);*/


/*double convertToNumber(char c);
double convertToNumber(string str);*/
//int search(char c, string str);

//------------------------------------------------

void clear(char c, string &str);
void clear(int i, string &str);

bool compare(string input , string keyword , int index);
int search_first(string input , string keyword);
vector<int> search_all(string input , string keyword);

bool exist_in(string input , string keyword);
bool exist_in(string input , char c);

string copy_part_of_string(string str, int down, int up);
void cut(string &input , int down , int up , string replacement);

void insert_at(string &input , int index , string to_be_inserted);
void insert_at(string &input , vector<int> index , string to_be_inserted);
void insert_after_each(string &input , string to_be_inserted , string after);

string char_to_string(char c);

bool isNumber(char c);
bool isArithmeticSign(char c);
bool is_letter(char c);
bool is_capital(char c);
bool is_small(char c);

string capitalize(string &input , int index);
string capitalize_all(string &input);
char capitalize(char c);

string uncapitalize(string &input , int index);
string uncapitalize_all(string &input);
char uncapitalize(char c);

#endif