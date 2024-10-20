#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"
#include <future>
#include <thread>
#include <vector>
#include <cctype>

#include "astcenc.h"
#include "astcenc_error_metrics.hpp"

PyObject *ASTCError;

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
        PyErr_SetString(ASTCError, astcenc_get_error_string(status));
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
    {Py_tp_dealloc, (void *)ASTCConfig_dealloc},
    {Py_tp_repr, (void *)ASTCConfig_repr},
    {Py_tp_doc, (void *)"ASTC Configuration"},
    {Py_tp_members, ASTCConfig_members},
    {Py_tp_init, (void *)ASTCConfig_init},
    {Py_tp_new, (void *)PyType_GenericNew},
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

static Py_ssize_t calc_ASTCImage_data_size(ASTCImageT *image)
{
    Py_ssize_t factor;
    astcenc_type data_type = image->image.data_type;
    if (data_type == ASTCENC_TYPE_U8)
    {
        factor = 4 * 1;
    }
    else if (data_type == ASTCENC_TYPE_F16)
    {
        factor = 4 * 2;
    }
    else if (data_type == ASTCENC_TYPE_F32)
    {
        factor = 4 * 4;
    }
    else
    {
        PyErr_SetString(ASTCError, "Invalid data type.");
        return -1;
    }
    return image->image.dim_x * image->image.dim_y * image->image.dim_z * factor;
}

static PyMemberDef ASTCImage_members[] = {
    {"dim_x", T_UINT, offsetof(ASTCImageT, image.dim_x), READONLY, "The X dimension of the image, in texels."},
    {"dim_y", T_UINT, offsetof(ASTCImageT, image.dim_y), READONLY, "The Y dimension of the image, in texels."},
    {"dim_z", T_UINT, offsetof(ASTCImageT, image.dim_z), READONLY, "The Z dimension of the image, in texels."},
    {"data_type", T_UINT, offsetof(ASTCImageT, image.data_type), READONLY, "The data type per component."},
    {NULL} /* Sentinel */
};

static PyObject *ASTCImage_get_data(ASTCImageT *self, void *closure)
{
    Py_IncRef(self->data);
    return self->data;
}

static int ASTCImage_set_data(ASTCImageT *self, PyObject *value, void *closure)
{
    if (value != Py_None && (!PyBytes_Check(value) || PyBytes_Size(value) != calc_ASTCImage_data_size(self)))
    {
        PyErr_SetString(ASTCError, "Image data size does not match the image dimensions with the given data type!");
        return -1;
    }
    Py_DecRef(self->data);
    Py_IncRef(value);
    self->data = value;
    return 0;
}

static PyGetSetDef ASTCImage_getseters[] = {
    {"data", (getter)ASTCImage_get_data, (setter)ASTCImage_set_data, "The array of 2D slices, of length dim_x * dim_y * dim_z * size_of(data_type) * 4.", NULL},
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

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "BII|IO!", (char **)kwlist, &data_type, &self->image.dim_x, &self->image.dim_y, &self->image.dim_z, &PyBytes_Type, &self->data))
    {
        return -1;
    }

    self->image.data_type = (astcenc_type)data_type;
    Py_IncRef(self->data);

    if (data_type != ASTCENC_TYPE_U8 && data_type != ASTCENC_TYPE_F16 && data_type != ASTCENC_TYPE_F32)
    {
        PyErr_SetString(ASTCError, "Invalid data type.");
        return -1;
    }

    if (self->data != Py_None && PyBytes_Size(self->data) != calc_ASTCImage_data_size(self))
    {
        PyErr_SetString(ASTCError, "Image data size does not match the image dimensions with the given data type!");
        return -1;
    }

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
    {Py_tp_dealloc, (void *)ASTCImage_dealloc},
    {Py_tp_doc, (void *)"ASTC Image"},
    {Py_tp_members, ASTCImage_members},
    {Py_tp_getset, ASTCImage_getseters},
    {Py_tp_init, (void *)ASTCImage_init},
    {Py_tp_new, (void *)PyType_GenericNew},
    {Py_tp_repr, (void *)ASTCImage_repr},
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

typedef struct StrSwizzle
{
    char chr;
    astcenc_swz value;
} StrSwizzle;

static StrSwizzle str_swizzle_map[] = {
    {'R', ASTCENC_SWZ_R},
    {'G', ASTCENC_SWZ_G},
    {'B', ASTCENC_SWZ_B},
    {'A', ASTCENC_SWZ_A},
    {'0', ASTCENC_SWZ_0},
    {'1', ASTCENC_SWZ_1},
    {'Z', ASTCENC_SWZ_Z},
};

static char swizzle_to_char(astcenc_swz swizzle)
{
    for (size_t i = 0; i < sizeof(str_swizzle_map) / sizeof(StrSwizzle); i++)
    {
        if (str_swizzle_map[i].value == swizzle)
        {
            return str_swizzle_map[i].chr;
        }
    }
    return '\0';
}

static bool char_to_swizzle(char chr, astcenc_swz *swizzle)
{
    for (size_t i = 0; i < sizeof(str_swizzle_map) / sizeof(StrSwizzle); i++)
    {
        if (str_swizzle_map[i].chr == chr)
        {
            *swizzle = str_swizzle_map[i].value;
            return true;
        }
    }
    return false;
}

static PyObject *ASTCSwizzle_repr(ASTCSwizzleT *self)
{
    return PyUnicode_FromFormat("ASTCSwizzle<%c%c%c%c>", swizzle_to_char(self->swizzle.r), swizzle_to_char(self->swizzle.g), swizzle_to_char(self->swizzle.b), swizzle_to_char(self->swizzle.a));
}

static PyObject *ASTCSwizzle_from_str(PyObject *cls, PyObject *args)
{
    char *str;
    if (!PyArg_ParseTuple(args, "s", &str))
    {
        return NULL;
    }

    astcenc_swizzle swizzle;
    if (strlen(str) != 4)
    {
        PyErr_SetString(ASTCError, "Swizzle string must be exactly 4 characters long.");
        return NULL;
    }

    str[0] = std::toupper(static_cast<unsigned char>(str[0]));
    str[1] = std::toupper(static_cast<unsigned char>(str[1]));
    str[2] = std::toupper(static_cast<unsigned char>(str[2]));
    str[3] = std::toupper(static_cast<unsigned char>(str[3]));

    if (!char_to_swizzle(str[0], &swizzle.r) || !char_to_swizzle(str[1], &swizzle.g) || !char_to_swizzle(str[2], &swizzle.b) || !char_to_swizzle(str[3], &swizzle.a))
    {
        PyErr_SetString(ASTCError, "Invalid swizzle character.");
        return NULL;
    }

    ASTCSwizzleT *swizzle_obj = PyObject_New(ASTCSwizzleT, (PyTypeObject *)cls);
    swizzle_obj->swizzle = swizzle;
    return (PyObject *)swizzle_obj;
}

static PyMethodDef ASTCSwizzle_methods[] = {
    {"from_str", (PyCFunction)ASTCSwizzle_from_str, METH_VARARGS | METH_CLASS,
     "Create a new ASTCSwizzle object from a string."},
    {NULL, NULL, 0, NULL}};

PyType_Slot ASTCSwizzle_slots[] = {
    {Py_tp_dealloc, (void *)ASTCSwizzle_dealloc},
    {Py_tp_doc, (void *)"ASTC Swizzle"},
    {Py_tp_members, ASTCSwizzle_members},
    {Py_tp_init, (void *)ASTCSwizzle_init},
    {Py_tp_new, (void *)PyType_GenericNew},
    {Py_tp_repr, (void *)ASTCSwizzle_repr},
    {Py_tp_methods, (void *)ASTCSwizzle_methods},
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

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!|I", (char **)kwlist, ASTCConfig_Object, &self->config, &self->threads))
    {
        return -1;
    }

    if (self->threads == 0)
    {
        self->threads = std::thread::hardware_concurrency();
    }

    Py_IncRef((PyObject *)self->config);
    astcenc_error status = astcenc_context_alloc((const astcenc_config *)&self->config->config, self->threads, &self->context);
    if (status != ASTCENC_SUCCESS)
    {
        PyErr_SetString(ASTCError, astcenc_get_error_string(status));
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
    static char *keywords[] = {(char *)"image", (char *)"swizzle", NULL};
    ASTCImageT *py_image = nullptr;
    ASTCSwizzleT *py_swizzle = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!", (char **)keywords, ASTCImage_Object, &py_image, ASTCSwizzle_Object, &py_swizzle))
    {
        return NULL;
    }

    astcenc_image *image = &py_image->image;
    astcenc_config *config = &self->config->config;

    // prepare image
    uint8_t *image_data = (uint8_t *)PyBytes_AsString(py_image->data);
    if (image_data == nullptr)
    {
        // PyBytes_AsString returns NULL if the object can't be interpreted as bytes
        // and sets an exception
        return NULL;
    }
    image->data = reinterpret_cast<void **>(&image_data);

    // Space needed for 16 bytes of output per compressed block
    unsigned int block_count_x = (image->dim_x + config->block_x - 1) / config->block_x;
    unsigned int block_count_y = (image->dim_y + config->block_y - 1) / config->block_y;
    unsigned int block_count_z = (image->dim_z + config->block_z - 1) / config->block_z;
    size_t comp_len = block_count_x * block_count_y * block_count_z * 16;

    PyObject *py_comp_data = PyBytes_FromStringAndSize(nullptr, comp_len);
    uint8_t *comp_data = (uint8_t *)PyBytes_AsString(py_comp_data);

    // run the compressor
    astcenc_error status;

    Py_BEGIN_ALLOW_THREADS;
    if (self->threads > 1)
    {
        status = ASTCENC_SUCCESS;

        std::vector<std::future<astcenc_error>> futures(self->threads);
        for (int thread_index = 0; thread_index < self->threads; thread_index++)
        {
            futures[thread_index] = std::async(astcenc_compress_image, self->context,
                                               image,
                                               &py_swizzle->swizzle,
                                               comp_data,
                                               comp_len,
                                               thread_index);
        }

        for (auto &future : futures)
        {
            astcenc_error future_status = future.get();
            if (future_status != ASTCENC_SUCCESS)
            {
                status = future_status;
            }
        }
    }
    else
    {
        status = astcenc_compress_image(
            self->context,
            image,
            &py_swizzle->swizzle,
            comp_data,
            comp_len,
            0);
    }
    Py_END_ALLOW_THREADS;

    if (status != ASTCENC_SUCCESS)
    {
        Py_DecRef(py_comp_data);
        PyErr_SetString(ASTCError, astcenc_get_error_string(status));
        py_comp_data = NULL;
    }

    status = astcenc_compress_reset(self->context);
    if (status != ASTCENC_SUCCESS)
    {
        Py_DecRef(py_comp_data);
        PyErr_SetString(ASTCError, astcenc_get_error_string(status));
        py_comp_data = NULL;
    }

    // cleanup
    image->data = nullptr;

    return py_comp_data;
}

PyObject *ASTCContext_method_decompress(ASTContextT *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {(char *)"data", (char *)"image", (char *)"swizzle", NULL};

    const uint8_t *comp_data;
    Py_ssize_t comp_len;
    ASTCImageT *py_image = nullptr;
    ASTCSwizzleT *py_swizzle = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y#O!O!", (char **)keywords, &comp_data, &comp_len, ASTCImage_Object, &py_image, ASTCSwizzle_Object, &py_swizzle))
    {
        return NULL;
    }

    astcenc_image *image = &py_image->image;
    astcenc_config *config = &self->config->config;

    // check if comp data is long enough
    // Space needed for 16 bytes of output per compressed block
    unsigned int block_count_x = (image->dim_x + config->block_x - 1) / config->block_x;
    unsigned int block_count_y = (image->dim_y + config->block_y - 1) / config->block_y;
    unsigned int block_count_z = (image->dim_z + config->block_z - 1) / config->block_z;
    Py_ssize_t expected_comp_len = block_count_x * block_count_y * block_count_z * 16;
    if (comp_len != expected_comp_len)
    {
        return PyErr_Format(ASTCError, "Compressed data size does not match the image dimensions. Expected at %d, got %d.", expected_comp_len, comp_len);
    }

    // prepare image
    Py_ssize_t image_len = calc_ASTCImage_data_size(py_image);

    PyObject *py_image_data = PyBytes_FromStringAndSize(nullptr, image_len);
    uint8_t *image_data = (uint8_t *)PyBytes_AsString(py_image_data);
    image->data = reinterpret_cast<void **>(&image_data);

    // run the decompressor
    astcenc_error status;

    Py_BEGIN_ALLOW_THREADS;
    if (self->threads > 1)
    {
        status = ASTCENC_SUCCESS;

        std::vector<std::future<astcenc_error>> futures(self->threads);
        for (int thread_index = 0; thread_index < self->threads; thread_index++)
        {
            futures[thread_index] = std::async(astcenc_decompress_image, self->context,
                                               comp_data,
                                               comp_len,
                                               image,
                                               &py_swizzle->swizzle,
                                               thread_index);
        }

        for (auto &future : futures)
        {
            astcenc_error future_status = future.get();
            if (future_status != ASTCENC_SUCCESS)
            {
                status = future_status;
            }
        }
    }
    else
    {
        status = astcenc_decompress_image(
            self->context,
            comp_data,
            comp_len,
            image,
            &py_swizzle->swizzle,
            0);
    }
    Py_END_ALLOW_THREADS;

    if (status != ASTCENC_SUCCESS)
    {
        Py_DecRef(py_image_data);
        py_image_data = NULL;
        PyErr_SetString(ASTCError, astcenc_get_error_string(status));
    }

    status = astcenc_decompress_reset(self->context);
    if (status != ASTCENC_SUCCESS)
    {
        Py_DecRef(py_image_data);
        py_image_data = NULL;
        PyErr_SetString(ASTCError, astcenc_get_error_string(status));
    }

    // create a python bytes object from the decompressed data
    Py_IncRef(py_image_data);
    Py_DecRef(py_image->data);
    py_image->data = py_image_data;

    // cleanup
    image->data = nullptr;

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
    {Py_tp_dealloc, (void *)ASTContext_dealloc},
    {Py_tp_doc, (void *)"ASTC Context"},
    {Py_tp_repr, (void *)ASTContext_repr},
    {Py_tp_members, ASTCContext_members},
    {Py_tp_init, (void *)ASTContext_init},
    {Py_tp_new, (void *)PyType_GenericNew},
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
static PyObject *compute_error_metrics_py(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *kwlist[] = {
        "compute_hdr_metrics",    // Compute HDR metrics.
        "compute_normal_metrics", // Compute normal map metrics.
        "input_components",       // The number of components in the input images.
        "img1",                   // The first image to compare.
        "img2",                   // The second image to compare.
        "fstop_lo",               // The low end of the f-stop range.
        "fstop_hi",               // The high end of the f-stop range.
        NULL};

    // python's p(redicate) for bool casts to int
    int compute_hdr_metrics;
    int compute_normal_metrics;
    int input_components;
    ASTCImageT *py_img1 = nullptr;
    ASTCImageT *py_img2 = nullptr;
    int fstop_lo;
    int fstop_hi;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ppiO!O!ii", (char **)kwlist, &compute_hdr_metrics, &compute_normal_metrics, &input_components, ASTCImage_Object, &py_img1, ASTCImage_Object, &py_img2, &fstop_lo, &fstop_hi))
    {
        return NULL;
    }

    if (input_components < 0 || input_components > 4)
    {
        PyErr_SetString(ASTCError, "Invalid input components (0-4).");
        return NULL;
    }

    astcenc_image *image1 = &py_img1->image;
    uint8_t *image1_data = (uint8_t *)PyBytes_AsString(py_img1->data);
    image1->data = reinterpret_cast<void **>(&image1_data);

    astcenc_image *image2 = &py_img2->image;
    uint8_t *image2_data = (uint8_t *)PyBytes_AsString(py_img2->data);
    image2->data = reinterpret_cast<void **>(&image2_data);

    astcenc_error_metrics metrics = compute_error_metrics(
        compute_hdr_metrics,
        compute_normal_metrics,
        input_components,
        image1,
        image2,
        fstop_lo,
        fstop_hi);

    return Py_BuildValue("{s:d,s:d,s:d,s:d,s:d,s:d,s:d,s:d}",
                         "psnr", metrics.psnr,
                         "psnr_rgb", metrics.psnr_rgb,
                         "psnr_alpha", metrics.psnr_alpha,
                         "peak_rgb", metrics.peak_rgb,
                         "mspnr_rgb", metrics.mspnr_rgb,
                         "log_rmse_rgb", metrics.log_rmse_rgb,
                         "mean_angular_errorsum", metrics.mean_angular_errorsum,
                         "worst_angular_errorsum", metrics.worst_angular_errorsum);
}

static PyMethodDef astc_encoder_functions[] = {
    {"compute_error_metrics", (PyCFunction)compute_error_metrics_py, METH_VARARGS | METH_KEYWORDS, "compute error metrics"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

// A struct contains the definition of a module
static PyModuleDef astc_encoder_module = {
    PyModuleDef_HEAD_INIT,
    MODULE_NAME, // Module name
    "a python wrapper for astc-encoder",
    -1,                     // Optional size of the module state memory
    astc_encoder_functions, // Optional table of module-level functions
    NULL,                   // Optional slot definitions
    NULL,                   // Optional traversal function
    NULL,                   // Optional clear function
    NULL                    // Optional module deallocation function
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

    ASTCError = PyErr_NewException("astc_encoder.ASTCError", nullptr, nullptr);
    if (add_object(m, "ASTCError", ASTCError) < 0)
    {
        return NULL;
    }

    return m;
}