/* Copyright (c) 2010-2018 the corto developers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CORTO_GEN_H_
#define CORTO_GEN_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct g_generator_s* g_generator;

typedef
int (*g_walkAction)(
    corto_object o,
    void* userData);

typedef
char *___ (*g_idAction)(
    char *in,
    corto_id out);

typedef
corto_int16 ___ (*g_startAction)(
    g_generator g);

typedef
void (*g_stopAction)(
    g_generator g);

typedef struct g_object {
    corto_object o;
    bool parseSelf;
    bool parseScope;
} g_object;

typedef struct g_attribute {
    char *key;
    char *value;
} g_attribute;

typedef enum g_idKind {
    CORTO_GENERATOR_ID_DEFAULT,
    CORTO_GENERATOR_ID_SHORT,
    CORTO_GENERATOR_ID_CLASS_UPPER,
    CORTO_GENERATOR_ID_CLASS_LOWER
}g_idKind;

struct g_generator_s {
    ut_ll objects;
    ut_ll files;
    ut_dl library;
    ut_ll imports;
    ut_ll private_imports;
    char *name;
    char *language;
    g_idKind idKind;
    ut_ll attributes; /* list<generatorAttribute> */

    g_startAction start_action;
    g_idAction id_action;

    g_object* current;
    corto_object package;
    bool inWalk;
    ut_ll anonymousObjects;
};

typedef struct g_fileSnippet {
    char *option;
    char *id;
    char *src;
    bool used;
}g_fileSnippet;

typedef struct g_file_s* g_file;
struct g_file_s {
    FILE* file;
    char *name;
    corto_uint32 indent;
    corto_object scope;
    bool endLine; /* If last written character was a '\n', the next write must insert indentation spaces. */
    ut_ll snippets; /* If file already exists, load existing snippets. */
    ut_ll headers; /* If file already exists, load existing headers-snippets */
    g_generator generator;
};

/* Create generator object. */
CORTO_G_EXPORT
g_generator g_new(
    char *name,
    char *language);

/* Control how id's are generated */
CORTO_G_EXPORT
g_idKind g_setIdKind(
    g_generator g,
    g_idKind kind);

/* Obtain generator name. */
CORTO_G_EXPORT
char *g_getName(
    g_generator g);

/* Obtain project name */
CORTO_G_EXPORT
char *g_getProjectName(
    g_generator g);

/* Get package object. */
CORTO_G_EXPORT
corto_package g_getPackage(
    g_generator g);

/* Obtain generator object that is currently parsed. */
CORTO_G_EXPORT
corto_object g_getCurrent(
    g_generator g);

/* Get generator language. */
CORTO_G_EXPORT
char *g_getLanguage(
    g_generator g);

/* Instruct the generator to generate for an object. */
CORTO_G_EXPORT
void g_parse(
    g_generator generator,
    corto_object object,
    bool parseSelf,
    bool parseScope);

/* Set attribute of generator */
CORTO_G_EXPORT
void g_setAttribute(
    g_generator g,
    char *key,
    char *value);

/* Get attribute from generator */
CORTO_G_EXPORT
char *g_getAttribute(
    g_generator g,
    char *key);

/* Load a generator library. */
CORTO_G_EXPORT
int16_t g_load(
    g_generator generator,
    char *library);

/* Free generator. */
CORTO_G_EXPORT
void g_free(
    g_generator generator);

/* Start generating. */
CORTO_G_EXPORT
int16_t g_start(
    g_generator generator);

/* === Generator utility functions */

/* Add import */
CORTO_G_EXPORT
int16_t g_import(
    g_generator generator,
    corto_object package);

/* Add private import */
CORTO_G_EXPORT
int16_t g_import_private(
    g_generator generator,
    corto_object package);

/* Walk generator objects. Parse scopes of generator objects when configured. */
CORTO_G_EXPORT
int g_walk(
    g_generator generator,
    g_walkAction action,
    void* userData);

/* Walk generator objects, do not parse scopes even if configured. */
CORTO_G_EXPORT
int g_walkNoScope(
    g_generator g,
    g_walkAction action,
    void* userData);

/* Recursively walk objects, will walk all objects under the scope of generator objects. */
CORTO_G_EXPORT
int g_walkRecursive(
    g_generator generator,
    g_walkAction action,
    void* userData);

/* Recursively walk all objects, include anonymous objects. */
CORTO_G_EXPORT
int g_walkAll(
    g_generator generator,
    g_walkAction action,
    void* userData);

/* Find generator object for object */
CORTO_G_EXPORT
g_object* g_findObject(
    g_generator g,
    corto_object o,
    corto_object* match);

CORTO_G_EXPORT
g_object* g_findObjectInclusive(
    g_generator g,
    corto_object o,
    corto_object* match);

/* Translate an object to a language-specific identifier. */
CORTO_G_EXPORT
char *g_fullOid(
    g_generator g,
    corto_object o,
    corto_id id);

/* Translate an object to a short identifier. */
CORTO_G_EXPORT
char *g_shortOid(
    g_generator g,
    corto_object o,
    corto_id id);

/* Translate an object to a language-specific identifier with idKind provided. */
CORTO_G_EXPORT
char *g_fullOidExt(
    g_generator g,
    corto_object o,
    corto_id id,
    g_idKind kind);

/* Translate an class-identifier to a language-specific identifier. */
CORTO_G_EXPORT
char *g_oid(
    g_generator g,
    corto_object o,
    corto_id id);

/* Translate an identifier to a language-specific identifier. */
CORTO_G_EXPORT
char *g_id(
    g_generator g,
    char *str,
    corto_id id);

/* A check on whether an object must be parsed or not. */
CORTO_G_EXPORT
bool g_mustParse(
    g_generator g,
    corto_object o);


/* === Generator file-utility class */

/* Open a file for writing. */
CORTO_G_EXPORT
g_file g_fileOpen(
    g_generator generator,
    const char *name,
    ...);

/* Open hidden file for writing. */
CORTO_G_EXPORT
g_file g_hiddenFileOpen(
    g_generator generator,
    const char *name,
    ...);

/* Get path for file */
CORTO_G_EXPORT
char* g_filePath(
    g_generator generator,
    corto_id buffer,
    const char *name, ...);

/* Get path for hidden file */
CORTO_G_EXPORT
char* g_hiddenFilePath(
    g_generator generator,
    corto_id buffer,
    const char *name, ...);

/* Close a file. */
CORTO_G_EXPORT
void g_fileClose(
    g_file file);

/* Return contents of a file. */
CORTO_G_EXPORT
char *g_fileRead(
    g_generator generator,
    const char *name);

/* Lookup an open file. */
CORTO_G_EXPORT
void g_fileGet(
    g_generator generator,
    const char *name);

/* Lookup an existing code-snippet */
CORTO_G_EXPORT
char *g_fileLookupSnippet(
    g_file file,
    const char *snippetId);

/* Lookup an existing code-header */
CORTO_G_EXPORT
char *g_fileLookupHeader(
    g_file file,
    const char *snippetId);

/* Increase indentation. */
CORTO_G_EXPORT
void g_fileIndent(
    g_file file);

/* Decrease indentation. */
CORTO_G_EXPORT
void g_fileDedent(
    g_file file);

/* Set scope of file */
CORTO_G_EXPORT
void g_fileScopeSet(
    g_file file,
    corto_object o);

/* Get scope of file */
CORTO_G_EXPORT
corto_object g_fileScopeGet(
    g_file file);

/* Write to a file. */
CORTO_G_EXPORT
int g_fileWrite(
    g_file file,
    char* fmt, ...);

/* Get generator */
CORTO_G_EXPORT
g_generator g_fileGetGenerator(
    g_file file);

/* Get dependencies based on metadata */
CORTO_G_EXPORT
ut_ll g_getDependencies(
    g_generator g);

/* == Generator unique name-generator for members utility */

/* Get name of member */
CORTO_G_EXPORT
char* corto_genMemberName(
    g_generator g,
    ut_ll cache,
    corto_member m,
    char *result);

/* Build cache to determine whether membernames occur more than once (due to inheritance) */
CORTO_G_EXPORT
ut_ll corto_genMemberCacheBuild(
    corto_interface o);

/* Clean cache */
CORTO_G_EXPORT
void corto_genMemberCacheClean(
    ut_ll cache);


#ifdef __cplusplus
}
#endif

#endif /* CORTO_GEN_H_ */
