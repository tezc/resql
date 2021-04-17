/*
 * MIT License
 *
 * Copyright (c) 2021 Ozan Tezcan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef RESQL_FILE_H
#define RESQL_FILE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct file {
	char *path;
	FILE *fp;
};

struct file *file_create();
void file_destroy(struct file *file);

void file_init(struct file *file);
void file_term(struct file *file);
int file_open(struct file *file, const char *path, const char *mode);
int file_close(struct file *file);

ssize_t file_size(struct file *file);

int file_write(struct file *file, const void *ptr, size_t size);
int file_write_at(struct file *file, size_t off, const void *ptr, size_t size);
int file_read(struct file *file, void *ptr, size_t size);
int file_lseek(struct file *file, size_t offset);
const char *file_path(struct file *file);
int file_remove(struct file *file);
int file_flush(struct file *file);

int64_t file_size_at(const char *path);
bool file_exists_at(const char *path);
int file_remove_path(const char *path);
int file_unlink(const char* path);
int file_mkdir(const char *path);
int file_rmdir(const char *path);
int file_clear_dir(const char *path, const char *pattern);
int file_copy(const char *dst, const char *src);


#endif
