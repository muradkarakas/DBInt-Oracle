/*
 * OCILIB - C Driver for Oracle (C Wrapper for Oracle OCI)
 *
 * Website: http://www.ocilib.net
 *
 * Copyright (c) 2007-2016 Vincent ROGIER <vince.rogier@ocilib.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ocilib_internal.h"

/* ********************************************************************************************* *
 *                             PRIVATE FUNCTIONS
 * ********************************************************************************************* */

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionClose
 * --------------------------------------------------------------------------------------------- */

boolean OCI_SubscriptionClose
(
    OCI_Subscription *sub
)
{
    boolean alloc = FALSE;

    OCI_CALL_DECLARE_CONTEXT(TRUE)
        
    OCI_CHECK(NULL == sub, FALSE);
  
    OCI_CALL_CONTEXT_SET_FROM_CONN(sub->con)

#if OCI_VERSION_COMPILE >= OCI_10_2

    /* unregister the subscription if connection still alive */

    if (sub->subhp)
    {
        if (!sub->con)
        {
            sub->con = OCI_ConnectionCreate(sub->saved_db, sub->saved_user,
                                            sub->saved_pwd, OCI_SESSION_DEFAULT);

            alloc = TRUE;
        }

        if (sub->con)
        {
            OCI_EXEC(OCISubscriptionUnRegister(sub->con->cxt, sub->subhp, sub->err, (ub4)OCI_DEFAULT))
 
            if (alloc)
            {
                OCI_ConnectionFree(sub->con);
            }
        }
    }

    /* free OCI handle */

    OCI_HandleFree((dvoid *) sub->subhp, OCI_HTYPE_SUBSCRIPTION);

    /* close error handle */

    if (sub->err)
    {
        OCI_HandleFree(sub->err, OCI_HTYPE_ERROR);
    }

#endif

    /* free event data */

    OCI_FREE(sub->event.dbname)
    OCI_FREE(sub->event.objname)
    OCI_FREE(sub->event.rowid)

    /* free strings */

    OCI_FREE(sub->saved_db)
    OCI_FREE(sub->saved_user)
    OCI_FREE(sub->saved_pwd)
    OCI_FREE(sub->name)

    return OCI_STATUS;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionDetachConnection
 * --------------------------------------------------------------------------------------------- */

boolean OCI_SubscriptionDetachConnection
(
    OCI_Connection *con
)
{
    OCI_List *list = OCILib.subs;
    OCI_Item *item = NULL;

    OCI_CHECK(NULL == list, FALSE);

    if (list->mutex)
    {
        OCI_MutexAcquire(list->mutex);
    }

    item = list->head;

    /* for each item in the list, check the connection */

    while (item)
    {
        OCI_Subscription * sub = (OCI_Subscription *) item->data;

        if (sub && (sub->con == con))
        {
            sub->con = NULL;

            sub->saved_db   = ostrdup(con->db);
            sub->saved_user = ostrdup(con->user);
            sub->saved_pwd  = ostrdup(con->pwd);
        }

        item = item->next;
    }

    if (list->mutex)
    {
        OCI_MutexRelease(list->mutex);
    }

    return TRUE;
}

/* ********************************************************************************************* *
 *                            PUBLIC FUNCTIONS
 * ********************************************************************************************* */

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionRegister
 * --------------------------------------------------------------------------------------------- */

OCI_Subscription * OCI_API OCI_SubscriptionRegister
(
    OCI_Connection *con,
    const otext    *name,
    unsigned int    type,
    POCI_NOTIFY     handler,
    unsigned int    port,
    unsigned int    timeout
)
{
    OCI_Subscription *sub = NULL;
    OCI_Item *item        = NULL;

    OCI_CALL_ENTER(OCI_Subscription*, NULL)
    OCI_CALL_CHECK_DATABASE_NOTIFY_ENABLED()
    OCI_CALL_CHECK_PTR(OCI_IPC_CONNECTION, con)
    OCI_CALL_CHECK_PTR(OCI_IPC_PROC, handler)
    OCI_CALL_CHECK_PTR(OCI_IPC_STRING, name)
    OCI_CALL_CONTEXT_SET_FROM_CONN(con)

#if OCI_VERSION_COMPILE >= OCI_10_2

    /* create subscription object */

    item = OCI_ListAppend(OCILib.subs, sizeof(*sub));

    if (item)
    {
        sub = (OCI_Subscription *) item->data;

        /* allocate error handle */

        OCI_STATUS = OCI_HandleAlloc(con->env, (dvoid **)(void *)&sub->err, OCI_HTYPE_ERROR);

        /* allocate subscription handle */

        OCI_STATUS = OCI_STATUS && OCI_HandleAlloc(con->env, (dvoid **)(void *)&sub->subhp, OCI_HTYPE_SUBSCRIPTION);

        if (OCI_STATUS)
        {
            ub4     attr   = 0;
            int     dbsize = -1;
            dbtext *dbstr  = NULL;

            sub->con       = con;
            sub->env       = con->env;
            sub->port      = (ub4) port;
            sub->timeout   = (ub4) timeout;
            sub->handler   = handler;
            sub->type      = type;
            sub->name      = ostrdup(name);
            sub->event.sub = sub;

            /* set/get port number */

            if (sub->port > 0)
            {

                OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_PORTNO, sub->subhp, &sub->port, sizeof(sub->port))
            }
            else
            {
                OCI_GET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_PORTNO, sub->subhp, &sub->port,NULL)
            }

            /* set timeout */

            if(sub->timeout > 0)
            {
                OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_TIMEOUT, sub->subhp, &sub->timeout, sizeof(sub->timeout))
            }

            /* name  */

            dbstr = OCI_StringGetOracleString(sub->name, &dbsize);

            OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_NAME, sub->subhp, dbstr, dbsize)

            OCI_StringReleaseOracleString(dbstr);

            /* namespace for CDN */

            attr =  OCI_SUBSCR_NAMESPACE_DBCHANGE;
            OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_NAMESPACE, sub->subhp, &attr, sizeof(attr))

            /* protocol for CDN */

            attr =  OCI_SUBSCR_PROTO_OCI;
            OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_RECPTPROTO, sub->subhp, &attr, sizeof(attr))

           /* On MSVC, casting a function pointer to a data pointer generates a warning.
              As there is no other to way to do regarding the OCI API, let's disable this
              warning just the time to set the callback attribute to the subscription handle */

            #ifdef _MSC_VER
            #pragma warning(disable: 4054)
            #endif

            /* internal callback handler */

            OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_CALLBACK, sub->subhp, OCI_ProcNotifyChanges, 0)

            #ifdef _MSC_VER
            #pragma warning(default: 4054)
            #endif

            /* RowIds handling */

            if (sub->type & OCI_CNT_ROWS)
            {
                attr = TRUE;
                OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_CHNF_ROWIDS, sub->subhp, &attr, sizeof(attr))
            }

            /* set subscription context pointer to our subscription structure */

            OCI_SET_ATTRIB(OCI_HTYPE_SUBSCRIPTION, OCI_ATTR_SUBSCR_CTX, sub->subhp, sub, 0)

            /* all attributes set, let's register the subscription ! */

            OCI_EXEC(OCISubscriptionRegister(sub->con->cxt, &sub->subhp, (ub2) 1, sub->err,(ub4) OCI_DEFAULT))
        }
    }

    if (OCI_STATUS)
    {
        OCI_RETVAL = sub;
    } 
    else if (sub)
    {
        OCI_SubscriptionClose(sub);
        OCI_ListRemove(OCILib.subs, sub);
        OCI_FREE(sub)
    }

#else

    OCI_NOT_USED(name)
    OCI_NOT_USED(type)
    OCI_NOT_USED(handler)
    OCI_NOT_USED(port)
    OCI_NOT_USED(timeout)
    OCI_NOT_USED(con)
    OCI_NOT_USED(item)

#endif

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionUnregister
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_SubscriptionUnregister
(
    OCI_Subscription *sub
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NOTIFY, sub)
    OCI_CALL_CONTEXT_SET_FROM_ERR(sub->err)

    OCI_RETVAL = OCI_STATUS = OCI_SubscriptionClose(sub);

    OCI_ListRemove(OCILib.subs, sub);

    OCI_FREE(sub);

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionAddStatement
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_SubscriptionAddStatement
(
    OCI_Subscription *sub,
    OCI_Statement    *stmt
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NOTIFY, sub)
    OCI_CALL_CHECK_PTR(OCI_IPC_STATEMENT, stmt)
    OCI_CALL_CHECK_STMT_STATUS(stmt, OCI_STMT_PREPARED)
    OCI_CALL_CONTEXT_SET_FROM_CONN(sub->con)

#if OCI_VERSION_COMPILE >= OCI_10_2

    /* register the statement query if provided */

    if (sub->type & OCI_CNT_OBJECTS)
    {
        OCI_SET_ATTRIB(OCI_HTYPE_STMT, OCI_ATTR_CHNF_REGHANDLE, stmt->stmt, sub->subhp, 0)

        OCI_STATUS = OCI_STATUS && OCI_Execute(stmt) && (NULL != OCI_GetResultset(stmt));
    }

#endif

    OCI_RETVAL = OCI_STATUS;

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionGetName
 * --------------------------------------------------------------------------------------------- */

const otext * OCI_API OCI_SubscriptionGetName
(
    OCI_Subscription *sub
)
{
    OCI_GET_PROP(const otext*, NULL, OCI_IPC_NOTIFY, sub, name, sub->con, NULL, sub->err)
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionGetPort
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_SubscriptionGetPort
(
    OCI_Subscription *sub
)
{
    OCI_GET_PROP(unsigned int, 0, OCI_IPC_NOTIFY, sub, port, sub->con, NULL, sub->err)
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionGetTimeout
 * --------------------------------------------------------------------------------------------- */

unsigned int OCI_API OCI_SubscriptionGetTimeout
(
    OCI_Subscription *sub
)
{
    OCI_GET_PROP(unsigned int, 0, OCI_IPC_NOTIFY, sub, timeout, sub->con, NULL, sub->err)
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_SubscriptionGetConnection
 * --------------------------------------------------------------------------------------------- */

OCI_Connection * OCI_API OCI_SubscriptionGetConnection
(
    OCI_Subscription *sub
)
{
    OCI_GET_PROP(OCI_Connection*, NULL, OCI_IPC_NOTIFY, sub, con, sub->con, NULL, sub->err)
}

