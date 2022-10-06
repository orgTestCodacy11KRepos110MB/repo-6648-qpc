/*$file${src::qxk::qxk.c} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: qpc.qm
* File:  ${src::qxk::qxk.c}
*
* This code has been generated by QM 5.2.2 <www.state-machine.com/qm>.
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This code is covered by the following QP license:
* License #    : LicenseRef-QL-dual
* Issued to    : Any user of the QP/C real-time embedded framework
* Framework(s) : qpc
* Support ends : 2023-12-31
* License scope:
*
* Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
*
* SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
*
* This software is dual-licensed under the terms of the open source GNU
* General Public License version 3 (or any later version), or alternatively,
* under the terms of one of the closed source Quantum Leaps commercial
* licenses.
*
* The terms of the open source GNU General Public License version 3
* can be found at: <www.gnu.org/licenses/gpl-3.0>
*
* The terms of the closed source Quantum Leaps commercial licenses
* can be found at: <www.state-machine.com/licensing>
*
* Redistributions in source code must retain this top-level comment block.
* Plagiarizing this software to sidestep the license obligations is illegal.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
*/
/*$endhead${src::qxk::qxk.c} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*! @file
* @brief QXK preemptive dual-mode kernel core functions
*/
#define QP_IMPL           /* this is QP implementation */
#include "qf_port.h"      /* QF port */
#include "qf_pkg.h"       /* QF package-scope internal interface */
#include "qassert.h"      /* QP embedded systems-friendly assertions */
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_port.h"  /* QS port */
    #include "qs_pkg.h"   /* QS facilities for pre-defined trace records */
#else
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

/* protection against including this source file in a wrong project */
#ifndef QP_INC_QXK_H_
    #error "Source file included in a project NOT based on the QXK kernel"
#endif /* QP_INC_QXK_H_ */

Q_DEFINE_THIS_MODULE("qxk")

/*==========================================================================*/
/*$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/* Check for the minimum required QP version */
#if (QP_VERSION < 690U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpc version 6.9.0 or higher required
#endif
/*$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*$define${QXK::QXK-base} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QXK::QXK-base::schedLock} ..............................................*/
QSchedStatus QXK_schedLock(uint_fast8_t const ceiling) {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    /*! @pre The QXK scheduler lock cannot be called from an ISR */
    Q_REQUIRE_ID(400, !QXK_ISR_CONTEXT_());

    QSchedStatus stat; /* saved lock status to be returned */

    /* is the lock ceiling being raised? */
    if (ceiling > (uint_fast8_t)QXK_attr_.lockCeil) {
        QS_BEGIN_NOCRIT_PRE_(QS_SCHED_LOCK, 0U)
            QS_TIME_PRE_(); /* timestamp */
            /* the previous lock ceiling & new lock ceiling */
            QS_2U8_PRE_(QXK_attr_.lockCeil, (uint8_t)ceiling);
        QS_END_NOCRIT_PRE_()

        /* previous status of the lock */
        stat  = (QSchedStatus)QXK_attr_.lockHolder;
        stat |= (QSchedStatus)QXK_attr_.lockCeil << 8U;

        /* new status of the lock */
        QXK_attr_.lockHolder = (QXK_attr_.curr != (QActive *)0)
                               ? QXK_attr_.curr->prio
                               : 0U;
        QXK_attr_.lockCeil   = (uint8_t)ceiling;
    }
    else {
       stat = 0xFFU; /* scheduler not locked */
    }
    QF_CRIT_X_();

    return stat; /* return the status to be saved in a stack variable */
}

/*${QXK::QXK-base::schedUnlock} ............................................*/
void QXK_schedUnlock(QSchedStatus const stat) {
    /* has the scheduler been actually locked by the last QXK_schedLock()? */
    if (stat != 0xFFU) {
        uint8_t const lockCeil = QXK_attr_.lockCeil;
        uint8_t const prevCeil = (uint8_t)(stat >> 8U);
        QF_CRIT_STAT_
        QF_CRIT_E_();

        /*! @pre The scheduler cannot be unlocked:
        * - from the ISR context; and
        * - the current lock ceiling must be greater than the previous
        */
        Q_REQUIRE_ID(500, (!QXK_ISR_CONTEXT_())
                          && (lockCeil > prevCeil));

        QS_BEGIN_NOCRIT_PRE_(QS_SCHED_UNLOCK, 0U)
            QS_TIME_PRE_(); /* timestamp */
            /* ceiling before unlocking & prio after unlocking */
            QS_2U8_PRE_(lockCeil, prevCeil);
        QS_END_NOCRIT_PRE_()

        /* restore the previous lock ceiling and lock holder */
        QXK_attr_.lockCeil   = prevCeil;
        QXK_attr_.lockHolder = (uint8_t)(stat & 0xFFU);

        /* find if any threads should be run after unlocking the scheduler */
        if (QXK_sched_() != 0U) { /* activation needed? */
            QXK_activate_(); /* synchronously activate basic-thred(s) */
        }

        QF_CRIT_X_();
    }
}
/*$enddef${QXK::QXK-base} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${QXK::QF-cust} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QXK::QF-cust::init} ....................................................*/
void QF_init(void) {
    #if (QF_MAX_EPOOL > 0U)
    QF_maxPool_ = 0U;
    #endif

    QF_bzero(&QTimeEvt_timeEvtHead_[0], sizeof(QTimeEvt_timeEvtHead_));
    QF_bzero(&QActive_registry_[0],     sizeof(QActive_registry_));
    QF_bzero(&QF_readySet_,             sizeof(QF_readySet_));
    QF_bzero(&QXK_attr_,                sizeof(QXK_attr_));

    /* setup the QXK scheduler as initially locked and not running */
    QXK_attr_.lockCeil = (QF_MAX_ACTIVE + 1U); /* scheduler locked */

    /* register the QXK idle AO object, cast "const" away */
    static QActive const idle_ao = { (struct QHsmVtable const *)0 };
    QActive_registry_[0] = QF_CONST_CAST_(QActive*, &idle_ao);

    #ifdef QXK_INIT
    QXK_INIT(); /* port-specific initialization of the QXK kernel */
    #endif
}

/*${QXK::QF-cust::stop} ....................................................*/
void QF_stop(void) {
    QF_onCleanup(); /* application-specific cleanup callback */
    /* nothing else to do for the cooperative QXK kernel */
}

/*${QXK::QF-cust::run} .....................................................*/
int_t QF_run(void) {
    QF_INT_DISABLE();
    QXK_attr_.prev = QActive_registry_[0]; /* the QXK idle thread */
    QXK_attr_.lockCeil = 0U; /* unlock the scheduler */

    /* any active objects need to be scheduled before starting event loop? */
    if (QXK_sched_() != 0U) { /* activation needed? */
        QXK_activate_(); /* synchronously activate basic-thred(s) */
    }

    QF_onStartup(); /* application-specific startup callback */

    /* produce the QS_QF_RUN trace record (no nested critical section) */
    QS_BEGIN_NOCRIT_PRE_(QS_QF_RUN, 0U)
    QS_END_NOCRIT_PRE_()

    QF_INT_ENABLE();

    /* the QXK idle loop... */
    for (;;) {
        QXK_onIdle(); /* application-specific QXK idle callback */
    }

    #ifdef __GNUC__
    return 0;
    #endif
}
/*$enddef${QXK::QF-cust} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${QXK::QActive} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QXK::QActive} ..........................................................*/

/*${QXK::QActive::start_} ..................................................*/
void QActive_start_(QActive * const me,
    QPrioSpec const prioSpec,
    QEvt const * * const qSto,
    uint_fast16_t const qLen,
    void * const stkSto,
    uint_fast16_t const stkSize,
    void const * const par)
{
    Q_UNUSED_PAR(stkSto);  /* not needed in QXK */
    Q_UNUSED_PAR(stkSize); /* not needed in QXK */

    /*! @pre AO cannot be started:
    * - from an ISR;
    * - the stack storage must NOT be provided (because the QXK kernel
    *   does not need per-AO stacks)
    * - preemption-threshold is NOT provided (because QXK kernel
    *   does not support preemption-threshold scheduling)
    */
    Q_REQUIRE_ID(200, (!QXK_ISR_CONTEXT_())
                      && (stkSto == (void *)0)
                      && ((prioSpec & 0xFF00U) == 0U));

    me->prio  = (uint8_t)(prioSpec & 0xFFU); /* QF-priority of the AO */
    me->pthre = 0U; /* preemption-threshold NOT used */
    QActive_register_(me); /* make QF aware of this active object */

    QEQueue_init(&me->eQueue, qSto, qLen);  /* init the built-in queue */
    me->osObject = (void *)0;  /* no private stack for the AO */

    QHSM_INIT(&me->super, par, me->prio); /* top-most initial tran. */
    QS_FLUSH(); /* flush the trace buffer to the host */

    /* see if this AO needs to be scheduled if QXK is already running */
    QF_CRIT_STAT_
    QF_CRIT_E_();
    if (QXK_attr_.lockCeil <= QF_MAX_ACTIVE) { /* scheduler running? */
        if (QXK_sched_() != 0U) { /* activation needed? */
            QXK_activate_(); /* synchronously activate basic-thred(s) */
        }
    }
    QF_CRIT_X_();
}
/*$enddef${QXK::QActive} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*$define${QXK::QXK-extern-C} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QXK::QXK-extern-C::attr_} ..............................................*/
QXK QXK_attr_;

/*${QXK::QXK-extern-C::sched_} .............................................*/
uint_fast8_t QXK_sched_(void) {
    uint_fast8_t p;

    if (QPSet_isEmpty(&QF_readySet_)) {
        p = 0U; /* no activation needed */
    }
    else {
        /* find the highest-prio thread ready to run */
        p = QPSet_findMax(&QF_readySet_);
        if (p <= QXK_attr_.lockCeil) {
            /* priority of the thread holding the lock */
            p = (uint_fast8_t)QActive_registry_[QXK_attr_.lockHolder]->prio;
            if (p != 0U) {
                Q_ASSERT_ID(610, QPSet_hasElement(&QF_readySet_, p));
            }
        }
    }
    QActive const * const curr = QXK_attr_.curr;
    QActive * const next = QActive_registry_[p];

    /* the next thread found must be registered in QF */
    Q_ASSERT_ID(620, next != (QActive *)0);

    /* is the current thread a basic-thread? */
    if (curr == (QActive *)0) {

        /* is the new priority above the active priority? */
        if (p > QXK_attr_.actPrio) {
            QXK_attr_.next = next; /* set the next AO to activate */

            if (next->osObject != (void *)0) { /* is next extended? */
                QXK_CONTEXT_SWITCH_();
                p = 0U; /* no activation needed */
            }
        }
        else { /* below the pre-thre */
            QXK_attr_.next = (QActive *)0;
            p = 0U; /* no activation needed */
        }
    }
    else { /* currently executing an extended-thread */
        /* is the current thread different from the next? */
        if (curr != next) {
            QXK_attr_.next = next;
            QXK_CONTEXT_SWITCH_();
        }
        else { /* current is the same as next */
            QXK_attr_.next = (QActive *)0; /* no need to context-switch */
        }
        p = 0U; /* no activation needed */
    }
    return p;
}

/*${QXK::QXK-extern-C::activate_} ..........................................*/
void QXK_activate_(void) {
    uint8_t const prio_in = QXK_attr_.actPrio;
    QActive *next = QXK_attr_.next; /* the next AO (basic-thread) to run */

    /*! @pre QXK_attr_.next must be valid and the prio must be in range */
    Q_REQUIRE_ID(700, (next != (QActive *)0) && (prio_in <= QF_MAX_ACTIVE));

    /* QXK Context switch callback defined or QS tracing enabled? */
    #if (defined QXK_ON_CONTEXT_SW) || (defined Q_SPY)
    QXK_contextSw(next);
    #endif /* QXK_ON_CONTEXT_SW || Q_SPY */

    QXK_attr_.next = (QActive *)0; /* clear the next AO */
    QXK_attr_.curr = (QActive *)0; /* current is basic-thread */

    /* priority of the next thread */
    uint_fast8_t p = (uint_fast8_t)next->prio;

    /* loop until no more ready-to-run AOs of higher prio than the initial */
    do  {
        QXK_attr_.actPrio = (uint8_t)p; /* next active prio */

        QF_INT_ENABLE(); /* unconditionally enable interrupts */

        /* perform the run-to-completion (RTC) step...
        * 1. retrieve the event from the AO's event queue, which by this
        *    time must be non-empty and QActive_get_() asserts it.
        * 2. dispatch the event to the AO's state machine.
        * 3. determine if event is garbage and collect it if so
        */
        QEvt const * const e = QActive_get_(next);
        QHSM_DISPATCH(&next->super, e, next->prio);
    #if (QF_MAX_EPOOL > 0U)
        QF_gc(e);
    #endif

        QF_INT_DISABLE(); /* unconditionally disable interrupts */

        if (next->eQueue.frontEvt == (QEvt *)0) { /* empty queue? */
            QPSet_remove(&QF_readySet_, p);
        }

        /* find new highest-prio AO ready to run... */
        p = QPSet_findMax(&QF_readySet_);
        next = QActive_registry_[p];

        /* next thread must be registered in QF */
        Q_ASSERT_ID(710, next != (QActive *)0);

        /* is the next priority below the lock-ceiling? */
        if (p <= (uint_fast8_t)QXK_attr_.lockCeil) {
            p = QXK_attr_.lockHolder; /* thread holding lock */
            if (p != 0U) {
                Q_ASSERT_ID(720, QPSet_hasElement(&QF_readySet_, p));
            }
        }

        /* is the next a basic thread? */
        if (next->osObject == (void *)0) {
            /* is the next priority above the initial priority? */
            if (p > QActive_registry_[prio_in]->prio) {
    #if (defined QXK_ON_CONTEXT_SW) || (defined Q_SPY)
                if (p != QXK_attr_.actPrio) {  /* changing threads? */
                    QXK_contextSw(next);
                }
    #endif /* QXK_ON_CONTEXT_SW || Q_SPY */
                QXK_attr_.next = next;
            }
            else {
                QXK_attr_.next = (QActive *)0;
                p = 0U; /* no activation needed */
            }
        }
        else {  /* next is the extended-thread */
            QXK_attr_.next = next;
            QXK_CONTEXT_SWITCH_();
            p = 0U; /* no activation needed */
        }
    } while (p != 0U); /* while activation needed */

    /* restore the active priority */
    QXK_attr_.actPrio = prio_in;

    #if (defined QXK_ON_CONTEXT_SW) || (defined Q_SPY)
    if (next->osObject == (void *)0) {
        QXK_contextSw((prio_in == 0U)
                       ? (QActive *)0
                       : QActive_registry_[prio_in]);
    }
    #endif /* QXK_ON_CONTEXT_SW || Q_SPY */
}

/*${QXK::QXK-extern-C::current} ............................................*/
QActive * QXK_current(void) {
    /*! @pre the QXK kernel must be running */
    Q_REQUIRE_ID(800, QXK_attr_.lockCeil <= QF_MAX_ACTIVE);

    QF_CRIT_STAT_
    QF_CRIT_E_();

    struct QActive *curr = QXK_attr_.curr;
    if (curr == (QActive *)0) { /* basic thread? */
        curr = QActive_registry_[QXK_attr_.actPrio];
    }
    QF_CRIT_X_();

    /*! @post the current thread must be valid */
    Q_ENSURE_ID(890, curr != (QActive *)0);

    return curr;
}

/*${QXK::QXK-extern-C::contextSw} ..........................................*/
#if defined(Q_SPY) || defined(QXK_ON_CONTEXT_SW)
void QXK_contextSw(QActive * const next) {
    uint8_t prev_prio = (QXK_attr_.prev != (QActive *)0)
                        ? QXK_attr_.prev->prio
                        : 0U;
    uint8_t next_prio = (next != (QActive *)0)
                        ? next->prio
                        : QXK_attr_.actPrio;

    if (next_prio == 0U) { /* going to idle? */
        QS_BEGIN_NOCRIT_PRE_(QS_SCHED_IDLE, 0U)
            QS_TIME_PRE_(); /* timestamp */
            QS_U8_PRE_(prev_prio);
        QS_END_NOCRIT_PRE_()

    #ifdef QXK_ON_CONTEXT_SW
        QXK_onContextSw(QXK_attr_.prev, (QActive *)0);
    #endif /* QXK_ON_CONTEXT_SW */
    }
    else {
        QS_BEGIN_NOCRIT_PRE_(QS_SCHED_NEXT, next_prio)
            QS_TIME_PRE_(); /* timestamp */
            QS_2U8_PRE_(next_prio, prev_prio);
        QS_END_NOCRIT_PRE_()

    #ifdef QXK_ON_CONTEXT_SW
        QXK_onContextSw(QXK_attr_.prev, next);
    #endif /* QXK_ON_CONTEXT_SW */
    }

    QXK_attr_.prev = next; /* update the previous thread */
}
#endif /*  defined(Q_SPY) || defined(QXK_ON_CONTEXT_SW) */
/*$enddef${QXK::QXK-extern-C} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*==========================================================================*/
/*$define${QXK-impl} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QXK-impl::QXK_threadExit_} .............................................*/
void QXK_threadExit_(void) {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QXThread const * const thr = QXTHREAD_CAST_(QXK_attr_.curr);

    /*! @pre this function must:
    * - NOT be called from an ISR;
    * - be called from an extended thread;
    */
    Q_REQUIRE_ID(900, (!QXK_ISR_CONTEXT_()) /* can't be in the ISR context */
        && (thr != (QXThread *)0)); /* current thread must be extended */
    /*! @pre also: the thread must NOT be holding a scheduler lock. */
    Q_REQUIRE_ID(901, QXK_attr_.lockHolder != thr->super.prio);

    uint_fast8_t const p = (uint_fast8_t)thr->super.prio;

    /* remove this thread from the QF */
    QActive_registry_[p] = (QActive *)0;
    QPSet_remove(&QF_readySet_, p);
    (void)QXK_sched_(); /* schedule other threads */
    QF_CRIT_X_();
}
/*$enddef${QXK-impl} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
