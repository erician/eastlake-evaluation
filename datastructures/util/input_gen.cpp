#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <random>

using namespace std;

int main (int argc, char* argv[])
{
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " [# of input]" << endl;
    return 1;
  }

  const size_t numInput = atoi(argv[1]);
  const size_t kKeyLength = 8;
  const size_t kValueLength = 8;

  ofstream ofs("data");
  stringstream ss;
  unordered_map<string, bool> keys;
  keys.reserve(1000000);

  random_device rd;
  default_random_engine dre(rd());
  uniform_int_distribution<int> uniform_dist(0, 160000000);

  cout << "# of input: " << numInput << endl;
  cout << "Key Length: " << kKeyLength << endl;
  cout << "Value Length: " << kValueLength << endl;

  size_t current_records = 0;

  while (current_records < numInput) {
    ss << static_cast<unsigned long>(uniform_dist(dre));
    if (keys.find(ss.str()) == end(keys)) {
      keys.insert({ss.str(), true});
      ofs << ss.str();
      ss.str("");
      ss.clear();
    } else {
      ss.str("");
      ss.clear();
      continue;
    }
    ofs << " ";
    ofs << static_cast<unsigned long>(uniform_dist(dre));
    ofs << endl;
    current_records++;
  }
  
  ofs.close();
  
  return 0;
}
