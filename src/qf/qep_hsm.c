/*$file${src::qf::qep_hsm.c} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/*
* Model: qpc.qm
* File:  ${src::qf::qep_hsm.c}
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
/*$endhead${src::qf::qep_hsm.c} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
/*! @file
* @brief ::QHsm implementation
*
* @tr{RQP103} @tr{RQP104} @tr{RQP120} @tr{RQP130}
*/
#define QP_IMPL           /* this is QP implementation */
#include "qep_port.h"     /* QEP port */
#include "qassert.h"      /* QP embedded systems-friendly assertions */
#ifdef Q_SPY              /* QS software tracing enabled? */
    #include "qs_port.h"  /* QS port */
    #include "qs_pkg.h"   /* QS facilities for pre-defined trace records */
#else
    #include "qs_dummy.h" /* disable the QS software tracing */
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qep_hsm")

/*==========================================================================*/
/*$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/
/* Check for the minimum required QP version */
#if (QP_VERSION < 700U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpc version 7.0.0 or higher required
#endif
/*$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*$define${QEP::QP_versionStr[8]} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QEP::QP_versionStr[8]} .................................................*/
char const QP_versionStr[8] = QP_VERSION_STR;
/*$enddef${QEP::QP_versionStr[8]} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/*! Immutable events corresponding to the reserved signals.
*
* @details
* Static, immutable reserved events that the QEP event processor sends
* to state handler functions of QHsm-style state machine to execute entry
* actions, exit actions, and initial transitions.
*/
static QEvt const l_reservedEvt_[] = {
    { (QSignal)Q_EMPTY_SIG, 0U, 0U },
    { (QSignal)Q_ENTRY_SIG, 0U, 0U },
    { (QSignal)Q_EXIT_SIG,  0U, 0U },
    { (QSignal)Q_INIT_SIG,  0U, 0U }
};

/*! helper function to trigger reserved event in an QHsm
* @private @memberof QHsm
*
* @param[in] state state handler function
* @param[in] sig   reserved signal to trigger
*/
static inline QState QHsm_reservedEvt_(
    QHsm * const me,
    QStateHandler const state,
    enum QReservedSig const sig)
{
    return (*state)(me, &l_reservedEvt_[sig]);
}

/*$define${QEP::QHsm} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

/*${QEP::QHsm} .............................................................*/

/*${QEP::QHsm::isIn} .......................................................*/
bool QHsm_isIn(QHsm * const me,
    QStateHandler const state)
{
    /*! @pre the state configuration must be stable */
    Q_REQUIRE_ID(600, me->temp.fun == me->state.fun);

    bool inState = false; /* assume that this HSM is not in 'state' */

    /* scan the state hierarchy bottom-up */
    QState r;
    do {
        /* do the states match? */
        if (me->temp.fun == state) {
            inState = true;            /* 'true' means that match found */
            r = Q_RET_IGNORED; /* break out of the loop */
        }
        else {
            r = QHsm_reservedEvt_(me, me->temp.fun, Q_EMPTY_SIG);
        }
    } while (r != Q_RET_IGNORED); /* QHsm_top() state not reached */
    me->temp.fun = me->state.fun; /* restore the stable state configuration */

    return inState; /* return the status */
}

/*${QEP::QHsm::childState} .................................................*/
QStateHandler QHsm_childState(QHsm * const me,
    QStateHandler const parent)
{
    QStateHandler child = me->state.fun; /* start with the current state */
    bool isFound = false; /* start with the child not found */

    /* establish stable state configuration */
    me->temp.fun = me->state.fun;
    QState r;
    do {
        /* is this the parent of the current child? */
        if (me->temp.fun == parent) {
            isFound = true; /* child is found */
            r = Q_RET_IGNORED; /* break out of the loop */
        }
        else {
            child = me->temp.fun;
            r = QHsm_reservedEvt_(me, me->temp.fun, Q_EMPTY_SIG);
        }
    } while (r != Q_RET_IGNORED); /* QHsm_top() state not reached */
    me->temp.fun = me->state.fun; /* establish stable state configuration */

    /*! @post the child must be found */
    Q_ENSURE_ID(810, isFound);

    #ifdef Q_NASSERT
    Q_UNUSED_PAR(isFound);
    #endif

    return child; /* return the child */
}

/*${QEP::QHsm::ctor} .......................................................*/
void QHsm_ctor(QHsm * const me,
    QStateHandler const initial)
{
    static struct QHsmVtable const vtable = { /* QHsm virtual table */
        &QHsm_init_,
        &QHsm_dispatch_
    #ifdef Q_SPY
        ,&QHsm_getStateHandler_
    #endif
    };
    me->vptr      = &vtable;
    me->state.fun = Q_STATE_CAST(&QHsm_top);
    me->temp.fun  = initial;
}

/*${QEP::QHsm::top} ........................................................*/
QState QHsm_top(QHsm const * const me,
    QEvt const * const e)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(e);
    return Q_RET_IGNORED; /* the top state ignores all events */
}

/*${QEP::QHsm::init_} ......................................................*/
void QHsm_init_(QHsm * const me,
    void const * const e,
    uint_fast8_t const qs_id)
{
    #ifdef Q_SPY
    if ((QS_priv_.flags & 0x01U) == 0U) {
        QS_priv_.flags |= 0x01U;
        QS_FUN_DICTIONARY(&QHsm_top);
    }
    #else
    Q_UNUSED_PAR(qs_id);
    #endif

    QStateHandler t = me->state.fun;

    /*! @pre the virtual pointer must be initialized, the top-most initial
    * transition must be initialized, and the initial transition must not
    * be taken yet.
    */
    Q_REQUIRE_ID(200, (me->vptr != (struct QHsmVtable *)0)
                      && (me->temp.fun != Q_STATE_CAST(0))
                      && (t == Q_STATE_CAST(&QHsm_top)));

    /* execute the top-most initial tran. */
    QState r = (*me->temp.fun)(me, Q_EVT_CAST(QEvt));

    /* the top-most initial transition must be taken */
    Q_ASSERT_ID(210, r == Q_RET_TRAN);

    QS_CRIT_STAT_
    QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qs_id)
        QS_OBJ_PRE_(me); /* this state machine object */
        QS_FUN_PRE_(t);  /* the source state */
        QS_FUN_PRE_(me->temp.fun); /* the target of the initial transition */
    QS_END_PRE_()

    /* drill down into the state hierarchy with initial transitions... */
    do {
        QStateHandler path[QHSM_MAX_NEST_DEPTH_]; /* tran entry path array */
        int_fast8_t ip = 0; /* tran entry path index */

        path[0] = me->temp.fun;
        (void)QHsm_reservedEvt_(me, me->temp.fun, Q_EMPTY_SIG);
        while (me->temp.fun != t) {
            ++ip;
            Q_ASSERT_ID(220, ip < QHSM_MAX_NEST_DEPTH_);
            path[ip] = me->temp.fun;
            (void)QHsm_reservedEvt_(me, me->temp.fun, Q_EMPTY_SIG);
        }
        me->temp.fun = path[0];

        /* nested initial transition, drill into the target hierarchy... */
        do {
            QHsm_state_entry_(me, path[ip], qs_id); /* enter path[ip] */
            --ip;
        } while (ip >= 0);

        t = path[0]; /* current state becomes the new source */

        r = QHsm_reservedEvt_(me, t, Q_INIT_SIG); /* execute initial transition */

    #ifdef Q_SPY
        if (r == Q_RET_TRAN) {
            QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qs_id)
                QS_OBJ_PRE_(me); /* this state machine object */
                QS_FUN_PRE_(t);  /* the source state */
                QS_FUN_PRE_(me->temp.fun); /* target of the initial tran. */
            QS_END_PRE_()
        }
    #endif /* Q_SPY */

    } while (r == Q_RET_TRAN);

    QS_BEGIN_PRE_(QS_QEP_INIT_TRAN, qs_id)
        QS_TIME_PRE_();  /* time stamp */
        QS_OBJ_PRE_(me); /* this state machine object */
        QS_FUN_PRE_(t);  /* the new active state */
    QS_END_PRE_()

    me->state.fun = t; /* change the current active state */
    me->temp.fun  = t; /* mark the configuration as stable */
}

/*${QEP::QHsm::dispatch_} ..................................................*/
void QHsm_dispatch_(QHsm * const me,
    QEvt const * const e,
    uint_fast8_t const qs_id)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(qs_id);
    #endif

    QStateHandler t = me->state.fun;
    QS_CRIT_STAT_

    /*! @pre the current state must be initialized and
    * the state configuration must be stable
    */
    Q_REQUIRE_ID(400, (t != Q_STATE_CAST(0))
                       && (t == me->temp.fun));

    QS_BEGIN_PRE_(QS_QEP_DISPATCH, qs_id)
        QS_TIME_PRE_();      /* time stamp */
        QS_SIG_PRE_(e->sig); /* the signal of the event */
        QS_OBJ_PRE_(me);     /* this state machine object */
        QS_FUN_PRE_(t);      /* the current state */
    QS_END_PRE_()

    QStateHandler s;
    QState r;
    /* process the event hierarchically... */
    do {
        s = me->temp.fun;
        r = (*s)(me, e); /* invoke state handler s */

        if (r == Q_RET_UNHANDLED) { /* unhandled due to a guard? */

            QS_BEGIN_PRE_(QS_QEP_UNHANDLED, qs_id)
                QS_SIG_PRE_(e->sig); /* the signal of the event */
                QS_OBJ_PRE_(me);     /* this state machine object */
                QS_FUN_PRE_(s);      /* the current state */
            QS_END_PRE_()

            r = QHsm_reservedEvt_(me, s, Q_EMPTY_SIG); /* find superstate of s */
        }
    } while (r == Q_RET_SUPER);

    /* regular transition taken? */
    /*! @tr{RQP120E} */
    if (r >= Q_RET_TRAN) {
        QStateHandler path[QHSM_MAX_NEST_DEPTH_];

        path[0] = me->temp.fun; /* save the target of the transition */
        path[1] = t;
        path[2] = s;

        /* exit current state to transition source s... */
        /*! @tr{RQP120C} */
        for (; t != s; t = me->temp.fun) {
            /* exit from t handled? */
            if (QHsm_state_exit_(me, t, qs_id)) {
                /* find superstate of t */
                (void)QHsm_reservedEvt_(me, t, Q_EMPTY_SIG);
            }
        }

        int_fast8_t ip = QHsm_tran_(me, path, qs_id); /* the HSM transition */

    #ifdef Q_SPY
        if (r == Q_RET_TRAN_HIST) {
            QS_BEGIN_PRE_(QS_QEP_TRAN_HIST, qs_id)
                QS_OBJ_PRE_(me); /* this state machine object */
                QS_FUN_PRE_(t);  /* the source of the transition */
                QS_FUN_PRE_(path[0]); /* the target of tran. to history */
            QS_END_PRE_()
        }
    #endif /* Q_SPY */

        /* execute state entry actions in the desired order... */
        /*! @tr{RQP120B} */
        for (; ip >= 0; --ip) {
            QHsm_state_entry_(me, path[ip], qs_id);  /* enter path[ip] */
        }

        t = path[0];      /* stick the target into register */
        me->temp.fun = t; /* update the next state */

        /* while nested initial transition... */
        /*! @tr{RQP120I} */
        while (QHsm_reservedEvt_(me, t, Q_INIT_SIG) == Q_RET_TRAN) {

            QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qs_id)
                QS_OBJ_PRE_(me); /* this state machine object */
                QS_FUN_PRE_(t);  /* the source (pseudo)state */
                QS_FUN_PRE_(me->temp.fun); /* the target of the tran. */
            QS_END_PRE_()

            ip = 0;
            path[0] = me->temp.fun;

            /* find superstate */
            (void)QHsm_reservedEvt_(me, me->temp.fun, Q_EMPTY_SIG);

            while (me->temp.fun != t) {
                ++ip;
                path[ip] = me->temp.fun;
                /* find superstate */
                (void)QHsm_reservedEvt_(me, me->temp.fun, Q_EMPTY_SIG);
            }
            me->temp.fun = path[0];

            /* entry path must not overflow */
            Q_ASSERT_ID(410, ip < QHSM_MAX_NEST_DEPTH_);

            /* retrace the entry path in reverse (correct) order... */
            do {
                QHsm_state_entry_(me, path[ip], qs_id); /* enter path[ip] */
                --ip;
            } while (ip >= 0);

            t = path[0]; /* current state becomes the new source */
        }

        QS_BEGIN_PRE_(QS_QEP_TRAN, qs_id)
            QS_TIME_PRE_();      /* time stamp */
            QS_SIG_PRE_(e->sig); /* the signal of the event */
            QS_OBJ_PRE_(me);     /* this state machine object */
            QS_FUN_PRE_(s);      /* the source of the transition */
            QS_FUN_PRE_(t);      /* the new active state */
        QS_END_PRE_()
    }

    #ifdef Q_SPY
    else if (r == Q_RET_HANDLED) {

        QS_BEGIN_PRE_(QS_QEP_INTERN_TRAN, qs_id)
            QS_TIME_PRE_();      /* time stamp */
            QS_SIG_PRE_(e->sig); /* the signal of the event */
            QS_OBJ_PRE_(me);     /* this state machine object */
            QS_FUN_PRE_(s);      /* the source state */
        QS_END_PRE_()

    }
    else {

        QS_BEGIN_PRE_(QS_QEP_IGNORED, qs_id)
            QS_TIME_PRE_();      /* time stamp */
            QS_SIG_PRE_(e->sig); /* the signal of the event */
            QS_OBJ_PRE_(me);     /* this state machine object */
            QS_FUN_PRE_(me->state.fun); /* the current state */
        QS_END_PRE_()

    }
    #endif /* Q_SPY */

    me->state.fun = t; /* change the current active state */
    me->temp.fun  = t; /* mark the configuration as stable */
}

/*${QEP::QHsm::getStateHandler_} ...........................................*/
#ifdef Q_SPY
QStateHandler QHsm_getStateHandler_(QHsm * const me) {
    return me->state.fun;
}
#endif /* def Q_SPY */

/*${QEP::QHsm::tran_} ......................................................*/
int_fast8_t QHsm_tran_(QHsm * const me,
    QStateHandler * const path,
    uint_fast8_t const qs_id)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(qs_id);
    #endif

    int_fast8_t ip = -1; /* transition entry path index */
    QStateHandler t = path[0];
    QStateHandler const s = path[2];

    /* (a) check source==target (transition to self)... */
    if (s == t) {
        (void)QHsm_state_exit_(me, s, qs_id); /* exit source */
        ip = 0; /* enter the target */
    }
    else {
        /* find superstate of target */
        (void)QHsm_reservedEvt_(me, t, Q_EMPTY_SIG);

        t = me->temp.fun;

        /* (b) check source==target->super... */
        if (s == t) {
            ip = 0; /* enter the target */
        }
        else {
            /* find superstate of src */
            (void)QHsm_reservedEvt_(me, s, Q_EMPTY_SIG);

            /* (c) check source->super==target->super... */
            if (me->temp.fun == t) {
                (void)QHsm_state_exit_(me, s, qs_id); /* exit source */
                ip = 0; /* enter the target */
            }
            else {
                /* (d) check source->super==target... */
                if (me->temp.fun == path[0]) {
                    (void)QHsm_state_exit_(me, s, qs_id); /* exit source */
                }
                else {
                    /* (e) check rest of source==target->super->super..
                    * and store the entry path along the way
                    */
                    int_fast8_t iq = 0; /* indicate that LCA not found */
                    ip = 1; /* enter target and its superstate */
                    path[1] = t;      /* save the superstate of target */
                    t = me->temp.fun; /* save source->super */

                    /* find target->super->super... */
                    QState r = QHsm_reservedEvt_(me, path[1], Q_EMPTY_SIG);
                    while (r == Q_RET_SUPER) {
                        ++ip;
                        path[ip] = me->temp.fun; /* store the entry path */
                        if (me->temp.fun == s) { /* is it the source? */
                            iq = 1; /* indicate that LCA found */

                            /* entry path must not overflow */
                            Q_ASSERT_ID(510,
                                ip < QHSM_MAX_NEST_DEPTH_);
                            --ip; /* do not enter the source */
                            r = Q_RET_HANDLED; /* terminate loop */
                        }
                        /* it is not the source, keep going up */
                        else {
                            r = QHsm_reservedEvt_(me, me->temp.fun,
                                                  Q_EMPTY_SIG);
                        }
                    }

                    /* the LCA not found yet? */
                    if (iq == 0) {

                        /* entry path must not overflow */
                        Q_ASSERT_ID(520, ip < QHSM_MAX_NEST_DEPTH_);

                        /* exit source */
                        (void)QHsm_state_exit_(me, s, qs_id);

                        /* (f) check the rest of source->super
                        *                  == target->super->super...
                        */
                        iq = ip;
                        r = Q_RET_IGNORED; /* LCA NOT found */
                        do {
                            if (t == path[iq]) { /* is this the LCA? */
                                r = Q_RET_HANDLED; /* LCA found */
                                ip = iq - 1; /* do not enter LCA */
                                iq = -1; /* cause termintion of the loop */
                            }
                            else {
                                --iq; /* try lower superstate of target */
                            }
                        } while (iq >= 0);

                        /* LCA not found? */
                        if (r != Q_RET_HANDLED) {
                            /* (g) check each source->super->...
                            * for each target->super...
                            */
                            r = Q_RET_IGNORED; /* keep looping */
                            do {
                                /* exit from t handled? */
                                if (QHsm_state_exit_(me, t, qs_id)) {
                                    /* find superstate of t */
                                    (void)QHsm_reservedEvt_(me, t, Q_EMPTY_SIG);
                                }
                                t = me->temp.fun; /* set to super of t */
                                iq = ip;
                                do {
                                    /* is this LCA? */
                                    if (t == path[iq]) {
                                        /* do not enter LCA */
                                        ip = (int_fast8_t)(iq - 1);
                                        iq = -1; /* break out of inner loop */
                                        /* break out of outer loop */
                                        r = Q_RET_HANDLED;
                                    }
                                    else {
                                        --iq;
                                    }
                                } while (iq >= 0);
                            } while (r != Q_RET_HANDLED);
                        }
                    }
                }
            }
        }
    }
    return ip;
}

/*${QEP::QHsm::state_entry_} ...............................................*/
void QHsm_state_entry_(QHsm * const me,
    QStateHandler const state,
    uint_fast8_t const qs_id)
{
    #ifdef Q_SPY
    if ((*state)(me, &l_reservedEvt_[Q_ENTRY_SIG]) == Q_RET_HANDLED) {
        QS_CRIT_STAT_
        QS_BEGIN_PRE_(QS_QEP_STATE_ENTRY, qs_id)
            QS_OBJ_PRE_(me);
            QS_FUN_PRE_(state);
        QS_END_PRE_()
    }
    #else
    Q_UNUSED_PAR(qs_id);
    (void)(*state)(me, &l_reservedEvt_[Q_ENTRY_SIG]);
    #endif /* Q_SPY */
}

/*${QEP::QHsm::state_exit_} ................................................*/
bool QHsm_state_exit_(QHsm * const me,
    QStateHandler const state,
    uint_fast8_t const qs_id)
{
    #ifdef Q_SPY
    bool isHandled;
    if ((*state)(me, &l_reservedEvt_[Q_EXIT_SIG]) == Q_RET_HANDLED) {
        QS_CRIT_STAT_
        QS_BEGIN_PRE_(QS_QEP_STATE_EXIT, qs_id)
            QS_OBJ_PRE_(me);
            QS_FUN_PRE_(state);
        QS_END_PRE_()
        isHandled = true;
    }
    else {
        isHandled = false;
    }
    return isHandled;
    #else
    Q_UNUSED_PAR(qs_id);
    return (*state)(me, &l_reservedEvt_[Q_EXIT_SIG]) == Q_RET_HANDLED;
    #endif /* Q_SPY */
}
/*$enddef${QEP::QHsm} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
