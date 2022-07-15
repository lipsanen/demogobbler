#pragma once

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  (size_t)(!(sizeof(a) % sizeof(*(a)))))
