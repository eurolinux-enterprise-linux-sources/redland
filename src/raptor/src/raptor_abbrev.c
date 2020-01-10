/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * raptor_abbrev.c - Code common to abbreviating serializers (ttl/rdfxmla)
 *
 * Copyright (C) 2006, Dave Robillard
 * Copyright (C) 2004-2006, David Beckett http://purl.org/net/dajobe/
 * Copyright (C) 2004-2005, University of Bristol, UK http://www.bristol.ac.uk/
 * Copyright (C) 2005, Steve Shepard steveshep@gmail.com
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 *
 */

#ifdef HAVE_CONFIG_H
#include <raptor_config.h>
#endif

#ifdef WIN32
#include <win32_raptor_config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* Raptor includes */
#include "raptor.h"
#include "raptor_internal.h"


/*
 * raptor_abbrev_node implementation.
 *
 * FIXME Duplicate code
 *
 * Parts of this is taken from redland librdf_node.h and librdf_node.c
 *
 **/

raptor_abbrev_node* 
raptor_new_abbrev_node(raptor_identifier_type node_type, const void *node_data,
                       raptor_uri *datatype, const unsigned char *language)
{
  unsigned char *string;
  raptor_abbrev_node* node=NULL;
  
  if(node_type == RAPTOR_IDENTIFIER_TYPE_UNKNOWN)
    return 0;

  node = (raptor_abbrev_node*)RAPTOR_CALLOC(raptor_abbrev_node, 1,
                                            sizeof(raptor_abbrev_node));

  if(node) {
    node->ref_count = 1;
    node->type = node_type;
    
    switch (node_type) {
        case RAPTOR_IDENTIFIER_TYPE_PREDICATE:
          node->type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
          /* intentional fall through */
        case RAPTOR_IDENTIFIER_TYPE_RESOURCE:
          node->value.resource.uri = raptor_uri_copy((raptor_uri*)node_data);
          break;
          
        case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS:
          string=(unsigned char*)RAPTOR_MALLOC(blank,
                                               strlen((char*)node_data)+1);
          strcpy((char*)string, (const char*) node_data);
          node->value.blank.string = string;
          break;
          
        case RAPTOR_IDENTIFIER_TYPE_ORDINAL:
          node->value.ordinal.ordinal = *(int *)node_data;
          break;
          
        case RAPTOR_IDENTIFIER_TYPE_LITERAL:
        case RAPTOR_IDENTIFIER_TYPE_XML_LITERAL:
          string = (unsigned char*)RAPTOR_MALLOC(literal,
                                                 strlen((char*)node_data)+1);
          strcpy((char*)string, (const char*)node_data);
          node->value.literal.string = string;

          if(datatype) {
            node->value.literal.datatype = raptor_uri_copy(datatype);
          }

          if(language) {
            unsigned char *lang;
            lang=(unsigned char*)RAPTOR_MALLOC(language,
                                               strlen((const char*)language)+1);
            strcpy((char*)lang, (const char*)language);
            node->value.literal.language = lang;
          }
          break;
          
        case RAPTOR_IDENTIFIER_TYPE_UNKNOWN: 
        default:
          RAPTOR_FREE(raptor_abbrev_node, node);
    }
    
  }

  return node;
}


void
raptor_free_abbrev_node(raptor_abbrev_node* node)
{
  if(!node)
    return;

  if(--node->ref_count)
    return;
  
  switch (node->type) {
      case RAPTOR_IDENTIFIER_TYPE_RESOURCE:
      case RAPTOR_IDENTIFIER_TYPE_PREDICATE:
        raptor_free_uri(node->value.resource.uri);
        break;
          
      case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS:
        RAPTOR_FREE(blank, node->value.blank.string);
        break;
          
      case RAPTOR_IDENTIFIER_TYPE_LITERAL:
      case RAPTOR_IDENTIFIER_TYPE_XML_LITERAL:
        RAPTOR_FREE(literal, node->value.literal.string);

        if(node->value.literal.datatype)
          raptor_free_uri(node->value.literal.datatype);

        if(node->value.literal.language)
          RAPTOR_FREE(language, node->value.literal.language);

        break;
          
      case RAPTOR_IDENTIFIER_TYPE_ORDINAL:
      case RAPTOR_IDENTIFIER_TYPE_UNKNOWN: 
      default:
        /* Nothing to do */
        break;
  }

  RAPTOR_FREE(raptor_abbrev_node, node);
}

/* compare two raptor_abbrev_nodes.
 *
 * This needs to be a strong ordering for use by raptor_avltree.
 * This is very performance critical, anything to make it faster is worth it.
 */
int
raptor_abbrev_node_cmp(raptor_abbrev_node* node1, raptor_abbrev_node* node2)
{
  int rv = 0;  

  if(node1 == node2) {
    return 0;
  } else if(node1->type < node2->type) {
    return -1;
  } else if(node1->type > node2->type) {
    return 1;
  }

  switch (node1->type) {
      case RAPTOR_IDENTIFIER_TYPE_RESOURCE:
      case RAPTOR_IDENTIFIER_TYPE_PREDICATE:
        rv = raptor_uri_compare(node1->value.resource.uri,
                                node2->value.resource.uri);
        break;
          
      case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS:
        rv = strcmp((const char*)node1->value.blank.string,
                    (const char*)node2->value.blank.string);
        break;
          
      case RAPTOR_IDENTIFIER_TYPE_LITERAL:
      case RAPTOR_IDENTIFIER_TYPE_XML_LITERAL:

        if((char *)node1->value.literal.string != NULL &&
            (char *)node2->value.literal.string != NULL) {

          /* string */
          rv = strcmp((const char*)node1->value.literal.string,
                       (const char*)node2->value.literal.string);

          if(rv != 0) {
            break;
          /* if string is equal, order by language */
          } else {
            if((const char*)node1->value.literal.language != NULL &&
                (const char*)node2->value.literal.language != NULL) {
              rv = strcmp((const char*)node1->value.literal.language,
                    (const char*)node2->value.literal.language);
            } else if(node1->value.literal.language == NULL) {
              rv = -1;
              break;
            } else {
              rv = 1;
              break;
            }
          }

          /* if string and language are equal, order by datatype */
          /* (rv == 0 if we get here) */
          if(node1->value.literal.datatype != NULL &&
              node2->value.literal.datatype != NULL) {
            rv = strcmp((char*)node1->value.literal.datatype,
                  (char*)node2->value.literal.datatype);
          } else if(node1->value.literal.datatype == NULL) {
            rv = -1;
          } else {
            rv = 1;
          }
          
        } else {
          RAPTOR_FATAL1("string must be non-NULL for literal or xml literal\n");
          rv = 0;
        }        

        break;
          
      case RAPTOR_IDENTIFIER_TYPE_ORDINAL:
        if(node1->value.ordinal.ordinal == node2->value.ordinal.ordinal)
          rv = 0;
        else if(node1->value.ordinal.ordinal < node2->value.ordinal.ordinal)
          rv = -1;
        else
          rv = 1;

        break;
        
      case RAPTOR_IDENTIFIER_TYPE_UNKNOWN: 
      default:
        /* Nothing to do */
        break;
  }

  return rv;
  
}

int
raptor_abbrev_node_equals(raptor_abbrev_node* node1, raptor_abbrev_node* node2)
{
  return raptor_abbrev_node_cmp(node1, node2) == 0;
}


/*
 * raptor_abbrev_node_matches:
 * @node: #raptor_abbrev_node to compare
 * @node_type: Raptor identifier type
 * @node_data: For node_type RAPTOR_IDENTIFIER_TYPE_ORDINAL, int* to the
 *             ordinal.
 * @datatype: Literal datatype or NULL
 * @language: Literal language or NULL
 *
 * Return value: non-zero if @node matches the node described by the rest of
 *   the parameters.
 */
int
raptor_abbrev_node_matches(raptor_abbrev_node* node,
                           raptor_identifier_type node_type,
                           const void *node_data, raptor_uri *datatype,
                           const unsigned char *language)
{
  int rv = 0;
  
  if(node->type != node_type)
    return 0;

  switch (node->type) {
      case RAPTOR_IDENTIFIER_TYPE_RESOURCE:
      case RAPTOR_IDENTIFIER_TYPE_PREDICATE:
        rv = raptor_uri_equals(node->value.resource.uri,
                               (raptor_uri *)node_data);
        break;
          
      case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS:
        rv = !strcmp((const char*)node->value.blank.string,
                     (const char *)node_data);
        break;
          
      case RAPTOR_IDENTIFIER_TYPE_LITERAL:
      case RAPTOR_IDENTIFIER_TYPE_XML_LITERAL:

        if((char *)node->value.literal.string != NULL &&
            (char *)node_data != NULL) {

          /* string */
          rv = (strcmp((char *)node->value.literal.string,
                       (char *)node_data) == 0);

          /* language */
          if((char *)node->value.literal.language != NULL &&
              (char *)language != NULL)
            rv &= (strcmp((char *)node->value.literal.language,
                          (char *)language) == 0);
          else if((char *)node->value.literal.language != NULL ||
                  (char *)language != NULL)
            rv= 0;

          /* datatype */
          if(node->value.literal.datatype != NULL && datatype != NULL)
            rv &= (raptor_uri_equals(node->value.literal.datatype,datatype) !=0);
          else if(node->value.literal.datatype != NULL || datatype != NULL)
            rv = 0;
          
        } else {
          RAPTOR_FATAL1("string must be non-NULL for literal or xml literal\n");
          rv = 0;
        }        
        
        break;
          
      case RAPTOR_IDENTIFIER_TYPE_ORDINAL:
        rv = (node->value.ordinal.ordinal == *(int *)node_data);
        break;
        
      case RAPTOR_IDENTIFIER_TYPE_UNKNOWN: 
      default:
        /* Nothing to do */
        break;
  }

  return rv;
}


/*
 * raptor_abbrev_node_lookup:
 * @nodes: Sequence of nodes to search
 * @node_type: Raptor identifier type
 * @node_value: Node value to search with (using raptor_abbrev_node_matches).
 * @datatype: Literal datatype or NULL
 * @language: Literal language or NULL
 *
 * Return value: non-zero if @node matches the node described by the rest of
 *   the parameters.
 */
raptor_abbrev_node* 
raptor_abbrev_node_lookup(raptor_avltree* nodes,
                          raptor_identifier_type node_type,
                          const void *node_value, raptor_uri *datatype,
                          const unsigned char *language)
{
  /* Create a temporary node for search comparison. */
  raptor_abbrev_node* lookup_node = raptor_new_abbrev_node(
		  node_type, node_value, datatype, language);
  
  raptor_abbrev_node* rv_node = raptor_avltree_search(nodes, lookup_node);
  
  /* If not found, insert/return a new one */
  if(!rv_node) {
    
    if(raptor_avltree_add(nodes, lookup_node)) {
      /* Insert failed */
      raptor_free_abbrev_node(lookup_node);
	  return NULL;
    } else {
      return lookup_node;
    }

  /* Found */
  } else {
    raptor_free_abbrev_node(lookup_node);
    return rv_node;
  }
}

/*
 * raptor_abbrev_subject implementation
 *
 * The subject of triples, with all predicates and values
 * linked from them.
 *
 **/
raptor_abbrev_subject*
raptor_new_abbrev_subject(raptor_abbrev_node* node)
{
  raptor_abbrev_subject* subject;
  
  if(!(node->type == RAPTOR_IDENTIFIER_TYPE_RESOURCE ||
        node->type == RAPTOR_IDENTIFIER_TYPE_ANONYMOUS ||
        node->type == RAPTOR_IDENTIFIER_TYPE_ORDINAL)) {
    RAPTOR_FATAL1("Subject node must be a resource, blank, or ordinal\n");
    return NULL;
  }  
  
  subject = (raptor_abbrev_subject*)RAPTOR_CALLOC(raptor_subject, 1,
                                                  sizeof(raptor_abbrev_subject));

  if(subject) {
    subject->node = node;
    subject->node->ref_count++;
    subject->node->count_as_subject++;
    
    subject->node_type = NULL;
    subject->properties =
      raptor_new_sequence((raptor_sequence_free_handler *)raptor_free_abbrev_node, NULL);
    subject->list_items =
      raptor_new_sequence((raptor_sequence_free_handler *)raptor_free_abbrev_node, NULL);

    if(!subject->node || !subject->properties || !subject->list_items) {
      raptor_free_abbrev_subject(subject);
      subject = NULL;
    }
  
  }

  return subject;
}


void
raptor_free_abbrev_subject(raptor_abbrev_subject* subject) 
{
  if(subject) {
    if(subject->node)
      raptor_free_abbrev_node(subject->node);
    
    if(subject->node_type)
      raptor_free_abbrev_node(subject->node_type);
    
    if(subject->properties)
      raptor_free_sequence(subject->properties);

    if(subject->list_items)
      raptor_free_sequence(subject->list_items);

    RAPTOR_FREE(raptor_subject, subject);
  }
  
}


/*
 * raptor_subject_add_property:
 * @subject: subject node to add to
 * @predicate: predicate node
 * @object: object node
 * 
 * Add predicate/object pair into properties array of a subject node.
 * The subject node takes ownership of the predicate/object nodes.
 * On error, predicate/object are freed immediately.
 * 
 * Return value: non-0 on failure
 **/
int
raptor_abbrev_subject_add_property(raptor_abbrev_subject* subject,
                                   raptor_abbrev_node* predicate,
                                   raptor_abbrev_node* object) 
{
  int err;
  
  err = raptor_sequence_push(subject->properties, predicate);
  if(err) {
    /* predicate was already deleted by raptor_sequence on error */
    raptor_free_abbrev_node(object);
    return err;
  }
  
  err = raptor_sequence_push(subject->properties, object);
  if(err) {
    /* pop and delete predicate */
    raptor_sequence_pop(subject->properties);
    raptor_free_abbrev_node(predicate);
    /* object was already deleted by raptor_sequence on error */
    return err;
  }
  
  predicate->ref_count++;
  object->ref_count++;
  
  return 0;
}


/**
 * raptor_abbrev_subject_add_list_element:
 * @subject: subject node to add to
 * @ordinal: ordinal index
 * @object: object node
 * 
 * Add rdf:li into list element array of a #raptor_abbrev_subject node.
 * 
 * Return value: 
 **/
int
raptor_abbrev_subject_add_list_element(raptor_abbrev_subject* subject, 
                                       int ordinal,
                                       raptor_abbrev_node* object)
{
  int rv = 1;
  raptor_abbrev_node* node;

  node = (raptor_abbrev_node*)raptor_sequence_get_at(subject->list_items,
                                                     ordinal);
  if(!node) {
    /* If there isn't already an entry */
    rv = raptor_sequence_set_at(subject->list_items, ordinal, object);
    if(!rv) {
      object->ref_count++;
      object->count_as_subject++;
    }
  }
  
  return rv;
}


raptor_abbrev_subject*
raptor_abbrev_subject_find(raptor_sequence *sequence,
                           raptor_identifier_type node_type,
                           const void *node_data, int *idx)
{
  raptor_abbrev_subject* rv_subject = NULL;
  int i;
  
  for(i=0; i < raptor_sequence_size(sequence); i++) {
    raptor_abbrev_subject* subject=(raptor_abbrev_subject*)raptor_sequence_get_at(sequence, i);

    if(subject &&
       raptor_abbrev_node_matches(subject->node, node_type, node_data, NULL, NULL)) {
      rv_subject = subject;
      break;
    }
    
  }

  if(idx)
    *idx = i;
  
  return rv_subject;
}


raptor_abbrev_subject* 
raptor_abbrev_subject_lookup(raptor_avltree* nodes,
                             raptor_sequence* subjects,
                             raptor_sequence* blanks,
                             raptor_identifier_type node_type,
                             const void *node_data)
{
  raptor_sequence *sequence;
  raptor_abbrev_subject* rv_subject;

  /* Search for specified resource in resources array.
   * FIXME: this should really be a hash, not a list.
   */
  sequence= (node_type == RAPTOR_IDENTIFIER_TYPE_ANONYMOUS) ?
            blanks : subjects;
  rv_subject= raptor_abbrev_subject_find(sequence, node_type,
                                         node_data, NULL);

  /* If not found, create one and insert it */
  if(!rv_subject) {
    raptor_abbrev_node* node = raptor_abbrev_node_lookup(nodes, node_type,
                                                         node_data, NULL, NULL);
    if(node) {      
      rv_subject = raptor_new_abbrev_subject(node);
      if(rv_subject) {
        if(raptor_sequence_push(sequence, rv_subject)) {
          rv_subject = NULL;
        }      
      }
    }
  }
  
  return rv_subject;
}


#ifdef ABBREV_DEBUG
void
raptor_print_subject(raptor_abbrev_subject* subject) 
{
  int i;
  unsigned char *subj;
  unsigned char *pred;
  unsigned char *obj;

  /* Note: The raptor_abbrev_node field passed as the first argument for
   * raptor_statement_part_as_string() is somewhat arbitrary, since as
   * the data structure is designed, the first word in the value union
   * is what was passed as the subject/predicate/object of the
   * statement.
   */
  subj = raptor_statement_part_as_string(subject->node->value.resource.uri,
                                         subject->node->type, NULL, NULL);

  if(subject->type) {
      obj=raptor_statement_part_as_string(subject->type->value.resource.uri,
                                          subject->type->type,
                                          subject->type->value.literal.datatype,
                                          subject->type->value.literal.language);
      fprintf(stderr,"[%s, http://www.w3.org/1999/02/22-rdf-syntax-ns#type, %s]\n", subj, obj);      
      RAPTOR_FREE(cstring, obj);
  }
  
  for(i=0; i < raptor_sequence_size(subject->elements); i++) {

    raptor_abbrev_node* o = raptor_sequence_get_at(subject->elements, i);
    if(o) {
      obj = raptor_statement_part_as_string(o->value.literal.string,
                                            o->type,
                                            o->value.literal.datatype,
                                            o->value.literal.language);
      fprintf(stderr,"[%s, [rdf:_%d], %s]\n", subj, i, obj);      
      RAPTOR_FREE(cstring, obj);
    }
    
  }

  i=0;
  while (i < raptor_sequence_size(subject->properties)) {

    raptor_abbrev_node* p = raptor_sequence_get_at(subject->properties, i++);
    raptor_abbrev_node* o = raptor_sequence_get_at(subject->properties, i++);

    if(p && o) {
      pred = raptor_statement_part_as_string(p->value.resource.uri, p->type,
                                             NULL, NULL);
      obj = raptor_statement_part_as_string(o->value.literal.string,
                                            o->type,
                                            o->value.literal.datatype,
                                            o->value.literal.language);
      fprintf(stderr,"[%s, %s, %s]\n", subj, pred, obj);      
      RAPTOR_FREE(cstring, pred);
      RAPTOR_FREE(cstring, obj);
    }
    
  }
  
  RAPTOR_FREE(cstring, subj);
  
}
#endif


/* helper functions */

/*
 * raptor_unique_id:
 * @base: base ID
 * 
 * Generate a node ID for serializing
 *
 * Raptor doesn't check that blank IDs it generates are unique from
 * any specified by rdf:nodeID. Here, we need to emit IDs that are
 * different from the ones the parser generates so that there is no
 * collision. For now, just prefix a '_' to the parser generated
 * name.
 * 
 * Return value: new node ID
 **/
unsigned char*
raptor_unique_id(unsigned char *base) 
{
  const char *prefix = "_";
  int prefix_len = strlen(prefix);
  int base_len = strlen((const char*)base);
  int len = prefix_len + base_len + 1;
  unsigned char *unique_id;

  unique_id= (unsigned char *)RAPTOR_MALLOC(cstring, len);
  strncpy((char*)unique_id, prefix, prefix_len);
  strncpy((char*)unique_id+prefix_len, (char *)base, base_len);
  unique_id[len-1]='\0';
    
  return unique_id;
}


/*
 * raptor_new_qname_from_resource:
 * @namespaces: sequence of namespaces (corresponding to nstack)
 * @nstack: #raptor_namespace_stack to use/update
 * @namespace_count: size of nstack (may be modified)
 * @node: #raptor_abbrev_node to use 
 * 
 * Make an XML QName from the URI associated with the node.
 * 
 * Return value: the QName or NULL on failure
 **/
raptor_qname*
raptor_new_qname_from_resource(raptor_sequence* namespaces,
                               raptor_namespace_stack* nstack,
                               int* namespace_count,
                               raptor_abbrev_node* node)
{
  unsigned char* name=NULL;  /* where to split predicate name */
  size_t name_len=1;
  unsigned char *uri_string;
  size_t uri_len;
  unsigned char c;
  unsigned char *p;
  raptor_uri *ns_uri;
  raptor_namespace *ns;
  raptor_qname *qname;
  
  if(node->type != RAPTOR_IDENTIFIER_TYPE_RESOURCE) {
    RAPTOR_FATAL1("Node must be a resource\n");
    return NULL;
  }

  qname=raptor_namespaces_qname_from_uri(nstack, 
                                         node->value.resource.uri, 10);
  if(qname)
    return qname;
  
  uri_string = raptor_uri_as_counted_string(node->value.resource.uri, &uri_len);

  p= uri_string;
  name_len=uri_len;
  while(name_len >0) {
    if(raptor_xml_name_check(p, name_len, 10)) {
      name=p;
      break;
    }
    p++; name_len--;
  }
      
  if(!name || (name == uri_string))
    return NULL;

  c=*name; *name='\0';
  ns_uri=raptor_new_uri(uri_string);
  *name=c;
  
  ns = raptor_namespaces_find_namespace_by_uri(nstack, ns_uri);
  if(!ns) {
    /* The namespace was not declared, so create one */
    unsigned char prefix[2 + MAX_ASCII_INT_SIZE + 1];
	*namespace_count = *namespace_count + 1;
    sprintf((char *)prefix, "ns%d", *namespace_count);

    ns = raptor_new_namespace_from_uri(nstack, prefix, ns_uri, 0);

    /* We'll most likely need this namespace again. Push it on our
     * stack.  It will be deleted in
     * raptor_rdfxmla_serialize_terminate
     */
    raptor_sequence_push(namespaces, ns);
  }

  qname = raptor_new_qname_from_namespace_local_name(ns, name,  NULL);
  
  raptor_free_uri(ns_uri);

  return qname;
}
