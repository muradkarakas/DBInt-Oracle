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

typedef struct MagicNumber
{
    unsigned char number[3];
    otext* name;
} MagicNumber;

static MagicNumber MagicNumbers[] =
{
    { { 2, 255, 101 }, OTEXT("~") },
    { { 1,   0,   0 }, OTEXT("-~") }
};

#define OCI_MAGIC_NUMBER_COUNT 2


#define OCI_NUMBER_OPERATION(func)                                                  \
                                                                                    \
    OCINumber src_num = { {0} };                                                    \
                                                                                    \
    OCI_CALL_ENTER(boolean, FALSE)                                                  \
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)                                      \
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)                                           \
                                                                                    \
    OCI_STATUS = OCI_NumberSetNativeValue(number->con, &src_num,                    \
                                           OCI_GetNumericTypeSize(type),            \
                                           type, SQLT_VNU, value);                  \
                                                                                    \
    OCI_EXEC(func(number->err, number->handle, &src_num, number->handle))           \
                                                                                    \
    OCI_RETVAL = OCI_STATUS;                                                        \
    OCI_CALL_EXIT()                                                                 \


/* --------------------------------------------------------------------------------------------- *
* OCI_GetNumericTypeSize
* --------------------------------------------------------------------------------------------- */

uword OCI_GetNumericTypeSize(unsigned int type)
{
    uword size = 0;

    if (type & OCI_NUM_SHORT)
    {
        size = sizeof(short);
    }
    else if (type & OCI_NUM_INT)
    {
        size = sizeof(int);
    }
    else if (type & OCI_NUM_BIGINT)
    {
        size = sizeof(big_int);
    }
    else if (type & OCI_NUM_FLOAT)
    {
        size = sizeof(float);
    }
    else if (type & OCI_NUM_DOUBLE)
    {
        size = sizeof(double);
    }
    return size;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberGetNativeValue
 * --------------------------------------------------------------------------------------------- */

boolean OCI_NumberGetNativeValue
(
    OCI_Connection *con,
    void           *number,
    uword           size,
    uword           type,
    int             sqlcode,
    void           *out_value
)
{
    OCI_CALL_DECLARE_CONTEXT(TRUE)
        
    OCI_CHECK(NULL == number, FALSE)
    OCI_CHECK(NULL == out_value, FALSE)

    OCI_CALL_CONTEXT_SET_FROM_CONN(con)

#if OCI_VERSION_COMPILE < OCI_10_1

    OCI_NOT_USED(sqlcode)

#endif

    if (OCI_NUM_NUMBER == type)
    {
        memcpy(out_value, number, size);
    }
    else if (type & OCI_NUM_DOUBLE || type & OCI_NUM_FLOAT)
    {

    #if OCI_VERSION_COMPILE >= OCI_10_1

        if ((OCILib.version_runtime >= OCI_10_1) && ((sqlcode != SQLT_VNU)))
        {
            if (((type & OCI_NUM_DOUBLE) && (SQLT_BDOUBLE == sqlcode)) ||
                ((type & OCI_NUM_FLOAT ) && (SQLT_BFLOAT  == sqlcode)))
            {
                memcpy(out_value, number, size);
            }
            else if (type & OCI_NUM_DOUBLE && (SQLT_BFLOAT == sqlcode))
            {
                *((double *) out_value) = (double) *((float *) number);
            }
            else if (type & OCI_NUM_FLOAT && (SQLT_BDOUBLE == sqlcode))
            {
                 *((float *) out_value) = (float) *((double *) number);
            }
        }
        else

    #endif

        {
            OCI_EXEC(OCINumberToReal(ctx->oci_err, (OCINumber *) number, size, out_value))
        }
    }
    else
    {
        uword sign = (type & OCI_NUM_UNSIGNED) ? OCI_NUMBER_UNSIGNED : OCI_NUMBER_SIGNED;

        OCI_EXEC(OCINumberToInt(ctx->oci_err, (OCINumber *) number, size, sign, out_value))
    }

    return OCI_STATUS;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberSetNativeValue
 * --------------------------------------------------------------------------------------------- */

boolean OCI_NumberSetNativeValue
(
    OCI_Connection *con,
    void           *number,
    uword           size,
    uword           type,
    int             sqlcode,
    void           *in_value
)
{
    OCI_CALL_DECLARE_CONTEXT(TRUE)
        
    OCI_CHECK(NULL == number, FALSE)
    OCI_CHECK(NULL == in_value, FALSE)

    OCI_CALL_CONTEXT_SET_FROM_CONN(con)

#if OCI_VERSION_COMPILE < OCI_10_1

    OCI_NOT_USED(sqlcode)

#endif

    if (type & OCI_NUM_NUMBER)
    {
        memcpy(number, in_value, sizeof(OCINumber));
    }
    else if (type & OCI_NUM_DOUBLE || type & OCI_NUM_FLOAT)
    {

    #if OCI_VERSION_COMPILE >= OCI_10_1

        if ((OCILib.version_runtime >= OCI_10_1) && ((sqlcode != SQLT_VNU)))
        {
            if (((type & OCI_NUM_DOUBLE) && (SQLT_BDOUBLE == sqlcode)) ||
                ((type & OCI_NUM_FLOAT ) && (SQLT_BFLOAT  == sqlcode)))
            {
                memcpy(number, in_value, size);
            }
            else if (type & OCI_NUM_DOUBLE && SQLT_BFLOAT == sqlcode)
            {
                *((double *) number) = (double) *((float *) in_value);
            }
            else if (type & OCI_NUM_FLOAT && SQLT_BDOUBLE == sqlcode)
            {
                 *((float *) number) = (float) *((double *) in_value);
            }
        }
        else

    #endif

        {
            OCI_EXEC(OCINumberFromReal(ctx->oci_err, in_value, size, (OCINumber *) number))
        }
    }
    else
    {
        uword sign = (type & OCI_NUM_UNSIGNED) ? OCI_NUMBER_UNSIGNED : OCI_NUMBER_SIGNED;

        OCI_EXEC(OCINumberFromInt(ctx->oci_err, in_value, size, sign, (OCINumber *)number))
    }

    return OCI_STATUS;
}


/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberFromString
 * --------------------------------------------------------------------------------------------- */

boolean OCI_NumberFromString
(
    OCI_Connection *con,
    void           *out_value,
    uword           size,
    uword           type,
    int             sqlcode,
    const otext    *in_value,
    const otext   * fmt
)
{
    boolean done = FALSE;

    OCI_CALL_DECLARE_CONTEXT(TRUE)
        
    OCI_CHECK(NULL == out_value, FALSE)
    OCI_CHECK(NULL == in_value, FALSE)

    OCI_CALL_CONTEXT_SET_FROM_CONN(con)

    /* For binary types, perform a C based conversion */

    if (type & OCI_NUM_DOUBLE || type & OCI_NUM_FLOAT || (SQLT_VNU != sqlcode))
    {

#if OCI_VERSION_COMPILE >= OCI_12_1

        if ((OCILib.version_runtime >= OCI_12_1) && ((SQLT_VNU != sqlcode)))
        {
            if (type & OCI_NUM_SHORT)
            {
                OCI_STATUS = (osscanf(in_value, OCI_STRING_FORMAT_NUM_SHORT, (short *)out_value) == 1);
                done = TRUE;
            }

            if (type & OCI_NUM_INT)
            {
                OCI_STATUS = (osscanf(in_value, OCI_STRING_FORMAT_NUM_INT, (int *)out_value) == 1);
                done = TRUE;
            }
        }

#endif

    #if OCI_VERSION_COMPILE >= OCI_10_1

        if (!done && OCILib.version_runtime >= OCI_10_1)
        {
            fmt = OCI_GetFormat(con, type & OCI_NUM_DOUBLE ? OCI_FMT_BINARY_DOUBLE : OCI_FMT_BINARY_FLOAT);

            if (type & OCI_NUM_DOUBLE)
            {
                OCI_STATUS = (osscanf(in_value, fmt, (double *)out_value) == 1);
            }
            else if (type & OCI_NUM_FLOAT)
            {
                OCI_STATUS = (osscanf(in_value, fmt, (float *)out_value) == 1);
            }

            done = TRUE;
        }

    #endif

    }

    /* use OCINumber conversion if not processed yet */

    if (!done)
    {
        int i;

        for (i = 0; i < OCI_MAGIC_NUMBER_COUNT; i++)
        {
            MagicNumber *mag_num = &MagicNumbers[i];

            if (ostrcmp(in_value, mag_num->name) == 0)
            {
                memset(out_value, 0, sizeof(OCINumber));
                memcpy(out_value, mag_num->number, sizeof(mag_num->number));
                OCI_STATUS = done = TRUE;
                break;
            }
        }

        if (!done)
        {
            dbtext *dbstr1  = NULL;
            dbtext *dbstr2  = NULL;
            int     dbsize1 = -1;
            int     dbsize2 = -1;
            OCINumber number;

            if (!fmt)
            {
                fmt = OCI_GetFormat(con, OCI_FMT_NUMERIC);
            }

            dbstr1 = OCI_StringGetOracleString(in_value, &dbsize1);
            dbstr2 = OCI_StringGetOracleString(fmt, &dbsize2);

            memset(&number, 0, sizeof(number));

            OCI_EXEC
            (
                OCINumberFromText(ctx->oci_err, (oratext *) dbstr1, (ub4) dbsize1, (oratext *) dbstr2,
                                   (ub4) dbsize2, (oratext *) NULL,  (ub4) 0, (OCINumber *) &number)
            )

            OCI_StringReleaseOracleString(dbstr2);
            OCI_StringReleaseOracleString(dbstr1);

            OCI_STATUS = OCI_STATUS && OCI_NumberGetNativeValue(con, (void *)&number, size, type, sqlcode, out_value);
        }
    }

    return OCI_STATUS;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberToString
 * --------------------------------------------------------------------------------------------- */

boolean OCI_NumberToString
(
    OCI_Connection *con,
    void           *number,
    unsigned int    type,
    int             sqlcode,
    otext          *out_value,
    int             out_value_size,
    const otext   * fmt
)
{
    boolean   done = FALSE;

    OCI_CALL_DECLARE_CONTEXT(TRUE)
        
    OCI_CHECK(NULL == out_value, FALSE)
    OCI_CHECK(NULL == number, FALSE)

    OCI_CALL_CONTEXT_SET_FROM_CONN(con)

    out_value[0] = 0;

    /* For binary types, perform a C based conversion */

    if (type & OCI_NUM_DOUBLE || type & OCI_NUM_FLOAT || (SQLT_VNU != sqlcode))
    {

    #if OCI_VERSION_COMPILE >= OCI_12_1

        if ((OCILib.version_runtime >= OCI_12_1) && ((SQLT_VNU != sqlcode)))
        {
            if (type & OCI_NUM_SHORT)
            {
                out_value_size = osprintf(out_value, out_value_size, OCI_STRING_FORMAT_NUM_SHORT, *((short *)number));
                done = TRUE;
            }

            if (type & OCI_NUM_INT)
            {
                out_value_size = osprintf(out_value, out_value_size, OCI_STRING_FORMAT_NUM_INT, *((int *)number));
                done = TRUE;
            }
        }

    #endif

    #if OCI_VERSION_COMPILE >= OCI_10_1

        if (!done && (OCILib.version_runtime >= OCI_10_1) && ((SQLT_VNU != sqlcode)))
        {
            if (!fmt)
            {
                fmt = OCI_GetFormat(con, type & OCI_NUM_DOUBLE ? OCI_FMT_BINARY_DOUBLE : OCI_FMT_BINARY_FLOAT);
            }

            if (type & OCI_NUM_DOUBLE && (SQLT_BDOUBLE == sqlcode))
            {
                out_value_size = osprintf(out_value, out_value_size, fmt, *((double *)number));
            }
            else if (type & OCI_NUM_FLOAT && (SQLT_BFLOAT == sqlcode))
            {
                out_value_size = osprintf(out_value, out_value_size, fmt, *((float *)number));
            }

            done = TRUE;

            if ((out_value_size) > 0)
            {
                while (out_value[out_value_size-1] == OTEXT('0'))
                {
                    out_value[out_value_size-1] = 0;
                }

                out_value--;
            }
        }

    #else

        OCI_NOT_USED(sqlcode)

    #endif

    }

    /* use OCINumber conversion if not processed yet */

    if (!done)
    {
        int i;

        for (i = 0; i < OCI_MAGIC_NUMBER_COUNT; i++)
        {
            MagicNumber *mag_num = &MagicNumbers[i];

            if (memcmp(number, mag_num->number, sizeof(mag_num->number)) == 0)
            {
                ostrcpy(out_value, mag_num->name);
                OCI_STATUS = done = TRUE;
                break;
            }
        }

        if (!done)
        {
            dbtext *dbstr1 = NULL;
            dbtext *dbstr2 = NULL;
            int     dbsize1 = out_value_size * (int) sizeof(otext);
            int     dbsize2 = -1;

            if (!fmt)
            {
                fmt = OCI_GetFormat(con, OCI_FMT_NUMERIC);
            }

            dbstr1 = OCI_StringGetOracleString(out_value, &dbsize1);
            dbstr2 = OCI_StringGetOracleString(fmt, &dbsize2);

            OCI_EXEC
            (
                OCINumberToText(ctx->oci_err, (OCINumber *)number, (oratext *)dbstr2,
                                (ub4)dbsize2, (oratext *)NULL, (ub4)0,
                                (ub4 *)&dbsize1, (oratext *)dbstr1)
            )

            OCI_StringCopyOracleStringToNativeString(dbstr1, out_value, dbcharcount(dbsize1));
            OCI_StringReleaseOracleString(dbstr2);
            OCI_StringReleaseOracleString(dbstr1);

            out_value_size = (dbsize1 / (int) sizeof(dbtext));
        }
    }

    /* do we need to suppress last '.' or ',' from integers */

    if ((--out_value_size) >= 0)
    {
        if ((out_value[out_value_size] == OTEXT('.')) ||
            (out_value[out_value_size] == OTEXT(',')))
        {
            out_value[out_value_size] = 0;
        }
    }

    return OCI_STATUS;
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberInit
 * --------------------------------------------------------------------------------------------- */

OCI_Number * OCI_NumberInit
(
    OCI_Connection  *con,
    OCI_Number      *number,
    OCINumber       *buffer
)
{
    OCI_CALL_DECLARE_CONTEXT(TRUE)
    OCI_CALL_CONTEXT_SET_FROM_CONN(con)

    OCI_ALLOCATE_DATA(OCI_IPC_NUMBER, number, 1);

    if (OCI_STATUS)
    {
        number->con = con;
        number->handle = buffer;

        /* get the right error handle */

        number->err = con ? con->err : OCILib.err;
        number->env = con ? con->env : OCILib.env;

        /* allocate buffer if needed */

        if (!number->handle || (OCI_OBJECT_ALLOCATED_ARRAY == number->hstate))
        {
            if (OCI_OBJECT_ALLOCATED_ARRAY != number->hstate)
            {
                number->hstate = OCI_OBJECT_ALLOCATED;
                OCI_ALLOCATE_DATA(OCI_IPC_ARRAY, number->handle, 1)
            }
        }
        else
        {
            number->hstate = OCI_OBJECT_FETCHED_CLEAN;
        }
    }

    /* check for failure */

    if (!OCI_STATUS && number)
    {
        OCI_NumberFree(number);
        number = NULL;
    }

    return number;
}

/* ********************************************************************************************* *
 *                            PUBLIC FUNCTIONS
 * ********************************************************************************************* */

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberCreate
 * --------------------------------------------------------------------------------------------- */

OCI_Number * OCI_API OCI_NumberCreate
(
    OCI_Connection *con
)
{
    OCI_CALL_ENTER(OCI_Number*, NULL)
    OCI_CALL_CHECK_INITIALIZED()
    OCI_CALL_CONTEXT_SET_FROM_CONN(con)

    OCI_RETVAL = OCI_NumberInit(con, NULL, NULL);
    OCI_STATUS = (NULL != OCI_RETVAL);

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberAssign
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberFree
(
    OCI_Number *number
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CHECK_OBJECT_FETCHED(number);
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    if (OCI_OBJECT_ALLOCATED == number->hstate)
    {
        OCI_FREE(number->handle)
    }

    if (OCI_OBJECT_ALLOCATED_ARRAY != number->hstate)
    {
        OCI_FREE(number)
    }

    OCI_RETVAL = OCI_STATUS;

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberArrayCreate
 * --------------------------------------------------------------------------------------------- */

OCI_Number ** OCI_API OCI_NumberArrayCreate
(
    OCI_Connection *con,
    unsigned int    nbelem
)
{
    OCI_Array *arr = NULL;

    OCI_CALL_ENTER(OCI_Number **, NULL)
    OCI_CALL_CHECK_INITIALIZED()

    arr = OCI_ArrayCreate(con, nbelem, OCI_CDT_NUMERIC, OCI_NUM_NUMBER, sizeof(OCINumber), sizeof(OCI_Number), 0, NULL);
    OCI_STATUS = (NULL != arr);

    if (OCI_STATUS)
    {
        OCI_RETVAL = (OCI_Number **)arr->tab_obj;
    }

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberArrayFree
 * --------------------------------------------------------------------------------------------- */

OCI_EXPORT boolean OCI_API OCI_NumberArrayFree
(
    OCI_Number **numbmers
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_ARRAY, numbmers)

    OCI_RETVAL = OCI_STATUS = OCI_ArrayFreeFromHandles((void **)numbmers);

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberAssign
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberAssign
(
    OCI_Number *number,
    OCI_Number *number_src
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number_src)
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    OCI_EXEC(OCINumberAssign(number->err, number_src->handle, number->handle))

    OCI_RETVAL = OCI_STATUS;

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberToText
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberToText
(
    OCI_Number  *number,
    const otext *fmt,
    int          size,
    otext       *str
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    OCI_RETVAL = OCI_NumberToString(number->con, number->handle, OCI_NUM_NUMBER, SQLT_VNU, str, size, fmt);

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberFromText
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberFromText
(
    OCI_Number  *number,
    const otext *str,
    const otext *fmt
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    OCI_RETVAL = OCI_NumberFromString(number->con, number->handle, sizeof(OCINumber), OCI_NUM_NUMBER, SQLT_VNU, str, fmt);

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberGetContent
 * --------------------------------------------------------------------------------------------- */

unsigned char * OCI_API OCI_NumberGetContent
(
    OCI_Number *number
)
{
    OCI_CALL_ENTER(unsigned char *, NULL)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    if (number->handle)
    {
        OCI_RETVAL = number->handle->OCINumberPart;
    }

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberSetContent
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberSetContent
(
    OCI_Number     *number,
    unsigned char  *content
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CHECK_PTR(OCI_IPC_VOID, content)
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    if (number->handle)
    {
        memcpy(number->handle->OCINumberPart, content, sizeof(number->handle->OCINumberPart));
    }

    OCI_RETVAL = OCI_STATUS;

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberSetValue
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberSetValue
(
    OCI_Number     *number,
    unsigned int    type,
    void           *value
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    OCI_RETVAL = OCI_STATUS = OCI_NumberSetNativeValue(number->con, number->handle,
                                                       OCI_GetNumericTypeSize(type),
                                                       type, SQLT_VNU, value);

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberGetValue
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberGetValue
(
    OCI_Number     *number,
    unsigned int    type,
    void           *value
)
{
    OCI_CALL_ENTER(boolean, FALSE)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number)
    OCI_CALL_CONTEXT_SET_FROM_OBJ(number)

    OCI_RETVAL = OCI_STATUS = OCI_NumberGetNativeValue(number->con, number->handle,
                                                       OCI_GetNumericTypeSize(type),
                                                       type, SQLT_VNU, value);

    OCI_CALL_EXIT()
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberAdd
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberAdd
(
    OCI_Number     *number,
    unsigned int    type,
    void           *value
)
{
    OCI_NUMBER_OPERATION(OCINumberAdd)
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberSub
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberSub
(
    OCI_Number     *number,
    unsigned int    type,
    void           *value
)
{
    OCI_NUMBER_OPERATION(OCINumberSub)
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberMultiply
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberMultiply
(
    OCI_Number     *number,
    unsigned int    type,
    void           *value
)
{
    OCI_NUMBER_OPERATION(OCINumberMul)
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberDivide
 * --------------------------------------------------------------------------------------------- */

boolean OCI_API OCI_NumberDivide
(
    OCI_Number     *number,
    unsigned int    type,
    void           *value
)
{
    OCI_NUMBER_OPERATION(OCINumberDiv)
}

/* --------------------------------------------------------------------------------------------- *
 * OCI_NumberCompare
 * --------------------------------------------------------------------------------------------- */

int OCI_API OCI_NumberCompare
(
    OCI_Number *number1,
    OCI_Number *number2
)
{
    sword value = OCI_ERROR;

    OCI_CALL_ENTER(int, value)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number1)
    OCI_CALL_CHECK_PTR(OCI_IPC_NUMBER, number2)
    OCI_CALL_CONTEXT_SET_FROM_CONN(number1->con)

    OCI_EXEC(OCINumberCmp(number1->err, number1->handle, number1->handle, &value))

    OCI_RETVAL = (int) value;

    OCI_CALL_EXIT()
}



