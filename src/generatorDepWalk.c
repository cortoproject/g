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

#include <corto.g>

static
int corto_genDepBuildAction(
    corto_object o,
    void* userData);

/* Walk objects in correct dependency order. */
typedef struct g_itemWalk_t *g_itemWalk_t;
struct g_itemWalk_t {
    g_generator g;
    void *userData;
    corto_depresolver resolver;
    corto_bool bootstrap;
    corto_depresolver_action onDeclare;
    corto_depresolver_action onDefine;
    ut_ll anonymousObjects;
};

typedef struct g_depWalk_t* g_depWalk_t;
struct g_depWalk_t  {
    corto_object o;
    g_itemWalk_t data;
    ut_ll anonymousObjects;
};

static
corto_object corto_genDepFindAnonymous(
    g_depWalk_t data,
    corto_object o)
{
    corto_object result = o;

    if (!corto_check_attr(o, CORTO_ATTR_NAMED) || !corto_childof(root_o, o)) {
        if (!data->anonymousObjects) {
            data->anonymousObjects = ut_ll_new();
        }

        ut_iter iter = ut_ll_iter(data->anonymousObjects);
        while (ut_iter_hasNext(&iter)) {
            corto_object a = ut_iter_next(&iter);
            if (corto_compare(o, a) == CORTO_EQ) {
                result = a;
                break;
            }
        }

        if (o == result) {
            ut_ll_append(data->anonymousObjects, o);
        }
    }

    return result;
}

/* Serialize dependencies on references */
static
corto_int16 corto_genDepReference(
    corto_walk_opt* s,
    corto_value* info,
    void* userData)
{
    corto_object o = *(corto_object*)corto_value_ptrof(info);
    g_depWalk_t data = userData;

    CORTO_UNUSED(s);

    if (o && g_mustParse(data->data->g, o)) {
        corto_member m;

        m = NULL;

        if (info->kind == CORTO_MEMBER) {
            m = info->is.member.member;
            if (!m->type->reference) {
                m = NULL;
            }
        }

        /* Include dependencies on anonymous types */
        if (!corto_check_attr(o, CORTO_ATTR_NAMED) ||
            !corto_childof(root_o, o))
        {
            /* Look for equivalent anonymous objects. Since anonymous objects do
             * not have their own identity, they are equal if they have the same
             * value. Therefore, if multiple anonymous objects are found with
             * the same value, only insert it once in the dependency administration.
             */

            o = corto_genDepFindAnonymous(data, o);
            corto_genDepBuildAction(o, data->data);
        }

        /* Add dependency on item */
        if (m) {
            corto_state state = m->state;

            if (m->stateCondExpr) {
                corto_value v = corto_value_object(o, NULL);
                corto_value out;

                if (corto_value_field(&v, m->stateCondExpr, &out)) {
                    ut_throw("invalid stateCondExpr '%s' for member '%s'",
                        m->stateCondExpr,
                        corto_fullpath(NULL, m));
                    goto error;
                }

                if (corto_value_typeof(&out) != corto_type(corto_bool_o)) {
                    if (corto_value_cast(&out, corto_bool_o, &out)) {
                        ut_throw(
            "stateCondExpr '%s' of member '%s' is not castable to a boolean",
                            m->stateCondExpr,
                            corto_fullpath(NULL, m));
                        goto error;
                    }
                }

                corto_bool *result = corto_value_ptrof(&out);

                if (!*result) {
                    switch(state) {
                    case CORTO_DECLARED | CORTO_VALID:
                        state = CORTO_VALID;
                        break;
                    case CORTO_DECLARED:
                        state = CORTO_VALID;
                        break;
                    case CORTO_VALID:
                        state = CORTO_DECLARED;
                        break;
                    }
                }
            }

            corto_depresolver_depend(
                data->data->resolver, data->o, CORTO_VALID, o, state);
        } else {
            corto_depresolver_depend(
                data->data->resolver, data->o, CORTO_VALID, o, CORTO_VALID);
        }
    }

    return 0;
error:
    return -1;
}

/* Dependency serializer */
corto_walk_opt corto_genDepSerializer(void) {
    corto_walk_opt s;

    corto_walk_init(&s);
    s.reference = corto_genDepReference;
    s.access = CORTO_LOCAL;
    s.accessKind = CORTO_NOT;
    s.traceKind = CORTO_WALK_TRACE_ON_FAIL;

    return s;
}

/* Add dependencies for function arguments */
static
int corto_genDepBuildProc(
    corto_function f,
    struct g_depWalk_t* data)
{
    corto_uint32 i;
    corto_type t;

    for(i=0; i<f->parameters.length; i++) {
        t = f->parameters.buffer[i].type;
        if (g_mustParse(data->data->g, t)) {
            t = corto_genDepFindAnonymous(data, t);

            /* Type must be at least declared when the function is declared. */
            corto_depresolver_depend(
                data->data->resolver,
                f,
                CORTO_DECLARED,
                t,
                CORTO_DECLARED | CORTO_VALID);
        }
    }

    return 0;
}

/* Build dependency-administration for object */
static
int corto_genDepBuildAction(
    corto_object o,
    void* userData)
{
    g_itemWalk_t data;
    struct g_depWalk_t walkData;
    corto_walk_opt s;
    corto_object parent = NULL;

    if (corto_check_attr(o, CORTO_ATTR_NAMED)) {
        parent = corto_parentof(o);
    }

    data = userData;

    walkData.o = o;
    walkData.data = data;
    walkData.anonymousObjects = data->anonymousObjects;

    /* Object can be declared only after its type is defined. */
    if (g_mustParse(data->g, corto_typeof(o))) {
        corto_type t = corto_genDepFindAnonymous(&walkData, corto_typeof(o));
        corto_depresolver_depend(
            data->resolver, o, CORTO_DECLARED, t, CORTO_VALID);
    }

    /* TODO: this is not nice */
    if (corto_class_instanceof(corto_procedure_o, corto_typeof(o))) {
        /* Insert base-dependency: methods may only be declared after the base
         * of a class has been defined. */
        if (corto_typeof(o) != corto_type(corto_function_o)) {
            if (corto_class_instanceof(corto_class_o, parent) &&
                corto_interface(parent)->base)
            {
                if (g_mustParse(data->g, corto_interface(parent)->base)) {
                    corto_depresolver_depend(
                        data->resolver,
                        o,
                        CORTO_DECLARED,
                        corto_interface(parent)->base,
                        CORTO_VALID);
                }
            }
        }

        /* Add dependencies on function parameters - types must be declared
         * before function is declared. */
        if (corto_genDepBuildProc(corto_function(o), &walkData)) {
            goto error;
        }
    }

    /* Insert dependency on parent */
    if (corto_check_attr(o, CORTO_ATTR_NAMED) && corto_parentof(o)) {
        if (parent != root_o) { /* Root is always available */
            corto_int8 parentState =
                corto_type(corto_typeof(o))->parent_state;

            corto_depresolver_depend(
                data->resolver, o, CORTO_DECLARED, parent, parentState);
            if (parentState == CORTO_DECLARED) {
                corto_depresolver_depend(
                    data->resolver, parent, CORTO_VALID, o, CORTO_VALID);
            }
        }
    }

    /* Guard to ensure that the object is added to the dependency
     * administration */
    corto_depresolver_insert(data->resolver, o);

    /* Insert dependencies on references in the object-value */
    s = corto_genDepSerializer();
    if (corto_walk(&s, o, &walkData)) {
        goto error;
    }
    data->anonymousObjects = walkData.anonymousObjects;

    return 1;
error:
    return 0;
}


static
int corto_genDeclareAction(
    corto_object o,
    void* userData)
{
    g_itemWalk_t data;
    data = userData;
    if (data->onDeclare) {
        data->onDeclare(o, data->userData);
    }

    return 1;
}

static
int corto_genDefineAction(
    corto_object o,
    void* userData)
{
    g_itemWalk_t data;
    data = userData;
    if ((corto_typeof(o)->kind != CORTO_VOID) || (corto_typeof(o)->reference)) {
        if (data->onDefine) {
            data->onDefine(o, data->userData);
        }
    }
    return 1;
}

int corto_genDepWalk(
    g_generator g,
    corto_depresolver_action onDeclare,
    corto_depresolver_action onDefine,
    void* userData)
{
    struct g_itemWalk_t walkData;
    corto_depresolver resolver = corto_depresolverCreate(
        corto_genDeclareAction, corto_genDefineAction, &walkData);
    bool bootstrap = !strcmp(g_getAttribute(g, "bootstrap"), "true");

    /* Prepare walkData */
    walkData.g = g;
    walkData.userData = userData;
    walkData.resolver = resolver;
    walkData.onDefine = onDefine;
    walkData.onDeclare = onDeclare;
    walkData.bootstrap = FALSE;
    walkData.anonymousObjects = NULL;

    /* Build dependency administration */
    if (!bootstrap) {
        if (!g_walkRecursive(g, corto_genDepBuildAction, &walkData)) {
            ut_trace("dependency-builder failed.");
            goto error;
        }
    } else {
        /* When generating for bootstrap, disregard dependencies */
        g_walkRecursive(g, corto_genDeclareAction, &walkData);
        g_walkRecursive(g, corto_genDefineAction, &walkData);

    }

    if (walkData.anonymousObjects) {
        ut_ll_free(walkData.anonymousObjects);
    }

    return corto_depresolver_walk(resolver);
error:
    return -1;
}
