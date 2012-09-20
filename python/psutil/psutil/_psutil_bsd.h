/*
 * $Id: _psutil_bsd.h 1498 2012-07-24 21:41:28Z g.rodola $
 *
 * Copyright (c) 2009, Jay Loden, Giampaolo Rodola'. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * BSD platform-specific module methods for _psutil_bsd
 */

#include <Python.h>

// --- per-process functions

static PyObject* get_process_cpu_times(PyObject* self, PyObject* args);
static PyObject* get_process_name(PyObject* self, PyObject* args);
static PyObject* get_process_exe(PyObject* self, PyObject* args);
static PyObject* get_process_cmdline(PyObject* self, PyObject* args);
static PyObject* get_process_ppid(PyObject* self, PyObject* args);
static PyObject* get_process_uids(PyObject* self, PyObject* args);
static PyObject* get_process_gids(PyObject* self, PyObject* args);
static PyObject* get_process_connections(PyObject* self, PyObject* args);
static PyObject* get_process_create_time(PyObject* self, PyObject* args);
static PyObject* get_process_memory_info(PyObject* self, PyObject* args);
static PyObject* get_process_num_threads(PyObject* self, PyObject* args);
static PyObject* get_process_num_fds(PyObject* self, PyObject* args);
static PyObject* get_process_threads(PyObject* self, PyObject* args);
static PyObject* get_process_status(PyObject* self, PyObject* args);
static PyObject* get_process_io_counters(PyObject* self, PyObject* args);
static PyObject* get_process_tty_nr(PyObject* self, PyObject* args);
static PyObject* get_process_memory_maps(PyObject* self, PyObject* args);
static PyObject* get_process_num_ctx_switches(PyObject* self, PyObject* args);
#if defined(__FreeBSD_version) && __FreeBSD_version >= 800000
static PyObject* get_process_open_files(PyObject* self, PyObject* args);
static PyObject* get_process_cwd(PyObject* self, PyObject* args);
#endif

// --- system-related functions

static PyObject* get_pid_list(PyObject* self, PyObject* args);
static PyObject* get_num_cpus(PyObject* self, PyObject* args);
static PyObject* get_virtual_mem(PyObject* self, PyObject* args);
static PyObject* get_swap_mem(PyObject* self, PyObject* args);
static PyObject* get_system_cpu_times(PyObject* self, PyObject* args);
#if defined(__FreeBSD_version) && __FreeBSD_version >= 800000
static PyObject* get_system_per_cpu_times(PyObject* self, PyObject* args);
#endif
static PyObject* get_system_boot_time(PyObject* self, PyObject* args);
static PyObject* get_disk_partitions(PyObject* self, PyObject* args);
static PyObject* get_network_io_counters(PyObject* self, PyObject* args);
static PyObject* get_disk_io_counters(PyObject* self, PyObject* args);
static PyObject* get_system_users(PyObject* self, PyObject* args);
