/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_CLASS_LOOKUP_H
#define RE_CLASS_LOOKUP_H

typedef const struct class *
re_dialect_class_lookup(const char *name);

re_dialect_class_lookup re_class_literal;
re_dialect_class_lookup re_class_like;
re_dialect_class_lookup re_class_glob;
re_dialect_class_lookup re_class_sql;
re_dialect_class_lookup re_class_native;
re_dialect_class_lookup re_class_pcre;

#endif
