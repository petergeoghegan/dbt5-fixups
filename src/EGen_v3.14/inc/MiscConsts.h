/*
 * Legal Notice
 *
 * This document and associated source code (the "Work") is a preliminary
 * version of a benchmark specification being developed by the TPC. The
 * Work is being made available to the public for review and comment only.
 * The TPC reserves all right, title, and interest to the Work as provided
 * under U.S. and international laws, including without limitation all patent
 * and trademark rights therein.
 *
 * No Warranty
 *
 * 1.1 TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THE INFORMATION
 *     CONTAINED HEREIN IS PROVIDED "AS IS" AND WITH ALL FAULTS, AND THE
 *     AUTHORS AND DEVELOPERS OF THE WORK HEREBY DISCLAIM ALL OTHER
 *     WARRANTIES AND CONDITIONS, EITHER EXPRESS, IMPLIED OR STATUTORY,
 *     INCLUDING, BUT NOT LIMITED TO, ANY (IF ANY) IMPLIED WARRANTIES,
 *     DUTIES OR CONDITIONS OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
 *     PURPOSE, OF ACCURACY OR COMPLETENESS OF RESPONSES, OF RESULTS, OF
 *     WORKMANLIKE EFFORT, OF LACK OF VIRUSES, AND OF LACK OF NEGLIGENCE.
 *     ALSO, THERE IS NO WARRANTY OR CONDITION OF TITLE, QUIET ENJOYMENT,
 *     QUIET POSSESSION, CORRESPONDENCE TO DESCRIPTION OR NON-INFRINGEMENT
 *     WITH REGARD TO THE WORK.
 * 1.2 IN NO EVENT WILL ANY AUTHOR OR DEVELOPER OF THE WORK BE LIABLE TO
 *     ANY OTHER PARTY FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO THE
 *     COST OF PROCURING SUBSTITUTE GOODS OR SERVICES, LOST PROFITS, LOSS
 *     OF USE, LOSS OF DATA, OR ANY INCIDENTAL, CONSEQUENTIAL, DIRECT,
 *     INDIRECT, OR SPECIAL DAMAGES WHETHER UNDER CONTRACT, TORT, WARRANTY,
 *     OR OTHERWISE, ARISING IN ANY WAY OUT OF THIS OR ANY OTHER AGREEMENT
 *     RELATING TO THE WORK, WHETHER OR NOT SUCH AUTHOR OR DEVELOPER HAD
 *     ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 * Contributors
 * - Sergey Vasilevskiy
 */

/*
*	Miscellaneous constants used throughout EGenLoader.
*/
#ifndef MISC_CONSTS_H
#define MISC_CONSTS_H

namespace TPCE
{

const int		iMaxHostname = 64;
const int		iMaxDBName = 64;
const int		iMaxPath = 512;

//#ifdef COMPILE_PGSQL_LOAD
const int		iMaxPGHost = 128;
const int		iMaxPGDBName = 32;
const int		iMaxPGPort = 32;
const int		iDateTimeFmt = 11;
//#endif //COMPILE_PGSQL_LOAD

// NOTE: Changing the initial trade populate base date
// can break code used in CCETxnInputGenerator for generating 
// Trade-Lookup data.
const INT16	InitialTradePopulationBaseYear		= 2005;
const UINT16	InitialTradePopulationBaseMonth		= 1;
const UINT16	InitialTradePopulationBaseDay		= 3;
const UINT16	InitialTradePopulationBaseHour		= 9;
const UINT16	InitialTradePopulationBaseMinute	= 0;
const UINT16	InitialTradePopulationBaseSecond	= 0;
const UINT64	InitialTradePopulationBaseFraction	= 0;

// At what trade count multiple to abort trades.
// One trade in every iAboutTrade block is aborted (trade id is thrown out).
//NOTE: this really is 10 * Trade-Order mix percentage!
//
const int	iAbortTrade = 101;

// Used at load and run time to determine which intial trades
// simulate rollback by "aborting" - I.e. used to skip over a 
// trade ID.
const int	iAbortedTradeModFactor = 51;

// Start date for DAILY_MARKET and FINANCIAL.
//
const int		iDailyMarketBaseYear	= 2000;
const int		iDailyMarketBaseMonth	= 1;
const int		iDailyMarketBaseDay	= 3;
const int		iDailyMarketBaseHour	= 0;
const int		iDailyMarketBaseMinute	= 0;
const int		iDailyMarketBaseSecond	= 0;
const int		iDailyMarketBaseMsec	= 0;

const double		MsPerSecondDivisor  	= 1000.000;
const INT32		MsPerSecond		= 1000;
const INT32		SecondsPerMinute	= 60;
const INT32		MinutesPerHour		= 60;
const INT32		HoursPerDay		= 24;
const INT32		HoursPerWorkDay		= 8;

const INT32		SecondsPerHour =	SecondsPerMinute * MinutesPerHour;
const INT32		SecondsPerDay =		SecondsPerMinute * MinutesPerHour * HoursPerDay;
const INT32		SecondsPerWorkDay =	SecondsPerMinute * MinutesPerHour * HoursPerWorkDay;
const INT32		MsPerDay =		SecondsPerDay * MsPerSecond;
const INT32		MsPerWorkDay =		SecondsPerWorkDay * MsPerSecond;

// Range of financial rows to return from Security Detail
const int		iSecurityDetailMinRows = 5;
const int		iSecurityDetailMaxRows = 20;	// max_fin_len 

// Trade-Lookup constants
const INT32 TradeLookupMaxTradeHistoryRowsReturned	= 3;	//Based on the maximum number of status changes a trade can go through.
const INT32 TradeLookupMaxRows				= 30;	// Max number of rows for the frames
const INT32 TradeLookupFrame1MaxRows			= TradeLookupMaxRows;
const INT32 TradeLookupFrame2MaxRows			= TradeLookupMaxRows;
const INT32 TradeLookupFrame3MaxRows			= TradeLookupMaxRows;
const INT32 TradeLookupFrame4MaxRows			= TradeLookupMaxRows;

// Trade-Update constants
const INT32 TradeUpdateMaxTradeHistoryRowsReturned	= 3;	//Based on the maximum number of status changes a trade can go through.
const INT32 TradeUpdateMaxRows				= 30;	// Max number of rows for the frames
const INT32 TradeUpdateFrame1MaxRows			= TradeUpdateMaxRows;
const INT32 TradeUpdateFrame2MaxRows			= TradeUpdateMaxRows;
const INT32 TradeUpdateFrame3MaxRows			= TradeUpdateMaxRows;

// These two arrays are used for platform independence
const char	UpperCaseLetters[]	=	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char	LowerCaseLetters[]	=	"abcdefghijklmnopqrstuvwxyz";
const char	Numerals[]		=	"0123456789";
const int	MaxLowerCaseLetters	=	sizeof(LowerCaseLetters) - 1;

}	// namespace TPCE

#endif //MISC_CONSTS_H
