/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#include <ctype.h>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <fds_assert.h>
#include <jansson.h>

namespace fds {

class JsEmitter;

class JsVarName
{
  public:
    static const int em_name_len = 256;

    JsVarName() {}
    virtual ~JsVarName() {}

    void em_assign_var(const char *name);

  protected:
    friend class JsEmitter;

    char em_key[JsVarName::em_name_len];
    char em_obj[JsVarName::em_name_len];
    char em_obj_tmpl[JsVarName::em_name_len];
    char em_obj_type[JsVarName::em_name_len];
    char em_obj_type_t[JsVarName::em_name_len];
    char em_obj_class[JsVarName::em_name_len];

    void em_conv_field_name(const char *in, char *out);
    void em_conv_object_name(const char *in, char *out);
};

class JsFileName
{
  public:
    JsFileName() {}
    virtual ~JsFileName() {}

    void em_assign_name(const char *in);
    void em_emit_header(std::ostream &os);
    void em_emit_footer(std::ostream &os);

  protected:
    friend class JsEmitter;

    char em_file[JsVarName::em_name_len];
    char em_hdr_guard[JsVarName::em_name_len];
};

class JsEmitter
{
  public:
    JsEmitter() {}
    virtual ~JsEmitter() {}

    void em_parse(json_t *obj, const char *parent, std::ostream &os);
    void em_file_header(std::ostream &os) { em_file.em_emit_header(os); }
    void em_file_footer(std::ostream &os) { em_file.em_emit_footer(os); }

  protected:
    JsVarName                em_var;
    JsFileName               em_file;

    void em_emit_struct(json_t *in, std::ostream &os);
    void em_emit_obj_methods(std::ostream &os);
    void em_emit_obj_tmpl_methods(const char *parent, std::ostream &os);
    void em_emit_simple_obj(json_t *in, const char *parent, std::ostream &os);
    void em_emit_decoder_cmnt(std::ostream &os);
    void em_emit_parse_code(json_t *in, const char *parent, std::ostream &os);

    void em_emit_nested_obj(json_t *in, const char *parent, std::ostream &os);
    void em_emit_array(json_t *in, const char *parent, std::ostream &os);
    void em_emit_array_struct(json_t *in, std::ostream &os);
    void em_emit_obj(json_t *in, const char *parent, std::ostream &os, bool simple);
};

/**
 * em_assign_var
 * -------------
 */
void
JsVarName::em_assign_var(const char *name)
{
    em_conv_object_name(name, em_obj);
    em_conv_field_name(name, em_obj_type_t);
    snprintf(em_obj_type, JsVarName::em_name_len, "%s%s", em_obj_type_t, "_in");
    snprintf(em_obj_type_t, JsVarName::em_name_len, "%s_t", em_obj_type);

    snprintf(em_obj_tmpl, JsVarName::em_name_len, "%sTmpl", em_obj);
    snprintf(em_obj_class, JsVarName::em_name_len, "%sObj", em_obj);
}

/**
 * em_conv_field_name
 * ------------------
 */
void
JsVarName::em_conv_field_name(const char *in, char *out)
{
    char       *dst;
    const char *src;

    /* Convert name to C var. */
    for (src = in, dst = out; *src != '\0'; src++) {
        if (*src == '-') {
            *dst++ = '_';
        } else {
            *dst++ = tolower(*src);
        }
    }
    *dst = '\0';
}

/**
 * em_conv_object_name
 * -------------------
 */
void
JsVarName::em_conv_object_name(const char *in, char *out)
{
    bool        upper;
    char       *dst;
    const char *src;

    /* Convert name to C++ CammelCase. */
    upper = true;
    for (src = in, dst = out; *src != '\0'; src++) {
        if (upper == true) {
            *dst++ = toupper(*src);
            upper  = false;
            continue;
        }
        if ((*src == '-') || (*src == '_')) {
            upper = true;
        } else {
            *dst++ = *src;
        }
    }
    *dst = '\0';
}

/**
 * em_assign_name
 * --------------
 */
void
JsFileName::em_assign_name(const char *in)
{
}

/**
 * em_emit_header
 * --------------
 */
void
JsFileName::em_emit_header(std::ostream &os)
{
    os <<
      "/*\n"
      " * Copyright 2014 by Formations Data System, Inc.\n"
      " *\n"
      " * *** This code is auto generate, DO NOT EDIT! ***\n"
      " */\n"
      "#include <shared/fds_types.h>\n"
      "#include <fds_assert.h>\n"
      "#include <jansson.h>\n"
      "#include <fds-probe/fds_probe.h>\n"
      "\n"
      "#ifndef _ABC_\n"
      "#define _ABC_\n\n"
      "namespace fds {\n\n";
}

/**
 * em_emit_footer
 * --------------
 */
void
JsFileName::em_emit_footer(std::ostream &os)
{
    os <<
      "}  // nsmespace fds\n"
      "#endif  // _ABC__\n";
}

/**
 * em_emit_struct
 * --------------
 */
void
JsEmitter::em_emit_struct(json_t *in, std::ostream &os)
{
    json_t     *val;
    const char *key;
    char        var[JsVarName::em_name_len];

    os <<
      "typedef struct " << em_var.em_obj_type << ' ' << em_var.em_obj_type_t << ";\n"
      "struct " << em_var.em_obj_type << "\n"
      "{\n";

    json_object_foreach(in, key, val) {
        os << "    ";
        em_var.em_conv_field_name(key, var);
        switch (json_typeof(val)) {
        case JSON_STRING:
            os << "char *\t\t\t" << var << ";\n";
            break;

        case JSON_TRUE:
        case JSON_FALSE:
            os << "bool\t\t\t" << var << ";\n";
            break;

        case JSON_INTEGER:
            os << "fds_uint32_t\t\t" << var << ";\n";
            break;

        default:
            break;
        }
    }
    os << "};\n\n";
}

/**
 * em_emit_array_struct
 * --------------------
 */
void
JsEmitter::em_emit_array_struct(json_t *in, std::ostream &os)
{
    em_emit_struct(in, os);
    os <<
      "typedef struct " << em_var.em_obj_type << "_arr "<< em_var.em_obj_type <<
      "_arr_t;\n" <<
      "struct " << em_var.em_obj_type << "_arr\n"
      "{\n"
      "    fds_uint32_t\t\t" << em_var.em_obj_type << "_cnt;\n"
      "    " << em_var.em_obj_type_t << "\t\t" << em_var.em_obj_type << "_arr[0];\n"
      "};\n\n";
}

/**
 * em_emit_decoder_cmnt
 * --------------------
 */
void
JsEmitter::em_emit_decoder_cmnt(std::ostream &os)
{
    os <<
      "/**\n"
      " * Decoder for:\n"
      " * \"" << em_var.em_obj << "\": { .. }\n"
      " */\n";
}

// em_emit_obj_methods
// -------------------
//
void
JsEmitter::em_emit_obj_methods(std::ostream &os)
{
    os <<
      "class " << em_var.em_obj_class << " : public JsObject\n"
      "{\n"
      "  public:\n"
      "    /**\n"
      "     * You need to provide the implementation for this method.\n"
      "     */\n"
      "    virtual JsObject *\n"
      "    js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out) "
      "override;\n\n";
}

// em_emit_obj_tmpl_methods
// ------------------------
//
void
JsEmitter::em_emit_obj_tmpl_methods(const char *parent, std::ostream &os)
{
    os <<
      "class " << em_var.em_obj_tmpl << " : public JsObjTemplate\n"
      "{\n"
      "  public:\n"
      "    explicit " << em_var.em_obj_tmpl <<
      "(JsObjManager *mgr) : JsObjTemplate(\"" << parent << "\", mgr) {}\n"
      "    virtual JsObject *js_new(json_t *in) override\n";
}

/**
 * em_emit_simple_obj
 * ------------------
 */
void
JsEmitter::em_emit_simple_obj(json_t *in, const char *parent, std::ostream &os)
{
    em_emit_obj_methods(os);
    os <<
      "    inline " << em_var.em_obj_type_t << " * " << em_var.em_obj_type << "() {\n"
      "        return static_cast<" << em_var.em_obj_type_t << " *>(js_pod_object());\n"
      "    }\n"
      "};\n\n";

    em_emit_decoder_cmnt(os);
    em_emit_obj_tmpl_methods(parent, os);
    os <<
      "    {\n"
      "        " << em_var.em_obj_type_t << " *p = new " << em_var.em_obj_type_t << ";\n"
      "        if (json_unpack(in, \"{\"\n";

    em_emit_parse_code(in, parent, os);
    os <<
      "            delete p;\n"
      "            return NULL;\n"
      "        }\n"
      "        return js_parse(new " << em_var.em_obj_class << "(), in, p);\n"
      "    }\n"
      "};\n\n";
}

/**
 * em_emit_array
 * -------------
 */
void
JsEmitter::em_emit_array(json_t *in, const char *parent, std::ostream &os)
{
    em_emit_obj_methods(os);
    os <<
      "    inline fds_uint32_t " << em_var.em_obj_type << "_elm() {\n"
      "        " << em_var.em_obj_type_t << " *data = static_cast<"
      << em_var.em_obj_type << "_arr_t *>(js_pod_object());\n"
      "        return data->" << em_var.em_obj_type << "_cnt;\n"
      "    }\n"
      "    inline " << em_var.em_obj_type_t << " * " << em_var.em_obj_type << "_arr() {\n"
      "        " << em_var.em_obj_type_t << " *data = static_cast<"
      << em_var.em_obj_type << "_arr_t *>(js_pod_object());\n"
      "        return data->" << em_var.em_obj_type << "_arr;\n"
      "    }\n"
      "};\n\n";

    em_emit_decoder_cmnt(os);
    em_emit_obj_tmpl_methods(parent, os);
    os <<
      "    {\n"
      "        size_t        i, num;\n"
      "        json_t       *arr, *elem;\n"
      "        " << em_var.em_obj_type << "_arr_t *data;\n"
      "        " << em_var.em_obj_type_t << " *args;\n"
      "\n"
      "        if (json_unpack(in, \"{s:o}\", &arr)) {\n"
      "            return NULL;\n"
      "        }\n"
      "        num = json_array_size(arr);\n"
      "        fds_verify(num > 0);\n"
      "\n"
      "        data = static_cast<" << em_var.em_obj_type_t << " *>(malloc("
      "sizeof(*data) + (num * sizeof(*args));\n"
      "        args = &data->" << em_var.em_obj_type << "_arr[0]\n"
      "        data->" << em_var.em_obj_type << "_cnt = num;\n"
      "\n"
      "        for (i = 0; i < num; i++) {\n"
      "            elem = json_array_get(arr, i);\n"
      "            if (json_unpack(in, \"{\"\n";

    em_emit_parse_code(in, parent, os);
    os<<
      "                free(data);\n"
      "                return NULL;\n"
      "            }\n"
      "        }\n"
      "        return js_parse(new " << em_var.em_obj_class << "(), in, data);\n"
      "    }\n"
      "};\n\n";
}

/**
 * em_emit_parse_code
 * ------------------
 */
void
JsEmitter::em_emit_parse_code(json_t *in, const char *parent, std::ostream &os)
{
    int         elm, cur;
    char        var[JsVarName::em_name_len];
    json_t     *val;
    const char *key;

    elm = 0;
    json_object_foreach(in, key, val) {
        elm++;
        switch (json_typeof(val)) {
        case JSON_STRING:
            os << "                     \"s:s \"\n";
            break;

        case JSON_TRUE:
        case JSON_FALSE:
            os << "                     \"s:b \"\n";
            break;

        case JSON_INTEGER:
            os << "                     \"s:i \"\n";
            break;

        case JSON_REAL:
            os << "                     \"s:f \"\n";
            break;

        default:
            break;
        }
    }
    os <<
      "                     \"}\",\n";
    cur = 0;
    json_object_foreach(in, key, val) {
        cur++;
        em_var.em_conv_field_name(key, var);
        switch (json_typeof(val)) {
        case JSON_STRING:
        case JSON_TRUE:
        case JSON_FALSE:
        case JSON_INTEGER:
        case JSON_REAL:
            os << "                     \"" << key << "\", &p->" << var;
            if (cur == elm) {
                os << ")) {\n";
            } else {
                os << ",\n";
            }
            break;

        default:
            break;
        }
    }
}

/**
 * em_emit_nested_obj
 * ------------------
 */
void
JsEmitter::em_emit_nested_obj(json_t *in, const char *parent, std::ostream &os)
{
    json_t     *val;
    const char *key;
    JsVarName   var;

    std::vector<JsVarName>   members;

    em_emit_decoder_cmnt(os);
    os <<
      "class " << em_var.em_obj_tmpl << " : public JsObjTemplate\n"
      "{\n"
      "  public:\n"
      "    explicit " << em_var.em_obj_tmpl << "(JsObjManager *mgr)\n"
      "        : JsObjTemplate(\"" << parent << "\", mgr)\n"
      "    {\n";

    json_object_foreach(in, key, val) {
        var.em_assign_var(key);
        os <<
          "        js_decode[\"" << key << "\"] = new " << var.em_obj_tmpl << "(mgr);\n";
    }
    os <<
      "    }\n"
      "    virtual JsObject *js_new(json_t *in) {\n"
      "        return js_parse(new JsObject(), in, NULL);\n"
      "    }\n"
      "};\n\n";
}

/**
 * em_emit_obj
 * -----------
 */
void
JsEmitter::em_emit_obj(json_t *in, const char *parent, std::ostream &os, bool simple)
{
    json_type    type, type_sub;
    bool         nested;
    json_t      *vcur, *vsub, *velm;
    const char  *kcur, *ksub;

    json_object_foreach(in, kcur, vcur) {
        type = json_typeof(vcur);
        if (type == JSON_ARRAY) {
            if (simple == true) {
                em_var.em_assign_var(kcur);
                velm = json_array_get(vcur, 0);
                em_emit_array_struct(velm, os);
                em_emit_array(velm, kcur, os);
                continue;
            }
        }
        if (type != JSON_OBJECT) {
            continue;
        }
        nested = false;
        json_object_foreach(vcur, ksub, vsub) {
            type_sub = json_typeof(vsub);
            if ((type_sub == JSON_OBJECT) || (type_sub == JSON_ARRAY)) {
                nested = true;
                break;
            }
         }
        em_var.em_assign_var(kcur);
        if ((nested == false) && (simple == true)) {
            em_emit_struct(vcur, os);
            em_emit_simple_obj(vcur, kcur, os);
        }
        em_emit_obj(vcur, kcur, os, simple);

        if ((nested == true) && (simple == false)) {
            /* Complex, recursive object. */
            em_var.em_assign_var(kcur);
            em_emit_nested_obj(vcur, kcur, os);
        }
    }
}


/**
 * em_parse
 * --------
 */
void
JsEmitter::em_parse(json_t *in, const char *parent, std::ostream &os)
{
    em_emit_obj(in, parent, os, true);
    em_emit_obj(in, parent, os, false);
}

}  // namespace fds

int
main(int argc, char **argv)
{
    json_t         *root;
    json_error_t    err;
    fds::JsEmitter  parser;

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <file.json>" << std::endl;
        exit(1);
    }
    root = json_load_file(argv[1], 0, &err);
    if (root == NULL) {
        std::cout << "Failed to load file " << err.source << '\n'
            << err.text << " at line " << err.line << ", col " << err.column
            << ", pos " << err.position << std::endl;
        exit(1);
    }
    parser.em_file_header(std::cout);
    parser.em_parse(root, "mgr", std::cout);
    parser.em_file_footer(std::cout);
    return 0;
}
