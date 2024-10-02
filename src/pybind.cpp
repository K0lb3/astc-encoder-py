#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"
#include "astcenc.h"

/*
 *************************************************
 *
 * ASTCConfig
 *
 ************************************************
 */

PyObject *ASTCConfig_Object = nullptr;

typedef struct ASTCConfig
{
    PyObject_HEAD
        astcenc_config config;
} ASTCConfigT;

static PyMemberDef ASTCConfig_members[] = {
    {"profile", T_UINT, offsetof(ASTCConfigT, config.profile), 0, "the color profile"},
    {"flags", T_UINT, offsetof(ASTCConfigT, config.flags), 0, "the set of set flags."},
    {"block_x", T_UINT, offsetof(ASTCConfigT, config.block_x), 0, "the ASTC block size X dimension"},
    {"block_y", T_UINT, offsetof(ASTCConfigT, config.block_y), 0, "the ASTC block size Y dimension"},
    {"block_z", T_UINT, offsetof(ASTCConfigT, config.block_z), 0, "the ASTC block size Z dimension"},
    {"cw_r_weight", T_FLOAT, offsetof(ASTCConfigT, config.cw_r_weight), 0, "the red component weight scale for error weighting (-cw)"},
    {"cw_g_weight", T_FLOAT, offsetof(ASTCConfigT, config.cw_g_weight), 0, "the green component weight scale for error weighting (-cw)"},
    {"cw_b_weight", T_FLOAT, offsetof(ASTCConfigT, config.cw_b_weight), 0, "the blue component weight scale for error weighting (-cw)"},
    {"cw_a_weight", T_FLOAT, offsetof(ASTCConfigT, config.cw_a_weight), 0, "the alpha component weight scale for error weighting (-cw)"},
    {"a_scale_radius", T_UINT, offsetof(ASTCConfigT, config.a_scale_radius), 0, "the radius for any alpha-weight scaling (-a)"},
    {"rgbm_m_scale", T_FLOAT, offsetof(ASTCConfigT, config.rgbm_m_scale), 0, "the RGBM scale factor for the shared multiplier (-rgbm)"},
    {"tune_partition_count_limit", T_UINT, offsetof(ASTCConfigT, config.tune_partition_count_limit), 0, "the maximum number of partitions searched (-partitioncountlimit)"},
    {"tune_2partition_index_limit", T_UINT, offsetof(ASTCConfigT, config.tune_2partition_index_limit), 0, "the maximum number of partitions searched (-2partitionindexlimit)"},
    {"tune_3partition_index_limit", T_UINT, offsetof(ASTCConfigT, config.tune_3partition_index_limit), 0, "the maximum number of partitions searched (-3partitionindexlimit)"},
    {"tune_4partition_index_limit", T_UINT, offsetof(ASTCConfigT, config.tune_4partition_index_limit), 0, "the maximum number of partitions searched (-4partitionindexlimit)"},
    {"tune_block_mode_limit", T_UINT, offsetof(ASTCConfigT, config.tune_block_mode_limit), 0, " the maximum centile for block modes searched (-blockmodelimit)"},
    {"tune_refinement_limit", T_UINT, offsetof(ASTCConfigT, config.tune_refinement_limit), 0, "the maximum iterative refinements applied (-refinementlimit)"},
    {"tune_candidate_limit", T_UINT, offsetof(ASTCConfigT, config.tune_candidate_limit), 0, "the number of trial candidates per mode search (-candidatelimit)"},
    {"tune_2partitioning_candidate_limit", T_UINT, offsetof(ASTCConfigT, config.tune_2partitioning_candidate_limit), 0, "the number of trial partitionings per search (-2partitioncandidatelimit)"},
    {"tune_3partitioning_candidate_limit", T_UINT, offsetof(ASTCConfigT, config.tune_3partitioning_candidate_limit), 0, "the number of trial partitionings per search (-3partitioncandidatelimit)"},
    {"tune_4partitioning_candidate_limit", T_UINT, offsetof(ASTCConfigT, config.tune_4partitioning_candidate_limit), 0, "the number of trial partitionings per search (-4partitioncandidatelimit)"},
    {"tune_db_limit", T_FLOAT, offsetof(ASTCConfigT, config.tune_db_limit), 0, "the dB threshold for stopping block search (-dblimit)"},
    {"tune_mse_overshoot", T_FLOAT, offsetof(ASTCConfigT, config.tune_mse_overshoot), 0, "the amount of MSE overshoot needed to early-out trials"},
    {"tune_2partition_early_out_limit_factor", T_FLOAT, offsetof(ASTCConfigT, config.tune_2partition_early_out_limit_factor), 0, "the threshold for skipping 3.1/4.1 trials (-2partitionlimitfactor)"},
    {"tune_3partition_early_out_limit_factor", T_FLOAT, offsetof(ASTCConfigT, config.tune_3partition_early_out_limit_factor), 0, "the threshold for skipping 4.1 trials (-3partitionlimitfactor)"},
    {"tune_2plane_early_out_limit_correlation", T_FLOAT, offsetof(ASTCConfigT, config.tune_2plane_early_out_limit_correlation), 0, "the threshold for skipping two weight planes (-2planelimitcorrelation)"},
    {"tune_search_mode0_enable", T_FLOAT, offsetof(ASTCConfigT, config.tune_search_mode0_enable), 0, "the config enable for the mode0 fast-path search"},
    //{"progress_callback", T_OBJECT, offsetof(ASTCConfigT, config.progress_callback), 0, "the progress callback, can be None"},
    {NULL} /* Sentinel */
};

static int ASTCConfig_init(ASTCConfigT *self, PyObject *args, PyObject *kwargs)
{
    const char *kwlist[] = {
        "profile", // Color profile.
        "block_x", // ASTC block size X dimension.
        "block_y", // ASTC block size Y dimension.
        "block_z", // ASTC block size Z dimension.
        "quality", // Search quality preset / effort level. Either an ASTCENC_PRE_* value, or a effort level between 0 and 100. Performance is not linear between 0 and 100.
        "flags",   // A valid set of ASTCENC_FLG_* flag bits.
        NULL};

    uint8_t profile_b;
    float quality = ASTCENC_PRE_MEDIUM;
    unsigned int flags = 0;
    unsigned int block_x = 0;
    unsigned int block_y = 0;
    unsigned int block_z = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "BII|IfI", (char **)kwlist, &profile_b, &block_x, &block_y, &block_z, &quality, &flags))
    {
        return -1;
    }

    astcenc_profile profile = (astcenc_profile)profile_b;
    astcenc_error status = astcenc_config_init(profile, block_x, block_y, block_z, quality, flags, &self->config);

    if (status != ASTCENC_SUCCESS)
    {
        PyErr_SetString(PyExc_RuntimeError, astcenc_get_error_string(status));
        return -1;
    }

    return 0;
}

static void ASTCConfig_dealloc(PyObject *self)
{
    PyObject_Del(self);
}

PyObject *ASTCConfig_repr(ASTCConfigT *self)
{
    return PyUnicode_FromFormat("ASTCConfig<(%d, %d, %d, %d)>", self->config.profile, self->config.block_x, self->config.block_y, self->config.block_z);
}

static PyType_Slot ASTCConfig_slots[] = {
    {Py_tp_dealloc, ASTCConfig_dealloc},
    {Py_tp_repr, ASTCConfig_repr},
    {Py_tp_doc, "ASTC Configuration"},
    //{Py_tp_methods, DBLogCursor_methods},
    {Py_tp_members, ASTCConfig_members},
    {Py_tp_init, ASTCConfig_init},
    {Py_tp_new, PyType_GenericNew},
    {0, NULL},
};

static PyType_Spec ASTCConfig_Spec = {
    "astcenc.ASTCConfig",                     // const char* name;
    sizeof(ASTCConfigT),                      // int basicsize;
    0,                                        // int itemsize;
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // unsigned int flags;
    ASTCConfig_slots,                         // PyType_Slot *slots;
};

/*
 *************************************************
 *
 * ASTCImage
 *
 ************************************************
 */

PyObject *ASTCImage_Object = nullptr;

typedef struct ASTCImage
{
    PyObject_HEAD
        astcenc_image image;
    PyObject *data;
} ASTCImageT;

static PyMemberDef ASTCImage_members[] = {
    {"dim_x", T_UINT, offsetof(ASTCImageT, image.dim_x), 0, "The X dimension of the image, in texels."},
    {"dim_y", T_UINT, offsetof(ASTCImageT, image.dim_y), 0, "The Y dimension of the image, in texels."},
    {"dim_z", T_UINT, offsetof(ASTCImageT, image.dim_z), 0, "The Z dimension of the image, in texels."},
    {"data_type", T_UINT, offsetof(ASTCImageT, image.data_type), 0, "The data type per component."},
    {"data", T_OBJECT_EX, offsetof(ASTCImageT, data), 0, "The array of 2D slices, of length @c dim_z."},
    {NULL} /* Sentinel */
};

static int ASTCImage_init(ASTCImageT *self, PyObject *args, PyObject *kwargs)
{
    const char *kwlist[] = {
        "data_type", // The data type per component.
        "dim_x",     // The X dimension of the image, in texels.
        "dim_y",     // The Y dimension of the image, in texels.
        "dim_z",     // The Z dimension of the image, in texels.
        "data",      // The array of 2D slices, of length @c dim_z.
        NULL};

    self->image.dim_x = 0;
    self->image.dim_y = 0;
    self->image.dim_z = 1;
    self->image.data_type = ASTCENC_TYPE_U8;
    self->image.data = nullptr;
    self->data = Py_None;

    uint8_t data_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "BII|IO", (char **)kwlist, &data_type, &self->image.dim_x, &self->image.dim_y, &self->image.dim_z, &self->data))
    {
        return -1;
    }

    self->image.data_type = (astcenc_type)data_type;
    Py_IncRef(self->data);

    return 0;
}

static void ASTCImage_dealloc(ASTCImageT *self)
{
    Py_DecRef(self->data);
    PyObject_Del(self);
}

static PyObject *ASTCImage_repr(ASTCImageT *self)
{
    return PyUnicode_FromFormat("ASTCImage(%d, %d, %d, %d)", self->image.dim_x, self->image.dim_y, self->image.dim_z, self->image.data_type);
}

PyType_Slot ASTCImage_slots[] = {
    {Py_tp_dealloc, ASTCImage_dealloc},
    {Py_tp_doc, "ASTC Image"},
    {Py_tp_members, ASTCImage_members},
    {Py_tp_init, ASTCImage_init},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_repr, ASTCImage_repr},
    {0, NULL},
};

static PyType_Spec ASTCImage_Spec = {
    "astcenc.ASTCImage",                      // const char* name;
    sizeof(ASTCImageT),                       // int basicsize;
    0,                                        // int itemsize;
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // unsigned int flags;
    ASTCImage_slots,                          // PyType_Slot *slots;
};

/*
 *************************************************
 *
 * ASTCSwizzle
 *
 ************************************************
 */

PyObject *ASTCSwizzle_Object = nullptr;

typedef struct ASTCSwizzle
{
    PyObject_HEAD
        astcenc_swizzle swizzle;
} ASTCSwizzleT;

static PyMemberDef ASTCSwizzle_members[] = {
    {"r", T_UINT, offsetof(ASTCSwizzleT, swizzle.r), 0, "the red component selector"},
    {"g", T_UINT, offsetof(ASTCSwizzleT, swizzle.g), 0, "the green component selector"},
    {"b", T_UINT, offsetof(ASTCSwizzleT, swizzle.b), 0, "the blue component selector"},
    {"a", T_UINT, offsetof(ASTCSwizzleT, swizzle.a), 0, "the alpha component selector"},
    {NULL} /* Sentinel */
};

static int ASTCSwizzle_init(ASTCSwizzleT *self, PyObject *args, PyObject *kwargs)
{
    const char *kwlist[] = {
        "r", // The red component selector.
        "g", // The green component selector.
        "b", // The blue component selector.
        "a", // The alpha component selector.
        NULL};

    self->swizzle.r = ASTCENC_SWZ_R;
    self->swizzle.g = ASTCENC_SWZ_G;
    self->swizzle.b = ASTCENC_SWZ_B;
    self->swizzle.a = ASTCENC_SWZ_A;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|IIII", (char **)kwlist, &self->swizzle.r, &self->swizzle.g, &self->swizzle.b, &self->swizzle.a))
    {
        return -1;
    }

    return 0;
}

static void ASTCSwizzle_dealloc(ASTCSwizzleT *self)
{
    PyObject_Del(self);
}

static PyObject *ASTCSwizzle_repr(ASTCSwizzleT *self)
{
    return PyUnicode_FromFormat("ASTCSwizzle<(%d, %d, %d, %d)>", self->swizzle.r, self->swizzle.g, self->swizzle.b, self->swizzle.a);
}

PyType_Slot ASTCSwizzle_slots[] = {
    {Py_tp_dealloc, ASTCSwizzle_dealloc},
    {Py_tp_doc, "ASTC Swizzle"},
    {Py_tp_members, ASTCSwizzle_members},
    {Py_tp_init, ASTCSwizzle_init},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_repr, ASTCSwizzle_repr},
    {0, NULL},
};

static PyType_Spec ASTCSwizzle_Spec = {
    "astcenc.ASTCSwizzle",                    // const char* name;
    sizeof(ASTCSwizzleT),                     // int basicsize;
    0,                                        // int itemsize;
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // unsigned int flags;
    ASTCSwizzle_slots,                        // PyType_Slot *slots;
};

/*
 *************************************************
 *
 * ASTCContext
 *
 ************************************************
 */

PyObject *ASTCContext_Object = nullptr;

typedef struct ASTCContext
{
    PyObject_HEAD astcenc_context *context;
    ASTCConfigT *config;
    unsigned int threads;
} ASTContextT;

static PyMemberDef ASTCContext_members[] = {
    {"config", T_OBJECT_EX, offsetof(ASTCContext, config), READONLY, "the configuration used by this context"},
    {"threads", T_UINT, offsetof(ASTCContext, threads), READONLY, "the thread count used by this context"},
    {NULL} /* Sentinel */
};

static int ASTContext_init(ASTContextT *self, PyObject *args, PyObject *kwargs)
{
    const char *kwlist[] = {
        "config",  // The configuration to use for encoding.
        "threads", // The number of threads to use for encoding.
        NULL};

    self->config = nullptr;
    self->threads = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|I", (char **)kwlist, &self->config, &self->threads))
    {
        return -1;
    }
    Py_IncRef((PyObject *)self->config);
    astcenc_error status = astcenc_context_alloc((const astcenc_config *)&self->config->config, self->threads, &self->context);
    if (status != ASTCENC_SUCCESS)
    {
        PyErr_SetString(PyExc_RuntimeError, astcenc_get_error_string(status));
        return -1;
    }

    return 0;
}

static void ASTContext_dealloc(ASTContextT *self)
{
    Py_DecRef((PyObject *)self->config);
    if (self->context != nullptr)
    {
        astcenc_context_free(self->context);
    }
    PyObject_Del(self);
}

static PyObject *ASTContext_repr(PyObject *self)
{
    return PyUnicode_FromString("ASTCContext");
}

PyObject *ASTCContext_method_comprocess(ASTContextT *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"image", "swizzle", NULL};
    ASTCImageT *py_image = nullptr;
    ASTCSwizzleT *py_swizzle = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", (char **)keywords, &py_image, &py_swizzle))
    {
        return NULL;
    }

    astcenc_image *image = &py_image->image;
    astcenc_config *config = &self->config->config;

    // prepare image
    uint8_t *image_data = (uint8_t *)PyBytes_AsString(py_image->data);
    image->data = reinterpret_cast<void **>(&image_data);

    // Space needed for 16 bytes of output per compressed block
    unsigned int block_count_x = (image->dim_x + config->block_x - 1) / config->block_x;
    unsigned int block_count_y = (image->dim_y + config->block_y - 1) / config->block_y;
    unsigned int block_count_z = (image->dim_z + config->block_z - 1) / config->block_z;
    size_t comp_len = block_count_x * block_count_y * block_count_z * 16;
    uint8_t *comp_data = new uint8_t[comp_len];

    // run the compressor
    astcenc_error status;
    for (unsigned int thread_index = 0; thread_index < self->threads; thread_index++)
    {
        status = astcenc_compress_image(
            self->context,
            image,
            &py_swizzle->swizzle,
            comp_data,
            comp_len,
            thread_index);
        if (status != ASTCENC_SUCCESS)
        {
            delete[] comp_data;
            image->data = nullptr;
            PyErr_SetString(PyExc_RuntimeError, astcenc_get_error_string(status));
            return NULL;
        }
    }
    status = astcenc_compress_reset(self->context);
    if (status != ASTCENC_SUCCESS)
    {
        delete[] comp_data;
        image->data = nullptr;
        PyErr_SetString(PyExc_RuntimeError, astcenc_get_error_string(status));
        return NULL;
    }

    // create a python bytes object from the compressed data
    PyObject *py_comp_data = PyBytes_FromStringAndSize((char *)comp_data, comp_len);

    // cleanup
    delete[] comp_data;
    image->data = nullptr;

    return py_comp_data;
}

PyObject *ASTCContext_method_decompress(ASTContextT *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"data", "image", "swizzle", NULL};

    const uint8_t *comp_data;
    Py_ssize_t comp_len;
    ASTCImageT *py_image = nullptr;
    ASTCSwizzleT *py_swizzle = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y#OO", (char **)keywords, &comp_data, &comp_len, &py_image, &py_swizzle))
    {
        return NULL;
    }

    // prepare image
    astcenc_image *image = &py_image->image;
    astcenc_config *config = &self->config->config;

    Py_DecRef(py_image->data);
    Py_ssize_t image_len = image->dim_x * image->dim_y * image->dim_z * 4;
    if (image->data_type == ASTCENC_TYPE_F16)
    {
        image_len *= 2;
    }
    else if (image->data_type == ASTCENC_TYPE_F32)
    {
        image_len *= 4;
    }
    uint8_t *image_data = new uint8_t[image_len];
    image->data = reinterpret_cast<void **>(&image_data);

    // run the decompressor
    astcenc_error status;
    for (unsigned int thread_index = 0; thread_index < self->threads; thread_index++)
    {
        status = astcenc_decompress_image(
            self->context,
            comp_data,
            comp_len,
            image,
            &py_swizzle->swizzle,
            thread_index);
        if (status != ASTCENC_SUCCESS)
        {
            delete[] image_data;
            image->data = nullptr;
            PyErr_SetString(PyExc_RuntimeError, astcenc_get_error_string(status));
            return NULL;
        }
    }
    status = astcenc_decompress_reset(self->context);
    if (status != ASTCENC_SUCCESS)
    {
        delete[] image_data;
        image->data = nullptr;
        PyErr_SetString(PyExc_RuntimeError, astcenc_get_error_string(status));
        return NULL;
    }

    // create a python bytes object from the decompressed data
    PyObject *py_image_data = PyBytes_FromStringAndSize((char *)image_data, image_len);
    Py_IncRef(py_image_data);
    Py_DecRef(py_image->data);
    py_image->data = py_image_data;

    // cleanup
    image->data = nullptr;
    delete[] image_data;

    // ref count gets decreased by one when the function returns
    // so we need to increase it here to keep the object alive
    Py_IncRef((PyObject *)py_image);
    return (PyObject *)py_image;
}

static PyMethodDef ASTCContext_methods[] = {
    {"compress", (PyCFunction)ASTCContext_method_comprocess, METH_VARARGS | METH_KEYWORDS, "compress an image."},
    {"decompress", (PyCFunction)ASTCContext_method_decompress, METH_VARARGS | METH_KEYWORDS, "decompress an image."},
    {NULL, NULL} /* Sentinel */
};

PyType_Slot ASTContext_slots[] = {
    {Py_tp_dealloc, ASTContext_dealloc},
    {Py_tp_doc, "ASTC Context"},
    {Py_tp_members, ASTCContext_members},
    {Py_tp_init, ASTContext_init},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_methods, ASTCContext_methods},
    {0, NULL},
};

static PyType_Spec ASTContext_Spec = {
    "astcenc.ASTCContext",                    // const char* name;
    sizeof(ASTContextT),                      // int basicsize;
    0,                                        // int itemsize;
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // unsigned int flags;
    ASTContext_slots,                         // PyType_Slot *slots;
};

/*
 *************************************************
 *
 * python connection
 *
 ************************************************
 */

// A struct contains the definition of a module
static PyModuleDef astc_encoder_module = {
    PyModuleDef_HEAD_INIT,
    MODULE_NAME, // Module name
    "a python wrapper for astc-encoder",
    -1,   // Optional size of the module state memory
    NULL, // Optional table of module-level functions
    NULL, // Optional slot definitions
    NULL, // Optional traversal function
    NULL, // Optional clear function
    NULL  // Optional module deallocation function
};

int add_object(PyObject *module, const char *name, PyObject *object)
{
    Py_IncRef(object);
    if (PyModule_AddObject(module, name, object) < 0)
    {
        Py_DecRef(object);
        Py_DecRef(module);
        return -1;
    }
    return 0;
}

// The module init function
PyMODINIT_FUNC INIT_FUNC_NAME(void)
{
    PyObject *m = PyModule_Create(&astc_encoder_module);
    if (m == NULL)
    {
        return NULL;
    }
    ASTCConfig_Object = PyType_FromSpec(&ASTCConfig_Spec);
    if (add_object(m, "ASTCConfig", ASTCConfig_Object) < 0)
    {
        return NULL;
    }

    ASTCImage_Object = PyType_FromSpec(&ASTCImage_Spec);
    if (add_object(m, "ASTCImage", ASTCImage_Object) < 0)
    {
        return NULL;
    }

    ASTCContext_Object = PyType_FromSpec(&ASTContext_Spec);
    if (add_object(m, "ASTCContext", ASTCContext_Object) < 0)
    {
        return NULL;
    }

    ASTCSwizzle_Object = PyType_FromSpec(&ASTCSwizzle_Spec);
    if (add_object(m, "ASTCSwizzle", ASTCSwizzle_Object) < 0)
    {
        return NULL;
    }

    return m;
}