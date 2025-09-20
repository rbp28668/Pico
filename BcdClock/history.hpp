#ifndef HISTORY_HPP
#define HISTORY_HPP

#include "pico/stdlib.h"

class History {

  uint64_t minute;
  int minuteBit;

  uint64_t hours[24];
  int hourBit;
  int hourOfDay;

  public:
  History();  
  void add(bool active);  

  uint64_t current(){ return minute;}
  uint64_t* past() { return hours;}
  
};

#endif