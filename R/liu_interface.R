#' @useDynLib liu
"_PACKAGE"
#'
#' @title
#' Build LIU Index
#'
#' @description
#' This function if foundation of LIU package. It builds a high-performance 
#' SQL-like B+Tree index for a data frame column.
#' Works only for columns of ints, doubles or strings. Values of given column will be 
#' put in B+Tree with their row numbers. It ignores NA.
#'
#' @param df Data frame to be indexed.
#' @param column_name Character string specifying the column to index. 
#' (int, double or string column)
#'
#' @return
#' C LIU index object (pointer).
#'
#' @examples
#' \dontrun{
#' df <- data.frame(
#' id = as.integer(c(1,2,3)),
#' val = c(3.5,6.7,2.1),
#' country = c("Poland", "Hungary", "England")
#' )
#' idx_int <- liu_build(df, "id")
#' idx_double <- liu_build(df, "val")
#' idx_string <- liu_build(df, "country")
#' }
#' @export
liu_build <- function(df, column_name) {
  if (!is.data.frame(df)) {
    stop("Input df must be of a data frame type")
  }
  if (!(column_name %in% names(df))) {
    stop("Column not found in data frame")
  }
  column <- df[[column_name]]
  if (is.numeric(column)) {
    ptr <- .Call("r_build_tree_from_df", column, PACKAGE = "liu")
    return(ptr)
  }
  if (is.character(column)){
    dictionary <- sort(unique(column))
    
    ptr <- .Call("r_build_tree_from_df", match(column, dictionary), PACKAGE = "liu")
    res <- structure(ptr, 
                     dictionary = dictionary,
                      class = "liu_pointer_string")
    return(res)
  } else {
    stop("Column must be numeric or charachter type")
  }
}
#'
#' @title
#' Search for Keys in LIU Index
#'
#' @description
#' Performs a fast lookup in the LIU index to find all row indices associated 
#' with given vector of keys. It ignores NA in keys vector.
#'
#' @param index LIU index object (external pointer).
#' @param key Vector of keys (integer, double or character must match LIU index type) to search for.
#' @return
#' Integer vector of row indices where the keys were found. 
#' Returns an empty vector if none of the keys exist.
#'
#' @examples
#' \dontrun{
#' df <- data.frame(
#'   id = as.integer(c(1,2,3,4)),
#'   val = c(0.5,6.7,2.1,0.5)
#' )
#' idx <- liu_build(df, "val")
#' 
#' row_idx <- liu_search(idx, 6.7)
#' # [1] 2
#'
#' row_ids <- liu_search(idx, c(6.7, 0.5))
#' # [1] 2 1 4
#' }
#' @export
liu_search <- function(index, key) {
  key <- as.vector(na.omit(key))
  if (is.null(index) || typeof(index) != "externalptr") {
    stop("Index must be a valid LIU external pointer.")
  }
  if (!inherits(index, "liu_pointer_int") && !inherits(index, "liu_pointer_double") && !inherits(index, "liu_pointer_string")){
    stop("External pointer is not liu_pointer")
  }
  if (inherits(index, "liu_pointer_int") && !is.integer(key)){
    stop("LIU pointer is type int, but given key isn't, R treats normal number as doubles")
  }
  if (inherits(index, "liu_pointer_double") && !is.double(key)){
    stop("LIU pointer is type double, but given key isn't")
  }
  if (inherits(index, "liu_pointer_string") && !is.character(key)){
    stop("LIU pointer is type string, but given key isn't")
  }
  if (inherits(index, "liu_pointer_string") && is.character(key)){
    ids <- match(key, attributes(index)$dictionary)
    ids <- as.vector(na.omit(ids))
    res <- .Call("r_search_by_key", index, ids, PACKAGE = "liu")
    return(res)
  }
  res <- .Call("r_search_by_key", index, key, PACKAGE = "liu")
  return(res)
}
#'
#' @title
#' Free LIU Index Memory
#'
#' @description
#' Explicitly releases the memory allocated for the LIU index in C. 
#' Use this when index is no longer needed to prevent memory leaks.
#'
#' @param index LIU index object (external pointer).
#'
#' @examples
#' \dontrun{
#' df <- data.frame(
#'   id = as.integer(c(1,2,3)),
#'   val = c(3.5,6.7,2.1)
#' )
#' idx <- liu_build(df, "id")
#' 
#' #...
#' 
#' liu_free(idx)
#'}
#' @export
liu_free <- function(index) {
  if (is.null(index) || typeof(index) != "externalptr") {
    stop("Index must be a valid LIU external pointer.")
  }
  if (inherits(index, "liu_pointer_int") || inherits(index, "liu_pointer_double") || inherits(index, "liu_pointer_string")){
  .Call("r_index_free", index, PACKAGE = "liu")
    attributes(index) <- NULL
    index <- NULL
    invisible(NULL)
  }
}
#'
#' @title
#' Range Search in LIU Index
#'
#' @description
#' Finds all row indices with keys within a specified numerical range [start, end) in LIU. 
#' index. This operation is very efficient due to the B+Tree structure. You can leave
#' the start, the end, or both blank for an unbounded search.
#'
#' @param index A LIU index object (external pointer).
#' @param start Numeric scalar, beginning of the range (inclusive). Must
#' match index type (int or double).
#' @param end Numeric scalar, end of the range (exclusive). Must
#' match index type (int or double).
#'
#' @return
#' Integer vector of row indices with keys within the range.
#' 
#' @examples
#' \dontrun{
#' df <- data.frame(
#'   id = as.integer(c(1,2,3,1)),
#'   val = c(0.5,6.7,2.1,0.5)
#' )
#' idx <- liu_build(df, "id")
#' # Find rows where 1 <= key < 3
#' row_ids <- liu_search_range(idx, as.integer(1), as.integer(3))
#' # [1] 1 2 4

#' # Row indices with keys greater or equal to 2
#' row_ids <- liu_search_range(idx, as.integer(2))
#' # [1] 2 3
#'}
#' @export
liu_search_range <- function(index, start=NA, end=NA) {
  if (is.null(index) || typeof(index) != "externalptr") {
    stop("Index must be a valid LIU external pointer.")
  }
  if (!inherits(index, "liu_pointer_int") && !inherits(index, "liu_pointer_double")){
    stop("External pointer is not liu_pointer")
  }
  if (inherits(index, "liu_pointer_double") && is.na(start)){
    start <- -Inf
  }
  if (inherits(index, "liu_pointer_double") && is.na(end)){
    end <- Inf
  }
  if (inherits(index, "liu_pointer_int") && is.na(start)){
    list <- .Call("r_search_min", index, PACKAGE = "liu")
    start <- as.integer(list[[2]])
  }
  if (inherits(index, "liu_pointer_int") && is.na(end)){
    list <- .Call("r_search_max", index, PACKAGE = "liu")
    end <- as.integer(list[[2]] + 1)
  }
  
  if (inherits(index, "liu_pointer_int") && (!is.integer(start) || !is.integer(end))){
    stop("LIU pointer is type int, but given start or end isn't, R treats normal number as doubles")
  }
  if (inherits(index, "liu_pointer_double") && (!is.double(start) || !is.double(end))){
    stop("LIU pointer is type double, but given start or end isn't")
  }
  if (start >= end) {
    return(integer(0))
  }
  
  res <- .Call("r_search_by_range", index, start, end, PACKAGE = "liu")
  res <- sort(res)
  return(res)
}
#'
#' @title
#' Minimum Key in LIU Index
#'
#' @description
#' Search for the smallest key in LIU index and returs their row indices.
#'
#' @param index A LIU index object (external pointer).
#'
#' @return
#' An integer vector of row indices corresponding to the minimum key.
#'
#' @examples
#' \dontrun{
#' df <- data.frame(
#'   id = as.integer(c(1,2,3,1)),
#'   val = c(0.5,6.7,2.1,0.5),
#'   country = c("Poland", "Hungary", "England")
#' )
#' # Could be be build also on "id" and "country"
#' idx <- liu_build(df, "val")
#' 
#' # Get row indices for the smallest and largest key in the index
#' min_rows <- liu_min(idx)
#' # [1] 1 4
#' max_rows <- liu_max(idx)
#' # [1] 2
#'}
#' @export
liu_min <- function(index) {
  if (is.null(index) || typeof(index) != "externalptr") {
    stop("Index must be a valid LIU external pointer.")
  }
  if (!inherits(index, "liu_pointer_int") && !inherits(index, "liu_pointer_double") && !inherits(index, "liu_pointer_string")){
    stop("External pointer is not liu_pointer")
  }
  
  res <- .Call("r_search_min", index, PACKAGE = "liu")
  return(res[[1]])
}
#'
#' @title
#' Maximum Key in LIU Index
#'
#' @description
#' Search for the largest key in LIU index and returns their row indices.
#'
#' @param index A LIU index object (external pointer).
#'
#' @return
#' An integer vector of row indices corresponding to the maximum key.
#'
#' @examples
#' \dontrun{
#' df <- data.frame(
#'   id = as.integer(c(1,2,3,1)),
#'   val = c(0.5,6.7,2.1,0.5),
#'   country = c("Poland", "Hungary", "England")
#' )
#' idx <- liu_build(df, "val")
#' 
#' # Get row indices for the smallest and largest key in the index
#' min_rows <- liu_min(idx)
#' # [1] 1 4
#' max_rows <- liu_max(idx)
#' # [1] 2
#' }
#' @export
liu_max <- function(index) {
  if (is.null(index) || typeof(index) != "externalptr") {
    stop("Index must be a valid LIU external pointer.")
  }
  if (!inherits(index, "liu_pointer_int") && !inherits(index, "liu_pointer_double") && !inherits(index, "liu_pointer_string")){
    stop("External pointer is not liu_pointer")
  }
  
  res <- .Call("r_search_max", index, PACKAGE = "liu")
  return(res[[1]])
}
#'
#' @title
#' Fast Inner Join
#'
#' @description
#' Performs a high-performance Join between two data frames using a LIU index. 
#' For now only inner (default) and left join are available. It takes 4
#' arguments: left data table, name of one of it's columns, right data
#' table, index built on it. Type of chosen column must match index type.
#' Indexes doesn't take NA, so NA in given column are ignored. It returns the same
#' data frame as merge(df_left, df_right, "id", incomparables = NA)
#' 
#' @param df_left Data frame (left side of the join).
#' @param column_name Name of the join column (must exist in left data frames).
#' @param df_right The "indexed" data frame (right side of the join).
#' @param index A LIU index object built on the join column of `df_right`.
#' @param how A character string "inner" or "left".
#'
#' @return
#' Returns merged data frame.
#'
#' @examples
#' \dontrun{
#' df_left <- data.frame(
#'   id1 = as.integer(c(1,2,2,3)),
#'   val = c(0.5,6.7,3.2,4.5)
#' )
#' df_right <- data.frame(
#'   id2 = as.integer(c(1,1,2)),
#'   val2 = c(2.2,0.7,8.9)
#' )
#' idx <- liu_build(df_right, "id2")
#' 
#' # inner:
#' merged <- liu_join(df_left, "id1", df_right, idx)
#' #   id1 val id2 val2
#' # 1   1 0.5   1  2.2
#' # 2   1 0.5   1  0.7
#' # 3   2 6.7   2  8.9
#' # 4   2 3.2   2  8.9
#' 
#' # left:
#' merged <- liu_join(df_left, "id1", df_right, idx, "left")
#' #   id1 val id2 val2
#' # 1   1 0.5   1  2.2
#' # 2   1 0.5   1  0.7
#' # 3   2 6.7   2  8.9
#' # 4   2 3.2   2  8.9
#' # 5   3 4.5  NA   NA
#' }
#' @export
liu_join <- function(df_left, column_name, df_right, index, how="inner") {
  if (!is.character(column_name)) {
    stop("Column_name must be character")
  }
  if (!is.data.frame(df_right) || !is.data.frame(df_left)) {
    stop("Input df must be data frame type")
  }
  if (!(column_name %in% names(df_left))) {
    stop("Column not in column names")
  }
  if (is.null(index) || typeof(index) != "externalptr") {
    stop("Index must be a valid LIU external pointer.")
  }
  
  is_int <- inherits(index, "liu_pointer_int")
  is_double <- inherits(index, "liu_pointer_double")
  is_string <- inherits(index, "liu_pointer_string")
  
  if (!is_int && !is_double && !is_string){
    stop("External pointer is not a valid liu_pointer")
  }
  
  if (how == "inner") {
    left = FALSE
  } else if (how == "left"){
    left = TRUE
  } else {
    stop("Only inner and left are available")
  }
  
  id_vector <- df_left[[column_name]]
  
  if (is_int && !is.integer(id_vector)){
    stop("Column type and index type don't match")
  }
  if (is_double && !is.double(id_vector)){
    stop("Column type and index type don't match")
  }
  if (is_string && !is.character(id_vector)){
    stop("Column type and index type don't match")
  }
  
  # translating string column to int
  if (is_string) {
    dictionary <- attr(index, "dictionary")
   
    if (left) {
    # we need to save words in left df that aren't in right index because of left join
    left_unique <- unique(id_vector)
    
    new_words <- left_unique[!(left_unique %in% dictionary)]
    new_words <- na.omit(new_words)
    
    dictionary <- c(dictionary, new_words)
    }
    
    df_left[[column_name]] <- as.integer(match(id_vector, dictionary))
    
    
  }
  
  merged <- .Call("r_inner_join", df_left, column_name, df_right, index, left)
  
  if (is_string) {
    merged[[column_name]] <- dictionary[merged[[column_name]]]
  }
  
  return (merged)
}
#'
#' @title
#' Is In LIU Index
#'
#' @description
#' Checks if given keys are present in LIU index. For NA it returns FALSE.
#'
#' @param index A LIU index object (external pointer).
#' @param keys Vector of keys (int or double must match LIU index type) to check.
#' @return
#'
#' @return
#' Vector of logical values.
#' 
#' @examples
#' \dontrun{
#' df <- data.frame(
#'   id = as.integer(c(1,2,3,1)),
#'   val = c(0.5,6.7,2.1,0.5)
#' )
#' idx <- liu_build(df, "val")
#' 
#' logical <- liu_isin(idx, c(0.5,NA,2.1,3.4))
#' # [1] TRUE FALSE TRUE FALSE
#' 
#' logical <- liu_isin(idx, 6.7)
#' # [1] TRUE
#' }
#' @export
liu_isin <- function(index, keys){
  if (length(keys) == 0){
    return (as.logical(c()))
  }
  if (is.null(index) || typeof(index) != "externalptr") {
    stop("Index must be a valid LIU external pointer.")
  }
  if (!inherits(index, "liu_pointer_int") && !inherits(index, "liu_pointer_double") && !inherits(index, "liu_pointer_string")){
    stop("External pointer is not liu_pointer")
  }
  if (inherits(index, "liu_pointer_int") && !is.integer(keys)){
    stop("LIU pointer is type int, but given key isn't, R treats normal number as doubles")
  }
  if (inherits(index, "liu_pointer_double") && !is.double(keys)){
    stop("LIU pointer is type double, but given key isn't")
  }
  if (inherits(index, "liu_pointer_string") && !is.character(keys)){
    stop("LIU pointer is type string, but given key isn't")
  }
  res <- logical(length(keys))
  
  for (i in 1:length(keys)){
    if (length(liu_search(index, keys[i])) != 0){
      res[i] <- TRUE
    }
  }
  return (res)
}
#'
#' @title
#' Prefix Search in String LIU Index
#' 
#' @description
#' Fast prefix search for string-based LIU index.
#' It identifies the range of matching keys in logarithmic time and retrieves 
#' corresponding row indices from the B+Tree.
#'
#' @param index A LIU string index object (external pointer).
#' @param prefix Character scalar.
#'
#' @return
#' An integer vector containing sorted row indices where the keys match the specified prefix.
#' 
#' @examples
#' \dontrun{
#' df <- data.frame(
#'   id = as.integer(c(1,2,3)),
#'   val = c(3.5,6.7,2.1),
#'   country = c("Poland", "Hungary", "Portugal")
#' )
#' idx <- liu_build(df, "country")
#' 
#' # Countries starting with "Po"
#' liu_search_prefix(idx, "Po")
#' # [1] 1 3
#'
#' # Countries starting with "H"
#' liu_search_prefix(idx, "H")
#' # [1] 2
#' }
#' @export
liu_search_prefix <- function(index, prefix) {
  if (!inherits(index, "liu_pointer_string") || !is.character(prefix)){
    stop("LIU pointer and prefix must be of type string")
  }
  dictionary <- attributes(index)$dictionary
  
  matching_indices <- which(startsWith(dictionary, prefix))
  
  if (length(matching_indices) == 0) {
    return(integer(0))
  }
  min_idx <- min(matching_indices)
  max_idx <- max(matching_indices) + 1
  
  res <- .Call("r_search_by_range", index, min_idx, max_idx, PACKAGE = "liu")
  
  res <- sort(res)
  return(res)
}








