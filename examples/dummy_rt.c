#include "utils.h"

static long step(void* arg) {
  (void) arg;
  return 0;
}

int main(int argc, char* argv[]) {
  return default_main(step, NULL, 100, argc, argv);
}
