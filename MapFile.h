#include <map>
#include <giomm.h>
#include <fstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
class MapFile{
    std::map<std::string , std::string> table;
public:
    void load_file(fs::path file);
    void save_file(fs::path file);
    std::string &value_of(std::string key);
};

bool string_to_bool(std::string str);
std::string bool_to_string(bool condition);