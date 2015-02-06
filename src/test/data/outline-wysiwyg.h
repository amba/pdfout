#define INPUT "\
title1 1 \n\
ζβσ \t2\n\
3 3\t\n\
    4 4\n\
    5 5\n\
        6 6\n\
        7 7\n\
            8 8\n\
\t      \n\
\t    d=1  \n\
\n\
    9 9\n\
empty titles: 9\n\
  9\n\
    .9\n"

#define EXPECTED "\
title1 1\n\
ζβσ 2\n\
3 3\n\
    4 4\n\
    5 5\n\
        6 6\n\
        7 7\n\
            8 8\n\
    9 10\n\
empty titles: 10\n\
 10\n\
     10\n"
