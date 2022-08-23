
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
std::vector<std::string> split(const std::string& text,
                               const std::string& delims) {
  std::vector<std::string> tokens;
  std::size_t start = text.find_first_not_of(delims), end = 0;

  while ((end = text.find_first_of(delims, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = text.find_first_not_of(delims, end);
  }
  if (start != std::string::npos) tokens.push_back(text.substr(start));

  return tokens;
}

int main(int argc, const char** argv) {
  vector<pair<int, string>> dests{};
  auto tokens = split("   8080    1233:8080[3]   4242:6346[223] 243412:51531  ", " ");
  std::transform(
      tokens.begin() + 1, tokens.end(), std::back_inserter(dests),
      [](const string& dest_str) -> pair<int, string> {
        if (auto itr = dest_str.find("["); itr != string::npos) {
          auto itr2 = dest_str.find("]", itr);
          auto priority = std::stoi(dest_str.substr(itr + 1, itr2 - itr - 1));
          return {priority, string(dest_str.begin(), dest_str.begin() + itr)};
        } else {
          return {1, dest_str};
        }
      });

  for (const auto& [prio, dests] : dests) {
    std::cout << prio << " " << dests << "\n";
  }
  return 0;
}