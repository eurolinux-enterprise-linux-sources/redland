/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * raptor_avltree.c - Balanced Binary Tree / AVL Tree
 *
 * This file is in the public domain.
 *
 * Based on public domain sources posted to comp.sources.misc in 1993
 *
 * From: p...@vix.com (Paul Vixie)
 * Newsgroups: comp.sources.unix
 * Subject: v27i034: REPOST AVL Tree subroutines (replaces v11i020 from 1987), Part01/01
 * Date: 6 Sep 1993 13:51:22 -0700
 * Message-ID: <1.747348668.4037@gw.home.vix.com>
 * 
 * ----------------------------------------------------------------------
 * Original headers below
 */

/* as_tree - tree library for as
 * vix 14dec85 [written]
 * vix 02feb86 [added tree balancing from wirth "a+ds=p" p. 220-221]
 * vix 06feb86 [added tree_mung()]
 * vix 20jun86 [added tree_delete per wirth a+ds (mod2 v.) p. 224]
 * vix 23jun86 [added delete uar to add for replaced nodes]
 * vix 22jan93 [revisited; uses RCS, ANSI, POSIX; has bug fixes]
 */


/* This program text was created by Paul Vixie using examples from the book:
 * "Algorithms & Data Structures," Niklaus Wirth, Prentice-Hall, 1986, ISBN
 * 0-13-022005-1.  This code and associated documentation is hereby placed
 * in the public domain.
 */


#ifdef HAVE_CONFIG_H
#include <raptor_config.h>
#endif

#ifdef WIN32
#include <win32_raptor_config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


/* Raptor includes */
#include "raptor.h"
#include "raptor_internal.h"


#if RAPTOR_DEBUG > 1
#define RAPTOR_AVLTREE_DEBUG1(msg) RAPTOR_DEBUG1(msg)
#else
#define RAPTOR_AVLTREE_DEBUG1(msg)
#endif


#ifndef STANDALONE
typedef struct raptor_avltree_node_s raptor_avltree_node;

/* AVL-tree node */
struct raptor_avltree_node_s {
  /* left child tree */
  struct raptor_avltree_node_s *left;

  /* right child tree */
  struct raptor_avltree_node_s *right;

  /* balance factor =
   *   height of the right tree minus the height of the left tree
   * i.e. equal: 0  left larger: -1  right larger: 1
   */
  short balance;

  /* actual data */
  raptor_avltree_t  data;
};


/* AVL-tree */
struct raptor_avltree_s {
  /* root node of tree */
  raptor_avltree_node* root;

  /* node comparison function (optional) */
  raptor_avltree_compare_function compare_fn;

  /* node deletion function (optional) */
  raptor_avltree_delete_function delete_fn;

  /* tree flags (none defined at present) */
  unsigned int flags;
};


#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif


/* local prototypes */
static int raptor_avltree_sprout(raptor_avltree* tree, raptor_avltree_node** node_pp, raptor_avltree_t p_data, int *rebalancing_p);
static int raptor_avltree_delete_internal(raptor_avltree* tree, raptor_avltree_node** node_pp, raptor_avltree_t p_data, int *rebalancing_p, int *delete_called_p);
static void raptor_avltree_delete_internal2(raptor_avltree* tree, raptor_avltree_node** ppr_r, int *rebalancing_p, raptor_avltree_node** ppr_q, int *delete_called_p);
static void raptor_avltree_balance_left(raptor_avltree* tree, raptor_avltree_node** node_pp, int *rebalancing_p);
static void raptor_avltree_balance_right(raptor_avltree* tree, raptor_avltree_node** node_pp, int *rebalancing_p);
static raptor_avltree_t raptor_avltree_search_internal(raptor_avltree* tree, raptor_avltree_node* node, raptor_avltree_t p_data);
static int raptor_avltree_visit_internal(raptor_avltree* tree, raptor_avltree_node* node, int depth, raptor_avltree_visit_function visit_fn, void* user_data);
static void raptor_free_avltree_internal(raptor_avltree* tree, raptor_avltree_node* node);



/* constructor */
raptor_avltree*
raptor_new_avltree(raptor_avltree_compare_function compare_fn,
                   raptor_avltree_delete_function delete_fn,
                   unsigned int flags)
{
  raptor_avltree* tree;
  
  tree=(raptor_avltree*)malloc(sizeof(*tree));
  if(!tree)
    return NULL;
  
  tree->root=NULL;
  tree->compare_fn=compare_fn;
  tree->delete_fn=delete_fn;
  tree->flags=flags;
  
  return tree;
}


/* destructor */
void
raptor_free_avltree(raptor_avltree* tree)
{
  raptor_free_avltree_internal(tree, tree->root);
  free(tree);
}


static void
raptor_free_avltree_internal(raptor_avltree* tree, raptor_avltree_node* node)
{
  if(node) {
    raptor_free_avltree_internal(tree, node->left);

    raptor_free_avltree_internal(tree, node->right);

    if(tree->delete_fn)
      tree->delete_fn(node->data);
    free(node);
  }
}


/* methods */

static raptor_avltree_t
raptor_avltree_search_internal(raptor_avltree* tree, raptor_avltree_node* node,
                               raptor_avltree_t p_data)
{
  if(node) {
    int cmp= tree->compare_fn(p_data, node->data);

    if(cmp > 0)
      return raptor_avltree_search_internal(tree, node->right, p_data);
    else if(cmp < 0)
      return raptor_avltree_search_internal(tree, node->left, p_data);

    /* found */
    return node->data;
  }

  /* otherwise not found */
  return NULL;
}


raptor_avltree_t
raptor_avltree_search(raptor_avltree* tree, raptor_avltree_t p_data)
{
  return raptor_avltree_search_internal(tree, tree->root, p_data);
}


int
raptor_avltree_add(raptor_avltree* tree, raptor_avltree_t p_data)
{
  int rebalancing= FALSE;
  return raptor_avltree_sprout(tree, &tree->root, p_data, &rebalancing);
}


int
raptor_avltree_delete(raptor_avltree* tree, raptor_avltree_t p_data)
{
  int rebalancing= FALSE;
  int delete_called= FALSE;

  return raptor_avltree_delete_internal(tree, &tree->root, p_data,
                                        &rebalancing, &delete_called);
}


static int
raptor_avltree_visit_internal(raptor_avltree* tree, raptor_avltree_node* node,
                              int depth,
                              raptor_avltree_visit_function visit_fn,
                              void* user_data)
{
  if(!node)
    return TRUE;

  if(!raptor_avltree_visit_internal(tree, node->left, depth+1, 
                                    visit_fn, user_data))
    return FALSE;

  if(!visit_fn(depth, node->data, user_data))
    return FALSE;

  if(!raptor_avltree_visit_internal(tree, node->right, depth+1,
                                    visit_fn, user_data))
    return FALSE;

  return TRUE;
}


int
raptor_avltree_visit(raptor_avltree* tree,
                     raptor_avltree_visit_function visit_fn,
                     void* user_data)
{
  return raptor_avltree_visit_internal(tree, tree->root, 0,
                                       visit_fn, user_data);
}


static int
raptor_avltree_sprout_left(raptor_avltree* tree, raptor_avltree_node** node_pp,
                           raptor_avltree_t p_data, int *rebalancing_p)
{
  raptor_avltree_node *p1, *p2;
  int rc;
  
  RAPTOR_AVLTREE_DEBUG1("LESS. raptor_avltree_sprouting left.\n");
  rc=raptor_avltree_sprout(tree, &(*node_pp)->left, p_data, rebalancing_p);
  if(rc)
    return rc;

  if(!*rebalancing_p)
    return FALSE;

  /* left branch has grown longer */
  RAPTOR_AVLTREE_DEBUG1("LESS: left branch has grown\n");
  switch((*node_pp)->balance) {
    case 1:		
      /* right branch WAS longer; balance is ok now */
      RAPTOR_AVLTREE_DEBUG1("LESS: case 1.. balance restored implicitly\n");
      (*node_pp)->balance= 0;
      *rebalancing_p= FALSE;
      break;

    case 0:
      /* balance WAS okay; now left branch longer */
      RAPTOR_AVLTREE_DEBUG1("LESS: case 0.. balance bad but still ok\n");
      (*node_pp)->balance= -1;
      break;

    case -1:
      /* left branch was already too long. rebalqnce */
      RAPTOR_AVLTREE_DEBUG1("LESS: case -1: rebalancing\n");
      p1= (*node_pp)->left;

      if(p1->balance == -1) {
        /* LL */
        RAPTOR_AVLTREE_DEBUG1("LESS: single LL\n");
        (*node_pp)->left= p1->right;
        p1->right = *node_pp;
        (*node_pp)->balance= 0;
        *node_pp= p1;

      } else {
        /* double LR */
        RAPTOR_AVLTREE_DEBUG1("LESS: double LR\n");
        p2= p1->right;
        p1->right= p2->left;
        p2->left= p1;

        (*node_pp)->left= p2->right;
        p2->right= *node_pp;

        if(p2->balance == -1)
          (*node_pp)->balance= 1;
        else
          (*node_pp)->balance= 0;

        if(p2->balance == 1)
          p1->balance= -1;
        else
          p1->balance= 0;
        *node_pp = p2;
      } /* end else */

      (*node_pp)->balance= 0;
      *rebalancing_p= FALSE;
  } /* end switch */

  return FALSE;
}


static int
raptor_avltree_sprout_right(raptor_avltree* tree,
                            raptor_avltree_node** node_pp, 
                            raptor_avltree_t p_data, int *rebalancing_p)
{
  raptor_avltree_node *p1, *p2;
  int rc;

  RAPTOR_AVLTREE_DEBUG1("MORE: raptor_avltree_sprouting to the right\n");
  rc=raptor_avltree_sprout(tree, &(*node_pp)->right, p_data, rebalancing_p);
  if(rc)
    return rc;

  if(!*rebalancing_p)
    return FALSE;
  
  /* right branch has grown longer */
  RAPTOR_AVLTREE_DEBUG1("MORE: right branch has grown\n");
  
  switch((*node_pp)->balance) {
    case -1:
      RAPTOR_AVLTREE_DEBUG1("MORE: balance was off, fixed implicitly\n");
      (*node_pp)->balance= 0;
      *rebalancing_p= FALSE;
      break;
      
    case 0:
      RAPTOR_AVLTREE_DEBUG1("MORE: balance was okay, now off but ok\n");
      (*node_pp)->balance= 1;
      break;
      
    case 1:
      RAPTOR_AVLTREE_DEBUG1("MORE: balance was off, need to rebalance\n");
      p1= (*node_pp)->right;
      
      if(p1->balance == 1) {
        /* RR */
        RAPTOR_AVLTREE_DEBUG1("MORE: single RR\n");
        (*node_pp)->right= p1->left;
        p1->left= *node_pp;
        (*node_pp)->balance= 0;
        *node_pp= p1;
      } else {
        /* double RL */
        RAPTOR_AVLTREE_DEBUG1("MORE: double RL\n");
        
        p2= p1->left;
        p1->left= p2->right;
        p2->right= p1;
        
        (*node_pp)->right= p2->left;
        p2->left= *node_pp;
        
        if(p2->balance == 1)
          (*node_pp)->balance= -1;
        else
          (*node_pp)->balance= 0;
        
        if(p2->balance == -1)
          p1->balance= 1;
        else
          p1->balance= 0;
        
        *node_pp= p2;
      } /* end else */
      
      (*node_pp)->balance= 0;
      *rebalancing_p= FALSE;
  } /* end switch */

  return FALSE;
}


static int
raptor_avltree_sprout(raptor_avltree* tree, raptor_avltree_node** node_pp,
                      raptor_avltree_t p_data, int *rebalancing_p)
{
  int cmp;

  RAPTOR_AVLTREE_DEBUG1("raptor_avltree_sprout\n");

  /* If grounded, add the node here, set the rebalance flag and return */
  if(!*node_pp) {
    RAPTOR_AVLTREE_DEBUG1("grounded. adding new node, setting rebalancing flag true\n");
    *node_pp= (raptor_avltree_node*)malloc(sizeof(**node_pp));
    if(!*node_pp)
      return TRUE;
    
    (*node_pp)->left= NULL;
    (*node_pp)->right= NULL;
    (*node_pp)->balance= 0;
    (*node_pp)->data= p_data;
    *rebalancing_p= TRUE;

    return FALSE;
  }

  /* compare the data */
  cmp= tree->compare_fn(p_data, (*node_pp)->data);
  if(cmp < 0)
    /* if LESS, prepare to move to the left. */
    return raptor_avltree_sprout_left(tree, node_pp, p_data, rebalancing_p);
  else if(cmp > 0)
    /* if MORE, prepare to move to the right. */
    return raptor_avltree_sprout_right(tree, node_pp, p_data, rebalancing_p);

  /* otherwise same key - replace */
  *rebalancing_p= FALSE;

  if(tree->delete_fn)
    tree->delete_fn((*node_pp)->data);
  (*node_pp)->data= p_data;

  return FALSE;
}


static int
raptor_avltree_delete_internal(raptor_avltree* tree,
                               raptor_avltree_node** node_pp,
                               raptor_avltree_t p_data,
                               int *rebalancing_p, int *delete_called_p)
{
  int cmp;
  int rc=FALSE;

  RAPTOR_AVLTREE_DEBUG1("delete\n");

  if(*node_pp == NULL) {
    RAPTOR_AVLTREE_DEBUG1("key not in tree\n");
    return FALSE;
  }

  cmp= tree->compare_fn((*node_pp)->data, p_data);

  if(cmp > 0) {
    RAPTOR_AVLTREE_DEBUG1("too high - scan left\n");
    rc= raptor_avltree_delete_internal(tree, &(*node_pp)->left, p_data,
                                       rebalancing_p, delete_called_p);
    if(*rebalancing_p)
      raptor_avltree_balance_left(tree, node_pp, rebalancing_p);

  } else if(cmp < 0) {
    RAPTOR_AVLTREE_DEBUG1("too low - scan right\n");
    rc= raptor_avltree_delete_internal(tree, &(*node_pp)->right, p_data,
                                       rebalancing_p, delete_called_p);
    if(*rebalancing_p)
      raptor_avltree_balance_right(tree, node_pp, rebalancing_p);

  } else {
    raptor_avltree_node *pr_q;

    RAPTOR_AVLTREE_DEBUG1("equal\n");
    pr_q= *node_pp;

    if(pr_q->right == NULL) {
      RAPTOR_AVLTREE_DEBUG1("right subtree null\n");
      *node_pp= pr_q->left;
      *rebalancing_p= TRUE;
    } else if(pr_q->left == NULL) {
      RAPTOR_AVLTREE_DEBUG1("right subtree non-null, left subtree null\n");
      *node_pp= pr_q->right;
      *rebalancing_p= TRUE;
    } else {
      RAPTOR_AVLTREE_DEBUG1("neither subtree null\n");
      raptor_avltree_delete_internal2(tree, &pr_q->left, rebalancing_p,
                                      &pr_q, delete_called_p);
      if(*rebalancing_p)
	raptor_avltree_balance_left(tree, node_pp, rebalancing_p);
    }

    if(!*delete_called_p && tree->delete_fn)
      tree->delete_fn(pr_q->data);

    free(pr_q);
    rc= TRUE;
  }

  return rc;
}


static void
raptor_avltree_delete_internal2(raptor_avltree* tree,
                                raptor_avltree_node** ppr_r,
                                int *rebalancing_p,
                                raptor_avltree_node** ppr_q,
                                int *delete_called_p)
{
  RAPTOR_AVLTREE_DEBUG1("del\n");

  if((*ppr_r)->right != NULL) {
    raptor_avltree_delete_internal2(tree,
                                    &(*ppr_r)->right, 
                                    rebalancing_p,
                                    ppr_q,
                                    delete_called_p);
    if(*rebalancing_p)
      raptor_avltree_balance_right(tree, ppr_r, rebalancing_p);

  } else {
    if(tree->delete_fn)
      tree->delete_fn((*ppr_q)->data);

    *delete_called_p= TRUE;
    (*ppr_q)->data= (*ppr_r)->data;
    *ppr_q= *ppr_r;
    *ppr_r= (*ppr_r)->left;
    *rebalancing_p= TRUE;
  }
}


static void
raptor_avltree_balance_left(raptor_avltree* tree,
                            raptor_avltree_node** node_pp, int *rebalancing_p)
{
  raptor_avltree_node *p1, *p2;
  int b1, b2;

  RAPTOR_AVLTREE_DEBUG1("left branch has shrunk\n");

  switch((*node_pp)->balance) {
    case -1:
      RAPTOR_AVLTREE_DEBUG1("was imbalanced, fixed implicitly\n");
      (*node_pp)->balance= 0;
      break;

    case 0:
      RAPTOR_AVLTREE_DEBUG1("was okay, is now one off\n");
      (*node_pp)->balance= 1;
      *rebalancing_p= FALSE;
      break;

    case 1:
      RAPTOR_AVLTREE_DEBUG1("was already off, this is too much\n");
      p1= (*node_pp)->right;
      b1= p1->balance;

      if(b1 >= 0) {
	RAPTOR_AVLTREE_DEBUG1("single RR\n");
        (*node_pp)->right= p1->left;
	p1->left= *node_pp;
	if(b1 == 0) {
	  RAPTOR_AVLTREE_DEBUG1("b1 == 0\n");
          (*node_pp)->balance= 1;
	  p1->balance= -1;
	  *rebalancing_p= FALSE;
	} else {
	  RAPTOR_AVLTREE_DEBUG1("b1 != 0\n");
          (*node_pp)->balance= 0;
	  p1->balance= 0;
	}
	*node_pp= p1;
      } else {
	RAPTOR_AVLTREE_DEBUG1("double RL\n");
        p2= p1->left;
	b2= p2->balance;
	p1->left= p2->right;
	p2->right= p1;
	(*node_pp)->right= p2->left;
	p2->left= *node_pp;
	if(b2 == 1)
	  (*node_pp)->balance= -1;
	else
	  (*node_pp)->balance= 0;
	if(b2 == -1)
	  p1->balance= 1;
	else
	  p1->balance= 0;
	*node_pp= p2;
	p2->balance= 0;
      }
      break;
  } /* end switch */
}


static void
raptor_avltree_balance_right(raptor_avltree* tree,
                             raptor_avltree_node** node_pp, int *rebalancing_p)
{
  raptor_avltree_node *p1, *p2;
  int b1, b2;

  RAPTOR_AVLTREE_DEBUG1("right branch has shrunk\n");

  switch((*node_pp)->balance) {
    case 1:
      RAPTOR_AVLTREE_DEBUG1("was imbalanced, fixed implicitly\n");
      (*node_pp)->balance= 0;
      break;

    case 0:
      RAPTOR_AVLTREE_DEBUG1("was okay, is now one off\n");
      (*node_pp)->balance= -1;
      *rebalancing_p= FALSE;
      break;

    case -1:
      RAPTOR_AVLTREE_DEBUG1("was already off, this is too much\n");
      p1= (*node_pp)->left;
      b1= p1->balance;

      if(b1 <= 0) {
	RAPTOR_AVLTREE_DEBUG1("single LL\n");
        (*node_pp)->left= p1->right;
	p1->right= *node_pp;
	if(b1 == 0) {
	  RAPTOR_AVLTREE_DEBUG1("b1 == 0\n");
          (*node_pp)->balance= -1;
	  p1->balance= 1;
	  *rebalancing_p= FALSE;
	} else {
	  RAPTOR_AVLTREE_DEBUG1("b1 != 0\n");
          (*node_pp)->balance= 0;
	  p1->balance= 0;
	}
	*node_pp= p1;
      } else {
	RAPTOR_AVLTREE_DEBUG1("double LR\n");
        p2= p1->right;
	b2= p2->balance;
	p1->right= p2->left;
	p2->left= p1;
	(*node_pp)->left= p2->right;
	p2->right= *node_pp;
	if(b2 == -1)
	  (*node_pp)->balance= 1;
	else
	  (*node_pp)->balance= 0;
	if(b2 == 1)
	  p1->balance= -1;
	else
	  p1->balance= 0;
	*node_pp= p2;
	p2->balance= 0;
      }
  } /* end switch */
}

#endif


#ifdef STANDALONE

#include <string.h>

typedef struct 
{
  FILE *fh;
  int count;
  const char** results;
  int failed;
} visit_state;
  
#if RAPTOR_DEBUG > 1
static int
print_string(int depth, raptor_avltree_t data, void *user_data) 
{
  visit_state* vs=(visit_state*)user_data;
  
  fprintf(vs->fh, "%3d: %s\n", vs->count, (char*) data);
  vs->count++;
  return 1;
}
#endif

static int
check_string(int depth, raptor_avltree_t data, void *user_data) 
{
  visit_state* vs=(visit_state*)user_data;
  const char* result=vs->results[vs->count];
  
  if(strcmp(data, result)) {
    fprintf(vs->fh, "%3d: Expected '%s' but found '%s'\n", vs->count,
            result, (char*)data);
    vs->failed=1;
  }
  vs->count++;
  
  return 1;
}

static int
compare_strings(void *l, void *r)
{
  return strcmp((const char*)l, (const char*)r);
}


/* one more prototype */
int main(int argc, char *argv[]);

int
main(int argc, char *argv[])
{
  const char *program=raptor_basename(argv[0]);
  const char *items[8] = { "ron", "amy", "jen", "bij", "jib", "daj", "jim", NULL };
  const char *delete_items[3] = { "jen", "jim", NULL };
  const char *results[8] = { "amy", "bij", "daj", "jib", "ron", NULL};

  raptor_avltree* tree;
  visit_state vs;
  int i;
  
  tree=raptor_new_avltree(compare_strings,
                          NULL, /* no free as they are static pointers above */
                          0);
  if(!tree) {
    fprintf(stderr, "%s: Failed to create tree\n", program);
    exit(1);
  }
  for(i=0; items[i]; i++) {
    int rc;

#if RAPTOR_DEBUG > 1
    fprintf(stderr, "%s: Adding tree item '%s'\n", program, items[i]);
#endif
  
    rc=raptor_avltree_add(tree, (void*)items[i]);
    if(rc) {
      fprintf(stderr,
              "%s: Adding tree item %d '%s' failed, returning error %d\n",
              program, i, items[i], rc);
      exit(1);
    }
  }

#if RAPTOR_DEBUG > 1
  fprintf(stderr, "%s: Printing tree\n", program);
  vs.fh=stderr;
  vs.count=0;
  raptor_avltree_visit(tree, print_string, &vs);
#endif

  for(i=0; delete_items[i]; i++) {
    int rc;

#if RAPTOR_DEBUG > 1
    fprintf(stderr, "%s: Deleting tree item '%s'\n", program, delete_items[i]);
#endif
  
    rc=raptor_avltree_delete(tree, (void*)delete_items[i]);
    if(!rc) {
      fprintf(stderr,
              "%s: Deleting tree item %d '%s' failed, returning error %d\n",
              program, i, delete_items[i], rc);
      exit(1);
    }
  }

#if RAPTOR_DEBUG > 1
  fprintf(stderr, "%s: Checking tree\n", program);
#endif
  vs.count=0;
  vs.results=results;
  vs.failed=0;
  raptor_avltree_visit(tree, check_string, &vs);
  if(vs.failed) {
    fprintf(stderr, "%s: Checking tree failed\n", program);
    exit(1);
  }

#if RAPTOR_DEBUG > 1
  fprintf(stderr, "%s: Freeing tree\n", program);
#endif
  raptor_free_avltree(tree);

  /* keep gcc -Wall happy */
  return(0);
}

#endif
