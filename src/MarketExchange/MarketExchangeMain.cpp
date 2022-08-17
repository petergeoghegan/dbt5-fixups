/*
 * This file is released under the terms of the Artistic License.  Please see
 * the file LICENSE, included in this package, for details.
 *
 * Copyright (C) 2006 Rilson Nascimento
 *               2010 Mark Wong
 *
 * executable that opens the Market Exchange to business
 * 30 July 2006
 */

#include "MarketExchange.h"
#include "DBT5Consts.h"

// Establish defaults for command line options
char szBHaddr[iMaxHostname + 1] = "localhost"; // Brokerage House address
int iListenPort = iMarketExchangePort; // socket port to listen
int iBHlistenPort = iBrokerageHousePort;
// # of customers for this instance
TIdent iConfiguredCustomerCount = iDefaultCustomerCount;
// total number of customers in the database
TIdent iActiveCustomerCount = iDefaultCustomerCount;

// EGen flat_ing directory location
char szFileLoc[iMaxPath + 1];
// path to output files
char outputDirectory[iMaxPath + 1] = ".";

// shows program usage
void usage()
{
	cout << "Usage: MarketExchangeMain [options]" << endl << endl;
	cout << "   Option      Default     Description" << endl;
	cout << "   ==========  ==========  =============================" << endl;
	printf("   -c integer  %-10ld  Configured customer count\n",
			iConfiguredCustomerCount);
	cout << "   -i string               Location of EGen flat_in directory" <<
			endl;
	printf("   -l integer  %-10d  Socket listen port\n", iListenPort);
	printf("   -h string   %-10s  Brokerage House address\n", szBHaddr);
	printf("   -o string   %-10s  directory for output files\n",
			outputDirectory);
	printf("   -t integer  %-10ld  Active customer count\n",
			iActiveCustomerCount);
	printf("   -p integer  %-10d  Brokerage House listen port\n",
			iBHlistenPort);
}

// Parse command line
void parse_command_line(int argc, char *argv[])
{
	int arg;
	char *sp;
	char *vp;

	// Scan the command line arguments
	for (arg = 1; arg < argc; ++arg) {

		// Look for a switch 
		sp = argv[arg];
		if (*sp == '-') {
			++sp;
		}
		*sp = (char)tolower(*sp);

		/*
		 *  Find the switch's argument.  It is either immediately after the
		 *  switch or in the next argv
		 */
		vp = sp + 1;
		// Allow for switched that don't have any parameters.
		// Need to check that the next argument is in fact a parameter
		// and not the next switch that starts with '-'.
		//
		if ((*vp == 0) && ((arg + 1) < argc) && (argv[arg + 1][0] != '-')) {
			vp = argv[++arg];
		}

		// Parse the switch
		switch (*sp) {
		case 'c':
			iActiveCustomerCount = atol(vp);
			break;
		case 'h':
			strncpy(szBHaddr, vp, iMaxHostname);
			break;
		case 'i':
			strncpy(szFileLoc, vp, iMaxPath);
			break;
		case 'l':
			iListenPort = atoi(vp);
			break;
		case 'o':
			strncpy(outputDirectory, vp, iMaxPath);
			break;
		case 'p':
			sscanf(vp, "%d", &iBHlistenPort);
			break;
		case 't':
			iConfiguredCustomerCount = atol(vp);
			break;
		default:
			usage();
			cout << endl << "Error: Unrecognized option: " << sp << endl;
			exit(ERROR_BAD_OPTION);
		}
	}
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <execinfo.h>
#include <signal.h>
#include <string.h>

#include <iostream>
#include <cstdlib>
#include <stdexcept>

void my_terminate(void);

namespace {
    // invoke set_terminate as part of global constant initialization
    static const bool SET_TERMINATE = std::set_terminate(my_terminate);
}

// This structure mirrors the one found in /usr/include/asm/ucontext.h
typedef struct _sig_ucontext {
   unsigned long     uc_flags;
   struct ucontext   *uc_link;
   stack_t           uc_stack;
   struct sigcontext uc_mcontext;
   sigset_t          uc_sigmask;
} sig_ucontext_t;

void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext) {
    sig_ucontext_t * uc = (sig_ucontext_t *)ucontext;

    // Get the address at the time the signal was raised from the EIP (x86)
    void * caller_address = (void *) uc->uc_mcontext.rip;

    
    std::cerr << "signal " << sig_num 
              << " (" << strsignal(sig_num) << "), address is " 
              << info->si_addr << " from " 
              << caller_address << std::endl;

    void * array[50];
    int size = backtrace(array, 50);

    std::cerr << __FUNCTION__ << " backtrace returned " 
              << size << " frames\n\n";

    // overwrite sigaction with caller's address
    array[1] = caller_address;

    char ** messages = backtrace_symbols(array, size);

    // skip first stack frame (points here)
    for (int i = 1; i < size && messages != NULL; ++i) {
        std::cerr << "[bt]: (" << i << ") " << messages[i] << std::endl;
    }
    std::cerr << std::endl;

    free(messages);

    exit(EXIT_FAILURE);
}

void my_terminate() {
    static bool tried_throw = false;

    try {
        // try once to re-throw currently active exception
        if (!tried_throw++) throw;
    }
    catch (const std::exception &e) {
        std::cerr << __FUNCTION__ << " caught unhandled exception. what(): "
                  << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << __FUNCTION__ << " caught unknown/unhandled exception." 
                  << std::endl;
    }

    void * array[50];
    int size = backtrace(array, 50);    

    std::cerr << __FUNCTION__ << " backtrace returned " 
              << size << " frames\n\n";

    char ** messages = backtrace_symbols(array, size);

    for (int i = 0; i < size && messages != NULL; ++i) {
        std::cerr << "[bt]: (" << i << ") " << messages[i] << std::endl;
    }
    std::cerr << std::endl;

    free(messages);

    abort();
}

int main(int argc, char *argv[])
{
    struct sigaction sigact;

    sigact.sa_sigaction = crit_err_hdlr;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    if (sigaction(SIGABRT, &sigact, (struct sigaction *)NULL) != 0) {
        std::cerr << "error setting handler for signal " << SIGABRT 
                  << " (" << strsignal(SIGABRT) << ")\n";
        exit(EXIT_FAILURE);
    }

	// Establish defaults for command line options
	strncpy(szFileLoc, "flat_in", iMaxPath);

	cout << "dbt5 - Market Exchange Main" << endl;
	cout << "Listener port: " << iListenPort << endl << endl;

	// Parse command line
	parse_command_line(argc, argv);

	// Let the user know what settings will be used.
	cout << "Using the following settings:" << endl << endl;
	cout << "EGen flat_in directory location: " << szFileLoc << endl;
	cout << "Configured customer count: " << iConfiguredCustomerCount << endl;
	cout << "Active customer count: " << iActiveCustomerCount << endl;
	cout << "Brokerage House address: " << szBHaddr << endl;
	cout << "Brokerage House port: " << iBHlistenPort << endl;

	try {
		CMarketExchange MarketExchange(szFileLoc, iConfiguredCustomerCount,
				iActiveCustomerCount, iListenPort, szBHaddr, iBHlistenPort,
				outputDirectory);
		cout << "Market Exchange started, waiting for trade requests..." <<
				endl;

		MarketExchange.startListener();
	} 
	catch (CBaseErr *pErr) {
		cout << "Error " << pErr->ErrorNum() << ": " <<
				pErr->ErrorText();
		if (pErr->ErrorLoc()) {
			cout << " at " << pErr->ErrorLoc();
		}
		cout << endl;
		return 1;
	} catch (std::bad_alloc err) {
		// operator new will throw std::bad_alloc exception if there is no
		// sufficient memory for the request.
		cout << "*** Out of memory ***" << endl;
		return 2;
	}

	pthread_exit(NULL);

	cout << "Market Exchange closed for business." << endl;
	return(0);
}
