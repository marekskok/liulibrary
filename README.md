<img src = "man/figures/liu_logo_2.jpg" alt="LIU Logo" width = "200">

## LIU - Lightning Index Units
[PAGE](https://marekskok.github.io/liu/) - documentation and performace show
#### Windows
To installl this packege you need to have Rtools installed on your system.
Download it from [CRAN](https://cran.r-project.org/bin/windows/Rtools/). Then run in R:
```R
install.packages("remotes")
remotes::install_github("marekskok/liu")
library(liu)
```
#### Linux (Ubuntu/Debian):
Ensure you have:
```Bash
sudo apt-get install r-base-dev
```
Then in R:
```R
install.packages("remotes")
remotes::install_github("marekskok/liu")
library(liu)
```
#### macOS:
To compile C code on macOS, you need Xcode Commnad Line Tools.
Opend your terminal and run: ```xcode-select --install```
Then in R:
```R
install.packages("remotes")
remotes::install_github("marekskok/liu")
library(liu)
```

### Why LIU?
Standard R data frames performs searches using vector scans (O(n)). This means that to find single value in a million-row table, R must check every single row. For large amounts of data that might not be enough.

### Structure
Learn more from this video: https://www.youtube.com/watch?v=o_2psWN8k_c
LIU implements SQL-like indexes for data frames in R. It allows to index columns of data frames by creating light B+trees with pairs ```key-row_number```. This shifts complexity of searching from linear to logaritmic (O(log n)), which makes few other functions much faster.
### Key Advantages
- **Lightning Fast Lookups:** Instead of scanning the whole column, LIU traverses short tree. 
- **Ordered Storage:** B+tree keeps keys in leaves ordered, which makes functions like ```liu_min```, ```liu_max```, or ```liu_search_range``` extremely efficient.
- **Memory Efficiency:** LIU stores only pairs ```key-row_number``` in compact C-structure, which keeps memory usage reasonable.
- **Optimized Joins:** By using index on the right data frame of join, ```liu_join()``` avoids expensie O(n*m) Cartesian product complexity and performs in O(n * log m).














