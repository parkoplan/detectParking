#include "ConfigLoad.h"

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>

using namespace std;

map<string, string> ConfigLoad::options;

string ConfigLoad::trim(string str)
{
  size_t first = str.find_first_not_of(' ');
  if (string::npos == first)
  {
    return str;
  }
  replace( str.begin(), str.end(), '\r', ' ');
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}

void ConfigLoad::parse()
{
  ifstream cfgfile("config.cfg");

  string line;
  while (getline(cfgfile, line))
  {
    istringstream is_line(line);
    string key;
    if (getline(is_line, key, '=') && line[0] != '#')
    {
      string value;
      if (getline(is_line, value))
      {
        key = trim(key);
        value = trim(value);
        options[key] = value;
      }
    }
  }
}



//int main()
//{
//    //char * dir = getcwd(NULL, 0); // Platform-dependent, see reference link below
//    //printf("Current dir: %s", dir);
//    
//    parse();
//    for (const auto& p : ConfigLoad::options)
//    {
//        cout << "Option['" << p.first << "'] = " << p.second << '\n';
//    }
//}
