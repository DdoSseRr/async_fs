#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>
#include <iostream>
#include <cstring>
#include <sys/stat.h>

// Вспомогательная функция для создания и завершения future
static PyObject* create_and_set_future(PyObject* loop, PyObject* result) {
    PyObject* future = PyObject_CallMethod(loop, "create_future", NULL);
    if (!future) {
        Py_XDECREF(result);
        return NULL;
    }
    if (PyObject_CallMethod(future, "set_result", "O", result) == NULL) {
        Py_DECREF(future);
        Py_XDECREF(result);
        return NULL;
    }
    Py_XDECREF(result);
    return future;
}

// Асинхронное чтение файла
static PyObject* async_read_file(PyObject* self, PyObject* args) {
    const char* filename;
    int buffer_size;

    if (!PyArg_ParseTuple(args, "si", &filename, &buffer_size)) {
        return NULL;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return PyErr_Format(PyExc_OSError, "Failed to open file: %s", filename);
    }

    struct io_uring ring;
    if (io_uring_queue_init(32, &ring, 0) < 0) {
        close(fd);
        return PyErr_Format(PyExc_RuntimeError, "Failed to initialize io_uring");
    }

    char* buffer = new(std::nothrow) char[buffer_size];
    if (!buffer) {
        io_uring_queue_exit(&ring);
        close(fd);
        return PyErr_NoMemory();
    }

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        delete[] buffer;
        io_uring_queue_exit(&ring);
        close(fd);
        return PyErr_Format(PyExc_RuntimeError, "Failed to get submission queue entry");
    }
    io_uring_prep_read(sqe, fd, buffer, buffer_size, 0);
    io_uring_submit(&ring);

    struct io_uring_cqe* cqe;
    if (io_uring_wait_cqe(&ring, &cqe) < 0) {
        delete[] buffer;
        io_uring_queue_exit(&ring);
        close(fd);
        return PyErr_Format(PyExc_RuntimeError, "Failed to wait for completion");
    }
    int ret = cqe->res;
    io_uring_cqe_seen(&ring, cqe);

    io_uring_queue_exit(&ring);
    close(fd);

    if (ret < 0) {
        delete[] buffer;
        return PyErr_Format(PyExc_OSError, "Read error: %s", strerror(-ret));
    }

    PyObject* loop = PyObject_CallMethod(PyImport_ImportModule("asyncio"), "get_event_loop", NULL);
    if (!loop) {
        delete[] buffer;
        return NULL;
    }
    PyObject* result = Py_BuildValue("s#", buffer, ret);
    delete[] buffer;

    return create_and_set_future(loop, result);
}

// Асинхронная запись в файл
static PyObject* async_write_file(PyObject* self, PyObject* args) {
    const char* filename;
    const char* data;

    if (!PyArg_ParseTuple(args, "ss", &filename, &data)) {
        return NULL;
    }

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return PyErr_Format(PyExc_OSError, "Failed to open or create file: %s", filename);
    }

    struct io_uring ring;
    if (io_uring_queue_init(32, &ring, 0) < 0) {
        close(fd);
        return PyErr_Format(PyExc_RuntimeError, "Failed to initialize io_uring");
    }

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        io_uring_queue_exit(&ring);
        close(fd);
        return PyErr_Format(PyExc_RuntimeError, "Failed to get submission queue entry");
    }
    io_uring_prep_write(sqe, fd, data, strlen(data), 0);
    io_uring_submit(&ring);

    struct io_uring_cqe* cqe;
    if (io_uring_wait_cqe(&ring, &cqe) < 0) {
        io_uring_queue_exit(&ring);
        close(fd);
        return PyErr_Format(PyExc_RuntimeError, "Failed to wait for completion");
    }
    int ret = cqe->res;
    io_uring_cqe_seen(&ring, cqe);

    io_uring_queue_exit(&ring);
    close(fd);

    if (ret < 0) {
        return PyErr_Format(PyExc_OSError, "Write error: %s", strerror(-ret));
    }

    PyObject* loop = PyObject_CallMethod(PyImport_ImportModule("asyncio"), "get_event_loop", NULL);
    if (!loop) {
        return NULL;
    }
    return create_and_set_future(loop, PyLong_FromLong(ret));
}

// Асинхронное удаление файла
static PyObject* async_delete_file(PyObject* self, PyObject* args) {
    const char* filename;

    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }

    if (unlink(filename) < 0) {
        return PyErr_Format(PyExc_OSError, "Failed to delete file: %s", filename);
    }

    PyObject* loop = PyObject_CallMethod(PyImport_ImportModule("asyncio"), "get_event_loop", NULL);
    if (!loop) {
        return NULL;
    }
    Py_RETURN_NONE;
}

// Асинхронное копирование файла
static PyObject* async_copy_file(PyObject* self, PyObject* args) {
    const char* src_filename;
    const char* dest_filename;
    int buffer_size = 4096;

    if (!PyArg_ParseTuple(args, "ss|i", &src_filename, &dest_filename, &buffer_size)) {
        return NULL;
    }

    int src_fd = open(src_filename, O_RDONLY);
    if (src_fd < 0) {
        return PyErr_Format(PyExc_OSError, "Failed to open source file: %s", src_filename);
    }

    int dest_fd = open(dest_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        close(src_fd);
        return PyErr_Format(PyExc_OSError, "Failed to open destination file: %s", dest_filename);
    }

    struct io_uring ring;
    if (io_uring_queue_init(32, &ring, 0) < 0) {
        close(src_fd);
        close(dest_fd);
        return PyErr_Format(PyExc_RuntimeError, "Failed to initialize io_uring");
    }

    char* buffer = new(std::nothrow) char[buffer_size];
    if (!buffer) {
        io_uring_queue_exit(&ring);
        close(src_fd);
        close(dest_fd);
        return PyErr_NoMemory();
    }

    ssize_t total_bytes_copied = 0;
    while (true) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            delete[] buffer;
            io_uring_queue_exit(&ring);
            close(src_fd);
            close(dest_fd);
            return PyErr_Format(PyExc_RuntimeError, "Failed to get submission queue entry");
        }
        io_uring_prep_read(sqe, src_fd, buffer, buffer_size, total_bytes_copied);
        io_uring_submit(&ring);

        struct io_uring_cqe* cqe;
        if (io_uring_wait_cqe(&ring, &cqe) < 0) {
            delete[] buffer;
            io_uring_queue_exit(&ring);
            close(src_fd);
            close(dest_fd);
            return PyErr_Format(PyExc_RuntimeError, "Failed to wait for completion");
        }
        ssize_t bytes_read = cqe->res;
        io_uring_cqe_seen(&ring, cqe);

        if (bytes_read <= 0) {
            break;
        }

        sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            delete[] buffer;
            io_uring_queue_exit(&ring);
            close(src_fd);
            close(dest_fd);
            return PyErr_Format(PyExc_RuntimeError, "Failed to get submission queue entry");
        }
        io_uring_prep_write(sqe, dest_fd, buffer, bytes_read, total_bytes_copied);
        io_uring_submit(&ring);

        if (io_uring_wait_cqe(&ring, &cqe) < 0) {
            delete[] buffer;
            io_uring_queue_exit(&ring);
            close(src_fd);
            close(dest_fd);
            return PyErr_Format(PyExc_RuntimeError, "Failed to wait for completion");
        }
        ssize_t bytes_written = cqe->res;
        io_uring_cqe_seen(&ring, cqe);

        if (bytes_written < 0) {
            delete[] buffer;
            io_uring_queue_exit(&ring);
            close(src_fd);
            close(dest_fd);
            return PyErr_Format(PyExc_OSError, "Write error: %s", strerror(-bytes_written));
        }

        total_bytes_copied += bytes_written;
    }

    delete[] buffer;
    io_uring_queue_exit(&ring);
    close(src_fd);
    close(dest_fd);

    PyObject* loop = PyObject_CallMethod(PyImport_ImportModule("asyncio"), "get_event_loop", NULL);
    if (!loop) {
        return NULL;
    }
    return create_and_set_future(loop, PyLong_FromSsize_t(total_bytes_copied));
}

// Асинхронное перемещение файла
static PyObject* async_move_file(PyObject* self, PyObject* args) {
    const char* src_filename;
    const char* dest_filename;

    if (!PyArg_ParseTuple(args, "ss", &src_filename, &dest_filename)) {
        return NULL;
    }

    if (rename(src_filename, dest_filename) < 0) {
        return PyErr_Format(PyExc_OSError, "Failed to move file: %s to %s", src_filename, dest_filename);
    }

    PyObject* loop = PyObject_CallMethod(PyImport_ImportModule("asyncio"), "get_event_loop", NULL);
    if (!loop) {
        return NULL;
    }
    Py_RETURN_NONE;
}

// Асинхронное получение информации о файле
static PyObject* async_file_info(PyObject* self, PyObject* args) {
    const char* filename;

    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }

    struct stat file_stat;
    if (stat(filename, &file_stat) < 0) {
        return PyErr_Format(PyExc_OSError, "Failed to get file info: %s", filename);
    }

    PyObject* loop = PyObject_CallMethod(PyImport_ImportModule("asyncio"), "get_event_loop", NULL);
    if (!loop) {
        return NULL;
    }
    PyObject* result = Py_BuildValue("{s:i,s:i,s:i,s:i}",
                                     "size", file_stat.st_size,
                                     "mode", file_stat.st_mode,
                                     "mtime", file_stat.st_mtime,
                                     "atime", file_stat.st_atime);
    return create_and_set_future(loop, result);
}

// Описание методов модуля
static PyMethodDef module_methods[] = {
    {"read_file", (PyCFunction)async_read_file, METH_VARARGS, "Asynchronously read a file"},
    {"write_file", (PyCFunction)async_write_file, METH_VARARGS, "Asynchronously write to a file"},
    {"delete_file", (PyCFunction)async_delete_file, METH_VARARGS, "Asynchronously delete a file"},
    {"copy_file", (PyCFunction)async_copy_file, METH_VARARGS, "Asynchronously copy a file"},
    {"move_file", (PyCFunction)async_move_file, METH_VARARGS, "Asynchronously move a file"},
    {"file_info", (PyCFunction)async_file_info, METH_VARARGS, "Asynchronously get file information"},
    {NULL, NULL, 0, NULL}
};

// Инициализация модуля
static struct PyModuleDef async_fs_module = {
    PyModuleDef_HEAD_INIT,
    "async_fs",
    NULL,
    -1,
    module_methods
};

// Инициализация расширения
PyMODINIT_FUNC PyInit_async_fs(void) {
    return PyModule_Create(&async_fs_module);
}