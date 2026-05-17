library(liu)
library(tinytest)

df <- data.frame(
  id1 = as.integer(c(2,8,2,NA,45)),
  val = c(6.7,NA,6.7,2.3,10.5),
  word = c("Poland", NA, "England", "Poland", "France")
)

# int tests
idx1 <- liu_build(df, "id1")
expect_error(liu_search(idx1, 2))
expect_error(liu_search(idx1, df))
expect_error(liu_search(2, 2))
expect_identical(liu_search(idx1, as.integer(2)), as.integer(c(1,3)))
expect_identical(liu_search(idx1, as.integer(c(2,45))), as.integer(c(1,3,5)))
expect_identical(liu_search(idx1, as.integer(c(NA, 8))), as.integer(c(2)))
expect_identical(liu_search(idx1, as.integer(c(2,NA, 8))), as.integer(c(1,3,2)))
expect_identical(liu_search(idx1, as.integer(67)), as.integer(c()))
expect_identical(liu_search(idx1, as.integer(c(67,2))), as.integer(c(1,3)))
expect_identical(liu_search(idx1, as.integer(c(2,67))), as.integer(c(1,3)))
expect_identical(liu_search(idx1, as.integer(c(67,2, NA))), as.integer(c(1,3)))
liu_free(idx1)

# double tests
idx2 <- liu_build(df, "val")
expect_error(liu_search(idx2, as.integer(2)))
expect_error(liu_search(idx2, df))
expect_identical(liu_search(idx2, 6.7), as.integer(c(1,3)))
expect_identical(liu_search(idx2, c(2.3,6.7)), as.integer(c(4,1,3)))
expect_identical(liu_search(idx2, as.double(NA)), as.integer(c()))
expect_identical(liu_search(idx2, c(NA, 10.5)), as.integer(c(5)))
expect_identical(liu_search(idx2, c(6.7,NA, 2.3)), as.integer(c(1,3,4)))
expect_identical(liu_search(idx2, 0.5), as.integer(c()))
expect_identical(liu_search(idx2, c(20.6,2.3)), as.integer(c(4)))
expect_identical(liu_search(idx2, c(2.3,20.6)), as.integer(c(4)))
expect_identical(liu_search(idx2, c(20.6,2.3, NA)), as.integer(c(4)))
liu_free(idx2)

# string trees (int in fact)
idx3 <- liu_build(df, "word")
expect_error(liu_search(idx3, 3))
expect_identical(liu_search(idx3, "Poland"), as.integer(c(1,4)))
expect_identical(liu_search(idx3, "England"), as.integer(c(3)))
expect_identical(liu_search(idx3, c("France", NA)), as.integer(5))
expect_identical(liu_search(idx3, "star"), as.integer(c()))
expect_identical(liu_search(idx3, c("Poland", "liu", "England")), as.integer(c(1,4,3)))
liu_free(idx3)

df <- data.frame(
  id1 = as.integer(NA),
  val = as.double(NA),
  word = as.character(NA)
)
idx1 <- liu_build(df, "id1")
expect_identical(liu_search(idx1, as.integer(2)), as.integer(c()))
expect_identical(liu_search(idx1, as.integer(NA)), as.integer(c()))
expect_identical(liu_search(idx1, as.integer(c(2,1,1,5))), as.integer(c()))
expect_warning(liu_free(idx1))

idx2 <- liu_build(df, "val")
expect_identical(liu_search(idx2, 2), as.integer(c()))
expect_identical(liu_search(idx2, as.double(NA)), as.integer(c()))
expect_identical(liu_search(idx2, c(6.7,4.2)), as.integer(c()))
expect_warning(liu_free(idx2))

idx3 <- liu_build(df, "word")
expect_identical(liu_search(idx3, as.character(NA)), as.integer(c()))
expect_identical(liu_search(idx3, "Poland"), as.integer(c()))
expect_warning(liu_free(idx3))

# test bigger dt
df <- data.frame(
  id1 = as.integer(1:1000000),
  val = c(1:1000000)/3,
  word = sprintf("str_%07d", 1:1000000)
)
idx1<-liu_build(df,"id1")
expect_identical(liu_search(idx1, as.integer(c(2,3,4))), as.integer(c(2,3,4)))
liu_free(idx1)

idx2 <- liu_build(df, "val")
expect_identical(liu_search(idx2, c(160,161)), as.integer(c(480,483)))
liu_free(idx2)

idx3 <- liu_build(df, "word")
expect_identical(liu_search(idx3, c("str_0999123", "str_0037456")), as.integer(c(999123, 37456)))
liu_free(idx3)



