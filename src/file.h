/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
int file_term(struct file *file);
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

ssize_t file_size_at(const char *path);
bool file_exists_at(const char *path);
int file_remove_path(const char *path);
int file_unlink(const char *path);
int file_mkdir(const char *path);
int file_rmdir(const char *path);
int file_clear_dir(const char *path, const char *pattern);
int file_copy(const char *dst, const char *src);
int file_rename(const char *dst, const char *src);
int file_fsync(const char *path);

#endif
