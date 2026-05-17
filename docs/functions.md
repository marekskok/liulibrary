---
layout: default
title: "Functions"
nav_order: 3
parent: "Home"
---
### Functions:
* [liu_build()](#liu_build) - Create an index.
* [liu_free()](#liu_free) - Memory management.
* [liu_search()](#liu_search) - Fast lookup.
* [liu_search_range()](#liu_search_range) - Range queries.
* [liu_min() / liu_max()](#liu_min--liu_max) - Extremes.
* [liu_join()](#liu_join) - High-performance joining.
* [liu_isin()](#liu_isin) - Membership testing.

#### liu_build()
This function is foundation of LIU package. It creates LIU index on given column. For now works only with columns of ints or doubles, but ignores NA. 
Technical: It inserts every value with its row number from column to the leaf of the B+tree.
```R
df <- data.frame(
    id = as.integer(c(1,2,3)),
    val = c(3.5,6.7,2.1)
)
idx_int <- liu_build(df, "id")
idx_double <- liu_build(df, "val")
```
Returns external pointer to the c object. You can check its type by calling.
```R
> idx_int
<pointer: 0x559b1e9c5b10>
attr(,"class")
[1] "liu_pointer_int"
attr(,"column_name")
[1] "id"
# or
> idx_double
<pointer: 0x559b1e9c5b10>
attr(,"class")
[1] "liu_pointer_double"
attr(,"column_name")
[1] "val"
```
#### liu_free() 
Explicitly releases the memory allocated for the LIU index in C. While R's Garbage Collector will eventually do it when pointer is no longer needed, calling this function allows to do it immediately. Useful when working with huge data frames.
Technical: It uses recursion to free all nodes of B+tree starting from the bottom(leafs).
```R
df <- data.frame(
    id = as.integer(c(1,2,3)),
    val = c(3.5,6.7,2.1)
)
idx <- liu_build(df, "id")
#
...
#
liu_free(idx)
```
#### liu_search()
 Performs a fast lookup in the LIU index to find all row indices associated with given vector of keys. Vector of keys (int or double must match LIU index type) to search for. NA's are ignored. 
 Techincal: On every level of B+tree it compares given key with values in nodes and choses its path to the correct leaf. Then it returns all row numbers assosiated with it.
```R
df <- data.frame(
    id = as.integer(c(1,2,3,4)),
    val = c(0.5,6.7,2.1,0.5)
)
idx <- liu_build(df, "val")

row_idx <- liu_search(idx, 6.7)
# [1] 2

row_ids <- liu_search(idx, c(6.7, 0.5))
# [1] 2 1 4
```
#### liu_search_range()
Finds all row indices with keys within a specified numerical range [start, end) in LIU. index. You can leave the start, the end, or both blank for an unbounded count.
Technical: It works in the same way as liu_search for 'start', Once the leaf node is reached, it traverses the leaf linked-list horizontally, collecting row indices until a key exceeding the end value is encountered. This makes searches extremely efficient, as it avoids returning to the root.
```R
df <- data.frame(
    id = as.integer(c(1,2,3,1)),
    val = c(0.5,6.7,2.1,0.5)
)
idx <- liu_build(df, "id")
# Find rows where 1 <= key < 3
row_ids <- liu_search_range(idx, as.integer(1), as.integer(3))
# [1] 1 2 4

# Row indices with keys greater or equal to 2
row_ids <- liu_search_range(idx, as.integer(2))
# [1] 2 3
```
#### liu_min() / liu_max()
Search for the smallest or largest key in LIU index and returs their row indices.
Techincal: Looks for the leftmost or rightmost key in tree.
```R
df <- data.frame(
    id = as.integer(c(1,2,3,1)),
    val = c(0.5,6.7,2.1,0.5)
)
idx <- liu_build(df, "val")

# Get row indices for the smallest and largest key in the index
min_rows <- liu_min(idx)
# [1] 1 4
max_rows <- liu_max(idx)
# [1] 2
```

#### liu_join()
Performs a high-performance multithreaded join between two data frames using a LIU index. For now only inner (default) and left join are available. It takes 4 arguments: left data table, name of one of its columns, right data table, index built on it. Type of chosen column must match index type. Indexes doesn't take NA, so NA in given column are ignored. 
It returns the same data frame as merge(df_left, df_right, "id", incomparables = NA)
Technical: The function iterates through the join column of the left data frame and performs a concurrent lookup for each key in the right data frame's LIU index. In order to maximize performace multiple threads are used. Builds two vectors with corresponding row indices from both. Finally, it assembles the resulting data frame based on these mappings.
```R
df_left <- data.frame(
    id1 = as.integer(c(1,2,2,3)),
    val = c(0.5,6.7,3.2,4.5)
)
df_right <- data.frame(
    id2 = as.integer(c(1,1,2)),
    val2 = c(2.2,0.7,8.9)
)
idx <- liu_build(df_right, "id2")

# inner:
merged <- liu_join(df_left, "id1", df_right, idx)
#   id1 val id2 val2
# 1   1 0.5   1  2.2
# 2   1 0.5   1  0.7
# 3   2 6.7   2  8.9
# 4   2 3.2   2  8.9

# left:
merged <- liu_join(df_left, "id1", df_right, idx, "left")
#   id1 val id2 val2
# 1   1 0.5   1  2.2
# 2   1 0.5   1  0.7
# 3   2 6.7   2  8.9
# 4   2 3.2   2  8.9
# 5   3 4.5  NA   NA
```
#### liu_isin()
Checks if given keys are present in LIU index. For NA it returns FALSE. Returns vector of logical values.
Technical: Just uses liu_search().
```R
df <- data.frame(
    id = as.integer(c(1,2,3,1)),
    val = c(0.5,6.7,2.1,0.5)
)
idx <- liu_build(df, "val")

logical <- liu_isin(idx, c(0.5,NA,2.1,3.4))
# [1] TRUE FALSE TRUE FALSE

logical <- liu_isin(idx, 6.7)
# [1] TRUE
```