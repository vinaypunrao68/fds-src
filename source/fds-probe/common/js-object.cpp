/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <fds_assert.h>
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
        js_global->js_add_global(obj, JSO_MGR_ADD_DEF);
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
    json_t        *val, *sub;
    JsObject      *obj;
    JsObjSet      *comp;
    JsArray       *arr;
    JsObjArray    *oarray;
    const char    *key;
    JsObjTemplate *decode;

    arr = NULL;
    num = json_array_size(in);
    if (num > 0) {
        arr = new JsArray(num);
        json_array_foreach(in, index, val) {
            if (json_is_object(val) || json_is_array(val)) {
                (*arr)[index] = js_new(val);
            } else {
                (*arr)[index] = js_decode_basic(val);
            }
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
        if (json_typeof(val) == JSON_ARRAY) {
            num    = json_array_size(val);
            oarray = new JsObjArray(num);

            json_array_foreach(val, index, sub) {
                if (decode != NULL) {
                    obj = decode->js_new(sub);
                } else {
                    obj = js_decode_basic(sub);
                }
                if (obj == NULL) {
                    delete oarray;
                    goto unknown;
                }
                (*oarray)[index] = obj;
            }
            (*comp)[key] = static_cast<JsObject *>(oarray);
        } else {
            if (decode == NULL) {
                obj = js_decode_basic(val);
            } else {
                obj = decode->js_new(val);
            }
            if (obj == NULL) {
                goto unknown;
            }
            (*comp)[key] = obj;
        }
    }
    return js_init(empty, in, NULL, comp, arr);

 unknown:
    delete comp;
    return NULL;
}

// js_decode_basic
// ---------------
//
JsObject *
JsObjTemplate::js_decode_basic(json_t *in)
{
    if (js_global == NULL) {
        return NULL;
    }
    return js_global->js_new_basic(in);
}

// js_get_template
// ---------------
//
JsObjTemplate *
JsObjTemplate::js_get_template(const char *key)
{
    return js_decode[key];
}

// js_register_template
// --------------------
// XXX: check for duplicate
//
void
JsObjTemplate::js_register_template(JsObjTemplate *templ)
{
    js_decode[templ->js_type_name] = templ;
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
        if (obj->js_exec_obj(NULL, this) != NULL) {
            delete obj;
        }
    }
}

// js_add_global
// -------------
//
JsObject *
JsObjTemplate::js_add_global(JsObject *obj, int opts)
{
    if (js_global != NULL) {
        return js_global->js_add_global(obj, opts);
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

// js_exec_simple
// --------------
//
JsObject *
JsObject::js_exec_simple(JsObject *array, JsObjTemplate *templ)
{
    if (js_array != NULL) {
        for (auto it = js_array->cbegin(); it != js_array->cend(); ++it) {
            (*it)->js_exec_obj(array, templ);
        }
    }
    if (js_comp != NULL) {
        for (auto it = js_comp->begin(); it != js_comp->end(); ++it) {
            it->second->js_exec_obj(array, templ);
        }
    }
    return this;
}

// js_exec_obj
// -----------
//
JsObject *
JsObject::js_exec_obj(JsObject *array, JsObjTemplate *templ)
{
    if (array != NULL) {
        int       i, num;
        JsObject *obj;

        num = array->js_array_size();
        for (i = 0; i < num; i++) {
            obj = array->js_array_elm(i);
            obj = obj->js_exec_simple(NULL, templ);

            // XXX: need to check remove out of array & free memory.
        }
        return NULL;
    }
    return js_exec_simple(NULL, templ);
}

// js_array_size
// -------------
// If the obj is an array, return number of elements.
//
int
JsObject::js_array_size()
{
    if (js_array != NULL) {
        return js_array->size();
    }
    return 0;
}

// ----------------------------------------------------------------------------
// JSON Array Obj Instance
// ----------------------------------------------------------------------------
JsObjArray::JsObjArray(int num) : JsObject()
{
    js_array = new JsArray(num);
}

JsObject *&
JsObjArray::operator[](int idx)
{
    fds_verify(idx < js_array->size());
    return (*js_array)[idx];
}

// js_exec_obj
// -----------
//
JsObject *
JsObjArray::js_exec_obj(JsObject *array, JsObjTemplate *templ)
{
    return (*js_array)[0]->js_exec_obj(this, templ);
}

// ----------------------------------------------------------------------------
// JSON Obj Manager/Factory
// ----------------------------------------------------------------------------
JsObjManager::JsObjManager()
    : js_mtx("JsObjManager Mtx"), js_decode(), js_objs()
{
    js_str_decode  = new JsObjStrTemplate<JsObject>(NULL, this);
    js_int_decode  = new JsObjIntTemplate<JsObject>(NULL, this);
    js_real_decode = new JsObjRealTemplate<JsObject>(NULL, this);
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
// XXX: check for duplicate.
//
void
JsObjManager::js_register_template(JsObjTemplate *templ)
{
    js_mtx.lock();
    js_decode[templ->js_type_name] = templ;
    js_mtx.unlock();
}

// js_get_template
// ---------------
//
JsObjTemplate *
JsObjManager::js_get_template(const char *name)
{
    JsObjTemplate *templ;

    js_mtx.lock();
    templ = js_decode[name];
    js_mtx.unlock();

    return templ;
}

// js_add_global
// -------------
//
JsObject *
JsObjManager::js_add_global(JsObject *obj, int opts)
{
    JsObject *old;

    old = NULL;
    js_mtx.lock();
    if (opts & JSO_MGR_ADD_REPLACE) {
        old = js_objs[obj->js_id_name];
        if ((old != NULL) && (opts & JSO_MGR_ADD_FREE_OLD)) {
            int cnt = js_objs.erase(obj->js_id_name);
            fds_verify(cnt == 1);
            delete old;
            old = NULL;
        }
    }
    js_objs[obj->js_id_name] = obj;
    js_mtx.unlock();
    return old;
}

// js_lookup
// ---------
//
JsObject *
JsObjManager::js_lookup(const char *id_name)
{
    JsObject *obj;

    js_mtx.lock();
    obj = js_objs[id_name];
    js_mtx.unlock();

    return obj;
}

// js_remove_global
// ----------------
//
JsObject *
JsObjManager::js_remove_global(const char *id_name)
{
    int       cnt;
    JsObject *obj;

    js_mtx.lock();
    obj = js_objs[id_name];
    if (obj != NULL) {
        cnt = js_objs.erase(id_name);
        fds_verify(cnt == 1);
    }
    js_mtx.unlock();
    return obj;
}

// js_new_basic
// ------------
//
JsObject *
JsObjManager::js_new_basic(json_t *in)
{
    JsObject *obj;

    if (json_is_string(in)) {
        return js_new_str_obj(in);
    }
    if (json_is_integer(in)) {
        return js_new_int_obj(in);
    }
    if (json_is_real(in)) {
        return js_new_real_obj(in);
    }
    return NULL;
}

}  // namespace fds
