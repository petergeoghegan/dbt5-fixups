/*
 * This file is released under the terms of the Artistic License.  Please see
 * the file LICENSE, included in this package, for details.
 *
 * Copyright (C) 2007-2010 Mark Wong
 *
 * Based on TPC-E Standard Specification Revision 1.10.0.
 */

#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <postgres.h>
#include <fmgr.h>
#include <executor/spi.h> /* this should include most necessary APIs */
#include <executor/executor.h>  /* for GetAttributeByName() */
#include <funcapi.h> /* for returning set of rows */
#include <utils/array.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>
#include <utils/numeric.h>
#include <access/tupmacs.h>
#include <catalog/pg_type.h>

#include "frame.h"
#include "dbt5common.h"

/*
 * Copied this out of the PostgreSQL backend code.  That probably means
 * there is a more appropriate way to do it...
 */
typedef int16 NumericDigit;
#define NUMERIC_NDIGITS(num) \
		((VARSIZE(num) - NUMERIC_HDRSZ) / sizeof(NumericDigit))

/*
 * Use now() all over the place because the value won't change within a
 * transaction.
 */

/*
 * The only reason we have the RETUNING clause here is so we can count how
 * many rows were updated.
 */
#ifdef DEBUG
#define SQLMFF1_1 \
		"UPDATE last_trade\n" \
		"SET    lt_price = %s,\n" \
		"       lt_vol = lt_vol + %d,\n" \
		"       lt_dts = now()\n" \
		"WHERE  lt_s_symb = '%s';"

#define SQLMFF1_2 \
		"SELECT tr_t_id,\n" \
		"       tr_bid_price,\n" \
		"       tr_tt_id,\n" \
		"       tr_qty\n" \
		"FROM   trade_request\n" \
		"WHERE  tr_s_symb = '%s'\n" \
		"       AND ((tr_tt_id = '%s'\n" \
		"             AND tr_bid_price >= %s)\n" \
		"             OR (tr_tt_id = '%s'\n" \
		"                 AND tr_bid_price <= %s)\n" \
		"             OR (tr_tt_id = '%s'\n" \
		"                 AND tr_bid_price >= %s));"

#define SQLMFF1_3 \
		"UPDATE trade\n" \
		"SET    t_dts = now(),\n" \
		"       t_st_id = '%s'\n" \
		"WHERE  t_id = %s;"

#define SQLMFF1_4 \
		"DELETE FROM trade_request\n" \
		"WHERE  tr_t_id = %s;"

#define SQLMFF1_5 \
		"INSERT INTO trade_history(th_t_id, th_dts, th_st_id)\n" \
		"VALUES (%s, now(), '%s');"
#endif /* End DEBUG */

#define MFF1_1 (*MFF1_statements[0].plan)
#define MFF1_2 (*MFF1_statements[1].plan)
#define MFF1_3 (*MFF1_statements[2].plan)
#define MFF1_4 (*MFF1_statements[3].plan)
#define MFF1_5 (*MFF1_statements[4].plan)

static MemoryContext MFF1_savedcxt = NULL;

static cached_statement MFF1_statements[] = {
	/* MFF1_1 */
	{
	"UPDATE last_trade\n" \
	"SET    lt_price = $1,\n" \
	"       lt_vol = lt_vol + $2,\n" \
	"       lt_dts = now()\n" \
	"WHERE  lt_s_symb = $3",
	3,
	{ FLOAT8OID, INT8OID, TEXTOID }
	},

	/* MFF1_2 */
	{
	"SELECT tr_t_id,\n" \
	"       tr_bid_price,\n" \
	"       tr_tt_id,\n" \
	"       tr_qty\n" \
	"FROM   trade_request\n" \
	"WHERE  tr_s_symb = $1\n" \
	"       AND ((tr_tt_id = $2\n" \
	"             AND tr_bid_price >= $3)\n" \
	"             OR (tr_tt_id = $4\n" \
	"                 AND tr_bid_price <= $5)\n" \
	"             OR (tr_tt_id = $6\n" \
	"                 AND tr_bid_price >= $7))",
	7,
	{ TEXTOID, TEXTOID, FLOAT8OID, TEXTOID,
		FLOAT8OID, TEXTOID, FLOAT8OID }
	},

	/* MFF1_3 */
	{
	"UPDATE trade\n" \
	"SET    t_dts = now(),\n" \
	"       t_st_id = $1\n" \
	"WHERE  t_id = $2",
	2,
	{ TEXTOID, INT8OID }
	},

	/* MFF1_4 */
	{
	"DELETE FROM trade_request\n" \
	"WHERE  tr_t_id = $1",
	1,
	{ INT8OID }
	},

	/* MFF1_5 */
	{
	"INSERT INTO trade_history(th_t_id, th_dts, th_st_id)\n" \
	"VALUES ($1, now(), $2)",
	2,
	{ INT8OID, TEXTOID }
	},

	{ NULL }
}; /*End of MFF1_statements */

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* Prototypes to prevent potential gcc warnings. */
Datum MarketFeedFrame1(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(MarketFeedFrame1);

void dump_mff1_inputs(ArrayType *, char *, ArrayType *, ArrayType *, char *,
		char *, char *);

void dump_mff1_inputs(ArrayType *price_quote_p, char *status_submitted_p,
		ArrayType *symbol_p, ArrayType *trade_qty, char *type_limit_buy_p,
		char *type_limit_sell_p, char *type_stop_loss_p) {
	int i;
	int nitems_pq;
	Datum *transdatums_pq;
	char *p_s;

	int16 typlen_s;
	bool typbyval_s;
	char typalign_s;

	int16 typlen_tq;
	bool typbyval_tq;
	char typalign_tq;

	int *p_tq;

	deconstruct_array(price_quote_p, NUMERICOID, -1, false, 'i',
			&transdatums_pq, NULL, &nitems_pq);

	get_typlenbyvalalign(ARR_ELEMTYPE(trade_qty), &typlen_tq, &typbyval_tq,
			&typalign_tq);
	p_tq = (int *) ARR_DATA_PTR(trade_qty);

	get_typlenbyvalalign(ARR_ELEMTYPE(symbol_p), &typlen_s, &typbyval_s,
			&typalign_s);
	p_s = ARR_DATA_PTR(symbol_p);

	elog(NOTICE, "MFF1: INPUTS START");
	for (i = 0; i < nitems_pq; i++) {
		elog(NOTICE, "MFF1: price_quote[%d] %s", i,
				DatumGetCString(DirectFunctionCall1(numeric_out,
						transdatums_pq[i])));
	}
	elog(NOTICE, "MFF1: status_submitted '%s'",
			DatumGetCString(DirectFunctionCall1(textout,
			PointerGetDatum(status_submitted_p))));
	for (i = 0; i < nitems_pq; i++) {
		elog(NOTICE, "MFF1: symbol[%d] '%s'", i,
				DatumGetCString(DirectFunctionCall1(textout,
						PointerGetDatum(p_s))));
	}
	for (i = 0; i < nitems_pq; i++) {
		elog(NOTICE, "MFF1: trade_qty[%d] %d", i, p_tq[i]);

		p_s = att_addlength_pointer(p_s, typlen_s, p_s);
		p_s = (char *) att_align_nominal(p_s, typalign_s);
	}

	elog(NOTICE, "MFF1: type_limit_buy '%s'",
			DatumGetCString(DirectFunctionCall1(textout,
			PointerGetDatum(type_limit_buy_p))));
	elog(NOTICE, "MFF1: type_limit_sell '%s'",
			DatumGetCString(DirectFunctionCall1(textout,
			PointerGetDatum(type_limit_sell_p))));
	elog(NOTICE, "MFF1: type_stop_loss '%s'",
			DatumGetCString(DirectFunctionCall1(textout,
			PointerGetDatum(type_stop_loss_p))));
	elog(NOTICE, "MFF1: INPUTS END");
}

/* Clause 3.3.1.3 */
Datum MarketFeedFrame1(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	AttInMetadata *attinmeta;
	int call_cntr;
	int max_calls;

	int i, j, n;
	int num_updated = 0;
	int rows_sent;
	int send_len = 0;
	int count = 0;

	int nitems_pq;
	int *p_tq;
	char *p_s;

	char **values = NULL;

	/* Stuff done only on the first call of the function. */
	if (SRF_IS_FIRSTCALL()) {
		ArrayType *price_quote_p = PG_GETARG_ARRAYTYPE_P(0);
		char *status_submitted_p = (char *) PG_GETARG_TEXT_P(1);
		ArrayType *symbol_p = PG_GETARG_ARRAYTYPE_P(2);
		ArrayType *trade_qty = PG_GETARG_ARRAYTYPE_P(3);
		char *type_limit_buy_p = (char *) PG_GETARG_TEXT_P(4);
		char *type_limit_sell_p = (char *) PG_GETARG_TEXT_P(5);
		char *type_stop_loss_p = (char *) PG_GETARG_TEXT_P(6);

		enum mff1 {
				i_num_updated=0, i_send_len, i_symbol, i_trade_id,
				i_price_quote, i_trade_qty, i_trade_type
		};

		Datum *transdatums_pq;

		int16 typlen_s;
		bool typbyval_s;
		char typalign_s;

		int16 typlen_tq;
		bool typbyval_tq;
		char typalign_tq;

		int ret;
		TupleDesc tupdesc;
		SPITupleTable *tuptable = NULL;
		HeapTuple tuple = NULL;
#ifdef DEBUG
		char sql[2048];
#endif
		Datum args[7];
		char nulls[7];
		char price_quote[S_PRICE_T_LEN + 1];
		char status_submitted[ST_ID_LEN + 1];
		char symbol[S_SYMB_LEN + 1];
		char type_limit_buy[TT_ID_LEN + 1];
		char type_limit_sell[TT_ID_LEN + 1];
		char type_stop_loss[TT_ID_LEN + 1];
		char *trade_id;
		char *req_price_quote;
		char *req_trade_type;
		char *req_trade_qty;

		/*
		 * Prepare a values array for building the returned tuple.
		 * This should be an array of C strings, which will
		 * be processed later by the type input functions.
		 */
		memset(nulls, 0, sizeof(nulls));
		values = (char **) palloc(sizeof(char *) * 7);
		values[i_num_updated] =
				(char *) palloc((INTEGER_LEN + 1) * sizeof(char));
		values[i_send_len] = (char *) palloc((INTEGER_LEN + 1) * sizeof(char));
		/*
		 * FIXME: We don't know how many rows could be returned.  The average
		 * is supposed to be 4.  Let's be prepared for 100, just to be safe.
		 */
		values[i_symbol] = (char *) palloc(((S_SYMB_LEN + 3) * 100 + 3) *
				sizeof(char));
		values[i_trade_id] = (char *) palloc(((IDENT_T_LEN + 1) * 100 + 2) *
				sizeof(char));
		values[i_price_quote] = (char *) palloc(((S_PRICE_T_LEN + 1) * 100 +
				2) * sizeof(char));
		values[i_trade_qty] = (char *) palloc(((INTEGER_LEN + 1) * 100 + 2) *
				sizeof(char));
		values[i_trade_type] = (char *) palloc(((TT_ID_LEN + 3) * 100 + 3) *
				sizeof(char));

		/*
		 * This might be overkill since we always expect single dimensions
		 * arrays.
		 * Should probably check the count of all the arrays to make sure
		 * they are the same...
		 */
		get_typlenbyvalalign(ARR_ELEMTYPE(symbol_p), &typlen_s, &typbyval_s,
				&typalign_s);
		p_s = ARR_DATA_PTR(symbol_p);

		get_typlenbyvalalign(ARR_ELEMTYPE(trade_qty), &typlen_tq, &typbyval_tq,
				&typalign_tq);
		p_tq = (int *) ARR_DATA_PTR(trade_qty);

		deconstruct_array(price_quote_p, NUMERICOID, -1, false, 'i',
				&transdatums_pq, NULL, &nitems_pq);

		strcpy(status_submitted, DatumGetCString(DirectFunctionCall1(textout,
				PointerGetDatum(status_submitted_p))));
		strcpy(type_limit_buy, DatumGetCString(DirectFunctionCall1(textout,
				PointerGetDatum(type_limit_buy_p))));
        strcpy(type_limit_sell, DatumGetCString(DirectFunctionCall1(textout,
                PointerGetDatum(type_limit_sell_p))));
		strcpy(type_stop_loss, DatumGetCString(DirectFunctionCall1(textout,
                PointerGetDatum(type_stop_loss_p))));

#ifdef DEBUG
		dump_mff1_inputs(price_quote_p, status_submitted_p, symbol_p,
				trade_qty, type_limit_buy_p, type_limit_sell_p,
				type_stop_loss_p);
#endif

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		MFF1_savedcxt = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		if (SPI_connect() != SPI_OK_CONNECT)
			elog(ERROR, "SPI connect failed");
		plan_queries(MFF1_statements);

		strcpy(values[i_symbol], "{");
		strcpy(values[i_trade_id], "{");
		strcpy(values[i_price_quote], "{");
		strcpy(values[i_trade_type], "{");
		strcpy(values[i_trade_qty], "{");
		for (i = 0; i < nitems_pq; i++) {
			rows_sent = 0;

			strcpy(price_quote,
					DatumGetCString(DirectFunctionCall1(numeric_out,
							transdatums_pq[i])));
			strcpy(symbol, DatumGetCString(DirectFunctionCall1(textout,
					PointerGetDatum(p_s))));
			/* FIXME: BEGIN/COMMIT statements not supported with SPI. */
/*
			ret = SPI_exec("BEGIN;", 0);
			if (ret == SPI_OK_SELECT) {
			} else {
				elog(NOTICE, "ERROR: BEGIN not ok = %d", ret);
			}
*/
#ifdef DEBUG
			sprintf(sql, SQLMFF1_1,
					DatumGetCString(DirectFunctionCall1(numeric_out,            
							transdatums_pq[i])),
					p_tq[i],
					symbol);
			elog(NOTICE, "SQL\n%s", sql);
#endif /* DEBUG */
			args[0] = Float8GetDatum(atof(price_quote));
			args[1] = Int64GetDatum(p_tq[i]);
			args[2] = CStringGetTextDatum(symbol);
			ret = SPI_execute_plan(MFF1_1, args, nulls, false, 0);
			if (ret != SPI_OK_UPDATE) {
				dump_mff1_inputs(price_quote_p, status_submitted_p, symbol_p,
						trade_qty, type_limit_buy_p, type_limit_sell_p,
						type_stop_loss_p);
				FAIL_FRAME_SET(&funcctx->max_calls, MFF1_statements[0].sql);
			}
			num_updated += SPI_processed;
#ifdef DEBUG
			elog(NOTICE, "%d row(s) updated", num_updated);
			sprintf(sql, SQLMFF1_2, symbol, type_stop_loss, price_quote,
					type_limit_sell, price_quote, type_limit_buy, price_quote);
			elog(NOTICE, "SQL\n%s", sql);
#endif /* DEBUG */
			args[0] = CStringGetTextDatum(symbol);
			args[1] = CStringGetTextDatum(type_stop_loss);
			args[2] = Float8GetDatum(atof(price_quote));
			args[3] = CStringGetTextDatum(type_limit_sell);
			args[4] = args[2];
			args[5] = CStringGetTextDatum(type_limit_buy);
			args[6] = args[2];
			ret = SPI_execute_plan(MFF1_2, args, nulls, true, 0);
			if (ret != SPI_OK_SELECT) {
				dump_mff1_inputs(price_quote_p, status_submitted_p, symbol_p,
						trade_qty, type_limit_buy_p, type_limit_sell_p,
						type_stop_loss_p);
				FAIL_FRAME_SET(&funcctx->max_calls, MFF1_statements[1].sql);
				continue;
			}
#ifdef DEBUG
			elog(NOTICE, "%ld row(s) returned", SPI_processed);
#endif /* DEBUG */

			tupdesc = SPI_tuptable->tupdesc;
			tuptable = SPI_tuptable;
			n = SPI_processed;
			for (j = 0; j < n; j++) {
				tuple = tuptable->vals[j];
				trade_id = SPI_getvalue(tuple, tupdesc, 1);
				req_price_quote = SPI_getvalue(tuple, tupdesc, 2);
				req_trade_type = SPI_getvalue(tuple, tupdesc, 3);
				req_trade_qty = SPI_getvalue(tuple, tupdesc, 4);
#ifdef DEBUG
				elog(NOTICE, "trade_id = %s", trade_id);
				sprintf(sql, SQLMFF1_3, status_submitted, trade_id);
				elog(NOTICE, "SQL\n%s", sql);
#endif /* DEBUG */
				args[0] = CStringGetTextDatum(status_submitted);
				args[1] = Int64GetDatum(atoll(trade_id));
				ret = SPI_execute_plan(MFF1_3, args, nulls, false, 0);
				if (ret != SPI_OK_UPDATE) {
					dump_mff1_inputs(price_quote_p, status_submitted_p,
							symbol_p, trade_qty, type_limit_buy_p,
							type_limit_sell_p, type_stop_loss_p);
					FAIL_FRAME_SET(&funcctx->max_calls, MFF1_statements[2].sql);
				}
#ifdef DEBUG
				sprintf(sql, SQLMFF1_4, trade_id);
				elog(NOTICE, "SQL\n%s", sql);
#endif /* DEBUG */
				args[0] = Int64GetDatum(atoll(trade_id));
				ret = SPI_execute_plan(MFF1_4, args, nulls, false, 0);
				if (ret != SPI_OK_DELETE) {
					dump_mff1_inputs(price_quote_p, status_submitted_p,
							symbol_p, trade_qty, type_limit_buy_p,
							type_limit_sell_p, type_stop_loss_p);
					FAIL_FRAME_SET(&funcctx->max_calls, MFF1_statements[3].sql);
				}
#ifdef DEBUG
				sprintf(sql, SQLMFF1_5, trade_id, status_submitted);
				elog(NOTICE, "SQL\n%s", sql);
#endif /* DEBUG */
				args[1] = CStringGetTextDatum(status_submitted);
				ret = SPI_execute_plan(MFF1_5, args, nulls, false, 0);
				if (ret != SPI_OK_INSERT) {
					dump_mff1_inputs(price_quote_p, status_submitted_p,
							symbol_p, trade_qty, type_limit_buy_p,
							type_limit_sell_p, type_stop_loss_p);
					FAIL_FRAME_SET(&funcctx->max_calls, MFF1_statements[4].sql);
				}
				++rows_sent;
#ifdef DEBUG
				elog(NOTICE, "%d row(s) sent", rows_sent);
#endif /* DEBUG */

				if (count > 0) {
					strcat(values[i_symbol], ",");
					strcat(values[i_trade_id], ",");
					strcat(values[i_price_quote], ",");
					strcat(values[i_trade_type], ",");
					strcat(values[i_trade_qty], ",");
				}
				strcat(values[i_symbol], symbol);
				strcat(values[i_trade_id], trade_id);
				strcat(values[i_price_quote], req_price_quote);
				strcat(values[i_trade_type], req_trade_type);
				strcat(values[i_trade_qty], req_trade_qty);
				++count;
			}

			/* FIXME: BEGIN/COMMIT statements not supported with SPI. */
/*
			ret = SPI_exec("COMMIT;", 0);
			if (ret == SPI_OK_SELECT) {
			} else {
				elog(NOTICE, "ERROR: COMMIT not ok = %d", ret);
			}
*/

			send_len += rows_sent;

			p_s = att_addlength_pointer(p_s, typlen_s, p_s);
			p_s = (char *) att_align_nominal(p_s, typalign_s);
		}
		strcat(values[i_symbol], "}");
		strcat(values[i_trade_id], "}");
		strcat(values[i_price_quote], "}");
		strcat(values[i_trade_qty], "}");
		strcat(values[i_trade_type], "}");

		sprintf(values[i_num_updated], "%d", num_updated);
		sprintf(values[i_send_len], "%d", send_len);
		funcctx->max_calls = 1;

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) !=
				TYPEFUNC_COMPOSITE) {
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("function returning record called in context "
							"that cannot accept type record")));
		}

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		MemoryContextSwitchTo(MFF1_savedcxt);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	attinmeta = funcctx->attinmeta;

	if (call_cntr < max_calls) {
		/* do when there is more left to send */
		HeapTuple tuple;
		Datum result;

#ifdef DEBUG                                                                    
		for (i = 0; i < 7; i++) {
			elog(NOTICE, "MFF1 OUT: %d %s", i, values[i]);
		}
#endif /* DEBUG */

		/* Build a tuple. */
		tuple = BuildTupleFromCStrings(attinmeta, values);

		/* Make the tuple into a datum. */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	} else {
		/* Do when there is no more left. */
		SPI_finish();
		if (MFF1_savedcxt) MemoryContextSwitchTo(MFF1_savedcxt);
		SRF_RETURN_DONE(funcctx);
	}
}
