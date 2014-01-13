/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <fds-probe/js-object.h>

namespace fds {

// ----------------------------------------------------------------------------
// JSON Obj Template
// ----------------------------------------------------------------------------
JsObjTemplate::JsObjTemplate(const char *type_name, JsObjManager *mgr)
    : js_type_name(type_name), js_decode(), js_global(mgr) {}

JsObjTemplate::~JsObjTemplate() {}

// js_init
// -------
//
JsObject *
JsObjTemplate::js_init(JsObject *obj, json_t *in, void *bin,
                       JsObjSet *comp, JsArray *array)
{
    char     *id_name;

    if (json_unpack(in, "{s:s}", "ID-Name", &id_name)) {
        id_name = NULL;
    }
    obj->js_init_obj(js_type_name, id_name, in, bin, comp, array);

    if ((js_global != NULL) && (id_name != NULL)) {
        js_global->js_add(obj);
    }
    return obj;
}

// js_parse
// --------
//
JsObject *
JsObjTemplate::js_parse(JsObject *empty, json_t *in, void *bin)
{
    size_t         num, index;
    json_t        *val;
    JsObject      *obj;
    JsObjSet      *comp;
    JsArray       *arr;
    const char    *key;
    JsObjTemplate *decode;

    arr = NULL;
    num = json_array_size(in);
    if (num > 0) {
        arr = new JsArray(num);
        json_array_foreach(in, index, val) {
            (*arr)[index] = js_new(val);
        }
    }
    if (bin != NULL) {
        // This is the basic type, the caller has the binary format.
        //
        return js_init(empty, in, bin, NULL, NULL);
    }
    // This is the complex type, we need to go further down the hierachy.
    //
    comp = new JsObjSet();
    json_object_foreach(in, key, val) {
        decode = js_decode[key];
        if (decode == NULL) {
            goto basic;
        }
        obj = decode->js_new(val);
        if (obj == NULL) {
            goto basic;
        }
        (*comp)[key] = obj;
    }
    return js_init(empty, in, NULL, comp, arr);

 basic:
    delete comp;
    return NULL;
}

// js_get_decode
// -------------
//
JsObjTemplate *
JsObjTemplate::js_get_decode(const char *key)
{
    return js_decode[key];
}

// js_exec
// -------
//
void
JsObjTemplate::js_exec(json_t *in)
{
    int           index;
    json_t       *val;
    JsObject     *obj;

    json_array_foreach(in, index, val) {
        js_exec(val);
    }
    obj = js_new(in);
    if (obj != NULL) {
        if (obj->js_exec_obj(this) != NULL) {
            delete obj;
        }
    }
}

// ----------------------------------------------------------------------------
// JSON Obj Instance
// ----------------------------------------------------------------------------
JsObject::JsObject()
    : js_data(NULL), js_array(NULL), js_comp(NULL),
      js_id_name(NULL), js_bin_data(NULL) {}

// js_init_obj
// -----------
//
void
JsObject::js_init_obj(const char *type, const char *id, json_t *in, void *bin,
                      JsObjSet *comp, JsArray *array)
{
    js_type_name = type;
    js_id_name   = id;
    js_bin_data  = bin;
    js_comp      = comp;
    js_data      = json_incref(in);
    js_array     = array;
}

JsObject::~JsObject()
{
    json_decref(js_data);
    if (js_bin_data != NULL) {
        delete reinterpret_cast<char *>(js_bin_data);
    }
    if (js_comp != NULL) {
        delete js_comp;
    }
    if (js_array != NULL) {
        delete js_array;
    }
}

// js_exec_obj
// -----------
//
JsObject *
JsObject::js_exec_obj(JsObjTemplate *templ)
{
    json_t        *val;
    const char    *key;
    JsObjTemplate *decode;

    json_object_foreach(js_data, key, val) {
        decode = templ->js_get_decode(key);
        if (decode != NULL) {
            decode->js_exec(val);
        }
    }
}

// ----------------------------------------------------------------------------
// JSON Obj Manager/Factory
// ----------------------------------------------------------------------------
JsObjManager::JsObjManager()
    : js_mtx("JsObjManager Mtx"), js_decode(), js_objs()
{
}

JsObjManager::~JsObjManager()
{
}

// js_exec
// -------
//
void
JsObjManager::js_exec(json_t *root)
{
    int            idx;
    json_t        *val;
    const char    *key;
    JsObjTemplate *decode;

    json_array_foreach(root, idx, val) {
        js_exec(val);
    }
    json_object_foreach(root, key, val) {
        decode = js_decode[key];
        if (decode == NULL) {
            continue;
        }
        decode->js_exec(val);
    }
}

// js_register_template
// --------------------
//
void
JsObjManager::js_register_template(JsObjTemplate *templ)
{
    js_mtx.lock();
    js_decode[templ->js_type_name] = templ;
    js_mtx.unlock();
}

// js_add
// ------
//
void
JsObjManager::js_add(JsObject *obj)
{
    js_mtx.lock();
    js_objs[obj->js_id_name] = obj;
    js_mtx.unlock();
}

// js_lookup
// ---------
//
JsObject *
JsObjManager::js_lookup(const char *id_name)
{
    return NULL;
}

// js_remove
// ---------
//
JsObject *
JsObjManager::js_remove(const char * id_name)
{
    return NULL;
}

}  // namespace fds
