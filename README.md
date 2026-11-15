# Programming Paradigms homework

Homework from the Programming Paradigms course at Free University of Tbilisi. The course is based on Stanford's CS107, so the handouts, starter code and checker binaries in each folder belong to the course. The files I wrote are in parentheses.

- `assn-01` random sentence generator, C++ (`rsg.cc`)
- `assn-02` six degrees: shortest chain of movies between two actors, read straight out of a binary IMDB dump, C++ (`imdb.cc`, `six-degrees.cc`)
- `assn-03` generic vector and hashset in C (`vector.c`, `hashset.c`)
- `assn-04` RSS news search built on that vector and hashset, C (`rss-news-search.c`)
- `assn-06` multithreaded bank with pthreads, C (`account.c`, `branch.c`, `bank.c`, `teller.c`, `report.c`)
- `assn-07` "Where am I?", finding a position from distances to stars, Scheme (`where-am-i.scm`)

Each folder still has the course README (in Georgian) with the build and test commands. Usually it is `make` and then the checker that comes with the assignment:

```
cd assn-01
make
make test_all
```

A few notes if you want to run them:

- I worked in the course's Debian VM (x86-64). The checkers are Linux binaries and need valgrind.
- Assignments 2, 3 and 4 download their test data on the first `make`.
- Assignment 4 links against a 32-bit library, so it needs the i386 packages listed in its README.
- Assignment 6 has no separate checker, the tests are built into the driver: `./bankdriver -t1 -w4`, tests 1 to 7.
- Assignment 7 runs in Kawa: `(load "where-am-i.scm")` and then `(test-all)`.
