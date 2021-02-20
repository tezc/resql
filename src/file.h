/*
 *  Resql
 *
 *  Copyright (C) 2021 Resql Authors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RESQL_FILE_H
#define RESQL_FILE_H

#include "rs.h"

#include "sc/sc_str.h"

#include <stdio.h>

struct file
{
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
char *file_full_path(const char *path, char *resolved);
bool file_exists_at(const char *path);
int file_remove_path(const char *path);
int file_remove_if_exists(const char *path);
int file_mkdir(const char *path);
int file_clear_dir(const char *path, const char *pattern);
int file_copy(const char *dst, const char *src);
void file_random(void* buf, size_t size);

#endif
