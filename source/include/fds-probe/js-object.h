/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_
#define SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_

#include <vector>
#include <boost/utility.hpp>
#include <boost/unordered_map.hpp>
#include <jansson.h>
#include <shared/fds_types.h>
#include <concurrency/Mutex.h>

namespace fds {

class JsObject;
class JsObjManager;
class JsObjTemplate;
typedef std::vector<JsObject *> JsArray;
typedef boost::unordered_map<std::string, JsObject *> JsObjSet;
typedef boost::unordered_map<std::string, JsObjTemplate *> JsObjMap;

/**
 * JSON Object Instance.
 */
class JsObject
{
  public:
    const char              *js_type_name;

    JsObject();
    virtual ~JsObject();
    virtual JsObject *js_exec_obj(JsObjTemplate *templ);

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
    inline JsObject *js_array_elm(int index) {
        if (js_array == NULL) {
            return NULL;
        }
        return (*js_array)[index];
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
 */
class JsObjTemplate
{
  public:
    const char              *js_type_name;

    JsObjTemplate(const char *type_name, JsObjManager *mgr);
    virtual ~JsObjTemplate();

    virtual JsObjTemplate *js_get_decode(const char *key);

    /**
     * Factory method to create an obj based on JSON format.
     */
    virtual JsObject *js_new(json_t *in) = 0;

    /**
     * Use the template to run exec function on an object instance in json fmt.
     */
    virtual void js_exec(json_t *in);

  protected:
    JsObjMap                 js_decode;
    JsObjManager            *js_global;

    virtual JsObject *js_parse(JsObject *empty, json_t *in, void *bin);
    virtual JsObject *js_init(JsObject *obj, json_t *in, void *d,
                              JsObjSet *c, JsArray *a);
};

/**
 * JsObject Manager/Factory.
 */
class JsObjManager : public boost::noncopyable
{
  public:
    JsObjManager();
    ~JsObjManager();

    virtual void js_exec(json_t *root);

    /**
     * Register a JsObject template to the factory.  The obj is identifed by
     * its typed name, not id name (e.g. kw "ID-Name").
     */
    void js_register_template(JsObjTemplate *templ);

    /**
     */
    void js_add(JsObject *obj);

    /**
     * Lookup the JsObject from the schema based on its name (kw "ID-Name").
     * @return the JsObject if found; NULL otherwise.
     */
    JsObject *js_lookup(const char *name);

    /**
     * Lookup and remove the JsObject matching with the name.
     */
    JsObject *js_remove(const char *name);

  protected:
    fds_mutex                 js_mtx;
    JsObjMap                  js_decode;
    JsObjSet                  js_objs;
};

}   // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_
