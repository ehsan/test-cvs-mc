/*-
 * Copyright (c) 2001-2007, by Cisco Systems, Inc. All rights reserved.
 * Copyright (c) 2008-2012, by Randall Stewart. All rights reserved.
 * Copyright (c) 2008-2012, by Michael Tuexen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * a) Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * b) Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the distribution.
 *
 * c) Neither the name of Cisco Systems, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <netinet/sctp_os.h>
#include <netinet/sctp_callout.h>
#include <netinet/sctp_pcb.h>

/*
 * Callout/Timer routines for OS that doesn't have them
 */
#if defined(__APPLE__) || defined(__Userspace__)
int ticks = 0;
#else
extern int ticks;
#endif

/*
 * SCTP_TIMERQ_LOCK protects:
 * - SCTP_BASE_INFO(callqueue)
 * - sctp_os_timer_current: current timer in process
 * - sctp_os_timer_next: next timer to check
 */
static sctp_os_timer_t *sctp_os_timer_current = NULL;
static sctp_os_timer_t *sctp_os_timer_next = NULL;

void
sctp_os_timer_init(sctp_os_timer_t *c)
{
	bzero(c, sizeof(*c));
}

void
sctp_os_timer_start(sctp_os_timer_t *c, int to_ticks, void (*ftn) (void *),
                    void *arg)
{
	/* paranoia */
	if ((c == NULL) || (ftn == NULL))
	    return;

	SCTP_TIMERQ_LOCK();
	/* check to see if we're rescheduling a timer */
	if (c->c_flags & SCTP_CALLOUT_PENDING) {
		if (c == sctp_os_timer_next) {
			sctp_os_timer_next = TAILQ_NEXT(c, tqe);
		}
		TAILQ_REMOVE(&SCTP_BASE_INFO(callqueue), c, tqe);
		/*
		 * part of the normal "stop a pending callout" process
		 * is to clear the CALLOUT_ACTIVE and CALLOUT_PENDING
		 * flags.  We don't bother since we are setting these
		 * below and we still hold the lock.
		 */
	}

	/*
	 * We could unlock/splx here and lock/spl at the TAILQ_INSERT_TAIL,
	 * but there's no point since doing this setup doesn't take much time.
	 */
	if (to_ticks <= 0)
		to_ticks = 1;

	c->c_arg = arg;
	c->c_flags = (SCTP_CALLOUT_ACTIVE | SCTP_CALLOUT_PENDING);
	c->c_func = ftn;
	c->c_time = ticks + to_ticks;
	TAILQ_INSERT_TAIL(&SCTP_BASE_INFO(callqueue), c, tqe);
	SCTP_TIMERQ_UNLOCK();
}

int
sctp_os_timer_stop(sctp_os_timer_t *c)
{
	SCTP_TIMERQ_LOCK();
	/*
	 * Don't attempt to delete a callout that's not on the queue.
	 */
	if (!(c->c_flags & SCTP_CALLOUT_PENDING)) {
		c->c_flags &= ~SCTP_CALLOUT_ACTIVE;
		SCTP_TIMERQ_UNLOCK();
		return (0);
	}
	c->c_flags &= ~(SCTP_CALLOUT_ACTIVE | SCTP_CALLOUT_PENDING);
	if (c == sctp_os_timer_next) {
		sctp_os_timer_next = TAILQ_NEXT(c, tqe);
	}
	TAILQ_REMOVE(&SCTP_BASE_INFO(callqueue), c, tqe);
	SCTP_TIMERQ_UNLOCK();
	return (1);
}

#if defined(__APPLE__)
/*
 * For __APPLE__, use a single main timer at a faster resolution than
 * fastim.  The timer just calls this existing callout infrastructure.
 */
#endif
void
sctp_timeout(void *arg SCTP_UNUSED)
{
	sctp_os_timer_t *c;
	void (*c_func)(void *);
	void *c_arg;

#if defined(__APPLE__)
	/* update our tick count */
	ticks += SCTP_BASE_VAR(sctp_main_timer_ticks);
#endif

	SCTP_TIMERQ_LOCK();
	c = TAILQ_FIRST(&SCTP_BASE_INFO(callqueue));
	while (c) {
		if (c->c_time <= ticks) {
			sctp_os_timer_next = TAILQ_NEXT(c, tqe);
			TAILQ_REMOVE(&SCTP_BASE_INFO(callqueue), c, tqe);
			c_func = c->c_func;
			c_arg = c->c_arg;
			c->c_flags &= ~SCTP_CALLOUT_PENDING;
			sctp_os_timer_current = c;
			SCTP_TIMERQ_UNLOCK();
			c_func(c_arg);
			SCTP_TIMERQ_LOCK();
			sctp_os_timer_current = NULL;
			c = sctp_os_timer_next;
		} else {
			c = TAILQ_NEXT(c, tqe);
		}
	}
	sctp_os_timer_next = NULL;
	SCTP_TIMERQ_UNLOCK();

#if defined(__APPLE__)
	/* restart the main timer */
	sctp_start_main_timer();
#endif
}
