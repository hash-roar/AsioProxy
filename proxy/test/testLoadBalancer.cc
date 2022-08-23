#include <algorithm>
#include <iostream>
#include <vector>

void testLambda() {
  std::vector<int> ve{1, 2, 3, 4, 5, 6};
  int sum = 0;
  std::for_each(ve.begin(), ve.end(), [&, i = 1](int a) mutable {
    sum += i;
    sum += a;
    i++;
  });
  std::cout << sum << "\n";
}

void testPriority() {
  std::vector<int> pri{1, 2, 1, 4};

  int acu = 0, last = 0;
  for (int i = 0; i < 20; i++) {
    if (acu == pri[last] - 1) {
      acu = 0;
      int old = last;
      last = (last + 1) % pri.size();
      std::cout << old << " ";
    } else {
      acu++;
      std::cout << last << " ";
    }
  }
}
int main(int argc, const char** argv) {
  testPriority();
  return 0;
}