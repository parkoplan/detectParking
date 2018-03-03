#pragma once

#include <string>
#include <map>

using namespace std;
class ConfigLoad {
public:
  static void parse();
  static string trim(string str);
  static map<string, string> options;
};
