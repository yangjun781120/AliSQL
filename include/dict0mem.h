/******************************************************
Data dictionary memory object creation

(c) 1996 Innobase Oy

Created 1/8/1996 Heikki Tuuri
*******************************************************/

#ifndef dict0mem_h
#define dict0mem_h

#include "univ.i"
#include "dict0types.h"
#include "data0type.h"
#include "data0data.h"
#include "mem0mem.h"
#include "rem0types.h"
#include "btr0types.h"
#include "ut0mem.h"
#include "ut0lst.h"
#include "ut0rnd.h"
#include "ut0byte.h"
#include "sync0rw.h"
#include "lock0types.h"
#include "hash0hash.h"
#include "que0types.h"

/* Type flags of an index: OR'ing of the flags is allowed to define a
combination of types */
#define DICT_CLUSTERED	1	/* clustered index */
#define DICT_UNIQUE	2	/* unique index */
#define	DICT_UNIVERSAL	4	/* index which can contain records from any
				other index */
#define	DICT_IBUF	8	/* insert buffer tree */

/* Types for a table object */
#define DICT_TABLE_ORDINARY		1
#if 0 /* not implemented */
#define	DICT_TABLE_CLUSTER_MEMBER	2
#define	DICT_TABLE_CLUSTER		3 /* this means that the table is
					  really a cluster definition */
#endif

/* Table flags */
#define DICT_TF_COMPACT			1	/* compact page format */

/**************************************************************************
Creates a table memory object. */

dict_table_t*
dict_mem_table_create(
/*==================*/
					/* out, own: table object */
	const char*	name,		/* in: table name */
	ulint		space,		/* in: space where the clustered index
					of the table is placed; this parameter
					is ignored if the table is made
					a member of a cluster */
	ulint		n_cols,		/* in: number of columns */
	ulint		flags);		/* in: table flags */
/********************************************************************
Free a table memory object. */

void
dict_mem_table_free(
/*================*/
	dict_table_t*	table);		/* in: table */
/**************************************************************************
Adds a column definition to a table. */

void
dict_mem_table_add_col(
/*===================*/
	dict_table_t*	table,	/* in: table */
	const char*	name,	/* in: column name */
	ulint		mtype,	/* in: main datatype */
	ulint		prtype,	/* in: precise type */
	ulint		len);	/* in: precision */
/**************************************************************************
Creates an index memory object. */

dict_index_t*
dict_mem_index_create(
/*==================*/
					/* out, own: index object */
	const char*	table_name,	/* in: table name */
	const char*	index_name,	/* in: index name */
	ulint		space,		/* in: space where the index tree is
					placed, ignored if the index is of
					the clustered type */
	ulint		type,		/* in: DICT_UNIQUE,
					DICT_CLUSTERED, ... ORed */
	ulint		n_fields);	/* in: number of fields */
/**************************************************************************
Adds a field definition to an index. NOTE: does not take a copy
of the column name if the field is a column. The memory occupied
by the column name may be released only after publishing the index. */

void
dict_mem_index_add_field(
/*=====================*/
	dict_index_t*	index,		/* in: index */
	const char*	name,		/* in: column name */
	ulint		prefix_len);	/* in: 0 or the column prefix length
					in a MySQL index like
					INDEX (textcol(25)) */
/**************************************************************************
Frees an index memory object. */

void
dict_mem_index_free(
/*================*/
	dict_index_t*	index);	/* in: index */
/**************************************************************************
Creates and initializes a foreign constraint memory object. */

dict_foreign_t*
dict_mem_foreign_create(void);
/*=========================*/
				/* out, own: foreign constraint struct */

/* Data structure for a column in a table */
struct dict_col_struct{
	ulint		ind:10;	/* table column position (they are numbered
				starting from 0) */
	ulint		clust_pos:10;/* position of the column in the
				clustered index */
	ulint		ord_part:12;/* count of how many times this column
				appears in ordering fields of an index */
	const char*	name;	/* name */
	dtype_t		type;	/* data type */
};

/* DICT_MAX_INDEX_COL_LEN is measured in bytes and is the max index column
length + 1. Starting from 4.1.6, we set it to < 3 * 256, so that one can
create a column prefix index on 255 characters of a TEXT field also in the
UTF-8 charset. In that charset, a character may take at most 3 bytes. */

#define DICT_MAX_INDEX_COL_LEN		768

/* Data structure for a field in an index */
struct dict_field_struct{
	dict_col_t*	col;		/* pointer to the table column */
	const char*	name;		/* name of the column */
	ulint		prefix_len:10;	/* 0 or the length of the column
					prefix in bytes in a MySQL index of
					type, e.g., INDEX (textcol(25));
					must be smaller than
					DICT_MAX_INDEX_COL_LEN; NOTE that
					in the UTF-8 charset, MySQL sets this
					to 3 * the prefix len in UTF-8 chars */
	ulint		fixed_len:10;	/* 0 or the fixed length of the
					column if smaller than
					DICT_MAX_INDEX_COL_LEN */
};

/* Data structure for an index tree */
struct dict_tree_struct{
	ulint		type;	/* tree type */
	dulint		id;	/* id of the index stored in the tree */
	ulint		space;	/* space of index tree */
	ulint		page;	/* index tree root page number */
	rw_lock_t	lock;	/* read-write lock protecting the upper levels
				of the index tree */
	dict_index_t*	tree_index; /* the index stored in the
				index tree */
#ifdef UNIV_DEBUG
	ulint		magic_n;/* magic number */
# define DICT_TREE_MAGIC_N	7545676
#endif /* UNIV_DEBUG */
};

/* Data structure for an index */
struct dict_index_struct{
	dulint		id;	/* id of the index */
	mem_heap_t*	heap;	/* memory heap */
	ulint		type;	/* index type */
	const char*	name;	/* index name */
	const char*	table_name; /* table name */
	dict_table_t*	table;	/* back pointer to table */
	ulint		space;	/* space where the index tree is placed */
	ulint		trx_id_offset:10;/* position of the the trx id column
				in a clustered index record, if the fields
				before it are known to be of a fixed size,
				0 otherwise */
	ulint		n_user_defined_cols:10;
				/* number of columns the user defined to
				be in the index: in the internal
				representation we add more columns */
	ulint		n_uniq:10;/* number of fields from the beginning
				which are enough to determine an index
				entry uniquely */
	ulint		n_def:10;/* number of fields defined so far */
	ulint		n_fields:10;/* number of fields in the index */
	ulint		n_nullable:10;/* number of nullable fields */
	ibool		cached:1;/* TRUE if the index object is in the
				dictionary cache */
	dict_field_t*	fields;	/* array of field descriptions */
	UT_LIST_NODE_T(dict_index_t)
			indexes;/* list of indexes of the table */
	dict_tree_t*	tree;	/* index tree struct */
	btr_search_t*	search_info; /* info used in optimistic searches */
	/*----------------------*/
	ib_longlong*	stat_n_diff_key_vals;
				/* approximate number of different key values
				for this index, for each n-column prefix
				where n <= dict_get_n_unique(index); we
				periodically calculate new estimates */
	ulint		stat_index_size;
				/* approximate index size in database pages */
	ulint		stat_n_leaf_pages;
				/* approximate number of leaf pages in the
				index tree */
#ifdef UNIV_DEBUG
	ulint		magic_n;/* magic number */
# define DICT_INDEX_MAGIC_N	76789786
#endif
};

/* Data structure for a foreign key constraint; an example:
FOREIGN KEY (A, B) REFERENCES TABLE2 (C, D) */

struct dict_foreign_struct{
	mem_heap_t*	heap;		/* this object is allocated from
					this memory heap */
	char*		id;		/* id of the constraint as a
					null-terminated string */
	ulint		n_fields:10;	/* number of indexes' first fields
					for which the the foreign key
					constraint is defined: we allow the
					indexes to contain more fields than
					mentioned in the constraint, as long
					as the first fields are as mentioned */
	ulint		type:6;		/* 0 or DICT_FOREIGN_ON_DELETE_CASCADE
					or DICT_FOREIGN_ON_DELETE_SET_NULL */
	char*		foreign_table_name;/* foreign table name */
	dict_table_t*	foreign_table;	/* table where the foreign key is */
	const char**	foreign_col_names;/* names of the columns in the
					foreign key */
	char*		referenced_table_name;/* referenced table name */
	dict_table_t*	referenced_table;/* table where the referenced key
					is */
	const char**	referenced_col_names;/* names of the referenced
					columns in the referenced table */
	dict_index_t*	foreign_index;	/* foreign index; we require that
					both tables contain explicitly defined
					indexes for the constraint: InnoDB
					does not generate new indexes
					implicitly */
	dict_index_t*	referenced_index;/* referenced index */
	UT_LIST_NODE_T(dict_foreign_t)
			foreign_list;	/* list node for foreign keys of the
					table */
	UT_LIST_NODE_T(dict_foreign_t)
			referenced_list;/* list node for referenced keys of the
					table */
};

/* The flags for ON_UPDATE and ON_DELETE can be ORed; the default is that
a foreign key constraint is enforced, therefore RESTRICT just means no flag */
#define DICT_FOREIGN_ON_DELETE_CASCADE	1
#define DICT_FOREIGN_ON_DELETE_SET_NULL	2
#define DICT_FOREIGN_ON_UPDATE_CASCADE	4
#define DICT_FOREIGN_ON_UPDATE_SET_NULL	8
#define DICT_FOREIGN_ON_DELETE_NO_ACTION 16
#define DICT_FOREIGN_ON_UPDATE_NO_ACTION 32


/* Data structure for a database table */
struct dict_table_struct{
	dulint		id;	/* id of the table */
	mem_heap_t*	heap;	/* memory heap */
	const char*	name;	/* table name */
	const char*	dir_path_of_temp_table;/* NULL or the directory path
				where a TEMPORARY table that was explicitly
				created by a user should be placed if
				innodb_file_per_table is defined in my.cnf;
				in Unix this is usually /tmp/..., in Windows
				\temp\... */
	ulint		space;	/* space where the clustered index of the
				table is placed */
	ibool		ibd_file_missing:1;/* TRUE if this is in a single-table
				tablespace and the .ibd file is missing; then
				we must return in ha_innodb.cc an error if the
				user tries to query such an orphaned table */
	ibool		tablespace_discarded:1;/* this flag is set TRUE when the
				user calls DISCARD TABLESPACE on this table,
				and reset to FALSE in IMPORT TABLESPACE */
	ibool		cached:1;/* TRUE if the table object has been added
				to the dictionary cache */
	ulint		flags:8;/* DICT_TF_COMPACT, ... */
	hash_node_t	name_hash; /* hash chain node */
	hash_node_t	id_hash; /* hash chain node */
	ulint		n_def:10;/* number of columns defined so far */
	ulint		n_cols:10;/* number of columns */
	dict_col_t*	cols;	/* array of column descriptions */
	UT_LIST_BASE_NODE_T(dict_index_t)
			indexes; /* list of indexes of the table */
	UT_LIST_BASE_NODE_T(dict_foreign_t)
			foreign_list;/* list of foreign key constraints
				in the table; these refer to columns
				in other tables */
	UT_LIST_BASE_NODE_T(dict_foreign_t)
			referenced_list;/* list of foreign key constraints
				which refer to this table */
	UT_LIST_NODE_T(dict_table_t)
			table_LRU; /* node of the LRU list of tables */
	ulint		n_mysql_handles_opened;
				/* count of how many handles MySQL has opened
				to this table; dropping of the table is
				NOT allowed until this count gets to zero;
				MySQL does NOT itself check the number of
				open handles at drop */
	ulint		n_foreign_key_checks_running;
				/* count of how many foreign key check
				operations are currently being performed
				on the table: we cannot drop the table while
				there are foreign key checks running on
				it! */
	lock_t*		auto_inc_lock;/* a buffer for an auto-inc lock
				for this table: we allocate the memory here
				so that individual transactions can get it
				and release it without a need to allocate
				space from the lock heap of the trx:
				otherwise the lock heap would grow rapidly
				if we do a large insert from a select */
	dulint		query_cache_inv_trx_id;
				/* transactions whose trx id < than this
				number are not allowed to store to the MySQL
				query cache or retrieve from it; when a trx
				with undo logs commits, it sets this to the
				value of the trx id counter for the tables it
				had an IX lock on */
	UT_LIST_BASE_NODE_T(lock_t)
			locks; /* list of locks on the table */
	ulint		max_row_size;
				/* maximum size of a single row in the
				table, not guaranteed to be especially
				accurate. it's ULINT_MAX if there are
				unbounded variable-width fields. initialized
				in dict_table_add_to_cache. */
#ifdef UNIV_DEBUG
	/*----------------------*/
	ibool		does_not_fit_in_memory;
				/* this field is used to specify in simulations
				tables which are so big that disk should be
				accessed: disk access is simulated by
				putting the thread to sleep for a while;
				NOTE that this flag is not stored to the data
				dictionary on disk, and the database will
				forget about value TRUE if it has to reload
				the table definition from disk */
#endif /* UNIV_DEBUG */
	/*----------------------*/
	ib_longlong	stat_n_rows;
				/* approximate number of rows in the table;
				we periodically calculate new estimates */
	ulint		stat_clustered_index_size;
				/* approximate clustered index size in
				database pages */
	ulint		stat_sum_of_other_index_sizes;
				/* other indexes in database pages */
	ibool		stat_initialized:1; /* TRUE if statistics have
				been calculated the first time
				after database startup or table creation */
	ulint		stat_modified_counter;
				/* when a row is inserted, updated, or deleted,
				we add 1 to this number; we calculate new
				estimates for the stat_... values for the
				table and the indexes at an interval of 2 GB
				or when about 1 / 16 of table has been
				modified; also when the estimate operation is
				called for MySQL SHOW TABLE STATUS; the
				counter is reset to zero at statistics
				calculation; this counter is not protected by
				any latch, because this is only used for
				heuristics */
	/*----------------------*/
	mutex_t		autoinc_mutex;
				/* mutex protecting the autoincrement
				counter */
	ibool		autoinc_inited:1;
				/* TRUE if the autoinc counter has been
				inited; MySQL gets the init value by executing
				SELECT MAX(auto inc column) */
	ib_longlong	autoinc;/* autoinc counter value to give to the
				next inserted row */
#ifdef UNIV_DEBUG
	ulint		magic_n;/* magic number */
# define DICT_TABLE_MAGIC_N	76333786
#endif /* UNIV_DEBUG */
};

#ifndef UNIV_NONINL
#include "dict0mem.ic"
#endif

#endif
