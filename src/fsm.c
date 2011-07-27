/*
 * This file is part of the OpenPTS project.
 *
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2010 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */

/**
 * \file src/fsm.c
 * \brief Finite State Machine
 * @author Seiji Munetoh <munetoh@users.sourceforge.jp>
 * @date 2010-04-01
 * cleanup 2011-01-21 SM
 * refactoring 2011-07-20 SM
 * 
 * Input
 *   FSM Model
 *   IML
 *   PROPERTY
 * Output
 *   SNAPSHOT
 *   PROPERTY
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>

#include <openpts.h>

/**
 * create new FSM context
 */
OPENPTS_FSM_CONTEXT *newFsmContext() {
    OPENPTS_FSM_CONTEXT *ctx = NULL;

    /* malloc */
    ctx = (OPENPTS_FSM_CONTEXT *) malloc(sizeof(OPENPTS_FSM_CONTEXT));
    if (ctx == NULL) {
        ERROR("ERROR\n");
        return NULL;
    }
    /* init */
    memset(ctx, 0 , sizeof(OPENPTS_FSM_CONTEXT));
    ctx->fsm_sub = NULL;
    ctx->fsm_trans = NULL;
    ctx->uml_file = NULL;
    ctx->state = 0;
    ctx->subvertex_num = 0;
    ctx->transition_num = 0;
    return ctx;
}

/**
 * Free OPENPTS_FSM_Transition chain
 */
void freeFsmTransitionChain(OPENPTS_FSM_Transition *fsm_trans) {
    if (fsm_trans->next != NULL) {
        freeFsmTransitionChain(fsm_trans->next);
    }

    /* free */
    if (fsm_trans->digest != NULL) {
        free(fsm_trans->digest);
    }

    free(fsm_trans);
}

/**
 * Free OPENPTS_FSM_Subvertex chain
 */
void freeFsmSubvertexChain(OPENPTS_FSM_Subvertex *fsm_sub) {
    if (fsm_sub->next != NULL) {
        freeFsmSubvertexChain(fsm_sub->next);
    }

    free(fsm_sub);
}

/**
 * free FSM context
 */
int freeFsmContext(OPENPTS_FSM_CONTEXT *ctx) {
    if (ctx == NULL) {
        ERROR("ERROR\n");
        return -1;
    }

    /* Transition */
    if (ctx->fsm_trans != NULL) {
        freeFsmTransitionChain(ctx->fsm_trans);
        ctx->fsm_trans = NULL;
    }


    /* Subvertex */
    if (ctx->fsm_sub != NULL) {
        freeFsmSubvertexChain(ctx->fsm_sub);
        ctx->fsm_sub = NULL;
    }

    /* UML filename */
    if (ctx->uml_file != NULL) {
        free(ctx->uml_file);
        ctx->uml_file = NULL;
    }

    free(ctx);
    return 0;
}


//// SUBVERTEX ////

/**
 * reset FSM subvertex
 */
void resetFsmSubvertex(OPENPTS_FSM_CONTEXT *ctx) {
    // fsm_sub=NULL;
    ctx->subvertex_num = 0;
}

/**
 * reset FSM transition
 */
void resetFsmTransition(OPENPTS_FSM_CONTEXT *ctx) {
    // fsm_trans=NULL;
    ctx->transition_num = 0;
}


/**
 * add FMS subvertex to context
 */ 
void addFsmSubvertex(
        OPENPTS_FSM_CONTEXT *ctx,
        char *type,
        char *id,
        char *name,
        char *action) {
    int i;
    OPENPTS_FSM_Subvertex *ptr = NULL;
    OPENPTS_FSM_Subvertex *ptr_pre = NULL;

    DEBUG_CAL("addFsmSubvertex - %d \n", ctx->subvertex_num);

    ptr = ctx->fsm_sub;
    for (i = 0; i <= ctx->subvertex_num; i++) {
        if (ptr == NULL) {
            /* add new PENPTS_FSM_Subvertex */
            DEBUG_FSM(" id=%s name=%s size=%d\n",
                      id, name,
                      (int)sizeof(OPENPTS_FSM_Subvertex));

            /* malloc OPENPTS_FSM_Subvertex */
            ptr = (OPENPTS_FSM_Subvertex *)
                    malloc(sizeof(OPENPTS_FSM_Subvertex));
            if (ptr == NULL) {
                ERROR("addFsmSubvertex - no memory\n");
                return;
            }

            /* init */
            memset(ptr, 0, sizeof(OPENPTS_FSM_Subvertex));
            /* setup new FSM_Subvertex */
            memcpy(ptr->type, type, FSM_BUF_SIZE);
            memcpy(ptr->id, id, FSM_BUF_SIZE);
            memcpy(ptr->name, name, FSM_BUF_SIZE);
            memcpy(ptr->action, action, FSM_BUF_SIZE);

            ptr->next = NULL;
            ptr->num = ctx->subvertex_num;
            ptr->incomming_num = 0;

            if (ctx->subvertex_num == 0) {  // first event
                ctx->fsm_sub = ptr;
                ptr->prev = NULL;
                // NG must be Start event ctx->curr_state = ptr;
            } else if (ptr_pre != NULL) {
                ptr_pre->next = ptr;  // else
                ptr->prev = ptr_pre;
            } else {
                ERROR("\n");
                free(ptr);  // free last one
                return;
            }

            ctx->subvertex_num++;
            return;
        }
        ptr_pre = ptr;
        ptr = ptr->next;
    }
}

/**
 * get Subvertex ptr by ID
 */
OPENPTS_FSM_Subvertex * getSubvertex(OPENPTS_FSM_CONTEXT *ctx, char * id) {
    OPENPTS_FSM_Subvertex *ptr;

    if (!strcmp(id, "Final")) return NULL;  // final state

    ptr = ctx->fsm_sub;

    while (ptr != NULL) {
        if (!strcmp(id, ptr->id)) return ptr;
        ptr = (OPENPTS_FSM_Subvertex *) ptr->next;
    }
    return NULL;
}

/**
 * get Subvertex Name by ID
 */
char * getSubvertexName(OPENPTS_FSM_CONTEXT *ctx, char * id) {
    int i;
    OPENPTS_FSM_Subvertex *ptr;

    if (!strcmp(id, "Final")) return id;

    ptr = ctx->fsm_sub;
    for (i = 0;i <= ctx->subvertex_num; i++) {
        if (!strcmp(id, ptr->id)) return ptr->name;
        ptr = (OPENPTS_FSM_Subvertex *) ptr->next;
    }

    return NULL;
}

/**
 * get Subvertex ID by Name
 */
char * getSubvertexId(OPENPTS_FSM_CONTEXT *ctx, char * name) {
    int i;
    OPENPTS_FSM_Subvertex *ptr;

    // if (!strcmp(id, "Final")) return id;

    ptr = ctx->fsm_sub;
    for (i = 0;i <= ctx->subvertex_num; i++) {
        if (!strcmp(name, ptr->name)) return ptr->id;
        ptr = (OPENPTS_FSM_Subvertex *) ptr->next;
    }

    return NULL;
}


/// TRANSITION ///

/**
 *   <body>eventtype == 0x01, digest == base64</body>
 * -1: error
 *  0: don't care
 *   1: ==
 *   2: !=
 *
 * Unit Test : check_fsm.c / test_getTypeFlag
 *
 */
int getTypeFlag(char * cond, UINT32 *eventtype) {
    char * loc;
    int len;
    int rc = 0;
    long int val;  // TODO uint64_t but fail to build on i386 platform

    len = strlen(cond);

    loc = strstr(cond, "eventtype");

    if (loc == NULL) {  // miss
        *eventtype = 0;
        return 0;
    } else {  // hit
        /* skip eventtype*/
        loc += 9;
        len -= (loc - cond);

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {    // space
                loc++;
                len--;
            } else {
                break;
            }
        }

        /* operation */
        if (len < 2) {
            ERROR("ERROR 001\n");
            return -1;  // end
        }
        if ((loc[0] == '=') && (loc[1] == '=')) {  // ==
            rc = 1;
        } else if ((loc[0] == '!') && (loc[1] == '=')) {  // ==
            rc = 2;
        } else {
            ERROR("ERROR 002 %c %c \n", loc[0], loc[1]);
            return -1;  // unknown operand
        }
        loc += 2;
        len -= 2;

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {  // space
                loc++;
                len--;
            } else {
                break;
            }
        }
        /* value */
        // 20110117 Ubuntu i386, 0x80000002 => 7FFFFFFF, => use strtoll
        if (len > 2) {
            if  ((loc[0] == '0') && (loc[1] == 'x')) {  // 0x HEX
                val = strtoll(loc, NULL, 16);
                *eventtype = (UINT32)val;
                // DEBUG("strtol [%s] => %X => %X\n", loc,val,*eventtype);
                return rc;
            }
        }
        val = strtoll(loc, NULL, 10);
        *eventtype = (UINT32)val;
        // DEBUG("strtol [%s] => %X => %X\n", loc,val, *eventtype);

        return rc;
    }
}

/**
 * Parse condition string and setup an internal digest condition structure
 *
 * Return
 *   0: don't care
 *   1: valid (=binary model),  return the digest => freed at freeFsmTransitionChain()
 *   2: ignore now (=behavior model)
 *  -1: Error?
 *
 * Unit Test : check_fsm.c / test_getDigestFlag
 *
 * TODO STA may complain the memory leak againt *digest.
 */
int getDigestFlag(char * cond, BYTE **digest, int *digest_size) {
    char * loc;   // loc at value
    char * loc2;  // loc at "base64"
    int len;
    int rc = 0;
    BYTE *buf;

    DEBUG_CAL("getDigestFlag -");

    len = strlen(cond);

    loc = strstr(cond, "digest");
    if (loc == NULL) {  // miss
        *digest_size = 0;
        return 0;
    } else {  // hit
        /* skip  digest */
        loc += 6;
        len -= (loc - cond);

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {  // space
                loc++;
                len--;
            } else {
                break;
            }
        }

        /* operation, "==" only */
        if (len < 2) {
            ERROR("ERROR 001\n");
            return -1;  // end
        }
        if ((loc[0] == '=') && (loc[1] == '=')) {  // ==
            rc = 1;
        } else {
            ERROR("ERROR 002 [%c%c]  not  == \n", loc[0], loc[1]);
            return -1;  // unknown operand
        }
        loc +=2;
        len -=2;

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {  // space
                loc++;
                len--;
            } else {
                break;
            }
        }

        /* digest == base64 (behavior model) */
        loc2 = strstr(loc, "base64");
        if (loc2 != NULL) {  // HIT, temp
            /* Behavior Model */
            return 2;
        } else {
            /* Binary Model */
            /* Base64 str -> BYTE[] */
            buf = (BYTE *) malloc(SHA1_DIGEST_SIZE + 1);
            if (buf == NULL) {
                ERROR("no memory");
                return -1;
            }

            // TODO(munetoh) get len, "<"
            rc = decodeBase64(buf, (unsigned char *)loc, SHA1_BASE64_DIGEST_SIZE);
            if (rc == SHA1_DIGEST_SIZE) {
                // TODO(munetoh) digest size change by alg
                // this code is SHA1 only
                *digest = buf;
                *digest_size = rc;
                return 1;
            } else {
                ERROR("getDigestFlag() - decodeBase64() was failed \n");
                free(buf);
                *digest = NULL;
                *digest_size = 0;
                return -1;
            }
        }
    }
}

/**
 * Parse condition string and setup an internal couter condition structure
 *
 *
 * 
 * Return
 *   COUNTER_FLAG_SKIP 0: don't care
 *   COUNTER_FLAG_LT   1:  < name
 *   COUNTER_FLAG_GTE  2:  >= name 
 *
 *   set counter name to name 
 *
 * Unit Test : check_fsm.c / test_getCounterFlag
 */
int getCounterFlag(char * cond, char **name) {
    char * loc;   // loc at value
    char * loc2;  // loc at name
    int len;
    int rc = COUNTER_FLAG_SKIP;

    // DEBUG_CAL("getCounterFlag\n");

    /* check */
    if (cond == NULL) {
        ERROR("getCounterFlag()");
        return COUNTER_FLAG_SKIP;
    }

    len = strlen(cond);
    loc = strstr(cond, "count");

    if (loc == NULL) {
        /* miss */
        return 0;
    } else {
        /* hit */

        /* skip  count */
        loc += 5;
        len -= (loc - cond);

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {  // space
                loc++;
                len--;
            } else {
                break;
            }
        }

        /* operation, "&lt;" ">=" only */
        if ((len >= 2) && (loc[0] == 'l') && (loc[1] == 't')) {
            /* >= */
            rc = COUNTER_FLAG_LT;
            loc +=2;
            len -=2;
        } else if ((len >= 2) && (loc[0] == '>') && (loc[1] == '=')) {
            /* >= */
            rc = COUNTER_FLAG_GTE;
            loc +=2;
            len -=2;
        } else {
            // ERROR("ERROR 002 [%s]  not  >= \n", &loc[0];
            return -1;  // unknown operand
        }

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {  // space
                loc++;
                len--;
            } else {
                break;
            }
        }

        // TODO check the end, this code only support if counter is the last equation

        loc2 = loc;
        len = strlen(loc2);

        *name = malloc(len + 1);
        if (*name == NULL) {
            ERROR("no memory\n");
            return -1;
        }
        memset(*name, 0, len + 1);
        memcpy(*name, loc2, len);
    }
    DEBUG_FSM("getCounterFlag cond=[%s],  prop=[%s] rc=%d (0:1:2=skip:<:>=),\n", cond,  *name, rc);
    return rc;
}



/**
 * Parse condition string and setup an internal couter condition structure
 *
 *
 * 
 * Return
 *                    -1: error
 *   LAST_FLAG_SKIP 0: don't care
 *   LAST_FLAG_EQ   1:  == last
 *   LAST_FLAG_NEQ  2:  != last 
 *
 * Unit Test : check_fsm.c / test_getCounterFlag
 */
int getLastFlag(char * cond) {
    char * loc;   // loc at value
    char * loc2;  // loc at name
    int len;
    int rc = LAST_FLAG_SKIP;

    len = strlen(cond);
    loc = strstr(cond, "last");

    if (loc == NULL) {
        /* miss */
        return LAST_FLAG_SKIP;
    } else {
        /* hit */
        // DEBUG("getLastFlag() - %s\n", cond);

        /* skip  count */
        loc += 5;
        len -= (loc - cond);

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {  // space
                loc++;
                len--;
            } else {
                break;
            }
        }

        /* operation, "&lt;" ">=" only */
        if ((len >= 2) && (loc[0] == '=') && (loc[1] == '=')) {
            /* >= */
            rc = LAST_FLAG_EQ;
            loc +=2;
            len -=2;
        } else if ((len >= 2) && (loc[0] == '!') && (loc[1] == '=')) {
            /* >= */
            rc = LAST_FLAG_NEQ;
            loc +=2;
            len -=2;
        } else {
            // ERROR("ERROR 002 [%s]  not  >= \n", &loc[0];
            return -1;  // unknown operand
        }

        /* skip space */
        while (len > 0) {
            if (loc[0] == 0) return -1;  // end
            else if (loc[0] == ' ') {  // space
                loc++;
                len--;
            } else {
                break;
            }
        }

        /* value */

        loc2 = loc;
        len = strlen(loc2);

        // DEBUG("[%d][%s]\n",len, loc2);
        if (!strncmp(loc2, "true", 4)) {
            // DEBUG("true\n");
            /* == true */
            /* != true => false */
        } else if (!strncmp(loc2, "false", 5)) {
            // DEBUG("false %d\n",rc);
            if (rc == LAST_FLAG_EQ) {
                rc = LAST_FLAG_NEQ;
            } else {
                rc = LAST_FLAG_EQ;
            }
            // DEBUG("false %d\n",rc);
        } else {
            ERROR("unknown value, %s\n", loc2);
        }
    }

    // DEBUG("getLastFlag  %s #=> %d\n",cond, rc);

    return rc;
}




/**
 * add FSM transition
 *
 * Return
 *   PTS_SUCCESS
 *   PTS_INTERNAL_ERROR
 */
int addFsmTransition(
        OPENPTS_FSM_CONTEXT *ctx,
        char *source,
        char *target,
        char *cond) {
    int i;
    OPENPTS_FSM_Transition *ptr = NULL;
    OPENPTS_FSM_Transition *ptr_pre = NULL;

    DEBUG_CAL("addFsmTransition - start\n");

    ptr = ctx->fsm_trans;
    for (i = 0; i <= ctx->transition_num; i++) {
        if (ptr == NULL) {  // new
            DEBUG_FSM(" src=%s -> dst=%s  cond[%s] %d\n",
                      source, target, cond,
                      (int)sizeof(OPENPTS_FSM_Transition));

            /* malloc OPENPTS_FSM_Transition */
            ptr = (OPENPTS_FSM_Transition *)
                    malloc(sizeof(OPENPTS_FSM_Transition));
            if (ptr == NULL) {
                ERROR("no memory\n");
                return PTS_INTERNAL_ERROR;
            }
            /* init */
            memset(ptr, 0, sizeof(OPENPTS_FSM_Transition));
            memcpy(ptr->source, source, FSM_BUF_SIZE);
            memcpy(ptr->target, target, FSM_BUF_SIZE);
            // memcpy(ptr->cond, cond, FSM_BUF_SIZE);
            ptr->num = ctx->transition_num;
            if (cond == NULL) {
                ptr->eventTypeFlag = 0;
                ptr->digestFlag = 0;
            } else if  (cond[0] == 0) {
                ptr->eventTypeFlag = 0;
                ptr->digestFlag = 0;
                memcpy(ptr->cond, cond, FSM_BUF_SIZE);
            } else {
                // 0:don't care, 1:care
                ptr->eventTypeFlag = getTypeFlag(cond, &ptr->eventType);
                // 0:don't care, 1:care, 2:temp
                ptr->digestFlag = getDigestFlag(cond, &ptr->digest, &ptr->digestSize);
                // 0:don't care, 1:<, 2:>=
                ptr->counter_flag = getCounterFlag(cond, &ptr->counter_name);
                // 0:don't care 1: ==last 2: != last
                ptr->last_flag = getLastFlag(cond);
                memcpy(ptr->cond, cond, FSM_BUF_SIZE);
            }
            /* subvertex link (ptr) */
            ptr->source_subvertex = getSubvertex(ctx, ptr->source);
            ptr->target_subvertex = getSubvertex(ctx, ptr->target);

            /* ptr */
            ptr->next = NULL;
            if (ctx->transition_num == 0) {
                ctx->fsm_trans = ptr;
                ptr->prev = NULL;  // first trans
            } else if (ptr_pre != NULL) {
                ptr_pre->next = ptr;
                ptr->prev = ptr_pre;
                ptr->next = NULL;  // last trans
            } else {
                ERROR("\n");
                free(ptr);  // free last one
                return PTS_INTERNAL_ERROR;
            }
            ctx->transition_num++;
            /* added */
            return PTS_SUCCESS;
        }
        /* next */
        ptr_pre = ptr;
        ptr = (OPENPTS_FSM_Transition *)ptr->next;
    }

    ERROR("missing?\n");
    return PTS_INTERNAL_ERROR;
}

/**
 * get Event String (malloc)
 */
char *getEventString(OPENPTS_PCR_EVENT_WRAPPER *eventWrapper) {
    // int len;
    int size = FSM_BUF_SIZE;  // TODO fixed size
    TSS_PCR_EVENT *event;
    char *buf;

    /* malloc */
    buf = malloc(size);
    if (buf == NULL) {
        ERROR("no memory\n");
        return NULL;
    }

    /* event */
    event = eventWrapper->event;
    if (event != NULL) {
        // len = snprintf(buf, size, "PCR[%d],TYPE=%d", (int)event->ulPcrIndex, event->eventType);
    } else {
        ERROR("NULL event\n");  // TODO(munetoh)
        free(buf);
        return NULL;
    }

    return buf;
}

/**
 * get counter(int) value from property
 * if property is missing or invalid, it returns 1;
 */
int getCountFromProperty(OPENPTS_CONTEXT *ctx, char * name) {
    int count = 0;  // TODO get from prop
    OPENPTS_PROPERTY *prop;

    prop = getProperty(ctx, name);
    if (prop != NULL) {
        /* Hit use this properties */
        count = atoi(prop->value);
        DEBUG_FSM("getCountFromProperty - prop %s = %d\n", name, count);
        if (count < 0) {
            count = 1;
        }
    } else {
        /* Miss -> 1 */
        // TODO
        // DEBUG("getCountFromProperty - prop %s is missing. set count to 1\n", name);
        count = 1;  // TODO
    }
    return count;
}

/**
 * Drive FSM Transition by Event
 *
 * @parm eventWrapper  NULL, push the FSM until Final state
 *
 * Return
 *
 *  OPENPTS_FSM_ERROR
 *
 *  OPENPTS_FSM_SUCCESS
 *  OPENPTS_FSM_FLASH
 *  OPENPTS_FSM_FINISH     reach Final State, move to the next snapshot(=model)
 *  OPENPTS_FSM_TRANSIT    transit to next FSM
 *  OPENPTS_FSM_ERROR_LOOP
 *
 * if eventWrapper is NULL, create and use dummy event 
 */
int updateFsm(
        OPENPTS_CONTEXT *ctx,
        OPENPTS_FSM_CONTEXT *fsm,
        OPENPTS_PCR_EVENT_WRAPPER *eventWrapper
    ) {
    int rc = OPENPTS_FSM_SUCCESS;
    OPENPTS_FSM_Subvertex  *curr_state = fsm->curr_state;
    OPENPTS_FSM_Transition *trans = fsm->fsm_trans;
    TSS_PCR_EVENT *event;
    int type_check;
    int digest_check;
    int counter_check;
    int last_check;
    int dont_care;
    int hit = 0;
    char *hex;
    OPENPTS_FSM_Transition *hit_trans = NULL;

    DEBUG_CAL("updateFsm - start\n");

    if (curr_state == NULL) {
        DEBUG_FSM("[RM%02d-PCR%02d] updateFsm() - curr_state == NULL => set the FSM state to 'Start'\n",
            fsm->level, fsm->pcr_index);
        curr_state = getSubvertex(fsm, "Start");
    }

    /* Null event ->  push FSM until Final state */
    // TODO(munetoh) dummy event does not need event. just add flag to the wrapper
    if (eventWrapper == NULL) {
        DEBUG_FSM("[RM%02d-PCR%02d] create dummy event to flash the FSM\n",
            fsm->level, fsm->pcr_index);

        /* dummy wrapper */
        eventWrapper = (OPENPTS_PCR_EVENT_WRAPPER *)
            malloc(sizeof(OPENPTS_PCR_EVENT_WRAPPER));
        memset(eventWrapper, 0, sizeof(OPENPTS_PCR_EVENT_WRAPPER));
        eventWrapper->event_type = OPENPTS_DUMMY_EVENT;
        eventWrapper->push_count = 0;
        eventWrapper->last = 1;

        /*  push */
        rc = updateFsm(ctx, fsm, eventWrapper);
        if (rc == OPENPTS_FSM_ERROR) {
            ERROR("updateFsm() - updateFsm push was fail\n");
        }
        if (rc == OPENPTS_FSM_ERROR_LOOP) {
            // DEBUG("updateFsm -- updateFsm push - loop \n");
        }

        /* free dummy wrapper */
        free(eventWrapper);
        eventWrapper = NULL;
        return rc;
    } else if (eventWrapper->event == NULL) {
        if (eventWrapper->event_type == OPENPTS_DUMMY_EVENT) {
            // DUMMY
            eventWrapper->push_count++;
            event = NULL;

            if (eventWrapper->push_count > 10) {
                /* LOOP */
                // TODO detect LOOP
                // DEBUG("LOOP?\n");
                return OPENPTS_FSM_ERROR_LOOP;
            }
        } else {
           ERROR("missing event body\n");
           return OPENPTS_FSM_ERROR;
        }
    } else {
        /* FSM update by event */
        event = eventWrapper->event;
    }

    DEBUG_FSM("[RM%02d-PCR%02d] updateFsm() - State='%s', action='%s'\n",
        fsm->level, fsm->pcr_index,
        curr_state->name, curr_state->action);

    if ((event != NULL) && (verbose & DEBUG_FSM_FLAG)) {
        hex = getHexString(event->rgbPcrValue, event->ulPcrValueLength);
        DEBUG_FSM("[RM%02d-PCR%02d] eventtype=0x%x, digest=0x%s\n",
            fsm->level, fsm->pcr_index,
            event->eventType, hex);
        free(hex);
    }

    if (eventWrapper->event_type == OPENPTS_DUMMY_EVENT) {
        // DEBUG("flash FSM\n");
        /* Flash the trans chain */
        hit_trans = NULL;
        while (trans != NULL) {
            if (!strcmp(trans->source, curr_state->id)) {
                /* ID HIT, this is the trans from current state */

                /* Is this Final (last)? */
                if (!strcmp(trans->target, UML2_SD_FINAL_STATE_STRING)) {
                    /* Final state => PENPTS_FSM_FINISH_WO_HIT */
                    DEBUG_FSM("[RM%02d-PCR%02d] Final state! move to the next snapshot\n",
                        fsm->level, fsm->pcr_index);
                    fsm->status = OPENPTS_FSM_FINISH;
                    return OPENPTS_FSM_FINISH_WO_HIT;
                }

                /* More stats */
                hit_trans = trans;

                if (trans->last_flag == LAST_FLAG_EQ) {
                    DEBUG_FSM("check last == true\n");
                    if (eventWrapper->last == 1) {
                        /* Hit */
                        break;
                    }
                } else if (trans->last_flag == LAST_FLAG_NEQ) {
                    DEBUG_FSM("check last != true\n");
                    if (eventWrapper->last == 0) {
                        /* Hit */
                        break;
                    }
                } else {
                    // DEBUG_FSM("last - don't care\n");
                }
            }  // hit
            trans = trans->next;
        }  // while


        if (hit_trans != NULL) {
            // DEBUG("hit_trans\n");
            hit = 1;  // SKIP with this trans
            DEBUG_FSM("[RM%02d-PCR%02d] '%s' -> '%s'\n",
                    fsm->level, fsm->pcr_index,
                    hit_trans->source, hit_trans->target);
            fsm->curr_state = getSubvertex(fsm, hit_trans->target);

            if (fsm->curr_state != NULL) {
                /* doActivity, update properties */
                rc = doActivity(ctx, (char *)fsm->curr_state->action, NULL);  // action.c
                if (rc == OPENPTS_FSM_FLASH) {
                    /* last event, Flash FSM */
                    DEBUG_FSM("\t\tFlash FSM (don't care trans)\n");

                    rc = updateFsm(ctx, fsm, NULL);

                    if (rc == OPENPTS_FSM_FINISH_WO_HIT) {
                        rc = OPENPTS_FSM_FINISH;
                    } else {
                        ERROR("updateFsm - flash FSM was failed\n");
                        rc = OPENPTS_FSM_ERROR;
                    }
                } else if (rc == OPENPTS_FSM_TRANSIT) {
                    /* transit FSM */
                    DEBUG_FSM("\t\tFlash FSM before transit \n");

                    rc = updateFsm(ctx, fsm, NULL);  // flash FSM

                    if  (rc == OPENPTS_FSM_FINISH_WO_HIT) {
                        rc = OPENPTS_FSM_TRANSIT;
                    } else {
                        ERROR("updateFsm - FSM did not finish\n");
                        rc = OPENPTS_FSM_ERROR;
                    }
                } else if (rc == OPENPTS_FSM_ERROR) {
                    ERROR("updateFsm - FSM doActivity False\n");
                    return rc;
                } else if (rc == OPENPTS_FSM_MIGRATE_EVENT) {
                    TODO("updateFsm - OPENPTS_FSM_MIGRATE_EVENT \n");
                    return rc;
                } else if (rc == OPENPTS_FSM_SUCCESS) {
                    rc = updateFsm(ctx, fsm, eventWrapper);
                } else if (rc == PTS_INTERNAL_ERROR) {
                    // TODO  << INFO:(TODO) action.c:97 addBIOSAction() - eventWrapper is NULL
                    rc = updateFsm(ctx, fsm, eventWrapper);
                } else {
                    TODO("updateFsm() - rc = %d, call updateFsm() again\n", rc);
                    rc = updateFsm(ctx, fsm, eventWrapper);
                }
            }  // curr state
        } else {  // hit
            TODO("no trans\n");
        }
    } else {
        /* check trans chain */
        // DEBUG("updateFsm - check trans\n");
        while (trans != NULL) {
            type_check = 0;
            digest_check = 0;
            dont_care = 0;

            if (!strcmp(trans->source, curr_state->id)) {
                /*  ID HIT, this is the trans from current state */

                /* check the last flag */
                hit_trans = trans;
                last_check = 3;

                if (trans->last_flag == LAST_FLAG_EQ) {
                    DEBUG_FSM("check last == true\n");
                    if (eventWrapper->last == 1) {
                        /* Hit */
                        last_check = 1;
                    } else {
                        last_check = -1;
                    }
                } else if (trans->last_flag == LAST_FLAG_NEQ) {
                    DEBUG_FSM("check last != true\n");
                    if (eventWrapper->last == 0) {
                        /* Hit */
                        last_check = 1;
                    } else {
                        last_check = -1;
                    }
                } else {
                    // DEBUG_FSM("last - don't care\n");
                }

                if (last_check == 1) {
                    DEBUG_FSM("last event push the FSM\n");
                } else {
                    // DEBUG("NOT last event??? last_check = %d\n", last_check);
                    // DEBUG("NOT last event??? eventWrapper->last = %d\n", eventWrapper->last);
                    // DEBUG("NOT last event??? trans->last_flag = %d\n", trans->last_flag);
                }


                /* check the event type */

                if (trans->eventTypeFlag == EVENTTYPE_FLAG_EQUAL) {
                    // DEBUG_FSM("eventtype == %d - ", trans->eventType);
                    if (trans->eventType == event->eventType) {
                        /* TYPE MATCH */
                        // DEBUG_FSM("- valid\n");
                        type_check = 1;
                    } else {
                        // DEBUG_FSM("- invalid type %d(model) != %d(iml)\n", trans->eventType, event->eventType);
                        type_check = -1;
                    }
                } else if (trans->eventTypeFlag == EVENTTYPE_FLAG_NOT_EQUAL) {
                    // DEBUG_FSM("eventtype != %d - ", trans->eventType);
                    if (trans->eventType != event->eventType) {
                        /* TYPE MATCH */
                        DEBUG_FSM("\t\t type %x(trans) != %x(event) \n", trans->eventType, event->eventType);
                        type_check = 2;
                    } else {
                        // DEBUG_FSM("- invalid type %d(model) == %d(iml)\n", trans->eventType, event->eventType);
                        type_check = -1;
                    }
                } else {
                    // DEBUG_FSM("eventtype == %d - don't care\n", trans->eventType);
                    type_check = 3;
                    dont_care++;
                }

                /* check the digest */
                if (trans->digestFlag == DIGEST_FLAG_EQUAL) {
                    // DEBUG_FSM("digest -");
                    if (!memcmp(trans->digest,
                                event->rgbPcrValue,
                                event->ulPcrValueLength)) {
                        /* DIGEST MATCH */
                        digest_check = 1;
                        // DEBUG_FSM("- valid\n");
                    } else {
                        digest_check = -1;
                        // DEBUG_FSM("- invalid\n");
                    }
                } else if (trans->digestFlag == DIGEST_FLAG_IGNORE) {
                    /* Behavior Model */
                    // DEBUG_FSM("digest - ignore\n");
                    digest_check = 2;
                } else {
                    // DEBUG_FSM("digest - don't care\n");
                    digest_check = 3;
                    dont_care++;
                }

                /* check the counter */
                counter_check = 3;
                if (trans->counter_flag == COUNTER_FLAG_LT) {
                    /* count < name */
                    int count;
                    count = getCountFromProperty(ctx, trans->counter_name);

                    if (ctx->count < count) {
                        DEBUG_FSM("COUNTER[%s] %d < %d - HIT\n",
                            trans->counter_name, ctx->count, count);
                        counter_check = 1;  // HIT
                    } else {
                        DEBUG_FSM("COUNTER[%s] %d < %d - MISS\n",
                            trans->counter_name, ctx->count, count);
                        counter_check = -1;  // MISS
                    }

                } else if (trans->counter_flag == COUNTER_FLAG_GTE) {
                    /* count >= name */
                    int count;
                    count = getCountFromProperty(ctx, trans->counter_name);

                    // TODO at this moment we ignore >= condition,
                    if (ctx->count >= count) {
                        DEBUG_FSM("[RM%02d-PCR%02d] COUNTER[%s] %d >=  %d - HIT\n",
                            fsm->level, fsm->pcr_index,
                            trans->counter_name,
                            ctx->count, count);
                        counter_check = 1;  // HIT
                    } else {
                        DEBUG_FSM("COUNTER[%s] %d >=  %d - MISS\n",
                            trans->counter_name, ctx->count, count);
                        counter_check = -1;  // MISS
                    }
                } else {
                    // DEBUG_FSM("counter - don't care\n");
                }


                /* Judge */
                // if ((type_check == 1) && (digest_check == 1)) {
                if ((type_check > 0) && (digest_check > 0) && (counter_check > 0) && (last_check > 0)) {
                    /* Hit this Trans */
                    /* If Final state, switch to next snapshot */
                    if (!strcmp(trans->target, UML2_SD_FINAL_STATE_STRING)) {
                        /* Final state */
                        DEBUG_FSM("\tPCR[%d] level %d, Final state!! move to the next snapshot\n",
                            fsm->pcr_index, fsm->level);
                        // ERROR("PCR[%d] level %d, Final\n", fsm->pcr_index, fsm->level);
                        fsm->status = OPENPTS_FSM_FINISH;
                        return OPENPTS_FSM_FINISH_WO_HIT;  // FINAL
                    }

                    /* create FSM-IML link */
                    eventWrapper->fsm_trans = trans;
                    trans->event = (void *) event;   // note) hold the last link of looped trans
                    trans->event_num++;  // # of shared trans > 1
                    hit = 1;

                    /* next trans */
                    if (dont_care == 2) {
                        // this transfer does not feed event,
                        // just move to next state and check again
                        DEBUG_FSM("[RM%02d-PCR%02d] '%s' -> '%s'\n",
                            fsm->level, fsm->pcr_index,
                            trans->source, trans->target);
                        fsm->curr_state = getSubvertex(fsm, trans->target);

                        if (fsm->curr_state != NULL) {
                            /* doActivity, update properties */
                            rc = doActivity(ctx, (char *)fsm->curr_state->action, NULL);  // action.c
                            if (rc == OPENPTS_FSM_FLASH) {
                                /* last event, Flash FSM */
                                DEBUG_FSM("\t\tFlash FSM (don't care trans)\n");

                                rc = updateFsm(ctx, fsm, NULL);

                                if (rc == OPENPTS_FSM_FINISH_WO_HIT) {
                                    rc = OPENPTS_FSM_FINISH;
                                } else {
                                    ERROR("flash FSM was failed\n");
                                    rc = OPENPTS_FSM_ERROR;
                                }
                            } else if (rc == OPENPTS_FSM_TRANSIT) {
                                /* transit FSM */
                                DEBUG_FSM("\t\tFlash FSM before transit \n");

                                rc = updateFsm(ctx, fsm, NULL);  // flash FSM

                                if (rc == OPENPTS_FSM_FINISH_WO_HIT) {
                                    rc = OPENPTS_FSM_TRANSIT;
                                } else {
                                    ERROR("updateFsm - FSM did not finish\n");
                                    rc = OPENPTS_FSM_ERROR;
                                }
                            } else if (rc == OPENPTS_FSM_ERROR) {
                                DEBUG("updateFsm - doActivity error\n");
                                // INFO("FSM validation of doActivity() was failed.
                                // (FSM state = %s)\n",fsm->curr_state->name);
                                addReason(ctx, "[PCR%02d-FSM] action '%s' fail at state '%s'",
                                    fsm->pcr_index, (char *)fsm->curr_state->action, fsm->curr_state->name);
                                return rc;
                            } else if (rc == OPENPTS_FSM_MIGRATE_EVENT) {
                                TODO("updateFsm - OPENPTS_FSM_MIGRATE_EVENT \n");
                                return rc;
                            } else if (rc == OPENPTS_FSM_SUCCESS) {
                                rc = updateFsm(ctx, fsm, eventWrapper);
                            } else {
                                TODO("rc = %d\n", rc);
                                rc = updateFsm(ctx, fsm, eventWrapper);
                            }
                        }
                        break;
                    } else {
                        /* Trans */
                        DEBUG_FSM("[RM%02d-PCR%02d] %s -> %s - HIT (type=%d, digest=%d) <== new curr_state\n",
                            fsm->level, fsm->pcr_index,
                            trans->source, trans->target, type_check, digest_check);

                        fsm->curr_state = getSubvertex(fsm, trans->target);

                        if (fsm->curr_state != NULL) {
                            /* doActivity, update properties */
                            rc = doActivity(ctx, (char *)fsm->curr_state->action, eventWrapper);  // action.c
                            if (rc == OPENPTS_FSM_FLASH) {
                                /* last event, Flash FSM */
                                DEBUG_FSM("[RM%02d-PCR%02d] Flash this FSM\n",
                                    fsm->level, fsm->pcr_index);

                                rc = updateFsm(ctx, fsm, NULL);

                                if (rc == OPENPTS_FSM_FINISH_WO_HIT) {
                                    rc = OPENPTS_FSM_FINISH;
                                } else {
                                    ERROR("updateFsm - flash FSM was failed, rc = %d\n", rc);
                                    rc = OPENPTS_FSM_ERROR;
                                }
                            } else if (rc == OPENPTS_FSM_TRANSIT) {
                                /* transit FSM */
                                DEBUG_FSM("\t\tFlash FSM before transit \n");

                                rc = updateFsm(ctx, fsm, NULL);

                                if (rc == OPENPTS_FSM_FINISH_WO_HIT) {
                                    rc = OPENPTS_FSM_TRANSIT;
                                } else {
                                    ERROR("updateFsm - FSM did not finish\n");
                                    rc = OPENPTS_FSM_ERROR;
                                }
                            } else if (rc == OPENPTS_FSM_ERROR) {
                                ERROR("updateFsm - FSM doActivity False, rc = %d\n", rc);
                                return rc;
                            } else if (rc == OPENPTS_FSM_MIGRATE_EVENT) {
                                // DEBUG("updateFsm - OPENPTS_FSM_MIGRATE_EVENT \n");
                                return rc;
                            } else if (rc == OPENPTS_FSM_SUCCESS) {
                                rc = OPENPTS_FSM_SUCCESS;
                            } else {
                                /* */
                                // DEBUG("rc = %d -> 0\n");  // fsm.c:1070 rc = 6 -> 0
                                rc = OPENPTS_FSM_SUCCESS;
                            }
                        } else {
                            ERROR("curr_state is NULL\n");
                            rc = OPENPTS_FSM_ERROR;
                            return rc;
                        }
                        break;
                    }
                } else {
                    // judge
                }
            }  // if trans hit
            trans = trans->next;
        }  // while
    }  // DUMMY

    /* MISS ALL? */
    if (hit == 0) {
        // 20101118 SM Reason generated at iml.c
        DEBUG_FSM("[RM%02d-PCR%02d] No transition => rc = OPENPTS_FSM_ERROR\n",
            fsm->level, fsm->pcr_index);

        rc = OPENPTS_FSM_ERROR;
    }

    /* success ? */
    return rc;
}


/**
 * Copy FSM
 *
 *   BHV->BIN
 *
 *   called from rm.c
 */
OPENPTS_FSM_CONTEXT *copyFsm(OPENPTS_FSM_CONTEXT *src_fsm) {
    OPENPTS_FSM_CONTEXT * dst_fsm = NULL;

    OPENPTS_FSM_Subvertex  *src_fsm_sub;
    OPENPTS_FSM_Subvertex  *dst_fsm_sub = NULL;
    OPENPTS_FSM_Subvertex  *dst_fsm_sub_prev = NULL;

    OPENPTS_FSM_Transition *src_fsm_trans;
    OPENPTS_FSM_Transition *dst_fsm_trans = NULL;
    OPENPTS_FSM_Transition *dst_fsm_trans_prev = NULL;

    int count;

    DEBUG_FSM("copyFsm - start, PCR[%d]\n", src_fsm->pcrIndex);

    if (src_fsm == NULL) {
        DEBUG("src_fsm == NULL, SKIP COPY\n");
        return NULL;
    }

    /* New FSM */
    dst_fsm = (OPENPTS_FSM_CONTEXT *) malloc(sizeof(OPENPTS_FSM_CONTEXT));
    if (dst_fsm  == NULL) {
        ERROR("copyFsm - no memory\n");
        return NULL;
    }
    memcpy((void *)dst_fsm, (void *)src_fsm, sizeof(OPENPTS_FSM_CONTEXT));

    /* delete BHV-FSM smalloc link */
    dst_fsm->uml_file = NULL;

    /* Copy Subvertexs */
    count = 0;
    src_fsm_sub = src_fsm->fsm_sub;
    if (src_fsm_sub == NULL) {
        ERROR("ERROR No FSM SUB\n");
        goto error;
    }

    while (src_fsm_sub != NULL) {
        /* malloc new sub */
        dst_fsm_sub = (OPENPTS_FSM_Subvertex *)
            malloc(sizeof(OPENPTS_FSM_Subvertex));
        /* copy */
        memcpy((void *)dst_fsm_sub,
               (void *)src_fsm_sub,
               sizeof(OPENPTS_FSM_Subvertex));

        /* next ptr */
        if (dst_fsm_sub_prev != NULL) {  // 2nd-
            dst_fsm_sub_prev->next = dst_fsm_sub;
            dst_fsm_sub->prev = dst_fsm_sub_prev;
        } else {  // 1st
            dst_fsm->fsm_sub = dst_fsm_sub;
        }
        dst_fsm_sub_prev = dst_fsm_sub;

        /* BHV-BIN link */
        dst_fsm_sub->link = src_fsm_sub;
        src_fsm_sub->link = dst_fsm_sub;

        /* go next */
        src_fsm_sub = src_fsm_sub->next;
        count++;
    }

    DEBUG_FSM("%d Subvertex was copied\n", count);

    /* Copy Transitions */
    count = 0;
    src_fsm_trans = src_fsm->fsm_trans;

    if (src_fsm_trans == NULL) {
        ERROR("ERROR No FSM TRANS\n");
        goto error;
    }

    while (src_fsm_trans != NULL) {
        /* malloc new sub */
        dst_fsm_trans = (OPENPTS_FSM_Transition *)
            malloc(sizeof(OPENPTS_FSM_Transition));
        /* copy */
        memcpy((void *)dst_fsm_trans,
               (void *)src_fsm_trans,
               sizeof(OPENPTS_FSM_Transition));

        /* ptr */
        if (dst_fsm_trans_prev != NULL) {  // 2nd-
            dst_fsm_trans_prev->next = dst_fsm_trans;
            dst_fsm_trans->prev = dst_fsm_trans_prev;
        } else {  // 1st
            dst_fsm->fsm_trans = dst_fsm_trans;
        }
        dst_fsm_trans_prev = dst_fsm_trans;

        /* links to sub, Start and Final */
        // TODO(munetoh) does NULL check need?
        src_fsm_sub = src_fsm_trans->source_subvertex;
        if (src_fsm_sub != NULL) {
            dst_fsm_trans->source_subvertex = src_fsm_sub->link;
        } else {
            ERROR("ERROR BHV trans %s source_subvertex is NULL\n",
                src_fsm_trans->source);
        }

        src_fsm_sub = src_fsm_trans->target_subvertex;

        if (src_fsm_sub != NULL)
            dst_fsm_trans->target_subvertex = src_fsm_sub->link;

        /* link between BIN and BHV FSM */
        dst_fsm_trans->link = src_fsm_trans;
        src_fsm_trans->link = dst_fsm_trans;

        /* go next */
        src_fsm_trans = src_fsm_trans->next;
        count++;
    }

    DEBUG_FSM("%d Transition was copied\n", count);
    DEBUG_FSM("copyFsm - done\n");

    return dst_fsm;

  error:
    if (dst_fsm != NULL) {
        free(dst_fsm);
    }
    return NULL;
}

/**

  S ----T--->Old_Sub

  S ----T--->New_Sub(--->Old_Sub)

          T(loop)
          |
  A---T---B---C
     

          T(loop)
           |   A
           V   |
  A---T---BN   B--C    << NG


*/
int changeTargetSubvertex(
        OPENPTS_FSM_CONTEXT *fsm_ctx,
        OPENPTS_FSM_Subvertex *old_sub,    // B
        OPENPTS_FSM_Subvertex *new_sub) {  // BN
    int rc = 0;
    OPENPTS_FSM_Transition *fsm_trans;
    int count = 0;

    fsm_trans = fsm_ctx->fsm_trans;

    /* check all trans to B */
    while (fsm_trans != NULL) {
        if (fsm_trans->target_subvertex == old_sub) {
            fsm_trans->target_subvertex = new_sub;
            snprintf(fsm_trans->target,
                     sizeof(fsm_trans->target),
                    "%s", new_sub->id);
        }
        fsm_trans = fsm_trans->next;
        count++;
    }

    return rc;
}

/**

  S ----T--->Old_Sub

  S ----T--->New_Sub(--->Old_Sub)

          T(loop)
          |
  A---T---B---C
     

               T(loop)
               |
  A---T---BN   B--C    << OK


*/
int changeTransTargetSubvertex(
        OPENPTS_FSM_CONTEXT *fsm_ctx,
        OPENPTS_FSM_Subvertex *old_sub,    // B
        OPENPTS_FSM_Subvertex *new_sub) {  // BN
    int rc = 0;
    OPENPTS_FSM_Transition *fsm_trans;
    int count = 0;

    fsm_trans = fsm_ctx->fsm_trans;

    /* check all trans to B */
    while (fsm_trans != NULL) {
        if (fsm_trans->target_subvertex == old_sub) {
            /* HIT */
            if (fsm_trans->target_subvertex == fsm_trans->source_subvertex) {
                // LOOP, belong to old sub
                DEBUG_FSM("changeTransTargetSubvertex - keep loop '%s) \n",
                    fsm_trans->source);
            } else {
                // move to new sub
                fsm_trans->target_subvertex = new_sub;
                snprintf(fsm_trans->target,
                         sizeof(fsm_trans->target),
                        "%s", new_sub->id);
                DEBUG_FSM("changeTransTargetSubvertex - trans move to new sub (%s -> %s)\n",
                    fsm_trans->source, fsm_trans->target);
            }
        }
        fsm_trans = fsm_trans->next;
        count++;
    }

    return rc;
}


/**

20100617 new alg

Behavior FSM

               
FSM   [A]---Ta---[B]---Tc---[C]
                  |
                 Tb(loop)


                  |<--loop-->|
FSM   [A]---Ta---[B]---Tb---[B]---Tc---[C]
            |          |          |
EW          e0       e1-e3        e4
                      (3)

Transfer Behavior to Binary FSM


                              |<--loop-->|
FSM   [A]---Ta---[B0]---Tb0--[B]---Tb---[B]---Tc---[C]
            |           |           |         |
EW          e0          e1         e2-e3      e4
                                   (2)

                                          |<--loop-->|
FSM   [A]---Ta---[B0]---Tb0--[B1]---tb1--[B]---Tb---[B]---Tc---[C]
            |           |           |          |          |
EW          e0          e1          e2         e3         e4
                                               (1)


FSM   [A]---Ta---[B0]---Tb0--[B1]---tb1--[B2]---Tb---[B]---Tc---[C]
            |           |           |           |          |
EW          e0          e1          e2          e3         e4
                                                (0)

*/

int insertFsmNew(
        OPENPTS_FSM_CONTEXT *fsm_ctx,       // BIN-FSM
        OPENPTS_FSM_Transition *fsm_trans,  // target Trans
        OPENPTS_PCR_EVENT_WRAPPER *eventWrapper) {
    int rc =0;
    OPENPTS_FSM_Subvertex *prev_sub;  // STRUCT LINK
    OPENPTS_FSM_Subvertex *new_sub;
    OPENPTS_FSM_Subvertex *dst_sub;
    OPENPTS_FSM_Transition *prev_trans;  // STRUCT LINK
    OPENPTS_FSM_Transition *new_trans;
    TSS_PCR_EVENT *event;

    DEBUG_FSM("insertFsm - start\n");

    /* check input */
    if (fsm_trans == NULL) {
        ERROR("ERROR fsm_trans == NULL\n");
        return -1;
    }
    if (fsm_trans->source_subvertex == NULL) {
        ERROR("ERROR fsm_trans->source_subvertex == NULL, %s -> %s\n",
            fsm_trans->source, fsm_trans->target);
        return -1;
    }
    if (fsm_trans->target_subvertex == NULL) {
        ERROR("ERROR fsm_trans->target_subvertex == NULL\n");
        return -1;
    }

    if (eventWrapper == NULL) {
        return -1;
    }

    /* start */

    event = eventWrapper->event;

    if (fsm_trans->source_subvertex == fsm_trans->target_subvertex) {
        /* OK, this is LOOP,  */
        DEBUG_FSM("Loop (%s->%s) has %d events\n",
            fsm_trans->source, fsm_trans->target, fsm_trans->event_num);

        /* Base subvertex, B */
        dst_sub = fsm_trans->target_subvertex;

        /* Add new subvertex, BN (->B) */

        new_sub = (OPENPTS_FSM_Subvertex *)
            malloc(sizeof(OPENPTS_FSM_Subvertex));
        if (new_sub == NULL) {
            ERROR("no memory");
            return -1;
        }
        /* copy */
        memcpy(new_sub,
               fsm_trans->source_subvertex,
               sizeof(OPENPTS_FSM_Subvertex));

        snprintf(new_sub->id,  sizeof(new_sub->id),
                 "%s_LOOP_%d",
                 dst_sub->id, fsm_trans->copy_num);
        snprintf(new_sub->name, sizeof(new_sub->name),
                 "%s_LOOP_%d",
                 dst_sub->name, fsm_trans->copy_num);
        fsm_ctx->subvertex_num++;

        /* Update the subvetex chain, A-B => A-BN-B  */

        /* A <-> BN */
        prev_sub       = dst_sub->prev;
        prev_sub->next  = new_sub;
        new_sub->prev  = prev_sub;

        /* BN <-> B */
        new_sub->next  = dst_sub;
        dst_sub->prev  = new_sub;

        /* Any trans to B move to BN */
        // BN->B trans is open
        rc = changeTransTargetSubvertex(
                fsm_ctx,
                dst_sub,   // B
                new_sub);  // BN

        DEBUG_FSM("\tnew sub id = %s, name = %s added\n",
            new_sub->id, new_sub->name);

        /*Next Updatre the Transition */

        if (fsm_trans->event_num > 1) {
            /* Many loops, B-B -> BN-B-B, add new Trans between BN and B */

            /* malloc */
            new_trans = (OPENPTS_FSM_Transition*)
                malloc(sizeof(OPENPTS_FSM_Transition));
            if (new_trans == NULL) {
                ERROR("no memory");
                return -1;
            }
            /* copy */
            memcpy(new_trans,
                   fsm_trans,
                   sizeof(OPENPTS_FSM_Transition));

            /* update the transition struct chain */

            prev_trans = fsm_trans->prev;
            prev_trans->next = new_trans;
            new_trans->prev  = prev_trans;

            new_trans->next = fsm_trans;
            fsm_trans->prev = new_trans;

            fsm_ctx->transition_num++;

            /* Update new Trans  */
            new_trans->source_subvertex = new_sub;
            snprintf(new_trans->source, sizeof(new_trans->source),
                     "%s", new_sub->id);

            new_trans->target_subvertex = dst_sub;
            snprintf(new_trans->target, sizeof(new_trans->target),
                     "%s", dst_sub->id);

            /* Update event link */
            // trans -> event
            new_trans->event = eventWrapper;  // TSS_PCR_EVENT_WRAPPER
            new_trans->event_num = 1;
            // event -> trans
            eventWrapper->fsm_trans = new_trans;

            /* Update Original Trans */
            fsm_trans->event_num--;
            fsm_trans->copy_num++;

            /* Copy digest value to trans  */
            new_trans->digestFlag = DIGEST_FLAG_EQUAL;
            new_trans->digestSize = event->ulPcrValueLength;
            new_trans->digest = malloc(event->ulPcrValueLength);
            if (new_trans->digest == NULL) {
                ERROR("no memory\n");
                return -1;
            }
            memcpy(new_trans->digest, event->rgbPcrValue, event->ulPcrValueLength);

            DEBUG_FSM("new  Trans BIN(%s -> %s)\n",
                      new_trans->source, new_trans->target);
            DEBUG_FSM("orig Trans BIN(%s -> %s) share = %d\n",
                      fsm_trans->source, fsm_trans->target, fsm_trans->event_num);

        } else if (fsm_trans->event_num == 1) {
            /* Last loop, B-B -> BN-B, just update the trans */

            /* Update new Trans  */
            fsm_trans->source_subvertex = new_sub;
            snprintf(fsm_trans->source, sizeof(new_trans->source),
                     "%s", new_sub->id);

            /* Copy digest value to FSM */
            fsm_trans->digestFlag = DIGEST_FLAG_EQUAL;
            fsm_trans->digestSize = event->ulPcrValueLength;
            fsm_trans->digest = malloc(event->ulPcrValueLength);
            if (fsm_trans->digest == NULL) {
                ERROR("no memory\n");
                return -1;
            }
            memcpy(fsm_trans->digest, event->rgbPcrValue, event->ulPcrValueLength);

            // DEBUG_FSM("\tupdate Trans %p->%p->%p\n",
            //          fsm_trans->prev, fsm_trans, fsm_trans->next);
            DEBUG_FSM("\tUpdate Trans BIN(%s -> %s)\n",
                      fsm_trans->source, fsm_trans->target);
        } else {
            ERROR("BAD LOOP\n");
        }
    } else {
        ERROR("Not a loop");
    }

    DEBUG_FSM("insertFsm - done\n");
    return rc;
}


/**
 *  remove the trans from transition chain
 */
int removeFsmTrans(
        OPENPTS_FSM_CONTEXT *fsm_ctx,
        OPENPTS_FSM_Transition * trans) {
    int rc =0;
    OPENPTS_FSM_Transition * trans_prev;
    OPENPTS_FSM_Transition * trans_next;

    trans_prev = trans->prev;
    trans_next = trans->next;

    /* remove link */
    if (trans_prev != NULL) {
        trans_prev->next = trans_next;
    } else {  // 1st trans
        fsm_ctx->fsm_trans = trans_next;
    }

    if (trans_next != NULL) {
        trans_next->prev = trans_prev;
    } else {  // last trans
        //
    }

    // TODO(munetoh) Free

    return rc;
}


/**
 * remove FSM subvertex
 */
int removeFsmSub(
        OPENPTS_FSM_CONTEXT *fsm_ctx,
        OPENPTS_FSM_Subvertex * sub) {
    int rc =0;

    OPENPTS_FSM_Subvertex * sub_prev;
    OPENPTS_FSM_Subvertex * sub_next;

    sub_prev = sub->prev;
    sub_next = sub->next;

    /* remove link */
    if (sub_prev != NULL) {
        sub_prev->next = sub_next;
    } else {  // 1st sub
        fsm_ctx->fsm_sub = sub_next;
    }
    if (sub_next != NULL) {
        sub_next->prev = sub_prev;
    } else {  // last sub
        //
    }

    // TODO(munetoh) Free

    return rc;
}

/**
 *  clean up FSM
 *   - delete unused BHV Transitions
 *   - delete unused BHV Subvertex
 *
 */
int cleanupFsm(OPENPTS_FSM_CONTEXT *fsm_ctx) {
    int rc = 0;
    int count = 0;
    int hit;
    OPENPTS_FSM_Transition * trans;
    OPENPTS_FSM_Transition * trans_next;

    OPENPTS_FSM_Subvertex * sub;
    OPENPTS_FSM_Subvertex * sub_next;

    if (fsm_ctx == NULL) {
        ERROR("ERROR No FSM TRANS\n");
        return -1;
    }

    DEBUG_FSM("cleanupFsm - start, PCR[%d]\n", fsm_ctx->pcrIndex);

    /* Delete BHV Transitions */

    trans = fsm_ctx->fsm_trans;

    if (trans == NULL) {
        ERROR("ERROR No FSM TRANS\n");
        return -1;
    }

    count = 0;
    while (trans != NULL) {
        trans_next = trans->next;
        if (trans->digestFlag == DIGEST_FLAG_IGNORE) {
            DEBUG_FSM("\tHIT %s->%s - removed\n",
                      trans->source, trans->target);
            rc = removeFsmTrans(fsm_ctx, trans);  // remove Trans
            if (rc < 0) {
                ERROR("removeFsmTrans of %s -> %s was failed\n",
                      trans->source, trans->target);
                return -1;
            }
            count++;
        } else {
            // printf("MISS \n");
        }
        trans = trans_next;
    }

    DEBUG_FSM("cleanupFsm - %d trans was removed\n", count);
    fsm_ctx->transition_num -= count;

    /* Delete state which does not have incomming trans */
    sub = fsm_ctx->fsm_sub;
    if (sub == NULL) {
        ERROR("ERROR No FSM SUB\n");
        return -1;
    }

    count = 0;
    while (sub != NULL) {
        sub_next = sub->next;
        if (!strcmp(sub->id, "Start")) {
            // START state
        } else if (!strcmp(sub->id, "Final")) {
            // FINAL state
        } else {
            // Other states
            /* check trans */
            trans = fsm_ctx->fsm_trans;
            hit = 0;
            while (trans != NULL) {
                if (!strcmp(trans->target, sub->id)) {
                    hit++;
                    if (trans->target_subvertex == sub) {
                        // hit++;
                        // TODO(munetoh)
                        //   EV_S_CRTM_VERSION is not detected. BAD link:-(
                        // break;
                    } else {
                        // printf("ERROR BAD LINK\n");
                        // printf("SUB id=%s name=%s \n",
                        //   sub->id,sub->name);
                        // printf("TRANS %s ->  %s \n",
                        //   trans->source,trans->target);
                    }
                }
                trans = trans->next;
            }

            if (hit == 0) {
                DEBUG_FSM("\tSub %p  id=%s name=%s not used\n",
                          sub, sub->id, sub->name);
                /* remove sub */
                removeFsmSub(fsm_ctx, sub);
            }
        }

        sub = sub_next;
    }

    DEBUG_FSM("cleanupFsm - %d trans was removed\n", count);
    fsm_ctx->subvertex_num -= count;


    /* Again, Delete trans which does not have source target */

    trans = fsm_ctx->fsm_trans;

    if (trans == NULL) {
        printf("ERROR No FSM TRANS\n");
        return -1;
    }

    count = 0;
    while (trans != NULL) {
        trans_next = trans->next;

        sub = getSubvertex(fsm_ctx, trans->source);
        if (sub == NULL) {
            DEBUG_FSM("\tMISSING SOURCE %s->%s\n",
                       trans->source, trans->target);
            removeFsmTrans(fsm_ctx, trans);
            count++;
        } else {
        }

        trans = trans_next;
    }


    DEBUG_FSM("cleanupFsm - %d trans was removed - missing source\n", count);
    fsm_ctx->transition_num -= count;


    DEBUG_FSM("cleanupFsm - done\n");
    return rc;
}











/**
 * write DOT State Diagram for Graphviz
 * dot -Tpng tests/bios_pcr0.dot -o tests/bios_pcr0.png; eog tests/bios_pcr0.png
 * @param ctx FSM_CONTEXT
 * @param filename dot filename to write
 *
 * Return
 *   PTS_SUCCESS
 *   PTS_OS_ERROR
 *   PTS_INTERNAL_ERROR
 */
int writeDotModel(OPENPTS_FSM_CONTEXT *ctx, char * filename) {
    int rc = PTS_SUCCESS;
    FILE *fp;
    int j;
    OPENPTS_FSM_Subvertex *sptr;
    OPENPTS_FSM_Transition *ptr;

    DEBUG("writeDotModel - start %s\n", filename);

    /* check */
    if (ctx == NULL) {
        ERROR("writeDotModel() - OPENPTS_FSM_CONTEXT is NULL\n");
        return PTS_INTERNAL_ERROR;
    }

    if (filename == NULL) {
        fp = stdout;
    } else {
        if ((fp = fopen(filename, "w")) == NULL) {
            ERROR("fopen fail %s\n", filename);
            return PTS_OS_ERROR;
        }
    }

    DEBUG_FSM("Subvertex  num= %d \n", ctx->subvertex_num);
    DEBUG_FSM("Transition num= %d \n", ctx->transition_num);

    fprintf(fp, "digraph G {\n");

    /* Subvertex */
    sptr =ctx->fsm_sub;

    while (sptr != NULL) {
        if (!strcmp(sptr->id, "Start")) {
            fprintf(fp, "\t%s [label =\"\", fillcolor=black];\n", sptr->id);
            // TODO(munetoh) fillcolor not work
        } else if (!strcmp(sptr->id, "Final")) {
            fprintf(fp, "\t%s [label =\"\", peripheries = 2];\n", sptr->id);
        } else if (strlen(sptr->action) > 0) {
            fprintf(fp, "\t%s [label=\"%s\\naction=%s\"];\n",
                    sptr->id,
                    sptr->name,
                    sptr->action);
        } else {
            fprintf(fp, "\t%s [label=\"%s\"];\n",
                    sptr->id,
                    sptr->name);
        }
        sptr = sptr->next;
    }

    /* Transition */
    ptr = ctx->fsm_trans;

    while (ptr != NULL) {
        DEBUG_FSM("\tTransition = (%s->%s)\n", ptr->source, ptr->target);
        /* cond */
        if (ptr->digestFlag == DIGEST_FLAG_EQUAL) {  // BIN
            fprintf(fp, "\t%s -> %s [label=\"",
                    ptr->source,
                    ptr->target);
            /* eventytpte */
            if (ptr->eventTypeFlag == EVENTTYPE_FLAG_EQUAL) {
                fprintf(fp, "eventtype == 0x%x, ", ptr->eventType);
            } else if (ptr->eventTypeFlag == EVENTTYPE_FLAG_NOT_EQUAL) {
                fprintf(fp, "eventtype != 0x%x, ", ptr->eventType);
            }
            /* digest */
            fprintf(fp, "\\nhexdigest == ");
            for (j = 0; j < ptr->digestSize; j++) {
                fprintf(fp, "%02x", ptr->digest[j]);
                // TODO(munetoh) Hex, also supports Base64
            }
            fprintf(fp, "\"];\n");
        } else {  // BHV
            fprintf(fp, "\t%s -> %s [label=\"%s\"];\n",
                    ptr->source,
                    ptr->target,
                    ptr->cond);
        }
        ptr = ptr->next;
    }

    fprintf(fp, "}\n");

    fclose(fp);

    DEBUG("writeDotModel - done\n");

    return rc;
}

/**
 * write CSV file, RFC 4180 style
 *
 * @param ctx FSM_CONTEXT
 * @param filename csv filename to write
 */
int writeCsvTable(OPENPTS_FSM_CONTEXT *ctx, char * filename) {
    int rc = 0;
    FILE *fp;
    int i;
    OPENPTS_FSM_Transition *ptr;

    /* check */
    if (filename == NULL) {
        ERROR("writeCsvTable - filename is NULL\n");
        return -1;
    }

    /* Open */
    if ((fp = fopen(filename, "w")) == NULL) {
        return -1;
    }

    fprintf(fp,
        "current state,condition type(hex), condition digest,next state\n");

    ptr = ctx->fsm_trans;
    for (i = 0; i < ctx->transition_num; i++) {
        fprintf(fp, "%s, ", getSubvertexName(ctx, ptr->source));

        if (ptr->eventTypeFlag == 1) {
            fprintf(fp, "type==0x%x,", ptr->eventType);
        } else if (ptr->eventTypeFlag == 1) {
            fprintf(fp, "type!=0x%x,", ptr->eventType);
        } else {
            fprintf(fp, ",");
        }


        if (ptr->digestFlag == 1) {
            fprintf(fp, "digest==0x");
            // for (i=0;i<DIGEST_SIZE;i++) fprintf(fp,"%02x",ptr->digest[i]);
            fprintf(fp, ",");
        } else if (ptr->digestFlag == 2) {
            fprintf(fp, "digest==base64,");
        } else {
            fprintf(fp, ",");
        }
        fprintf(fp, "%s\n", getSubvertexName(ctx, ptr->target));

        ptr = ptr->next;
    }

    /* close */
    fclose(fp);
    return rc;
}


/**
 * print FSM State Diagram
 *
 * @param ctx FSM_CONTEXT
 */
int printFsmModel(OPENPTS_FSM_CONTEXT *ctx) {
    int rc = 0;
    int i, j;
    OPENPTS_FSM_Transition *ptr;

    printf("ctx->transition_num = %d\n", ctx->transition_num);

    printf("trans\t\tcurrent state\t\t\tcondition\t\t\\ttnext state\n");
    printf("  id  \t\t\t\t\ttype(hex)\tdigest(hex)\n");
    printf("----------------------------------------------------------------------------------------------\n");


    ptr = ctx->fsm_trans;
    for (i = 0; i < ctx->transition_num; i++) {
        if (ptr == NULL) {
            ERROR("PTR is NULL at %d\n", i);
            return -1;
        }
        printf("%5d ", i);
        printf("%30s ", getSubvertexName(ctx, ptr->source));

        if (ptr->eventTypeFlag == 1) {
            printf(" 0x%08x  ", ptr->eventType);
        } else if (ptr->eventTypeFlag == 1) {
            printf("!0x%08x  ", ptr->eventType);
        } else {
            printf("             ");
        }

        if (ptr->digestFlag == 1) {
            printf("0x");
            for (j = 0; j < ptr->digestSize; j++) printf("%02x", ptr->digest[j]);
            printf(" ");
        } else if (ptr->digestFlag == 2) {
            printf("base64                                     ");
        } else {
            printf("                                           ");
        }
        printf("%-30s\n", getSubvertexName(ctx, ptr->target));

        ptr = ptr->next;
    }

    return rc;
}












