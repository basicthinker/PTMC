/* Passes for transactional memory support.
   Copyright (C) 2008 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "tree-flow.h"
#include "tree-pass.h"
#include "tree-inline.h"
#include "except.h"
#include "diagnostic.h"
#include "toplev.h"
#include "flags.h"
#include "demangle.h"
#include "output.h"
static void lower_sequence_tm (unsigned *, tree);
static void lower_sequence_no_tm (tree);
int gimple_call_flags (tree stmt);


/* Return the tree node representing the function called by call
   statement GS.  */

static inline tree
gimple_call_fn (tree gs)
{
  gcc_assert (TREE_CODE(gs) == CALL_EXPR);
  /* For 4.2 the function operand is 0 not 1 */
  return TREE_OPERAND (gs, 0);
}


/* If IN_TM_ATOMIC_P is true, GIMPLE_CALL S is within the dynamic scope of
   a GIMPLE_TM_ATOMIC transaction.  */

static inline void
gimple_call_set_in_tm_atomic (tree s, bool in_tm_atomic_p)
{
  gcc_assert (TREE_CODE(s) == CALL_EXPR);
  if (in_tm_atomic_p)
    s->common.in_tm_atomic_flag |= GF_CALL_IN_TM_ATOMIC;
  else
    s->common.in_tm_atomic_flag &= ~GF_CALL_IN_TM_ATOMIC;
}

/* If a given GIMPLE_CALL's callee is a FUNCTION_DECL, return it.
   Otherwise return NULL.  This function is analogous to
   get_callee_fndecl in tree land.  */

static inline tree
gimple_call_fndecl (tree gs)
{
  tree addr = gimple_call_fn (gs);
  if (TREE_CODE (addr) == ADDR_EXPR)
    {
      gcc_assert (TREE_CODE (TREE_OPERAND (addr, 0)) == FUNCTION_DECL);
      return TREE_OPERAND (addr, 0);
    }
  return NULL_TREE;
}

/* Detect flags from a GIMPLE_CALL.  This is just like
   call_expr_flags, but for gimple tuples.  */

int
gimple_call_flags (tree stmt)
{
  int flags;
  tree decl = gimple_call_fndecl (stmt);
  tree t;

  if (decl)
    flags = flags_from_decl_or_type (decl);
  else
    {
      t = TREE_TYPE (gimple_call_fn (stmt));
      if (t && TREE_CODE (t) == POINTER_TYPE)
        flags = flags_from_decl_or_type (TREE_TYPE (t));
      else
        flags = 0;
    }

  return flags;
}


/* Determine whether X has to be instrumented using a read
   or write barrier.  */

static bool
requires_barrier (tree x)
{
  while (handled_component_p (x))
    x = TREE_OPERAND (x, 0);

  switch (TREE_CODE (x))
    {
    case INDIRECT_REF:
      return true;

    case ALIGN_INDIRECT_REF:
    case MISALIGNED_INDIRECT_REF:
      /* ??? Insert an irrevokable when it comes to vectorized loops,
	 or handle these somehow.  */
      gcc_unreachable ();

    case TARGET_MEM_REF:
      x = TMR_SYMBOL (x);
      if (x == NULL)
	return true;
      if (TREE_CODE (x) == PARM_DECL)
	return false;
      gcc_assert (TREE_CODE (x) == VAR_DECL);
      /* FALLTHRU */

    case VAR_DECL:
      if (is_global_var (x))
	return !TREE_READONLY (x);
      /* ??? For local memory that doesn't escape, we can either save the
	 value at the beginning of the transaction and restore on restart,
	 or call a tm function to dynamically save and restore on restart.
	 We don't actually need a full barrier here.  */
      return needs_to_live_in_memory (x);

    default:
      return false;
    }
}

static void
examine_call_tm (unsigned *state, tree stmt);

/* Mark the GIMPLE_ASSIGN statement as appropriate for being inside
   a transaction region.  */

static void
examine_assign_tm (unsigned *state, tree_stmt_iterator *tsi)
{
  tree stmt = tsi_stmt (*tsi);

  /* Test rhs if it is a call */
  if( TREE_CODE (TREE_OPERAND (stmt, 1)) == CALL_EXPR)
  {
	  examine_call_tm (state, TREE_OPERAND (stmt, 1) );
  }
  /* Test barrier for rhs */
  if (requires_barrier (TREE_OPERAND (stmt, 1) ))
    *state |= GTMA_HAVE_LOAD;
  /* Test barrier for lhs */
  if (requires_barrier (TREE_OPERAND (stmt, 0) ))
    *state |= GTMA_HAVE_STORE;
}

/* Return true if X has been marked TM_PURE.  */

bool
is_tm_pure (tree x)
{
  /* check if function decl */
  if (TREE_CODE (x) != FUNCTION_DECL)
    return false;

  return FUNCTION_DECL_CHECK (x)->function_decl.tm_pure_flag;
}

/* Return true if STMT may alter control flow via a transactional edge.  */

bool
is_transactional_stmt (tree stmt)
{
  switch (TREE_CODE (stmt))
    {
    case CALL_EXPR:
      return (gimple_call_flags (stmt) & ECF_TM_OPS) != 0;
    case TM_ATOMIC_STMT:
      return true;
    default:
      return false;
    }
}

/* Return true if CALL is const, or tm_pure.  */
static bool
is_tm_pure_call (tree call)
{
  tree fn = gimple_call_fn(call);

  unsigned flags;

  if (is_tm_pure (TREE_TYPE (fn)))
    return true;

  if (TREE_CODE (fn) == ADDR_EXPR)
    {
      fn = TREE_OPERAND (fn, 0);
      gcc_assert (TREE_CODE (fn) == FUNCTION_DECL);
    }
  else
    fn = TREE_TYPE (fn);
  flags = flags_from_decl_or_type (fn);

  return (flags & ECF_CONST) != 0;
}

static void
examine_call_tm (unsigned *state, tree stmt)
{
  tree fn;

  gimple_call_set_in_tm_atomic (stmt, true);

  if (is_tm_pure_call (stmt))
	return;

  fn = gimple_call_fndecl (stmt);
  if (fn && DECL_BUILT_IN_CLASS (fn) == BUILT_IN_NORMAL
	  && DECL_FUNCTION_CODE (fn) == BUILT_IN_TM_ABORT)
	*state |= GTMA_HAVE_ABORT;

  /* Note that something may happen.  */
  *state |= GTMA_HAVE_LOAD | GTMA_HAVE_STORE;
}

/* Mark a GIMPLE_CALL as appropriate for being inside a transaction.  */
static void
examine_call_tm_it (unsigned *state, tree_stmt_iterator *tsi)
{
  tree stmt = tsi_stmt (*tsi);
  examine_call_tm(state, stmt);
}


/* Lower a GIMPLE_TM_ATOMIC statement.  The GSI is advanced.  */
static void
lower_tm_atomic (unsigned int *outer_state, tree_stmt_iterator *tsi)
{
  tree g, stmt_list, stmt = tsi_stmt (*tsi);
  tree_stmt_iterator  it;

  unsigned int this_state = 0;
  tree b;

  /* First, lower the body.  The scanning that we do inside gives
     us some idea of what we're dealing with.  */
  lower_sequence_tm (&this_state, TM_ATOMIC_BODY (stmt));

  /* If there was absolutely nothing transaction related inside the
     transaction, we may elide it.  Likewise if this is a nested
     transaction and does not contain an abort.  */
  if (this_state == 0
      || (!(this_state & GTMA_HAVE_ABORT) && outer_state != NULL))
    {
      if (outer_state)
        *outer_state |= this_state;

      tsi_link_before (tsi, TM_ATOMIC_BODY (stmt), TSI_SAME_STMT);
      TM_ATOMIC_BODY (stmt) = NULL;
      TM_ATOMIC_BLOCK (stmt) = NULL;
      tsi_delink (tsi);
      return;
    }

  /* Wrap the body of the transaction in a try-finally node so that
     the commit call is always properly called.  */
  if(TM_ATOMIC_BLOCK (stmt) != NULL && ! tsi_end_p(tsi_start(TM_ATOMIC_BLOCK (stmt))))
  {
    /* search for the insertion point */
    stmt_list = TM_ATOMIC_BLOCK (stmt);
    it = tsi_start(stmt_list);
    tsi_next(&it);
    stmt_list = tsi_stmt (it);
    stmt_list = TREE_OPERAND(stmt_list,0);
    it = tsi_start(stmt_list);
    stmt_list = tsi_stmt (it);

    append_to_statement_list(TM_ATOMIC_BODY (stmt), &TREE_OPERAND(stmt_list,0));

    TM_ATOMIC_BODY (stmt) = TM_ATOMIC_BLOCK(stmt);
    TM_ATOMIC_BLOCK (stmt) = NULL_TREE;
  }else{
    g = built_in_decls[BUILT_IN_TM_COMMIT];
    g = build_function_call_expr (g, NULL);

    b = alloc_stmt_list ();
    append_to_statement_list(g, &b);
    g = build2 (TRY_FINALLY_EXPR, void_type_node,
                TM_ATOMIC_BODY (stmt), b);
    TM_ATOMIC_BODY (stmt) = g;
  }
  /* If the transaction calls abort, add an "over" label afterwards.  */
  if (this_state & GTMA_HAVE_ABORT)
    {
      tree label = create_artificial_label ();
      TM_ATOMIC_LABEL (stmt) = label;
      tsi_link_after (tsi, build1 (LABEL_EXPR, void_type_node,label), TSI_CONTINUE_LINKING);
    }

  /* Always update the iterator.  */
  tsi_next (tsi);
}

/* Iterate through the statements in the sequence, lowering them all
   as appropriate for being in a transaction.  */

static void
lower_sequence_tm (unsigned int *state, tree seq)
{
  tree_stmt_iterator tsi;

  for (tsi = tsi_start (seq); !tsi_end_p (tsi); )
    {
      tree stmt = tsi_stmt (tsi);
      switch (TREE_CODE (stmt))
        {
        case MODIFY_EXPR:
          /* Only memory reads/writes need to be instrumented.  */
          examine_assign_tm (state, &tsi);
          break;

        case CALL_EXPR:
          examine_call_tm_it (state, &tsi);
          break;

        case ASM_EXPR:
          *state |= GTMA_HAVE_CALL_IRREVOKABLE;
          break;

        case TM_ATOMIC_STMT:
          lower_tm_atomic (state, &tsi);
          goto no_update;
        case TRY_FINALLY_EXPR:
        case TRY_CATCH_EXPR:
          lower_sequence_tm(state, TREE_OPERAND(stmt, 0));
          lower_sequence_tm(state, TREE_OPERAND(stmt, 1));
          break;
        default:
          break;
        }
      tsi_next (&tsi);

    no_update:;
    }
}


/* Iterate through the statements in the sequence, lowering them all
   as appropriate for being outside of a transaction.  */

static void
lower_sequence_no_tm (tree seq)
{
  tree_stmt_iterator tsi;

  for (tsi = tsi_start (seq); !tsi_end_p (tsi); )
  {
	  tree stmt = tsi_stmt (tsi);
	  switch (TREE_CODE (stmt))
	  {
		  case TM_ATOMIC_STMT:
			  lower_tm_atomic (NULL, &tsi);
			  break;
		  case TRY_FINALLY_EXPR:
		  case TRY_CATCH_EXPR:
			  lower_sequence_no_tm(TREE_OPERAND(stmt, 0));
			  lower_sequence_no_tm(TREE_OPERAND(stmt, 1));
			  /* fall through*/
		  default:
			  tsi_next (&tsi);
	  }
  }
}


/* Main entry point for flattening GIMPLE_TM_ATOMIC constructs.  After
   this, GIMPLE_TM_ATOMIC nodes still exist, but the nested body has
   been moved out, and all the data required for constructing a proper
   CFG has been recorded.  */

static unsigned int
execute_lower_tm (void)
{
  /* Transactional clones aren't created until a later pass.  */
  gcc_assert (!DECL_IS_TM_CLONE (current_function_decl));

  lower_sequence_no_tm (DECL_SAVED_TREE (current_function_decl));

  return 0;
}

static bool
gate_tm (void)
{
  return flag_tm;
}

struct tree_opt_pass pass_lower_tm =
{
  "tmlower",                            /* name */
  gate_tm,                              /* gate */
  execute_lower_tm,                     /* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  0,                                    /* tv_id */
  PROP_gimple_lcf,                      /* properties_required */
  0,                                    /* properties_provided */
  0,                                    /* properties_destroyed */
  0,                                    /* todo_flags_start */
  TODO_dump_func,                       /* todo_flags_finish */
  0					/* letter */
};

bool
has_tm_pure_attribute(tree decl)
{
  return lookup_attribute ("tm_pure", DECL_ATTRIBUTES (decl)) ||
    lookup_attribute ("transaction_pure", DECL_ATTRIBUTES (decl));
}

bool
has_tm_callable_attribute(tree decl)
{
  return lookup_attribute ("tm_callable", DECL_ATTRIBUTES (decl)) ||
    lookup_attribute ("transaction_callable", DECL_ATTRIBUTES (decl));
}

bool
has_tm_wrapper_attribute(tree decl)
{
  return lookup_attribute ("tm_wrapper", DECL_ATTRIBUTES (decl)) ||
    lookup_attribute ("transaction_wrap", DECL_ATTRIBUTES (decl));
}
