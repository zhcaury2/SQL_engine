#include <iostream>
#include <string>
#include <vector>
#include "tokenize.h"
using namespace std;

int main() {
    string query;
    cout << "Enter SQL query: " << endl;
    getline(cin, query);
    vector<Token> tokens = parseSQLquery(query);
    return 0;
}