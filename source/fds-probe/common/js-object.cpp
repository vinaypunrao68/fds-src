/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <list>
#include <string>
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

// js_parse_array
// --------------
//
JsObject *
JsObjTemplate::js_parse_array(json_t *in, JsObjTemplate *decode)
{
    size_t         num, index;
    json_t        *sub;
    JsArray       *arr;
    JsObject      *obj;

    num = json_array_size(in);
    arr = new JsArray(num);
    fds_verify(num > 0);

    json_array_foreach(in, index, sub) {
        if (json_typeof(sub) == JSON_ARRAY) {
            obj = js_parse_array(sub, decode);
        } else {
            if (decode != NULL) {
                obj = decode->js_new(sub);
            } else {
                obj = js_decode_basic(sub);
            }
        }
        if (obj == NULL) {
            delete arr;
            return NULL;
        }
        (*arr)[index] = obj;
    }
    return js_init(new JsObject(), in, NULL, NULL, arr);
}

// js_parse
// --------
//
JsObject *
JsObjTemplate::js_parse(JsObject *empty, json_t *in, void *bin)
{
    json_t        *val;
    JsObject      *obj;
    JsObjSet      *comp;
    const char    *key;
    JsObjTemplate *decode;

    if (bin != NULL) {
        // This is the basic type, the caller has the binary format.
        //
        return js_init(empty, in, bin, NULL, NULL);
    }
    if (json_typeof(in) == JSON_ARRAY) {
        // This is the array.  The component name is the parent object.
        //
        return js_parse_array(in, this);
    }
    // This is the complex type, we need to go further down the hierachy.
    //
    comp = new JsObjSet();
    json_object_foreach(in, key, val) {
        decode = js_decode[key];
        if (json_typeof(val) == JSON_ARRAY) {
            obj = js_parse_array(val, decode);
         } else {
            if (decode == NULL) {
                obj = js_decode_basic(val);
            } else {
                obj = decode->js_new(val);
            }
        }
        if (obj == NULL) {
            continue;
        }
        (*comp)[key] = obj;
    }
    return js_init(empty, in, NULL, comp, NULL);

    // unknown:
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
JsObjTemplate::js_exec(json_t *in, JsObjOutput *out)
{
    uint           index;
    json_t       *val;
    JsObject     *obj;

    json_array_foreach(in, index, val) {
        js_exec(val, out);
    }
    obj = js_new(in);
    if (obj != NULL) {
        if (obj->js_exec_obj(NULL, this, out) != NULL) {
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
    return NULL;
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
JsObject::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    if (js_array != NULL) {
        if ((*js_array)[0]->js_array_size() == 0) {
            (*js_array)[0]->js_exec_obj(this, templ, out);
        } else {
            for (auto it = js_array->cbegin(); it != js_array->cend(); ++it) {
                (*it)->js_exec_obj(this, templ, out);
            }
        }
    }
    if (js_comp != NULL) {
        for (auto it = js_comp->cbegin(); it != js_comp->cend(); ++it) {
            // XXX: need to check remove out of the table & free memory.
            it->second->js_exec_obj(this, templ, out);
        }
    }
    // js_output(out);
    return this;
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

JsObject *&
JsObject::operator[](int idx)
{
    fds_verify((uint)idx < js_array->size());
    return (*js_array)[idx];
}

// js_output
// ---------
//
fds_bool_t
JsObject::js_output(JsObjOutput *out, int indent)
{
    fds_bool_t  ret, more;
    const char *beg, *end;

    ret = false;
    if (js_array != NULL) {
        for (auto it = js_array->cbegin(); it != js_array->cend(); ++it) {
            if ((*it)->js_output(out, indent + 1) == true) {
                if ((it + 1) != js_array->cend()) {
                    out->js_push_str(", ");
                }
                ret = true;
            }
        }
    }
    if (js_comp != NULL) {
        for (auto it = js_comp->cbegin(); it != js_comp->cend(); ++it) {
            out->js_push_str("\"");
            out->js_push_str(it->first.c_str());
            out->js_push_str("\"");

            auto lst = it;
            more = (++lst != js_comp->cend()) ? true : false;
            if (it->second->js_array_size() == 0) {
                if (js_is_basic()) {
                    beg = NULL;
                    end = (more == true) ? ",\n" : NULL;
                } else {
                    beg = ":{\n";
                    end = (more == true) ? "},\n" : "}\n";
                }
            } else {
                beg = ":[\n";
                end = (more == true) ? "],\n" : "]\n";
            }
            if (beg != NULL) {
                ret = true;
                out->js_push_str(beg);
            }
            it->second->js_output(out, indent + 1);
            if (end != NULL) {
                out->js_push_str(end);
            }
        }
    }
    return ret;
}

// js_output
// ---------
// Format output for basic data types.
//
fds_bool_t
JsObjBasic::js_output(JsObjOutput *out, int indent)
{
    char number[64];

    if (json_is_string(js_data)) {
        out->js_push_str("\"");
        out->js_push_str(reinterpret_cast<char *>(js_pod_object()));
        out->js_push_str("\"");
        return true;
    }
    if (json_is_integer(js_data)) {
        snprintf(number, sizeof(number),
                 "%ud", *(reinterpret_cast<unsigned int *>(js_pod_object())));
    } else if (json_is_real(js_data)) {
        snprintf(number, sizeof(number),
                 "%f", *(reinterpret_cast<double *>(js_pod_object())));
    } else {
        return false;
    }
    out->js_push_str(number);
    return true;
}

// ----------------------------------------------------------------------------
// JSON Obj Output
// ----------------------------------------------------------------------------

// js_push_str
// -----------
//
void
JsObjOutput::js_push_str(const char *str)
{
    js_output.push_back(str);
    js_outlen += js_output.back().size();
}

// js_output
// ---------
//
size_t
JsObjOutput::js_out(std::list<std::string>::iterator *it, char *buf, size_t len)
{
    size_t size, used;

    used = 0;
    size = 0;
    for (; (*it) != js_output.cend(); ++(*it)) {
        if ((**it).size() > len) {
            return size;
        }
        used  = snprintf(buf, len, "%s", (**it).c_str());
        size += used;
        buf  += used;
        len  -= used;
    }
    return size;
}

// ----------------------------------------------------------------------------
// JSON Obj Manager/Factory
// ----------------------------------------------------------------------------
JsObjManager::JsObjManager()
    : js_mtx("JsObjManager Mtx"), js_decode(), js_objs()
{
    js_str_decode  = new JsObjStrTemplate<JsObjBasic>(NULL, this);
    js_int_decode  = new JsObjIntTemplate<JsObjBasic>(NULL, this);
    js_real_decode = new JsObjRealTemplate<JsObjBasic>(NULL, this);
}

JsObjManager::~JsObjManager()
{
}

// js_exec
// -------
//
void
JsObjManager::js_exec(json_t *root, JsObjOutput *out)
{
    uint            idx;
    json_t        *val;
    const char    *key;
    JsObjTemplate *decode;

    json_array_foreach(root, idx, val) {
        js_exec(val, out);
    }
    json_object_foreach(root, key, val) {
        decode = js_decode[key];
        if (decode == NULL) {
            continue;
        }
        decode->js_exec(val, out);
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
