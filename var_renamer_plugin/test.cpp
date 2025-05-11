#include <iostream>

int counter;
static int global_state;

void process(int input) {
  static int cache;
  int temp = input;
  counter++;
  global_state = temp;
}