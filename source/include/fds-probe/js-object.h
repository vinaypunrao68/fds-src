/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_
#define SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_

#include <boost/utility.h>
#include <jansson.h>

namespace fds {

/**
 * Template to decode a json object to binary format.  A JsObject type is:
 * - A JSON object w/out any nested sub JSON object under it.
 * - Has a unique name in its schema.
 * - Can only contains char *, int, real, true, false types.
 * - Can be array of such types.
 */
typedef struct json_decode json_decode_t;
struct json_decode
{
    json_type                js_type;
    fds_uint32_t             js_arr_cnt;
    size_t                   js_obj_size;
    const char              *js_keyword;
};

/**
 * Generic JSON Object.
 */
class JsObject
{
  public:
    JsObject(const json_decode_t *decode, int cnt);
    virtual ~JsObject() {}

    /**
     * Factory method to create this obj based on JSON format.
     *
     * @param spec (i) - the JSON format must not have any sub obj under it.
     *     If it's the array, elements must be simple object.
     */
    virtual JsObject *json_new(const json_t *spec) = 0;

    /**
     * Return binary type for this object.
     *
     * @param size (o) - size of the returned element.
     * @return cast the pointer to expected native binary object.
     */
    void *json_pod_object(size_t *size);

    /**
     * Return binary type for the given field name.
     *
     * @param keyword (i) - name of the field.
     * @param size (o) - size of the returned element.
     * @return cast the pointer to expected element (e.g. char *, int *...)
     */
    void *json_pod_field(const char *keyword, size_t *size);

    /**
     * Put POD type binary data to this object.
     *
     * @param bin (i) - cast from the binary data to copy to this obj.
     * @param size (i) - size of the struct for consistency check with the
     *     decoder.
     * @return true if the size matches & data is copied; false otherwise.
     */
    fds_bool_t json_put_pod_object(const void *bin, size_t size);

    /**
     * Put POD type binary field data to this object.
     *
     * @param keyword (i) - name of the field.
     * @param size (o) - size of the returned element.
     * @return true if the size matches & data is copied; false otherwise.
     */
    fds_bool_t json_put_pod_field(const char *kw, const void *field, size_t sz);

    /**
     * Return the JSON structure from raw binary data in this obj.
     * @return the new JSON obj; caller must free it.
     */
    json_t *json_encode_object();

  protected:
    int                      js_dc_cnt;
    const json_decode_t     *js_decode;
    const json_t            *js_obj;
    const char              *js_name;

    size_t                   js_bin_size;
    void                    *js_bin_data;
};

/**
 * JsObject Factory.
 *
 * Data from a .json schema is parsed to break down to JsObject type
 * (e.g. basic object, no sub object).  Each object is identifyed by
 * its unique name.
 *
 */
class JsObjFactory : public boost::noncopyable
{
  public:
    JsObjFactory();
    ~JsObjFactory();

    /**
     * Register a JsObject template to the factory.
     */
    void json_register(const JsObject *template);

    /**
     * Create a JsObject from JSON format with the given name.
     * The object is also registered to the factory for lookup.
     *
     * @return the new object if it meets all the requirements.
     */
    JsObject *json_create(const json_t *spec, const char *name);

    /**
     * Lookup the JsObject from the schema based on its name.
     * @return the JsObject if found; NULL otherwise.
     */
    JsObject *json_lookup(const char *name);

    /**
     * Lookup and remove the JsObject matching with the name.
     */
    JsObject *json_remove(const char *name);

  protected:
};

}   // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROBE_JS_OBJECT_H_
