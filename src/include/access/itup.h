/*-------------------------------------------------------------------------
 *
 * itup.h
 *	  POSTGRES index tuple definitions.
 *
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/itup.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef ITUP_H
#define ITUP_H

#include "access/tupdesc.h"
#include "access/tupmacs.h"
#include "storage/bufpage.h"
#include "storage/itemptr.h"

/*
 * Index tuple header structure
 *
 * All index tuples start with IndexTupleData.  If the HasNulls bit is set,
 * this is followed by an IndexAttributeBitMapData.  The index attribute
 * values follow, beginning at a MAXALIGN boundary.
 *
 * Note that the space allocated for the bitmap does not vary with the number
 * of attributes; that is because we don't have room to store the number of
 * attributes in the header.  Given the MAXALIGN constraint there's no space
 * savings to be had anyway, for usual values of INDEX_MAX_KEYS.
 */

typedef struct IndexTupleData
{
	ItemPointerData t_tid;		/* reference TID to heap tuple */

	/* ---------------
	 * t_info is laid out in the following fashion:
	 *
	 * 15th (high) bit: has nulls
	 * 14th bit: has var-width attributes
	 * 13th bit: AM-defined meaning
	 * 12-0 bit: size of tuple
	 * ---------------
	 */

	unsigned short t_info;		/* various info about tuple */

} IndexTupleData;				/* MORE DATA FOLLOWS AT END OF STRUCT */

typedef IndexTupleData *IndexTuple;

typedef struct IndexAttributeBitMapData
{
	bits8		bits[(INDEX_MAX_KEYS + 8 - 1) / 8];
}			IndexAttributeBitMapData;

typedef IndexAttributeBitMapData * IndexAttributeBitMap;

/*
 * t_info manipulation macros
 */
#define INDEX_SIZE_MASK 0x1FFF
#define INDEX_AM_RESERVED_BIT 0x2000	/* reserved for index-AM specific
										 * usage */
#define INDEX_VAR_MASK	0x4000
#define INDEX_NULL_MASK 0x8000

static inline Size
IndexTupleSize(const IndexTupleData *itup)
{
	return (itup->t_info & INDEX_SIZE_MASK);
}

static inline bool
IndexTupleHasNulls(const IndexTupleData *itup)
{
	return itup->t_info & INDEX_NULL_MASK;
}

static inline bool
IndexTupleHasVarwidths(const IndexTupleData *itup)
{
	return itup->t_info & INDEX_VAR_MASK;
}


/* routines in indextuple.c */
extern IndexTuple index_form_tuple(TupleDesc tupleDescriptor,
								   const Datum *values, const bool *isnull);
extern IndexTuple index_form_tuple_context(TupleDesc tupleDescriptor,
										   const Datum *values, const bool *isnull,
										   MemoryContext context);
extern Datum nocache_index_getattr(IndexTuple tup, int attnum,
								   TupleDesc tupleDesc);
extern void index_deform_tuple(IndexTuple tup, TupleDesc tupleDescriptor,
							   Datum *values, bool *isnull);
extern void index_deform_tuple_internal(TupleDesc tupleDescriptor,
										Datum *values, bool *isnull,
										char *tp, bits8 *bp, int hasnulls);
extern IndexTuple CopyIndexTuple(IndexTuple source);
extern IndexTuple index_truncate_tuple(TupleDesc sourceDescriptor,
									   IndexTuple source, int leavenatts);


/*
 * Takes an infomask as argument (primarily because this needs to be usable
 * at index_form_tuple time so enough space is allocated).
 */
static inline Size
IndexInfoFindDataOffset(unsigned short t_info)
{
	if (!(t_info & INDEX_NULL_MASK))
		return MAXALIGN(sizeof(IndexTupleData));
	else
		return MAXALIGN(sizeof(IndexTupleData) + sizeof(IndexAttributeBitMapData));
}

#ifndef FRONTEND

/* ----------------
 *		index_getattr
 *
 *		This gets called many times, so we macro the cacheable and NULL
 *		lookups, and call nocache_index_getattr() for the rest.
 *
 * ----------------
 */
static inline Datum
index_getattr(IndexTuple tup, int attnum, TupleDesc tupleDesc, bool *isnull)
{
	Assert(PointerIsValid(isnull));
	Assert(attnum > 0);

	*isnull = false;

	if (!IndexTupleHasNulls(tup))
	{
		CompactAttribute *attr = TupleDescCompactAttr(tupleDesc, attnum - 1);

		if (attr->attcacheoff >= 0)
		{
			return fetchatt(attr,
							(char *) tup + IndexInfoFindDataOffset(tup->t_info) +
							attr->attcacheoff);
		}
		else
			return nocache_index_getattr(tup, attnum, tupleDesc);
	}
	else
	{
		if (att_isnull(attnum - 1, (bits8 *) tup + sizeof(IndexTupleData)))
		{
			*isnull = true;
			return (Datum) NULL;
		}
		else
			return nocache_index_getattr(tup, attnum, tupleDesc);
	}
}

#endif

/*
 * MaxIndexTuplesPerPage is an upper bound on the number of tuples that can
 * fit on one index page.  An index tuple must have either data or a null
 * bitmap, so we can safely assume it's at least 1 byte bigger than a bare
 * IndexTupleData struct.  We arrive at the divisor because each tuple
 * must be maxaligned, and it must have an associated line pointer.
 *
 * To be index-type-independent, this does not account for any special space
 * on the page, and is thus conservative.
 *
 * Note: in btree non-leaf pages, the first tuple has no key (it's implicitly
 * minus infinity), thus breaking the "at least 1 byte bigger" assumption.
 * On such a page, N tuples could take one MAXALIGN quantum less space than
 * estimated here, seemingly allowing one more tuple than estimated here.
 * But such a page always has at least MAXALIGN special space, so we're safe.
 */
#define MaxIndexTuplesPerPage	\
	((int) ((BLCKSZ - SizeOfPageHeaderData) / \
			(MAXALIGN(sizeof(IndexTupleData) + 1) + sizeof(ItemIdData))))

#endif							/* ITUP_H */
