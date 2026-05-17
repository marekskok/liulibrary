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
* [liu_search_prefix()](#liu_search_prefix) - Prefix lookup
* [liu_min() / liu_max()](#liu_min--liu_max) - Extremes.
* [liu_join()](#liu_join) - High-performance joining.
* [liu_isin()](#liu_isin) - Membership testing.

#### liu_build()
This function is foundation of LIU package. It builds LIU index for specified column. Works only for columns of type integer, double and string. The values of given column are stored in B+Tree with their row numbers. It ignores ```NA```.

Params: 
```df``` - Data frame to be indexed.
```column_name``` - Character string specifying the column to index. (int, double or string column)

Technical: The function extract vector from df and iterates through it performing ```insert()```. For integers and doubles there are diffrent C structures, but strings are just mapped by internal dictionary and then it builds integer index. Thats why ```idx_string``` have one more attribute.
```R
df <- data.frame(
    id = as.integer(c(1,2,3)),
    val = c(3.5,6.7,2.1),
    country = c("Poland", "Hungary", "England")
)
idx_int <- liu_build(df, "id")
idx_double <- liu_build(df, "val")
idx_string <- liu_build(df, "country")
```
Returns external pointer to the C object. You can check its type by calling.
```R
# int
> idx_int
<pointer: 0x561c13c4a480>
attr(,"class")
[1] "liu_pointer_int"
# double
> idx_double
<pointer: 0x561c0d41def0>
attr(,"class")
[1] "liu_pointer_double"
# string
> idx_string
<pointer: 0x561c13c39f70>
attr(,"class")
[1] "liu_pointer_string"
attr(,"dictionary")
[1] "England" "Hungary" "Poland" 
```
#### liu_free() 
Explicitly releases the memory allocated for the LIU index in C. While R's Garbage Collector will eventually do it when pointer is no longer needed, calling this function allows to do it immediately. 

Params:
```index``` - LIU index object (external pointer)

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
 Performs a fast lookup in the LIU index to find all row indices associated with given vector of keys. Vector of keys (integer, double or character(string) must match LIU index type) to search for. ```NA```'s are ignored. 

Params:
```index``` - LIU index object (external pointer).
```key``` - Vector of keys (int or double must match LIU index type) to search for.

Return:
Integer vector of row indices where the keys were found. Returns an empty vector if none of the keys exist.

 Technical: On every level of B+tree it compares given key with values in nodes and choses its path to the correct leaf. Then it returns all row numbers assosiated with it (assosiated with key).
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
Finds all row indices with keys within a specified numerical range ```[start, end)``` in LIU. index. You can leave the start, the end, or both blank for an unbounded count. Function works only for integer and double LIU indices (don't try to search words in range in string LIU index).

Params:
```index``` - A LIU index object (external pointer).
```start``` - Numeric scalar, beginning of the range (inclusive). Must match index type (int or double).
```end``` - Numeric scalar, end of the range (exclusive). Must match index type (int or double).

Return:
Integer vector of row indices with keys within the range.

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
#### liu_search_prefix()
Fast prefix search for string-based LIU index. Works only for single prefixes not vectorized.

Params:
```index``` A LIU string index object (external pointer).
```prefix``` Character scalar.

Return:
An integer vector containing sorted row indices where the keys match the specified prefix.

Technical: It identifies the range of matching keys in logarithmic time and retrieves corresponding row indices from the integer B+Tree using ```liu_search_range```.
```R
df <- data.frame(
    id = as.integer(c(1,2,3)),
    val = c(3.5,6.7,2.1),
    country = c("Poland", "Hungary", "Portugal")
)
idx <- liu_build(df, "country")

# Countries starting with "Po"
liu_search_prefix(idx, "Po")
# [1] 1 3

# Countries starting with "H"
liu_search_prefix(idx, "H")
# [1] 2
```



#### liu_min() / liu_max()
Search for the smallest or largest key in LIU index and returs their row indices. Works for all types of LIU index.

Params:
```index``` - A LIU index object (external pointer).

Return:
An integer vector of row indices corresponding to the minimum/maximum key.

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
Performs a high-performance multithreaded join between two data frames using a LIU index. For now only inner (default) and left join are available. It takes 4 arguments: left data table, name of one of its columns, right data table, index built on it. Type of chosen column must match index type. Indexes doesn't take ```NA```, so ```NA``` in given column are ignored. Works for all types of LIU index.

Params:
```df_left``` - Data frame (left side of the join).
```column_name``` - Name of the join column (must exist in left data frames).
```df_right``` - The "indexed" data frame (right side of the join).
```index``` - A LIU index object built on the join column of `df_right`.
```how``` A character string "inner" or "left".

Return:
It returns the same data frame as ```merge(df_left, df_right, "id", incomparables = NA)```

Technical: The function iterates through the join column of the left data frame and performs a concurrent lookup for each key in the right data frame's LIU index. In order to maximize performace multiple threads are used. Builds two vectors with corresponding row indices from both. Finally, it assembles the resulting data frame based on these mappings. Notice that for type string LIU index this function need to translate whole column from left df to integers with right's df index. Thats why it is slightly slower.
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
Checks if given keys are present in LIU index. For ```NA``` it returns ```FALSE```. Works for all types of LIU index.

Params:
```index``` A LIU index object (external pointer).
```keys``` Vector of keys (int or double must match LIU index type) to check.

Return:
Vector of logical values

Technical: Just uses ```liu_search()```.
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