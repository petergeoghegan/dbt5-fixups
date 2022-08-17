/*
 * This file is released under the terms of the Artistic License.  Please see
 * the file LICENSE, included in this package, for details.
 *
 * Copyright (C) 2006-2010 Rilson Nascimento
 *
 * 30 July 2006
 */

#include "MEESUT.h"

void *TradeResultAsync(void *data)
{
	PMEESUTThreadParam pThrParam = reinterpret_cast<PMEESUTThreadParam>(data);
	struct TMsgDriverBrokerage request;

	memset(&request, 0, sizeof(TMsgDriverBrokerage));

	request.TxnType = TRADE_RESULT;
	memcpy(&(request.TxnInput.TradeResultTxnInput),
			&(pThrParam->TxnInput.m_TradeResultTxnInput),
			sizeof(request.TxnInput.TradeResultTxnInput));

	// communicate with the SUT and log response time
	pThrParam->pCMEESUT->m_SocketLock.lock();
	pThrParam->pCMEESUT->talkToSUT(&request);
	pThrParam->pCMEESUT->m_SocketLock.unlock();

	delete pThrParam;
	return NULL;
}

bool RunTradeResultAsync(void *data)
{
	PMEESUTThreadParam pThrParam = reinterpret_cast<PMEESUTThreadParam>(data);

	pthread_t threadID; // thread ID
	pthread_attr_t threadAttribute; // thread attribute

	try
	{
		// initialize the attribute object
		int status = pthread_attr_init(&threadAttribute);
		if (status != 0) {
			throw new CThreadErr(CThreadErr::ERR_THREAD_ATTR_INIT);
		}
	
		// set the detachstate attribute to detached
		status = pthread_attr_setdetachstate(&threadAttribute,
				PTHREAD_CREATE_DETACHED);
		if (status != 0) {
			throw new CThreadErr(CThreadErr::ERR_THREAD_ATTR_DETACH);
		}
	
		// create the thread in the detached state - Call Trade Result
		// asyncronously
		for (;;)
		{
			status = pthread_create(&threadID, &threadAttribute, &TradeResultAsync,
					data);
			if (status == 0)
				break;
			usleep(1000 * 5) ;
		}

		if (status != 0) {
			throw new CThreadErr(CThreadErr::ERR_THREAD_CREATE);
		}
	} catch(CThreadErr *pErr) {
		ostringstream osErr;
		osErr << "Error: " << pErr->ErrorText() <<
				" at MEESUT::RunTradeResultAsync" << endl;
		pThrParam->pCMEESUT->logErrorMessage(osErr.str());
		delete pErr;
		return false;
	}
	catch (std::exception &e)
	{
		ostringstream osErr;
		osErr << "Error: fffffffff fdsjkfjskdjf" << endl;
		pThrParam->pCMEESUT->logErrorMessage(osErr.str());
		return false;
	}

	// return immediatelly
	return true;	
}

bool CMEESUT::TradeResult(PTradeResultTxnInput pTxnInput)
{
	PMEESUTThreadParam pThrParam = new TMEESUTThreadParam;
	memset(pThrParam, 0, sizeof(TMEESUTThreadParam));

	pThrParam->pCMEESUT = this;
	memcpy(&(pThrParam->TxnInput.m_TradeResultTxnInput), pTxnInput,
			sizeof(TTradeResultTxnInput));

	pThrParam->pCMEESUT->m_SocketLock.lock();
	bool result = RunTradeResultAsync(reinterpret_cast<void *>(pThrParam));
	pThrParam->pCMEESUT->m_SocketLock.lock();

	return result;
}

// Market Feed
//
void *MarketFeedAsync(void* data)
{
	PMEESUTThreadParam pThrParam = reinterpret_cast<PMEESUTThreadParam>(data);
	struct TMsgDriverBrokerage request;

	memset(&request, 0, sizeof(TMsgDriverBrokerage));

	request.TxnType = MARKET_FEED;
	memcpy(&(request.TxnInput.MarketFeedTxnInput),
			&(pThrParam->TxnInput.m_MarketFeedTxnInput),
			sizeof(request.TxnInput.MarketFeedTxnInput));

	// communicate with the SUT and log response time
	pThrParam->pCMEESUT->m_SocketLock.lock();
	pThrParam->pCMEESUT->talkToSUT(&request);
	pThrParam->pCMEESUT->m_SocketLock.unlock();

	delete pThrParam;
	return NULL;
}

bool RunMarketFeedAsync(void *data)
{
	PMEESUTThreadParam pThrParam = reinterpret_cast<PMEESUTThreadParam>(data);

	pthread_t threadID; // thread ID
	pthread_attr_t threadAttribute; // thread attribute

	try {
		// initialize the attribute object
		int status = pthread_attr_init(&threadAttribute);
		if (status != 0) {
			throw std::runtime_error("pthread_attr_init failed");
			/* throw new CThreadErr(CThreadErr::ERR_THREAD_ATTR_INIT); */
		}
	
		// set the detachstate attribute to detached
		status = pthread_attr_setdetachstate(&threadAttribute,
				PTHREAD_CREATE_DETACHED);
		if (status != 0) {
			throw std::runtime_error("pthread_attr_setdetachstate failed");
			/* throw new CThreadErr(CThreadErr::ERR_THREAD_ATTR_DETACH); */
		}
	
		// create the thread in the detached state - Call Trade Result
		// asyncronously
		for (;;)
		{
			status = pthread_create(&threadID, &threadAttribute, &MarketFeedAsync,
					data);
			if (status == 0)
				break;
			usleep(1000 * 5) ;
			/* if (status != 0) { */
			/* 	ostringstream osErr; */
			/* 	osErr << "pthread_create failed: " << status; */
			/* 	throw std::runtime_error(osErr.str()); */
			/* } */
		}
	} catch(CThreadErr *pErr) {
		ostringstream osErr;
		osErr << "Error: " << pErr->ErrorText() <<
				" at MEESUT::RunMarketFeedAsync" << endl;
		pThrParam->pCMEESUT->logErrorMessage(osErr.str());
		delete pErr;
		return false;
	}

	// return immediatelly
	return true;
}

bool CMEESUT::MarketFeed(PMarketFeedTxnInput pTxnInput)
{
	PMEESUTThreadParam pThrParam = new TMEESUTThreadParam;
	memset(pThrParam, 0, sizeof(TMEESUTThreadParam));

	pThrParam->pCMEESUT = this;
	memcpy(&(pThrParam->TxnInput.m_MarketFeedTxnInput), pTxnInput,
			sizeof(TMarketFeedTxnInput));

	pThrParam->pCMEESUT->m_SocketLock.lock();
	bool result = RunMarketFeedAsync(reinterpret_cast<void *>(pThrParam));
	pThrParam->pCMEESUT->m_SocketLock.lock();

	return result;
}
