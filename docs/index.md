---
layout: default
title: "Home"
nav_order: 1
has_children: true
---
<img src = "liu_logo_2.jpg" alt="LIU Logo" width = "200">

#### Why LIU?
Standard R data frames performs searches using vector scans (O(n)). This means that to find single value in a million-row table, R must check every single row. For large amounts of data that might not be enough.

#### Structure
Learn more from [this video](https://www.youtube.com/watch?v=o_2psWN8k_c).

LIU implements SQL-like indexes for data frames in R. It allows to index columns of data frames by creating light B+trees with pairs ```key-row_number```. This shifts complexity of searching from linear to logaritmic (O(log n)), which makes few other functions (exp. min, max, count, merge) much faster.

#### Key Advantages
- **Lightning Fast Lookups:** Instead of scanning the whole column, LIU traverses short tree. 
- **Ordered Storage:** B+tree keeps keys in leaves ordered, which mkes functions like ```liu_min```, ```liu_max```, or ```liu_search_range``` extremely efficient.
- **Memory Efficiency:** LIU stores only pairs ```key-row_number``` in compact C-structure, which keeps memory usage reasonable.
- **Optimized Joins:** By using index on the right data frame of join, ```liu_join()``` avoids expensie O(n*m) Cartesian product complexity and performs in O(n * log m).

#### Contents:
1. [Installation](installation.md)
1. [Functions](functions.md)
2. [Performance](performance.md)











