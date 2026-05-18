## [0.6] - 16-05-2026
### Added
String handling.
Function ```liu_search_prefix``` for string LIU index.
New unit tests.
New documentation.

### Deleted
```liu_count_range``` - useless and bringing nothing important
```liu_sum_range``` - same


## [0.5] - 10-05-2026
### Added
README.md, Github page.

### Changed
Passed CRAN test.

## [0.4] - 08-05-2026
### Added
Unit tests for all functions.
Included ```NA``` handling.

### Changed
```liu_join``` now uese multithreading for better performance.
```liu_search_range``` can perform unbounded search.

## [0.3] - 05-05-2026
### Added
Better exception handling, checking for strange inputs. 
Function ```liu_sum_range``` that uses ```liu_search_range```.
Added left join in ```liu_join```.

## [0.2] - 03-05-2026
### Added
Double handling. Every functions got their copy changed for doubles.
File ```functions_double_tree.c``` - with C functions for searching in double B+tree.
New function to ```functions_double_tree.c``` and ```functions_int_tree.c``` that performs inner join on two vectors.
### Changed
Files names. 

## [0.1] - 01-05-2026
### Added
Foundation of LIU package.
File ```build_tree.c``` with working mechanism of building B+tree with pairs ```key-row_number```.
File ```tree_logic.h``` with declarations.
File ```search_tree.c``` with C functions for searching in integer B+tree.
    - find_indices()
    - find_indices_max()
    - find_indices_min()
    - find_indices_interval()
File ```apiR_tree.c``` with functions translating R objcets to C.
File ```tree_interface.R``` functions in R with roxygen adnotstions.