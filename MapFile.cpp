#include "MapFile.h"
using namespace std;
namespace fs = std::experimental::filesystem;

void MapFile::load_file(fs::path file)
{
    ifstream fin(file.string());

    string line , key , value;

    //This line was to get rid of [Desktop Entry] line at the begining of the file
    //Since I'm not going to support it in this implementation so I'm deleting it
    //getline(fin, line);
    while (getline(fin, line))
    {
        stringstream ss(line);
        getline(ss, key, '=');
        getline(ss, value);

        if (table[key] == "")
        {
            table[key] = value;
        }
    }
}

void MapFile::save_file(fs::path file){
    ofstream fout(file.string());
    for(auto prop : table){
        fout << prop.first << "=" << prop.second << endl;
    }
}

std::string &MapFile::value_of(std::string key){
    return table[key];
}

bool string_to_bool(std::string str){
    if(str == "true"){
        return true;
    }else if(str == "false"){
        return false;
    }
}

std::string bool_to_string(bool condition){
    if(condition){
        return "true";
    }else{
        return "false";
    }
}
