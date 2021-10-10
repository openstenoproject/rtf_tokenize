#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"


#define RTF_TOKENIZER_REWIND_SIZE  8

typedef struct
{
    PyObject_HEAD
    char      *text;
    char      *text_ptr;
    size_t     lnum;
    size_t     cnum;
    size_t     lnum_next;
    size_t     cnum_next;
    PyObject **rewind_buffer;
    unsigned   rewind_count;
    unsigned   rewind_size;

} RtfTokenizer;

static int RtfTokenizer_init(RtfTokenizer *self, PyObject *args, PyObject *kwargs)
{
    const char *text;
    Py_ssize_t  text_size;

    if (!PyArg_ParseTuple(args, "s", &text))
        goto error_0;

    text_size = strlen(text);

    self->text = PyMem_Malloc(text_size + 1);
    if (self->text == NULL)
        goto error_1;

    self->text_ptr = memcpy(self->text, text, text_size);
    self->text[text_size] = '\0';

    self->rewind_size = RTF_TOKENIZER_REWIND_SIZE;
    self->rewind_buffer = PyMem_Malloc(self->rewind_size * sizeof (*self->rewind_buffer));
    if (self->rewind_buffer == NULL)
        goto error_1;

    return 0;

error_1:
    PyErr_NoMemory();
error_0:
    return -1;
}

static void RtfTokenizer_dealloc(RtfTokenizer *self)
{
    while (self->rewind_count)
        Py_DECREF(self->rewind_buffer[--self->rewind_count]);
    PyMem_Free(self->rewind_buffer);
    PyMem_Free(self->text);
}

static PyObject *RtfTokenizer_next_token(RtfTokenizer *self, PyObject *Py_UNUSED(ignored))
{
    const char *token_next;
    const char *token_start;
    size_t      token_len;
    int         linc, cinc;
    char        c;

    if (self->rewind_count)
        return self->rewind_buffer[--self->rewind_count];

    self->lnum = self->lnum_next;
    self->cnum = self->cnum_next;

    token_start = token_next = self->text_ptr;
    token_len = 0;
    linc = cinc = 0;

    while ((c = *token_start) != '\0')
    {
        switch (c)
        {
        case '\n':
            ++self->lnum;
            self->cnum = 0;
        case '\r':
            token_start = ++token_next;
            continue;
        case '{':
        case '}':
            token_len = 1;
            ++token_next;
            break;
        case '\\':
            while ((((c = *++token_next) >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z')))
                ;
            if (token_start[1] == '\n') {
                linc = +1;
                cinc = -2 - self->cnum;
            }
            token_len = token_next - token_start;
            if (token_len == 1) {
                if (c != '\0') {
                    ++token_next;
                    ++token_len;
                }
                break;
            }
            if (c == '-' || (c >= '0' && c <= '9'))
                while ((c = *++token_next) >= '0' && c <= '9')
                    ;
            token_len = token_next - token_start;
            if (c == ' ')
                ++token_next;
            break;
        default:
            while ((c = *++token_next) != '\0' && c != '\\' &&
                   c != '{' && c != '}' && c != '\r' && c != '\n')
                ;
            token_len = token_next - token_start;
            break;
        }
        break;
    }

    self->text_ptr = (char *)token_next;
    self->lnum_next = self->lnum + linc;
    self->cnum_next = self->cnum + cinc + token_next - token_start;

    if (token_len == 0)
        Py_RETURN_NONE;

    return PyUnicode_FromStringAndSize(token_start, token_len);
}

static PyObject *RtfTokenizer_rewind_token(RtfTokenizer *self, PyObject *token)
{
    if (token != Py_None && !PyUnicode_Check(token))
    {
        PyErr_SetString(PyExc_TypeError, "expected a string");
        return NULL;
    }

    if (self->rewind_count == self->rewind_size)
    {
        PyObject **new_rewind_buffer;
        unsigned   new_rewind_size;

        new_rewind_size = self->rewind_size + RTF_TOKENIZER_REWIND_SIZE;
        new_rewind_buffer = PyMem_Realloc(self->rewind_buffer, new_rewind_size * sizeof (*self->rewind_buffer));
        if (NULL == new_rewind_buffer)
        {
            PyErr_NoMemory();
            return NULL;
        }

        self->rewind_size = new_rewind_size;
        self->rewind_buffer = new_rewind_buffer;
    }

    self->rewind_buffer[self->rewind_count++] = token;
    Py_INCREF(token);

    Py_RETURN_NONE;
}

static PyMemberDef RtfTokenizer_members[] =
{
    {"lnum", T_ULONG, offsetof(RtfTokenizer, lnum), 0, "Line number."},
    {"cnum", T_ULONG, offsetof(RtfTokenizer, cnum), 0, "Column number."},
    {NULL}
};

static PyMethodDef RtfTokenizer_methods[] =
{
    {"next_token", (PyCFunction)RtfTokenizer_next_token, METH_NOARGS, "Return the next token."},
    {"rewind_token", (PyCFunction)RtfTokenizer_rewind_token, METH_O, "Rewind token so it's returned next."},
    {NULL}
};

static PyTypeObject RtfTokenizerType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "rtf_tokenize.RtfTokenizer",
    .tp_doc = "RTF Tokenizer.",
    .tp_basicsize = sizeof (RtfTokenizer),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)RtfTokenizer_init,
    .tp_dealloc = (destructor)RtfTokenizer_dealloc,
    .tp_members = RtfTokenizer_members,
    .tp_methods = RtfTokenizer_methods,
};

static struct PyModuleDef module =
{
    PyModuleDef_HEAD_INIT,
    .m_name = "rtf_tokenize",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit_rtf_tokenize(void)
{
    PyObject *m;

    if (PyType_Ready(&RtfTokenizerType) < 0)
        return NULL;

    m = PyModule_Create(&module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&RtfTokenizerType);

    if (PyModule_AddObject(m, "RtfTokenizer", (PyObject *)&RtfTokenizerType) < 0)
    {
        Py_DECREF(&RtfTokenizerType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
