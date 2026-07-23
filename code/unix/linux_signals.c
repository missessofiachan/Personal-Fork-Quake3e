/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2011-2026 Quake3e project

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <execinfo.h>
#include <dlfcn.h>
#endif

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

static qboolean signalcaught = qfalse;

extern void NORETURN Sys_Exit( int code );

static const char *Sys_SignalName( int sig ) {
	switch ( sig ) {
		case SIGSEGV: return "SIGSEGV (Segmentation Fault)";
		case SIGFPE:  return "SIGFPE (Floating Point Exception)";
		case SIGILL:  return "SIGILL (Illegal Instruction)";
		case SIGABRT: return "SIGABRT (Abort)";
		case SIGBUS:  return "SIGBUS (Bus Error)";
		case SIGTERM: return "SIGTERM (Termination Request)";
		case SIGHUP:  return "SIGHUP (Hangup)";
		case SIGQUIT: return "SIGQUIT (Quit)";
		default:      return "Unknown Signal";
	}
}

static void signal_handler( int sig )
{
	char logBuf[4096];
	int offset = 0;

	if ( signalcaught == qtrue )
	{
		printf( "DOUBLE SIGNAL FAULT: Received signal %d (%s), exiting...\n", sig, Sys_SignalName( sig ) );
		Sys_Exit( 1 );
	}

	signalcaught = qtrue;

	offset += snprintf( logBuf + offset, sizeof( logBuf ) - offset,
		"=================== CRASH LOG TELEMETRY ===================\n"
		"Signal: %s (Code %d)\n"
		"Uptime: %dms\n",
		Sys_SignalName( sig ), sig,
		Sys_Milliseconds() );

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
	void *callstack[32];
	int frames = backtrace( callstack, 32 );
	char **strs = backtrace_symbols( callstack, frames );

	if ( strs && frames > 0 ) {
		offset += snprintf( logBuf + offset, sizeof( logBuf ) - offset,
			"\nCallstack / Backtrace (%d frames):\n", frames );
		for ( int i = 0; i < frames; i++ ) {
			Dl_info dlinfo;
			if ( dladdr( callstack[i], &dlinfo ) && dlinfo.dli_sname ) {
				offset += snprintf( logBuf + offset, sizeof( logBuf ) - offset,
					"  [%02d] %s (%s + 0x%tx) [%p]\n",
					i,
					dlinfo.dli_fname ? dlinfo.dli_fname : strs[i],
					dlinfo.dli_sname,
					(char *)callstack[i] - (char *)dlinfo.dli_saddr,
					callstack[i] );
			} else if ( dladdr( callstack[i], &dlinfo ) && dlinfo.dli_fname ) {
				offset += snprintf( logBuf + offset, sizeof( logBuf ) - offset,
					"  [%02d] %s (+0x%tx) [%p]\n",
					i,
					dlinfo.dli_fname,
					(char *)callstack[i] - (char *)dlinfo.dli_fbase,
					callstack[i] );
			} else {
				offset += snprintf( logBuf + offset, sizeof( logBuf ) - offset,
					"  [%02d] %s\n", i, strs[i] );
			}
		}
		free( strs );
	}
#endif

	offset += snprintf( logBuf + offset, sizeof( logBuf ) - offset,
		"===========================================================\n" );

	fprintf( stderr, "\n%s\n", logBuf );

	FILE *f = fopen( "crash.log", "a" );
	if ( f ) {
		fputs( logBuf, f );
		fputs( "\n", f );
		fclose( f );
	}

	char msg[64];
	Com_sprintf( msg, sizeof( msg ), "Signal caught (%d)", sig );
	VM_Forced_Unload_Start();
#ifndef DEDICATED
	CL_Shutdown( msg, qtrue );
#endif
	SV_Shutdown( msg );
	VM_Forced_Unload_Done();
	Sys_Exit( 0 );
}

void InitSig( void )
{
	signal( SIGINT, SIG_IGN );
	signal( SIGHUP, signal_handler );
	signal( SIGQUIT, signal_handler );
	signal( SIGILL, signal_handler );
	signal( SIGTRAP, signal_handler );
	signal( SIGIOT, signal_handler );
	signal( SIGBUS, signal_handler );
	signal( SIGFPE, signal_handler );
	signal( SIGSEGV, signal_handler );
	signal( SIGTERM, signal_handler );
}
