/*$file${src::qxk::qxk_xthr.c} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: qpc.qm
* File:  ${src::qxk::qxk_xthr.c}
*
* This code has been generated by QM 5.2.5 <www.state-machine.com/qm>.
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
/*$endhead${src::qxk::qxk_xthr.c} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*! @file
* @brief QXK preemptive kernel extended (blocking) thread functions
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

Q_DEFINE_THIS_MODULE("qxk_xthr")

/*==========================================================================*/
/*$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/* Check for the minimum required QP version */
#if (QP_VERSION < 700U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpc version 7.0.0 or higher required
#endif
/*$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*$define${QXK::QXThread} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QXK::QXThread} .........................................................*/
QXThread const * QXThread_dummy;

/*${QXK::QXThread::ctor} ...................................................*/
void QXThread_ctor(QXThread * const me,
    QXThreadHandler const handler,
    uint_fast8_t const tickRate)
{
    static QXThreadVtable const vtable = { /* QXThread virtual table */
        { &QXThread_init_,       /* not used in QXThread */
          &QXThread_dispatch_    /* not used in QXThread */
    #ifdef Q_SPY
         ,&QHsm_getStateHandler_ /* not used in QXThread */
    #endif
        },
        &QXThread_start_,
        &QXThread_post_,
        &QXThread_postLIFO_
    };
    union QHsmAttr tmp;
    tmp.thr = handler;

    QActive_ctor(&me->super, tmp.fun); /* superclass' ctor */
    me->super.super.vptr = &vtable.super; /* hook to QXThread vtable */
    me->super.super.state.act = Q_ACTION_CAST(0); /*mark as extended thread */

    /* construct the time event member added in the QXThread class */
    QTimeEvt_ctorX(&me->timeEvt, &me->super,
                   (enum_t)QXK_DELAY_SIG, tickRate);
}

/*${QXK::QXThread::delay} ..................................................*/
bool QXThread_delay(uint_fast16_t const nTicks) {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QXThread * const thr = QXTHREAD_CAST_(QXK_attr_.curr);

    /*! @pre this function must:
    * - NOT be called from an ISR;
    * - number of ticks cannot be zero
    * - be called from an extended thread;
    * - the thread must NOT be already blocked on any object.
    */
    Q_REQUIRE_ID(800, (!QXK_ISR_CONTEXT_()) /* can't block inside an ISR */
        && (nTicks != 0U) /* number of ticks cannot be zero */
        && (thr != (QXThread *)0) /* current thread must be extended */
        && (thr->super.super.temp.obj == (QMState *)0)); /* !blocked */
    /*! @pre also: the thread must NOT be holding a scheduler lock. */
    Q_REQUIRE_ID(801, QXK_attr_.lockHolder != thr->super.prio);

    /* remember the blocking object */
    thr->super.super.temp.obj = QXK_PTR_CAST_(QMState const*, &thr->timeEvt);
    QXThread_teArm_(thr, (enum_t)QXK_DELAY_SIG, nTicks);
    QXThread_block_(thr);
    QF_CRIT_X_();
    QF_CRIT_EXIT_NOP(); /* BLOCK here */

    QF_CRIT_E_();
    /* the blocking object must be the time event */
    Q_ENSURE_ID(890, thr->super.super.temp.obj
                          == QXK_PTR_CAST_(QMState const*, &thr->timeEvt));
    thr->super.super.temp.obj =  (QMState *)0; /* clear */
    QF_CRIT_X_();

    /* signal of zero means that the time event was posted without
    * being canceled.
    */
    return thr->timeEvt.super.sig == 0U;
}

/*${QXK::QXThread::delayCancel} ............................................*/
bool QXThread_delayCancel(QXThread * const me) {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    bool wasArmed;
    if (me->super.super.temp.obj == QXK_PTR_CAST_(QMState*, &me->timeEvt)) {
        wasArmed = QXThread_teDisarm_(me);
        QXThread_unblock_(me);
    }
    else {
        wasArmed = false;
    }
    QF_CRIT_X_();

    return wasArmed;
}

/*${QXK::QXThread::queueGet} ...............................................*/
QEvt const * QXThread_queueGet(uint_fast16_t const nTicks) {
    QF_CRIT_STAT_
    QF_CRIT_E_();

    QXThread * const thr = QXTHREAD_CAST_(QXK_attr_.curr);

    /*! @pre this function must:
    * - NOT be called from an ISR;
    * - be called from an extended thread;
    * - the thread must NOT be already blocked on any object.
    */
    Q_REQUIRE_ID(500, (!QXK_ISR_CONTEXT_()) /* can't block inside an ISR */
        && (thr != (QXThread *)0) /* current thread must be extended */
        && (thr->super.super.temp.obj == (QMState *)0)); /* !blocked */
    /*! @pre also: the thread must NOT be holding a scheduler lock. */
    Q_REQUIRE_ID(501, QXK_attr_.lockHolder != thr->super.prio);

    /* is the queue empty? */
    if (thr->super.eQueue.frontEvt == (QEvt *)0) {

        /* remember the blocking object (the thread's queue) */
        thr->super.super.temp.obj
            = QXK_PTR_CAST_(QMState const*, &thr->super.eQueue);

        QXThread_teArm_(thr, (enum_t)QXK_TIMEOUT_SIG, nTicks);
        QPSet_remove(&QF_readySet_, (uint_fast8_t)thr->super.prio);
        (void)QXK_sched_(); /* schedule other threads */
        QF_CRIT_X_();
        QF_CRIT_EXIT_NOP(); /* BLOCK here */

        QF_CRIT_E_();
        /* the blocking object must be this queue */
        Q_ASSERT_ID(510, thr->super.super.temp.obj
                        == QXK_PTR_CAST_(QMState const*, &thr->super.eQueue));
        thr->super.super.temp.obj = (QMState *)0; /* clear */
    }

    /* is the queue not empty? */
    QEvt const *e;
    if (thr->super.eQueue.frontEvt != (QEvt *)0) {
        e = thr->super.eQueue.frontEvt; /* remove from the front */
        QEQueueCtr const nFree= thr->super.eQueue.nFree + 1U;
        thr->super.eQueue.nFree = nFree; /* update the number of free */

        /* any events in the ring buffer? */
        if (nFree <= thr->super.eQueue.end) {

            /* remove event from the tail */
            thr->super.eQueue.frontEvt =
                thr->super.eQueue.ring[thr->super.eQueue.tail];
            if (thr->super.eQueue.tail == 0U) { /* need to wrap? */
                thr->super.eQueue.tail = thr->super.eQueue.end;  /* wrap */
            }
            --thr->super.eQueue.tail;

            QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_GET, thr->super.prio)
                QS_TIME_PRE_();      /* timestamp */
                QS_SIG_PRE_(e->sig); /* the signal of this event */
                QS_OBJ_PRE_(&thr->super); /* this active object */
                QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
                QS_EQC_PRE_(nFree);  /* number of free entries */
            QS_END_NOCRIT_PRE_()
        }
        else {
            thr->super.eQueue.frontEvt = (QEvt *)0; /* empty queue */

            /* all entries in the queue must be free (+1 for fronEvt) */
            Q_ASSERT_ID(520, nFree == (thr->super.eQueue.end + 1U));

            QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_GET_LAST, thr->super.prio)
                QS_TIME_PRE_();      /* timestamp */
                QS_SIG_PRE_(e->sig); /* the signal of this event */
                QS_OBJ_PRE_(&thr->super); /* this active object */
                QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
            QS_END_NOCRIT_PRE_()
        }
    }
    else { /* the queue is still empty -- the timeout must have fired */
         e = (QEvt *)0;
    }
    QF_CRIT_X_();

    return e;

}

/*${QXK::QXThread::init_} ..................................................*/
void QXThread_init_(
    QHsm * const me,
    void const * const par,
    uint_fast8_t const qs_id)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(par);
    Q_UNUSED_PAR(qs_id);

    Q_ERROR_ID(110);
}

/*${QXK::QXThread::dispatch_} ..............................................*/
void QXThread_dispatch_(
    QHsm * const me,
    QEvt const * const e,
    uint_fast8_t const qs_id)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(e);
    Q_UNUSED_PAR(qs_id);

    Q_ERROR_ID(120);
}

/*${QXK::QXThread::start_} .................................................*/
void QXThread_start_(
    QActive * const me,
    QPrioSpec const prioSpec,
    QEvt const * * const qSto,
    uint_fast16_t const qLen,
    void * const stkSto,
    uint_fast16_t const stkSize,
    void const * const par)
{
    Q_UNUSED_PAR(par);

    /*! @pre this function must:
    * - NOT be called from an ISR;
    * - the stack storage must be provided;
    * - the thread must be instantiated (see QXThread_ctor())
    * - preemption-threshold is NOT provided (because QXK kernel
    *   does not support preemption-threshold scheduling)
    */
    Q_REQUIRE_ID(200, (!QXK_ISR_CONTEXT_()) /* don't call from an ISR! */
        && (stkSto != (void *)0) /* stack must be provided */
        && (stkSize != 0U)
        && (me->super.state.act == (QActionHandler)0)
        && ((prioSpec & 0xFF00U) == 0U));

    /* is storage for the queue buffer provided? */
    if (qSto != (QEvt const **)0) {
        QEQueue_init(&me->eQueue, qSto, qLen);
    }

    /* extended thread constructor puts the thread handler in place of
    * the top-most initial transition 'me->super.temp.act'
    */
    QXK_stackInit_(me, me->super.temp.thr, stkSto, stkSize);

    /* the new thread is not blocked on any object */
    me->super.temp.obj = (QMState *)0;

    me->prio  = (uint8_t)(prioSpec & 0xFFU); /* QF-priority of the AO */
    me->pthre = 0U; /* preemption-threshold NOT used */
    QActive_register_(me); /* make QF aware of this active object */

    QF_CRIT_STAT_
    QF_CRIT_E_();
    /* extended-thread becomes ready immediately */
    QPSet_insert(&QF_readySet_, (uint_fast8_t)me->prio);

    /* see if this thread needs to be scheduled in case QXK is running */
    if (QXK_attr_.lockCeil <= QF_MAX_ACTIVE) {
        (void)QXK_sched_(); /* schedule other threads */
    }
    QF_CRIT_X_();
}

/*${QXK::QXThread::post_} ..................................................*/
bool QXThread_post_(
    QActive * const me,
    QEvt const * const e,
    uint_fast16_t const margin,
    void const * const sender)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(sender);
    #endif

    QF_CRIT_STAT_
    QS_TEST_PROBE_DEF(&QXThread_post_)

    /* is it the private time event? */
    bool status;
    if (e == &QXTHREAD_CAST_(me)->timeEvt.super) {
        QF_CRIT_E_();
        /* the private time event is disarmed and not in any queue,
        * so it is safe to change its signal. The signal of 0 means
        * that the time event has expired.
        */
        QXTHREAD_CAST_(me)->timeEvt.super.sig = 0U;

        QXThread_unblock_(QXTHREAD_CAST_(me));
        QF_CRIT_X_();

        status = true;
    }
    /* is the event queue provided? */
    else if (me->eQueue.end != 0U) {
        QEQueueCtr nFree; /* temporary to avoid UB for volatile access */

        /*! @pre event pointer must be valid */
        Q_REQUIRE_ID(300, e != (QEvt *)0);

        QF_CRIT_E_();
        nFree = me->eQueue.nFree; /* get volatile into the temporary */

        /* test-probe#1 for faking queue overflow */
        QS_TEST_PROBE_ID(1,
            nFree = 0U;
        )

        if (margin == QF_NO_MARGIN) {
            if (nFree > 0U) {
                status = true; /* can post */
            }
            else {
                status = false; /* cannot post */
                Q_ERROR_CRIT_(310); /* must be able to post the event */
            }
        }
        else if (nFree > (QEQueueCtr)margin) {
            status = true; /* can post */
        }
        else {
            status = false; /* cannot post, but don't assert */
        }

        /* is it a dynamic event? */
        if (e->poolId_ != 0U) {
            QEvt_refCtr_inc_(e); /* increment the reference counter */
        }

        if (status) { /* can post the event? */

            --nFree; /* one free entry just used up */
            me->eQueue.nFree = nFree;       /* update the volatile */
            if (me->eQueue.nMin > nFree) {
                me->eQueue.nMin = nFree;    /* update minimum so far */
            }

            QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST, me->prio)
                QS_TIME_PRE_();        /* timestamp */
                QS_OBJ_PRE_(sender);   /* the sender object */
                QS_SIG_PRE_(e->sig);   /* the signal of the event */
                QS_OBJ_PRE_(me);       /* this active object (recipient) */
                QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
                QS_EQC_PRE_(nFree);    /* number of free entries */
                QS_EQC_PRE_(me->eQueue.nMin); /* min number of free entries */
            QS_END_NOCRIT_PRE_()

            /* queue empty? */
            if (me->eQueue.frontEvt == (QEvt *)0) {
                me->eQueue.frontEvt = e; /* deliver event directly */

                /* is this thread blocked on the queue? */
                if (me->super.temp.obj
                    == QXK_PTR_CAST_(QMState*, &me->eQueue))
                {
                    (void)QXThread_teDisarm_(QXTHREAD_CAST_(me));
                    QPSet_insert(&QF_readySet_, (uint_fast8_t)me->prio);
                    if (!QXK_ISR_CONTEXT_()) {
                        (void)QXK_sched_(); /* schedule other threads */
                    }
                }
            }
            /* queue is not empty, insert event into the ring-buffer */
            else {
                /* insert event into the ring buffer (FIFO) */
                me->eQueue.ring[me->eQueue.head] = e;

                /* need to wrap the head counter? */
                if (me->eQueue.head == 0U) {
                    me->eQueue.head = me->eQueue.end;   /* wrap around */
                }
                --me->eQueue.head; /* advance the head (counter clockwise) */
            }

            QF_CRIT_X_();
        }
        else { /* cannot post the event */

            QS_BEGIN_NOCRIT_PRE_(QS_QF_ACTIVE_POST_ATTEMPT, me->prio)
                QS_TIME_PRE_();        /* timestamp */
                QS_OBJ_PRE_(sender);   /* the sender object */
                QS_SIG_PRE_(e->sig);   /* the signal of the event */
                QS_OBJ_PRE_(me);       /* this active object (recipient) */
                QS_2U8_PRE_(e->poolId_, e->refCtr_); /* pool Id & ref Count */
                QS_EQC_PRE_(nFree);    /* number of free entries */
                QS_EQC_PRE_(margin);  /* margin requested */
            QS_END_NOCRIT_PRE_()

            QF_CRIT_X_();

    #if (QF_MAX_EPOOL > 0U)
            QF_gc(e); /* recycle the event to avoid a leak */
    #endif
        }
    }
    else { /* the queue is not available */
    #if (QF_MAX_EPOOL > 0U)
         QF_gc(e); /* make sure the event is not leaked */
    #endif
         status = false;
         Q_ERROR_ID(320); /* this extended thread cannot accept events */
    }

    return status;
}

/*${QXK::QXThread::postLIFO_} ..............................................*/
void QXThread_postLIFO_(
    QActive * const me,
    QEvt const * const e)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(e);

    Q_ERROR_ID(410);
}

/*${QXK::QXThread::block_} .................................................*/
void QXThread_block_(QXThread const * const me) {
    /*! @pre the thread holding the lock cannot block! */
    Q_REQUIRE_ID(600, (QXK_attr_.lockHolder != me->super.prio));

    QPSet_remove(&QF_readySet_, (uint_fast8_t)me->super.prio);
    (void)QXK_sched_(); /* schedule other threads */
}

/*${QXK::QXThread::unblock_} ...............................................*/
void QXThread_unblock_(QXThread const * const me) {
    QPSet_insert(&QF_readySet_, (uint_fast8_t)me->super.prio);
    if ((!QXK_ISR_CONTEXT_()) /* not inside ISR? */
        && (QActive_registry_[0] != (QActive *)0))  /* kernel started? */
    {
        (void)QXK_sched_(); /* schedule other threads */
    }
}

/*${QXK::QXThread::teArm_} .................................................*/
void QXThread_teArm_(QXThread * const me,
    enum_t const sig,
    uint_fast16_t const nTicks)
{
    /*! @pre the time event must be unused */
    Q_REQUIRE_ID(700, me->timeEvt.ctr == 0U);

    me->timeEvt.super.sig = (QSignal)sig;

    if (nTicks != QXTHREAD_NO_TIMEOUT) {
        me->timeEvt.ctr = (QTimeEvtCtr)nTicks;
        me->timeEvt.interval = 0U;

        /* is the time event unlinked?
        * NOTE: For the duration of a single clock tick of the specified tick
        * rate a time event can be disarmed and yet still linked in the list,
        * because un-linking is performed exclusively in QTimeEvt_tick_().
        */
        if ((me->timeEvt.super.refCtr_ & QTE_IS_LINKED) == 0U) {
            uint_fast8_t const tickRate
                 = ((uint_fast8_t)me->timeEvt.super.refCtr_ & QTE_TICK_RATE);
            Q_ASSERT_ID(710, tickRate < QF_MAX_TICK_RATE);

            me->timeEvt.super.refCtr_ |= QTE_IS_LINKED;

            /* The time event is initially inserted into the separate
            * "freshly armed" list based on QTimeEvt_timeEvtHead_[tickRate].act.
            * Only later, inside the QTimeEvt_tick_() function, the "freshly
            * armed" list is appended to the main list of armed time events
            * based on QTimeEvt_timeEvtHead_[tickRate].next. Again, this is
            * to keep any changes to the main list exclusively inside
            * QTimeEvt_tick_().
            */
            me->timeEvt.next
                = QXK_PTR_CAST_(QTimeEvt*, QTimeEvt_timeEvtHead_[tickRate].act);
            QTimeEvt_timeEvtHead_[tickRate].act = &me->timeEvt;
        }
    }
}

/*${QXK::QXThread::teDisarm_} ..............................................*/
bool QXThread_teDisarm_(QXThread * const me) {
    bool wasArmed;
    /* is the time evt running? */
    if (me->timeEvt.ctr != 0U) {
        wasArmed = true;
        me->timeEvt.ctr = 0U; /* schedule removal from list */
    }
    /* the time event was already automatically disarmed */
    else {
        wasArmed = false;
    }
    return wasArmed;
}
/*$enddef${QXK::QXThread} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
