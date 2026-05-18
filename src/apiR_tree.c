// This file is translator between liu_interface.R nad functions_tree
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <string.h>
#include "declarations.h"
#ifdef _OPENMP
#include <omp.h>
#endif

void r_index_free(SEXP index_ptr) {
    // This function recognizes index type and uses right free function
    if (TYPEOF(index_ptr) != EXTPTRSXP) return;

    if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_int")) {
        // Getting root
        int_node* root = (int_node*)R_ExternalPtrAddr(index_ptr);
        if (root == NULL) return;

        free_tree_int(root);
        R_ClearExternalPtr(index_ptr);
        return;
    } else if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_double")) {
        // Getting root
        double_node* root = (double_node*)R_ExternalPtrAddr(index_ptr);
        if (root == NULL) return;

        free_tree_double(root);
        R_ClearExternalPtr(index_ptr);
        return;
    } else {
    Rf_error("Provided pointer is not a LIU index object");
    }
}

SEXP r_build_tree_from_df(SEXP column) {
    // This function doesn't validate arguments because R file did it.
    // It translate R objects and build C tree.

    if (!Rf_isInteger(column) && !Rf_isReal(column)) {
    Rf_error("Column must be integer or double");
    }

    // Case when int
    if (Rf_isInteger(column)){
        int *data_ptr = INTEGER(column);
        size_t n = Rf_length(column);

        int_node *root = NULL; 
        for (size_t i = 0; i < n; i++) {
            if (data_ptr[i] != NA_INTEGER) {
                insert_int(&root, data_ptr[i], i + 1);
            }
        }
        // Prepering external pointer to send
        SEXP root_ptr = PROTECT(R_MakeExternalPtr(root, Rf_install("liu_pointer_int"), R_NilValue));
        Rf_setAttrib(root_ptr, R_ClassSymbol, Rf_mkString("liu_pointer_int"));
        R_RegisterCFinalizerEx(root_ptr, r_index_free, TRUE);

        UNPROTECT(1);
        return root_ptr;
    } else { // when double
        double *data_ptr = REAL(column);
        size_t n = Rf_length(column);

        double_node *root = NULL; 
        for (size_t i = 0; i < n; i++) {
            if (!ISNA(data_ptr[i]) && !ISNAN(data_ptr[i])){
                insert_double(&root, data_ptr[i], i+1);
            }
        }

        SEXP root_ptr = PROTECT(R_MakeExternalPtr(root, Rf_install("liu_pointer_double"), R_NilValue));
        Rf_setAttrib(root_ptr, R_ClassSymbol, Rf_mkString("liu_pointer_double"));
        R_RegisterCFinalizerEx(root_ptr, r_index_free, TRUE);

        UNPROTECT(1);
        return root_ptr;
    }
}

SEXP r_search_by_key(SEXP index_ptr, SEXP keys_to_find) {
    // Checking for index class and calling right function int or double
    if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_int")) {
        // Getting root
        int_node* root = (int_node*)R_ExternalPtrAddr(index_ptr);

        // Converting key to int
        int_table keys;
        keys.size = LENGTH(keys_to_find);
        keys.pointer = INTEGER(keys_to_find);

        //  Create table for saving found row indices for every element of given vector
        int_table table = {NULL, 0};
        for (size_t i = 0; i<keys.size; i++){
            find_indices_int(root, keys.pointer[i], &table);
        }

        // Create returning vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));
        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));

        free(table.pointer);
        Rf_unprotect(1);
        return indices;
    } else if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_double")){
        // Getting root
        double_node* root = (double_node*)R_ExternalPtrAddr(index_ptr);

        // Converting key to int
        double_table keys;
        keys.size = LENGTH(keys_to_find);
        keys.pointer = REAL(keys_to_find);
        int_table table = {NULL, 0};

        //  Create table for saving found row indices for every element of given vector
        for (size_t i = 0; i<keys.size; i++){
            find_indices_double(root, keys.pointer[i], &table);
        }

        // Create returning vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));
        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));

        free(table.pointer);
        Rf_unprotect(1);
        return indices;
    } else Rf_error("Provided pointer is not a liu_pointer, maybe you freed memory");
}

SEXP r_search_by_range(SEXP index_ptr, SEXP start, SEXP end) {
    // Checking for index class and calling right function int or double
    if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_int") || R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_string")) {
        // Getting root
        int_node* root = (int_node*)R_ExternalPtrAddr(index_ptr);

        // Converting key to int
        int start1 = Rf_asInteger(start);
        int end1 = Rf_asInteger(end);

        int_table table = find_indices_interval_int(root, start1, end1);

        // Create final vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));

        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));
        free(table.pointer);
        Rf_unprotect(1);
        return indices;
    } else if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_double")){
        // Getting root
        double_node* root = (double_node*)R_ExternalPtrAddr(index_ptr);
        
        // Converting key to double
        double start1 = Rf_asReal(start);
        double end1 = Rf_asReal(end);

        int_table table = find_indices_interval_double(root, start1, end1);

        // Creating final vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));

        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));
        free(table.pointer);
        Rf_unprotect(1);
        return indices;
    } else {
       Rf_error("Provided pointer is not a liu_pointer");
    }
}

SEXP r_search_min(SEXP index_ptr) {
    if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_int")) {
        // Getting root
        int_node* root = (int_node*)R_ExternalPtrAddr(index_ptr);
        
        int min = 0;
        int_table table = find_indices_min_int(root, &min);

        // Creating final vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));
        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));
        free(table.pointer);

        SEXP minimum = Rf_protect(Rf_allocVector(INTSXP, 1));
        INTEGER(minimum)[0] = min;

        SEXP res_list = Rf_protect(Rf_allocVector(VECSXP, 2));
        SET_VECTOR_ELT(res_list, 0, indices);
        SET_VECTOR_ELT(res_list, 1, minimum);

        Rf_unprotect(3);
        return res_list;
    } else if(R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_double")) {
        // Getting root
        double_node* root = (double_node*)R_ExternalPtrAddr(index_ptr);

        double min = 0;
        int_table table = find_indices_min_double(root, &min);

        // Creating final vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));
        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));
        free(table.pointer);

        SEXP minimum = Rf_protect(Rf_allocVector(REALSXP, 1));
        REAL(minimum)[0] = min;

        SEXP res_list = Rf_protect(Rf_allocVector(VECSXP, 2));
        SET_VECTOR_ELT(res_list, 0, indices);
        SET_VECTOR_ELT(res_list, 1, minimum);

        Rf_unprotect(3);
        return res_list;
    } else {
    Rf_error("Provided pointer is not a liu_pointer");
    }
}

SEXP r_search_max(SEXP index_ptr) {
    if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_int")) {
        // Getting root
        int_node* root = (int_node*)R_ExternalPtrAddr(index_ptr);

        int max = 0;
        int_table table = find_indices_max_int(root, &max);

        // Creating final vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));
        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));
        free(table.pointer);

        SEXP maximum = Rf_protect(Rf_allocVector(INTSXP, 1));
        INTEGER(maximum)[0] = max;

        SEXP res_list = Rf_protect(Rf_allocVector(VECSXP, 2));
        SET_VECTOR_ELT(res_list, 0, indices);
        SET_VECTOR_ELT(res_list, 1, maximum);

        Rf_unprotect(3);
        return res_list;
    } else if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_double")) {
        // Getting root
        double_node* root = (double_node*)R_ExternalPtrAddr(index_ptr);
        
        double max = 0;
        int_table table = find_indices_max_double(root, &max);

        // Creating final vector
        SEXP indices = Rf_protect(Rf_allocVector(INTSXP, table.size));
        memcpy(INTEGER(indices), table.pointer, table.size * sizeof(int));
        free(table.pointer);

        SEXP maximum = Rf_protect(Rf_allocVector(REALSXP, 1));
        REAL(maximum)[0] = max;

        SEXP res_list = Rf_protect(Rf_allocVector(VECSXP, 2));
        SET_VECTOR_ELT(res_list, 0, indices);
        SET_VECTOR_ELT(res_list, 1, maximum);

        Rf_unprotect(3);
        return res_list;
    } else {
        Rf_error("Provided pointer is not a liu_pointer");
    }
}

SEXP r_inner_join(SEXP df_left, SEXP col_name, SEXP df_right, SEXP index_ptr, SEXP left){
    // This function is sending arguments to inner_join int or double
    // but it takes argument if it is supposed to be left join
    // It wasn't that long in the begining but using multiple threads forced much harder 
    // assembling of final data frame (but it is much faster).
    if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_int") || R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_string") ) {
        // Saving as C objects
        int_node* root = (int_node*)R_ExternalPtrAddr(index_ptr);
        // Finding column
        const char* target_col = CHAR(Rf_asChar(col_name));

        // Finding index of column
        int col_left_idx = -1;
        int left_cols = LENGTH(df_left);
        SEXP names_left = Rf_getAttrib(df_left, R_NamesSymbol);
        for (int i = 0; i < left_cols; i++) {
            if (strcmp(CHAR(STRING_ELT(names_left, i)), target_col) == 0) {
                col_left_idx = i;
                break;
            }
        }
        SEXP column = VECTOR_ELT(df_left, col_left_idx);

        size_t total_size = 0;
        int max_threads = 1;
        #ifdef _OPENMP
        max_threads = omp_get_max_threads();
        #endif
        int_table id_vector = {INTEGER(column), LENGTH(column)};
        dual_int_table* common_thread_table = inner_join_int(id_vector, root, Rf_asLogical(left), &total_size);
        
        // Names of right data frame
        int right_cols = LENGTH(df_right);
        SEXP names_right = Rf_getAttrib(df_right, R_NamesSymbol);

        // Creating data frame
        int total_cols = left_cols + right_cols;
        SEXP res_df = PROTECT(Rf_allocVector(VECSXP, total_cols));

        // filling new data frame with data from left one
        for (size_t j = 0; j < left_cols; j++) {
            SEXP col_src = VECTOR_ELT(df_left, j);
            SEXP new_col = PROTECT(Rf_allocVector(TYPEOF(col_src), total_size));
            // checking for type
            if (TYPEOF(col_src) == INTSXP || TYPEOF(col_src) == LGLSXP) {
                int *src_ptr = (TYPEOF(col_src) == INTSXP) ? INTEGER(col_src) : LOGICAL(col_src);
                int *dest_ptr = INTEGER(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        dest_ptr[offset + i] = src_ptr[common_thread_table[t].left_indices[i] - 1];
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == REALSXP) {
                double *src_ptr = REAL(col_src);
                double *dest_ptr = REAL(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        dest_ptr[offset + i] = src_ptr[common_thread_table[t].left_indices[i] - 1];
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == STRSXP) {
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        SET_STRING_ELT(new_col, i + offset, STRING_ELT(col_src, common_thread_table[t].left_indices[i] - 1));
                    }
                    offset += n_t;
                }
            }
            SET_VECTOR_ELT(res_df, j, new_col);
            UNPROTECT(1);
        }
        // filling data from right data frame
        for (size_t j = 0; j < right_cols; j++) {
            SEXP col_src = VECTOR_ELT(df_right, j);
            SEXP new_col = PROTECT(Rf_allocVector(TYPEOF(col_src), total_size));

            if (TYPEOF(col_src) == INTSXP || TYPEOF(col_src) == LGLSXP) {
                int *src_ptr = (TYPEOF(col_src) == INTSXP) ? INTEGER(col_src) : LOGICAL(col_src);
                int *dest_ptr = INTEGER(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        int idx = common_thread_table[t].right_indices[i];
                        if (idx == NA_INTEGER) {
                            dest_ptr[offset + i] = NA_INTEGER;
                        } else {
                            dest_ptr[offset + i] = src_ptr[idx - 1];
                        }
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == REALSXP) {
                double *src_ptr = REAL(col_src);
                double *dest_ptr = REAL(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        int idx = common_thread_table[t].right_indices[i];
                        if (idx == NA_INTEGER) {
                            dest_ptr[offset + i] = NA_REAL;
                        } else {
                            dest_ptr[offset + i] = src_ptr[idx - 1];
                        }
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == STRSXP) {
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        int idx = common_thread_table[t].right_indices[i];
                        if (idx == NA_INTEGER) {
                            SET_STRING_ELT(new_col, i + offset, NA_STRING);
                        } else {
                            SET_STRING_ELT(new_col, i + offset, STRING_ELT(col_src, idx - 1));
                        }
                    }
                    offset += n_t;
                }
            }
            SET_VECTOR_ELT(res_df, j + left_cols, new_col);
            UNPROTECT(1);
        }
        // column names
        SEXP out_names = PROTECT(Rf_allocVector(STRSXP, total_cols));
        for (int i = 0; i < left_cols; i++) SET_STRING_ELT(out_names, i, STRING_ELT(names_left, i));
        for (int i = 0; i < right_cols; i++) SET_STRING_ELT(out_names, left_cols + i, STRING_ELT(names_right, i));
        Rf_setAttrib(res_df, R_NamesSymbol, out_names);
        // data frame class
        SEXP class_name = PROTECT(Rf_allocVector(STRSXP, 1));
        SET_STRING_ELT(class_name, 0, Rf_mkChar("data.frame"));
        Rf_classgets(res_df, class_name);
        
        // thanks to that R knows how to number columns
        SEXP row_names = PROTECT(Rf_allocVector(INTSXP, 2));
        INTEGER(row_names)[0] = NA_INTEGER;
        INTEGER(row_names)[1] = -total_size;
        Rf_setAttrib(res_df, R_RowNamesSymbol, row_names);
        
        for (int i=0; i<max_threads; i++){
            free(common_thread_table[i].left_indices);
            free(common_thread_table[i].right_indices);
        }
        free(common_thread_table);
        UNPROTECT(4);
        return res_df;
    }
     else if (R_ExternalPtrTag(index_ptr) == Rf_install("liu_pointer_double")) {
        // Saving as C objects
        double_node* root = (double_node*)R_ExternalPtrAddr(index_ptr);
        // Finding column
        const char* target_col = CHAR(Rf_asChar(col_name));

        // Finding index of column
        int col_left_idx = -1;
        int left_cols = LENGTH(df_left);
        SEXP names_left = Rf_getAttrib(df_left, R_NamesSymbol);
        for (int i = 0; i < left_cols; i++) {
            if (strcmp(CHAR(STRING_ELT(names_left, i)), target_col) == 0) {
                col_left_idx = i;
                break;
            }
        }
        SEXP column = VECTOR_ELT(df_left, col_left_idx);

        size_t total_size = 0;
        int max_threads = 1;
        #ifdef _OPENMP
        max_threads = omp_get_max_threads();
        #endif
        double_table id_vector = {REAL(column), LENGTH(column)};
        dual_int_table* common_thread_table = inner_join_double(id_vector, root, Rf_asLogical(left), &total_size);
        
        // Names of right data frame
        int right_cols = LENGTH(df_right);
        SEXP names_right = Rf_getAttrib(df_right, R_NamesSymbol);

        // Creating data frame
        int total_cols = left_cols + right_cols;
        SEXP res_df = PROTECT(Rf_allocVector(VECSXP, total_cols));

        // filling new data frame with data from left one
        for (size_t j = 0; j < left_cols; j++) {
            SEXP col_src = VECTOR_ELT(df_left, j);
            SEXP new_col = PROTECT(Rf_allocVector(TYPEOF(col_src), total_size));
            // checking for type
            if (TYPEOF(col_src) == INTSXP || TYPEOF(col_src) == LGLSXP) {
                int *src_ptr = (TYPEOF(col_src) == INTSXP) ? INTEGER(col_src) : LOGICAL(col_src);
                int *dest_ptr = INTEGER(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        dest_ptr[offset + i] = src_ptr[common_thread_table[t].left_indices[i] - 1];
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == REALSXP) {
                double *src_ptr = REAL(col_src);
                double *dest_ptr = REAL(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        dest_ptr[offset + i] = src_ptr[common_thread_table[t].left_indices[i] - 1];
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == STRSXP) {
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        SET_STRING_ELT(new_col, i + offset, STRING_ELT(col_src, common_thread_table[t].left_indices[i] - 1));
                    }
                    offset += n_t;
                }
            }
            SET_VECTOR_ELT(res_df, j, new_col);
            UNPROTECT(1);
        }
        // filling data from right data frame
        for (size_t j = 0; j < right_cols; j++) {
            SEXP col_src = VECTOR_ELT(df_right, j);
            SEXP new_col = PROTECT(Rf_allocVector(TYPEOF(col_src), total_size));

            if (TYPEOF(col_src) == INTSXP || TYPEOF(col_src) == LGLSXP) {
                int *src_ptr = (TYPEOF(col_src) == INTSXP) ? INTEGER(col_src) : LOGICAL(col_src);
                int *dest_ptr = INTEGER(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        // POPRAWIONE: right_indices i poprawna logika NA
                        int idx = common_thread_table[t].right_indices[i];
                        if (idx == NA_INTEGER) {
                            dest_ptr[offset + i] = NA_INTEGER;
                        } else {
                            dest_ptr[offset + i] = src_ptr[idx - 1];
                        }
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == REALSXP) {
                double *src_ptr = REAL(col_src);
                double *dest_ptr = REAL(new_col);
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        // POPRAWIONE: right_indices i użycie NA_REAL dla double
                        int idx = common_thread_table[t].right_indices[i];
                        if (idx == NA_INTEGER) {
                            dest_ptr[offset + i] = NA_REAL;
                        } else {
                            dest_ptr[offset + i] = src_ptr[idx - 1];
                        }
                    }
                    offset += n_t;
                }
            } 
            else if (TYPEOF(col_src) == STRSXP) {
                size_t offset = 0;
                for (int t = 0; t < max_threads; t++) {
                    size_t n_t = common_thread_table[t].size;
                    for (size_t i = 0; i < n_t; i++) {
                        // POPRAWIONE: common_thread_table[t], right_indices, offset dla NA
                        int idx = common_thread_table[t].right_indices[i];
                        if (idx == NA_INTEGER) {
                            SET_STRING_ELT(new_col, i + offset, NA_STRING);
                        } else {
                            SET_STRING_ELT(new_col, i + offset, STRING_ELT(col_src, idx - 1));
                        }
                    }
                    offset += n_t;
                }
            }
            SET_VECTOR_ELT(res_df, j + left_cols, new_col);
            UNPROTECT(1);
        }
            
        // column names
        SEXP out_names = PROTECT(Rf_allocVector(STRSXP, total_cols));
        for (int i = 0; i < left_cols; i++) SET_STRING_ELT(out_names, i, STRING_ELT(names_left, i));
        for (int i = 0; i < right_cols; i++) SET_STRING_ELT(out_names, left_cols + i, STRING_ELT(names_right, i));
        Rf_setAttrib(res_df, R_NamesSymbol, out_names);

        // data frame class
        SEXP class_name = PROTECT(Rf_allocVector(STRSXP, 1));
        SET_STRING_ELT(class_name, 0, Rf_mkChar("data.frame"));
        Rf_classgets(res_df, class_name);
        
        // thanks to that R knows how to number columns
        SEXP row_names = PROTECT(Rf_allocVector(INTSXP, 2));
        INTEGER(row_names)[0] = NA_INTEGER;
        INTEGER(row_names)[1] = -total_size;
        Rf_setAttrib(res_df, R_RowNamesSymbol, row_names);
        
        for (int i=0; i<max_threads; i++){
            free(common_thread_table[i].left_indices);
            free(common_thread_table[i].right_indices);
        }
        free(common_thread_table);

        UNPROTECT(4);
        return res_df;
    } else {
    Rf_error("Provided pointer is not a liu pointer");
    }
}   
