/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_
#define SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_

#include <list>
#include <vector>
#include <string>
#include <boost/utility.hpp>
#include <boost/unordered_map.hpp>
#include <jansson.h>
#include <shared/fds_types.h>
#include <concurrency/Mutex.h>

namespace fds {

class ProbeMod;
class JsObject;
class JsObjOutput;
class JsObjManager;
class JsObjTemplate;
typedef std::vector<JsObject *> JsArray;
typedef boost::unordered_map<std::string, JsObject *> JsObjSet;
typedef boost::unordered_map<std::string, JsObjTemplate *> JsObjMap;

/**
 * JSON Object Instance.
 * XXX: must have refcount to prevent deletion.
 */
class JsObject
{
  public:
    const char              *js_type_name;

    JsObject();
    explicit JsObject(int arr_size);

    virtual ~JsObject();
    virtual JsObject *js_exec_obj(JsObject *parent,
                                  JsObjTemplate *templ, JsObjOutput *out);

    /**
     * Decoder interface, map back to json formated string.
     */
    virtual fds_bool_t js_output(JsObjOutput *out, int indent = 0);

    /**
     * For array object, return the number of elements in the array.
     */
    virtual int js_array_size();
    virtual JsObject *&operator[](int idx);

    /**
     * Return binary type for this object.
     * @return cast the pointer to expected native binary object.
     */
    inline void *js_pod_object() {
        return js_bin_data;
    }
    /**
     * For complex object, return the sub object matching with the key.
     */
    inline JsObject *js_obj_field(const char *key) {
        if (js_comp == NULL) {
            return NULL;
        }
        return (*js_comp)[key];
    }
    inline fds_bool_t js_is_basic() {
        return js_bin_data != NULL;
    }

  protected:
    friend class JsObjManager;
    friend class JsObjTemplate;

    json_t                  *js_data;
    JsArray                 *js_array;
    JsObjSet                *js_comp;
    const char              *js_id_name;
    void                    *js_bin_data;

    void js_init_obj(const char *type, const char *id, json_t *in, void *d,
                     JsObjSet *c, JsArray *a);
};

/**
 * JSON Object Template to create object above.
 * XXX: make constructor to create a singleton object.
 */
class JsObjTemplate
{
  public:
    const char              *js_type_name;

    JsObjTemplate(const char *type_name, JsObjManager *mgr);
    virtual ~JsObjTemplate();

    /**
     * For complex object, return/register a decoder template.
     */
    virtual JsObjTemplate *js_get_template(const char *key);
    virtual void           js_register_template(JsObjTemplate *templ);

    /**
     * If don't have any obj decoder, try basic json types (e.g. str, int...)
     */
    virtual JsObject *js_decode_basic(json_t *in);

    /**
     * Factory method to create an obj based on JSON format.
     */
    virtual JsObject *js_new(json_t *in) = 0;


    /**
     * Use the template to run exec function on an object instance in json fmt.
     */
    virtual void js_exec(json_t *in, JsObjOutput *out);

    /**
     * Wrapper API for an obj to request to add itself in the global name space.
     */
    virtual JsObject *js_add_global(JsObject *obj, int opts);

    /**
     * Return the global object manager where this template registered to.
     */
    inline JsObjManager *js_global_mgr() {
        return js_global;
    }

  protected:
    JsObjMap                 js_decode;
    JsObjManager            *js_global;

    virtual JsObject *js_parse_array(json_t *in, JsObjTemplate *decode);
    virtual JsObject *js_parse(JsObject *empty, json_t *in, void *bin);
    virtual JsObject *js_init(JsObject *obj, json_t *in, void *d,
                              JsObjSet *c, JsArray *a);
};

/**
 * Output obj to manage fragment of output strings from other objs.
 */
class JsObjOutput
{
  public:
    JsObjOutput() : js_outlen(0) {}
    ~JsObjOutput() {}

    /**
     * Push the string to output list.  The string will be copied to the list.
     */
    virtual void js_push_str(const char *str);

    /**
     * Initialize the iterator that will be used for js_output method to pull
     * output string out.
     */
    inline std::list<std::string>::iterator js_output_init() {
        return js_output.begin();
    }
    /**
     * Return total length would be required to copy all output fragments to
     * a flat, linear string.
     */
    inline size_t js_output_len() {
        return js_outlen;
    }
    /**
     * Dump the output from the list of strings to one linear string, len.
     * @return the length consumed by the buffer and iterator that can be
     * used for the next time.
     */
    virtual size_t
    js_out(std::list<std::string>::iterator *it, char *buf, size_t len);

    /**
     * Enable the probe module to set/get its working context.
     */
    inline void js_set_context(ProbeMod *ctx) {
        js_ctx = ctx;
    }
    inline ProbeMod *js_get_context() {
        return js_ctx;
    }
  protected:
    ProbeMod                *js_ctx;
    size_t                   js_outlen;
    std::list<std::string>   js_output;
};

/**
 * Common objects that can be shared.  All T classes must inherit from the
 * base JsObject and provide the js_exec_obj() method.
 */
class JsObjBasic : public JsObject
{
  public:
    virtual fds_bool_t js_output(JsObjOutput *out, int indent);
};

template <class T>
class JsObjStrTemplate : public JsObjTemplate
{
  public:
    virtual ~JsObjStrTemplate() {}
    explicit JsObjStrTemplate(const char *type_name, JsObjManager *mgr)
        : JsObjTemplate(type_name, mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        size_t      len;
        char       *str;
        const char *val;

        val = json_string_value(in);
        if (val != NULL) {
            len = strlen(val);
            str = new char [len + 1];
            strncpy(str, val, len);
            str[len] = '\0';
            return js_parse(new T(), in, static_cast<void *>(str));
        }
        return NULL;
    }
};

template <class T>
class JsObjIntTemplate : public JsObjTemplate
{
  public:
    virtual ~JsObjIntTemplate() {}
    explicit JsObjIntTemplate(const char *type_name, JsObjManager *mgr)
        : JsObjTemplate(type_name, mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        if (json_is_integer(in)) {
            int *num = new int(json_integer_value(in));
            return js_parse(new T(), in, static_cast<void *>(num));
        }
        return NULL;
    }
};

template <class T>
class JsObjRealTemplate : public JsObjTemplate
{
  public:
    virtual ~JsObjRealTemplate() {}
    explicit JsObjRealTemplate(const char *type_name, JsObjManager *mgr)
        : JsObjTemplate(type_name, mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        if (json_is_real(in)) {
            double *num = new double(json_real_value(in));
            return js_parse(new T(), in, static_cast<void *>(num));
        }
        return NULL;
    }
};

const int JSO_MGR_ADD_REPLACE  = 0x0001;
const int JSO_MGR_ADD_FREE_OLD = 0x0002;
const int JSO_MGR_ADD_DEF      = (JSO_MGR_ADD_REPLACE | JSO_MGR_ADD_FREE_OLD);

/**
 * JsObject Manager/Factory.
 */
class JsObjManager : public boost::noncopyable
{
  public:
    JsObjManager();
    virtual ~JsObjManager();

    /**
     * Main entry to exec data parsed in json format.
     */
    virtual void js_exec(json_t *root, JsObjOutput *out);

    /**
     * Register a JsObject template to the factory.  The obj is identifed by
     * its typed name, not id name (e.g. keyword "ID-Name").
     */
    virtual void js_register_template(JsObjTemplate *templ);
    virtual JsObjTemplate *js_get_template(const char *name);

    /**
     * Add an object instance to the global name space.  The obj is identified
     * by the keyword "ID-Name"
     */
    virtual JsObject *js_add_global(JsObject *obj, int opts);

    /**
     * Lookup the JsObject from the schema based on its name (kw "ID-Name").
     * @return the JsObject if found; NULL otherwise.
     */
    virtual JsObject *js_lookup(const char *name);

    /**
     * Lookup and remove the JsObject matching with the name.
     */
    virtual JsObject *js_remove_global(const char *name);

    /**
     * Factory methods to allocate basic JSON types.
     */
    virtual JsObject *js_new_basic(json_t *in);

    inline JsObject *js_new_str_obj(json_t *str) {
        return js_str_decode->js_new(str);
    }
    inline JsObject *js_new_int_obj(json_t *in) {
        return js_str_decode->js_new(in);
    }
    inline JsObject *js_new_real_obj(json_t *in) {
        return js_str_decode->js_new(in);
    }

  protected:
    fds_mutex                      js_mtx;
    JsObjMap                       js_decode;
    JsObjSet                       js_objs;
    JsObjStrTemplate<JsObjBasic>  *js_str_decode;
    JsObjIntTemplate<JsObjBasic>  *js_int_decode;
    JsObjRealTemplate<JsObjBasic> *js_real_decode;
};

}   // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_
