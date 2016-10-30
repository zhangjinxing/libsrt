/*
 * smap.c
 *
 * Map handling.
 *
 * Copyright (c) 2015-2016, F. Aragon. All rights reserved. Released under
 * the BSD 3-Clause License (see the doc/LICENSE file included).
 */ 

#include "smap.h"
#include "saux/scommon.h"

/*
 * Internal functions
 */

S_INLINE int cmp_ni_i(const struct SMapii *a, int32_t b)
{
	return a->k > b ? 1 : a->k < b ? -1 : 0;
}

static int cmp_i(const struct SMapii *a, const struct SMapii *b)
{
	return a->k > b->k ? 1 : a->k < b->k ? -1 : 0;
}

S_INLINE int cmp_nu_u(const struct SMapuu *a, uint32_t b)
{
	return a->k > b ? 1 : a->k < b ? -1 : 0;
}

static int cmp_u(const struct SMapuu *a, const struct SMapuu *b)
{
	return a->k > b->k ? 1 : a->k < b->k ? -1 : 0;
}

S_INLINE int cmp_nI_I(const struct SMapIx *a, uint64_t b)
{
	return a->k > b ? 1 : a->k < b ? -1 : 0;
}

static int cmp_I(const struct SMapIx *a, const struct SMapIx *b)
{
	return a->k > b->k ? 1 : a->k < b->k ? -1 : 0;
}

S_INLINE int cmp_ns_s(const struct SMapSx *a, const ss_t *b)
{
	return ss_cmp(a->k, b);
}

static int cmp_s(const struct SMapSx *a, const struct SMapSx *b)
{
	return ss_cmp(a->k, b->k);
}

static void rw_inc_SM_II32(stn_t *node, const stn_t *new_data,
			   const sbool_t existing)
{
	if (existing)
		((struct SMapii *)node)->v += ((const struct SMapii *)new_data)->v;
}

static void rw_inc_SM_UU32(stn_t *node, const stn_t *new_data,
			   const sbool_t existing)
{
	if (existing)
		((struct SMapuu *)node)->v += ((const struct SMapuu *)new_data)->v;
}

static void rw_inc_SM_II(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	if (existing)
		((struct SMapII *)node)->v += ((const struct SMapII *)new_data)->v;
}

static void rw_add_SM_SP(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	struct SMapSP *n = (struct SMapSP *)node;
	const struct SMapSP *m = (const struct SMapSP *)new_data;
	if (!existing)
		n->x.k = NULL;
	ss_cpy(&n->x.k, m->x.k);
}

static void rw_add_SM_SS(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	struct SMapSS *n = (struct SMapSS *)node;
	const struct SMapSS *m = (const struct SMapSS *)new_data;
	if (!existing)
		n->x.k = n->v = NULL;
	ss_cpy(&n->x.k, m->x.k);
	ss_cpy(&n->v, m->v);
}

static void rw_add_SM_SI(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	struct SMapSI *n = (struct SMapSI *)node;
	const struct SMapSI *m = (const struct SMapSI *)new_data;
	if (!existing)
		n->x.k = NULL;
	ss_cpy(&n->x.k, m->x.k);
}

static void rw_inc_SM_SI(stn_t *node, const stn_t *new_data,
			 const sbool_t existing)
{
	if (!existing)
		rw_add_SM_SI(node, new_data, existing);
	else
		((struct SMapSI *)node)->v += ((const struct SMapSI *)new_data)->v;
}

static void aux_is_delete(void *node)
{
	ss_free(&((struct SMapIS *)node)->v);
}

static void aux_si_delete(void *node)
{
	ss_free(&((struct SMapSI *)node)->x.k);
}

static void aux_ss_delete(void *node)
{
	ss_free(&((struct SMapSS *)node)->x.k, &((struct SMapSS *)node)->v);
}

static void aux_sp_delete(void *node)
{
	ss_free(&((struct SMapSP *)node)->x.k);
}

struct SV2X { sv_t *kv, *vv; };

static int aux_ii32_sort(struct STraverseParams *tp)
{
	const struct SMapii *cn = (const struct SMapii *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_uu32_sort(struct STraverseParams *tp)
{
	const struct SMapuu *cn = (const struct SMapuu *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_u(&v2x->kv, cn->k);
		sv_push_u(&v2x->vv, cn->v);
	}
	return 0;
}
static int aux_ii_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn = (const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_is_ip_sort(struct STraverseParams *tp)
{
	const struct SMapIP *cn = (const struct SMapIP *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push_i(&v2x->kv, cn->x.k);
		sv_push(&v2x->vv, &cn->v);
	}
	return 0;
}

static int aux_si_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn = (const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, &cn->x.k);
		sv_push_i(&v2x->vv, cn->v);
	}
	return 0;
}

static int aux_sp_ss_sort(struct STraverseParams *tp)
{
	const struct SMapII *cn = (const struct SMapII *)get_node_r(tp->t, tp->c);
	if (cn) {
		struct SV2X *v2x = (struct SV2X *)tp->context;
		sv_push(&v2x->kv, &cn->x.k);
		sv_push(&v2x->vv, &cn->v);
	}
	return 0;
}

static st_cmp_t type2cmpf(const enum eSM_Type t)
{
	switch (t) {
	case SM_UU32:
		return (st_cmp_t)cmp_u;
	case SM_II32:
		return (st_cmp_t)cmp_i;
	case SM_II: case SM_IS: case SM_IP:
		return (st_cmp_t)cmp_I;
	case SM_SI: case SM_SS: case SM_SP:
		return (st_cmp_t)cmp_s;
	default:
		break;
	}
	return NULL;
}

#define SM_ENUM_INORDER_XX(FN, CALLBACK_T, MAP_TYPE, KEY_T, TR_CMP_MIN,	     \
			   TR_CMP_MAX,TR_CALLBACK)			     \
	size_t FN(const sm_t *m, KEY_T kmin, KEY_T kmax, CALLBACK_T f,	     \
		  void *context)					     \
	{								     \
		RETURN_IF(!m, 0); /* null tree */			     \
		RETURN_IF(m->d.sub_type != MAP_TYPE, 0); /* wrong type */    \
		const size_t ts = sm_size(m);				     \
		RETURN_IF(!ts, S_FALSE); /* empty tree */		     \
		ssize_t level = 0;					     \
		size_t nelems = 0, rbt_max_depth = 2 * (slog2(ts) + 1);	     \
		struct STreeScan *p = (struct STreeScan *)		     \
					alloca(sizeof(struct STreeScan) *    \
						 (rbt_max_depth + 3));	     \
		ASSERT_RETURN_IF(!p, 0); /* BEHAVIOR: stack error */	     \
		p[0].p = ST_NIL;					     \
		p[0].c = m->root;					     \
		p[0].s = STS_ScanStart;					     \
		const stn_t *cn;					     \
		int cmpmin, cmpmax;					     \
		while (level >= 0) {					     \
			S_ASSERT(level <= (ssize_t)rbt_max_depth);	     \
			switch (p[level].s) {				     \
			case STS_ScanStart:				     \
				cn = get_node_r(m, p[level].c);		     \
				cmpmin = TR_CMP_MIN;			     \
				cmpmax = TR_CMP_MAX;			     \
				if (cn->x.l != ST_NIL && cmpmin > 0) {	     \
					p[level].s = STS_ScanLeft;	     \
					level++;			     \
					cn = get_node_r(m, p[level - 1].c);  \
					p[level].c = cn->x.l;		     \
				} else {				     \
					/* node with null left children */   \
					if (cmpmin >= 0 && cmpmax <= 0) {    \
						if (f && !TR_CALLBACK)	     \
							return nelems;	     \
						nelems++;		     \
					}				     \
					if (cn->r != ST_NIL && cmpmax < 0) { \
						p[level].s = STS_ScanRight;  \
						level++;		     \
						cn = get_node_r(m,	     \
							  p[level - 1].c);   \
						p[level].c = cn->r;	     \
					} else {			     \
						p[level].s = STS_ScanDone;   \
						level--;		     \
						continue;		     \
					}				     \
				}					     \
				p[level].p = p[level - 1].c;		     \
				p[level].s = STS_ScanStart;		     \
				continue;				     \
			case STS_ScanLeft:				     \
				cn = get_node_r(m, p[level].c);		     \
				cmpmin = TR_CMP_MIN;			     \
				cmpmax = TR_CMP_MAX;			     \
				if (cmpmin >= 0 && cmpmax <= 0) {	     \
					if (f && !TR_CALLBACK)		     \
						return nelems;		     \
					nelems++;			     \
				}					     \
				if (cn->r != ST_NIL && cmpmax < 0) {	     \
					p[level].s = STS_ScanRight;	     \
					level++;			     \
					p[level].p = p[level - 1].c;	     \
					cn = get_node_r(m, p[level - 1].c);  \
					p[level].c = cn->r;		     \
					p[level].s = STS_ScanStart;	     \
				} else {				     \
					p[level].s = STS_ScanDone;	     \
					level--;			     \
					continue;			     \
				}					     \
				continue;				     \
			case STS_ScanRight:				     \
				/* don't break */			     \
			default:					     \
				p[level].s = STS_ScanDone;		     \
				level--;				     \
				continue;				     \
			}						     \
		}							     \
		return nelems;						     \
	}

SM_ENUM_INORDER_XX(sm_itr_ii32, sm_it_ii32_t, SM_II32, int32_t,
		   cmp_ni_i((const struct SMapii *)cn, kmin),
		   cmp_ni_i((const struct SMapii *)cn, kmax),
		   f(((const struct SMapii *)cn)->k,
		     ((const struct SMapii *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_uu32, sm_it_uu32_t, SM_UU32, uint32_t,
		   cmp_nu_u((const struct SMapuu *)cn, kmin),
		   cmp_nu_u((const struct SMapuu *)cn, kmax),
		   f(((const struct SMapuu *)cn)->k,
		     ((const struct SMapuu *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ii, sm_it_ii_t, SM_II, int64_t,
		   cmp_nI_I((const struct SMapIx *)cn, kmin),
		   cmp_nI_I((const struct SMapIx *)cn, kmax),
		   f(((const struct SMapIx *)cn)->k,
		     ((const struct SMapII *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_is, sm_it_is_t, SM_IS, int64_t,
		   cmp_nI_I((const struct SMapIx *)cn, kmin),
		   cmp_nI_I((const struct SMapIx *)cn, kmax),
		   f(((const struct SMapIx *)cn)->k,
		     ((const struct SMapIS *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ip, sm_it_ip_t, SM_IP, int64_t,
		   cmp_nI_I((const struct SMapIx *)cn, kmin),
		   cmp_nI_I((const struct SMapIx *)cn, kmax),
		   f(((const struct SMapIx *)cn)->k,
		     ((const struct SMapIP *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_si, sm_it_si_t, SM_SI, const ss_t *,
		   cmp_ns_s((const struct SMapSx *)cn, kmin),
		   cmp_ns_s((const struct SMapSx *)cn, kmax),
		   f(((const struct SMapSx *)cn)->k,
		     ((const struct SMapSI *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_ss, sm_it_ss_t, SM_SS, const ss_t *,
		   cmp_ns_s((const struct SMapSx *)cn, kmin),
		   cmp_ns_s((const struct SMapSx *)cn, kmax),
		   f(((const struct SMapSx *)cn)->k,
		     ((const struct SMapSS *)cn)->v, context))

SM_ENUM_INORDER_XX(sm_itr_sp, sm_it_sp_t, SM_SP, const ss_t *,
		   cmp_ns_s((const struct SMapSx *)cn, kmin),
		   cmp_ns_s((const struct SMapSx *)cn, kmax),
		   f(((const struct SMapSx *)cn)->k,
		     ((const struct SMapSP *)cn)->v, context))

/*
 * Allocation
 */

sm_t *sm_alloc_raw(const enum eSM_Type t, const sbool_t ext_buf, void *buffer,
		   const size_t elem_size, const size_t max_size)
{
	RETURN_IF(!buffer || !max_size, NULL);
	sm_t *m = (sm_t *)st_alloc_raw(type2cmpf(t), ext_buf, buffer, elem_size,
				       max_size);
	m->d.sub_type = t;
	return m;
}

sm_t *sm_alloc(const enum eSM_Type t, const size_t init_size)
{
	sm_t *m = (sm_t *)st_alloc(type2cmpf(t), sm_elem_size(t), init_size);
	m->d.sub_type = t;
	return m;
}

void sm_free_aux(sm_t **m, ...)
{
	va_list ap;
	va_start(ap, m);
	sm_t **next = m;
	while (!s_varg_tail_ptr_tag(next)) { /* last element tag */
		if (next) {
			sm_clear(*next); /* release associated dynamic memory */
			sd_free((sd_t **)next);
		}
		next = (sm_t **)va_arg(ap, sm_t **);
	}
	va_end(ap);
}

sm_t *sm_dup(const sm_t *src)
{
	sm_t *m = NULL;
	return sm_cpy(&m, src);
}

void sm_clear(sm_t *m)
{
	if (!m || !m->d.size)
		return;
	stn_callback_t delete_callback = NULL;
	switch (m->d.sub_type) {
	case SM_IS: delete_callback = aux_is_delete; break;
	case SM_SI: delete_callback = aux_si_delete; break;
	case SM_SS: delete_callback = aux_ss_delete; break;
	case SM_SP: delete_callback = aux_sp_delete; break;
	}
	if (delete_callback) {	/* deletion of dynamic memory elems */
		stndx_t i = 0;
		for (; i < (stndx_t)m->d.size; i++) {
			stn_t *n = st_enum(m, i);
			delete_callback(n);
		}
	}
	st_set_size((st_t *)m, 0);
}

/*
 * Copy
 */

sm_t *sm_cpy(sm_t **m, const sm_t *src)
{
	RETURN_IF(!m || !src, NULL); /* BEHAVIOR */
	const enum eSM_Type t = (enum eSM_Type)src->d.sub_type;
	size_t ss = sm_size(src),
	       src_buf_size = src->d.elem_size * src->d.size;
	RETURN_IF(ss > ST_NDX_MAX, NULL); /* BEHAVIOR */
	if (*m) {
		if (src->d.f.ext_buffer)
		{	/* If using ext buffer, we'll have grow limits */
			sm_clear(*m);
			*m = sm_alloc_raw(t, S_TRUE, *m, (*m)->d.elem_size,
					  (*m)->d.max_size);
		} else {
			st_reserve(m, ss);
		}
	}
	if (!*m)
		*m = sm_alloc(t, ss);
	RETURN_IF(!*m || st_max_size(*m) < ss, NULL); /* BEHAVIOR */
	/*
	 * Bulk tree copy: tree structure can be copied as is, because of
	 * of using indexes instead of pointers.
	 */
	memcpy(sm_get_buffer(*m), sm_get_buffer_r(src), src_buf_size);
	sm_set_size(*m, ss);
	(*m)->root = src->root;
	/*
	 * Copy elements using external dynamic memory (string data)
	 */
	stndx_t i;
	switch (t) {
	case SM_IS:
		for (i = 0; i < ss; i++) {
			const struct SMapIS *ms = (const struct SMapIS *)st_enum_r(src, i);
			struct SMapIS *mt = (struct SMapIS *)st_enum(*m, i);
			mt->v = ss_dup(ms->v);
		}
		break;
	case SM_SI:
	case SM_SP:
		for (i = 0; i < ss; i++) {
			const struct SMapSx *ms = (const struct SMapSx *)st_enum_r(src, i);
			struct SMapSx *mt = (struct SMapSx *)st_enum(*m, i);
			mt->k = ss_dup(ms->k);
		}
		break;
	case SM_SS:
		for (i = 0; i < ss; i++) {
			const struct SMapSS *ms = (const struct SMapSS *)st_enum_r(src, i);
			struct SMapSS *mt = (struct SMapSS *)st_enum(*m, i);
			mt->x.k = ss_dup(ms->x.k);
			mt->v = ss_dup(ms->v);
		}
		break;
	case SM_II32: case SM_UU32: case SM_II: case SM_IP:
	default: /* no additional action required */
		break;
	}
	return *m;
}

/*
 * Random access
 */

int32_t sm_at_ii32(const sm_t *m, const int32_t k)
{
	ASSERT_RETURN_IF(!m, SINT32_MIN);
	struct SMapii n;
	n.k = k;
	const struct SMapii *nr =
			(const struct SMapii *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

uint32_t sm_at_uu32(const sm_t *m, const uint32_t k)
{
	ASSERT_RETURN_IF(!m, 0);
	struct SMapuu n;
	n.k = k;
	const struct SMapuu *nr =
			(const struct SMapuu *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

int64_t sm_at_ii(const sm_t *m, const int64_t k)
{
	ASSERT_RETURN_IF(!m, SINT64_MAX);
	struct SMapII n;
	n.x.k = k;
	const struct SMapII *nr =
			(const struct SMapII *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

const ss_t *sm_at_is(const sm_t *m, const int64_t k)
{
	ASSERT_RETURN_IF(!m, ss_void);
	struct SMapIS n;
	n.x.k = k;
	const struct SMapIS *nr =
			(const struct SMapIS *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

const void *sm_at_ip(const sm_t *m, const int64_t k)
{
	ASSERT_RETURN_IF(!m, NULL);
	struct SMapIP n;
	n.x.k = k;
	const struct SMapIP *nr =
			(const struct SMapIP *)st_locate(m, (const stn_t *)&n);
	return nr ? nr->v : NULL;
}

int64_t sm_at_si(const sm_t *m, const ss_t *k)
{
	ASSERT_RETURN_IF(!m, SINT64_MIN);
	struct SMapSI n;
	n.x.k = (ss_t *)k;	/* not going to be overwritten */
	const struct SMapSI *nr =
			(const struct SMapSI *)st_locate(m, &n.x.n);
	return nr ? nr->v : 0; /* BEHAVIOR */
}

const ss_t *sm_at_ss(const sm_t *m, const ss_t *k)
{
	ASSERT_RETURN_IF(!m, ss_void);
	struct SMapSS n;
	n.x.k = (ss_t *)k;	/* not going to be overwritten */
	const struct SMapSS *nr =
			(const struct SMapSS *)st_locate(m, &n.x.n);
	return nr ? nr->v : ss_void;
}

const void *sm_at_sp(const sm_t *m, const ss_t *k)
{
	ASSERT_RETURN_IF(!m, NULL);
	struct SMapSP n;
	n.x.k = (ss_t *)k;	/* not going to be overwritten */
	const struct SMapSP *nr =
			(const struct SMapSP *)st_locate(m, &n.x.n);
	return nr ? nr->v : NULL;
}

/*
 * Existence check
 */

sbool_t sm_count_u(const sm_t *m, const uint32_t k)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapuu n;
	n.k = k;
	return st_locate(m, (const stn_t *)&n) ? S_TRUE : S_FALSE;
}

sbool_t sm_count_i(const sm_t *m, const int64_t k)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapIx n;
	n.k = k;
	return st_locate(m, (const stn_t *)&n) ? S_TRUE : S_FALSE;
}

sbool_t sm_count_s(const sm_t *m, const ss_t *k)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapSX n;
	n.k = k;
	return st_locate(m, (const stn_t *)&n) ? S_TRUE : S_FALSE;
}

/*
 * Insert
 */

S_INLINE sbool_t sm_insert_ii32_aux(sm_t **m, const int32_t k,
				    const int32_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapii n;
	n.k = k;
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
}

sbool_t sm_insert_ii32(sm_t **m, const int32_t k, const int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, NULL);
}

sbool_t sm_inc_ii32(sm_t **m, const int32_t k, const int32_t v)
{
	return sm_insert_ii32_aux(m, k, v, rw_inc_SM_II32);
}

S_INLINE sbool_t sm_insert_uu32_aux(sm_t **m, const uint32_t k,
				    const uint32_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapuu n;
	n.k = k;
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
}

sbool_t sm_insert_uu32(sm_t **m, const uint32_t k, const uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, NULL);
}

sbool_t sm_inc_uu32(sm_t **m, const uint32_t k, const uint32_t v)
{
	return sm_insert_uu32_aux(m, k, v, rw_inc_SM_UU32);
}

S_INLINE sbool_t sm_insert_ii_aux(sm_t **m, const int64_t k,
			          const int64_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapII n;
	n.x.k = k;
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
}

sbool_t sm_insert_ii(sm_t **m, const int64_t k, const int64_t v)
{
	return sm_insert_ii_aux(m, k, v, NULL);
}

sbool_t sm_inc_ii(sm_t **m, const int64_t k, const int64_t v)
{
	return sm_insert_ii_aux(m, k, v, rw_inc_SM_II);
}

sbool_t sm_insert_is(sm_t **m, const int64_t k, const ss_t *v)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapIS n;
	n.x.k = k;
	n.v = ss_dup(v);
	return st_insert((st_t **)m, (const stn_t *)&n);
}

sbool_t sm_insert_ip(sm_t **m, const int64_t k, const void *v)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapIP n;
	n.x.k = k;
	n.v = v;
	return st_insert((st_t **)m, (const stn_t *)&n);
}

S_INLINE sbool_t sm_insert_si_aux(sm_t **m, const ss_t *k,
				  const int64_t v, const st_rewrite_t rw_f)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapSI n;
	n.x.k = (ss_t *)k;
	n.v = v;
	sbool_t r = st_insert_rw((st_t **)m, (const stn_t *)&n, rw_f);
	return r;
}

sbool_t sm_insert_si(sm_t **m, const ss_t *k, const int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_add_SM_SI);
}

sbool_t sm_inc_si(sm_t **m, const ss_t *k, const int64_t v)
{
	return sm_insert_si_aux(m, k, v, rw_inc_SM_SI);
}

sbool_t sm_insert_ss(sm_t **m, const ss_t *k, const ss_t *v)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapSS n;
	n.x.k = (ss_t *)k;
	n.v = (ss_t *)v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_add_SM_SS);
}

sbool_t sm_insert_sp(sm_t **m, const ss_t *k, const void *v)
{
	ASSERT_RETURN_IF(!m, S_FALSE);
	struct SMapSP n;
	n.x.k = (ss_t *)k;
	n.v = v;
	return st_insert_rw((st_t **)m, (const stn_t *)&n, rw_add_SM_SP);
}

/*
 * Delete
 */

sbool_t sm_delete_i(sm_t *m, const int64_t k)
{
	struct SMapIx ix;
	struct SMapii ii;
	struct SMapuu uu;
	const stn_t *n;
	stn_callback_t callback = NULL;
	switch (m->d.sub_type) {
	case SM_II32:
		RETURN_IF(k > SINT32_MAX || k < SINT32_MIN, S_FALSE);
		ii.k = (int32_t)k;
		n = (const stn_t *)&ii;
		break;
	case SM_UU32:
		RETURN_IF(k > SUINT32_MAX, S_FALSE);
		uu.k = (uint32_t)k;
		n = (const stn_t *)&uu;
		break;
	case SM_IS:
		callback = aux_is_delete;
		/* don't break */
	case SM_II: case SM_IP:
		ix.k = k;
		n = (const stn_t *)&ix;
		break;
	default:
		return S_FALSE;
	}
	return st_delete(m, n, callback);
}

sbool_t sm_delete_s(sm_t *m, const ss_t *k)
{
	stn_callback_t callback = NULL;
	struct SMapSx sx;
	sx.k = (ss_t *)k;	/* not going to be overwritten */
	switch (m->d.sub_type) {
		case SM_SI: callback = aux_si_delete; break;
		case SM_SS: callback = aux_ss_delete; break;
		case SM_SP: callback = aux_sp_delete; break;
	}
	return st_delete(m, (const stn_t *)&sx, callback);
}

/*
 * Enumeration / export data
 */

ssize_t sm_sort_to_vectors(const sm_t *m, sv_t **kv, sv_t **vv)
{
	RETURN_IF(!kv || !vv, 0);
	struct SV2X v2x = { *kv, *vv };
	st_traverse traverse_f = NULL;
	enum eSV_Type kt, vt;
	switch (m->d.sub_type) {
	case SM_II32:
		kt = vt = SV_I32;
		break;
	case SM_UU32:
		kt = vt = SV_U32;
		break;
	case SM_II: case SM_IS: case SM_IP:
		kt = SV_I64;
		vt = m->d.sub_type == SM_II ? SV_I64 : SV_GEN;
		break;
	case SM_SI: case SM_SS: case SM_SP:
		kt = SV_GEN;
		vt = m->d.sub_type == SM_SI ? SV_I64 : SV_GEN;
		break;
	default: return 0; /* BEHAVIOR: invalid type */
	}
	if (v2x.kv) {
		if (v2x.kv->d.sub_type != (uint8_t)kt)
			sv_free(&v2x.kv);
		else
			sv_reserve(&v2x.kv, m->d.size);
	}
	if (v2x.vv) {
		if (v2x.vv->d.sub_type != (uint8_t)vt)
			sv_free(&v2x.vv);
		else
			sv_reserve(&v2x.vv, m->d.size);
	}
	if (!v2x.kv)
		v2x.kv = sv_alloc_t(kt, m->d.size);
	if (!v2x.vv)
		v2x.vv = sv_alloc_t(vt, m->d.size);
	switch (m->d.sub_type) {
	case SM_II32: traverse_f = aux_ii32_sort; break;
	case SM_UU32: traverse_f = aux_uu32_sort; break;
	case SM_II: traverse_f = aux_ii_sort; break;
	case SM_IS: case SM_IP: traverse_f = aux_is_ip_sort; break;
	case SM_SI: traverse_f = aux_si_sort; break;
	case SM_SS: case SM_SP: traverse_f = aux_sp_ss_sort; break;
	}
	ssize_t r = st_traverse_inorder((const st_t *)m, traverse_f,
					(void *)&v2x);
	*kv = v2x.kv;
	*vv = v2x.vv;
	return r;
}

