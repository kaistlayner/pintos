#include "tests/main.h"
#include "tests/vm/parallel-merge.h"

void
test_main (void) 
{
  exit(-1);
  parallel_merge ("child-qsort", 72);
}
