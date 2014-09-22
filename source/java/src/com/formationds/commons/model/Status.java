/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;
import io.netty.handler.codec.http.HttpResponseStatus;

import javax.xml.bind.annotation.XmlRootElement;

/**
 * @author ptinius
 */
@XmlRootElement
public class Status extends ModelBase {
    private static final long serialVersionUID = -1;

    private String status = null;
    private int code = 0;

    /**
     * default constructor for the Status Model object
     */
    private Status() {
        super();
    }

    public Status(HttpResponseStatus status) {
        this.status = status.reasonPhrase();
        this.code = status.code();
    }

    /**
     * @return Returns a {@link String} representing the status response
     */
    public String getStatus() {
        return status;
    }

    /**
     * @param status the {@link String} representing the status response
     */
    public void setStatus(final String status) {
        this.status = status;
    }

    /**
     * @return Returns {@code int} representing the HTTP/1.1 static code
     */
    public int getCode() {
        return code;
    }

    /**
     * @param code the {@code int} representing the HTTP/1.1 static code
     */
    public void setCode(final int code) {
        this.code = code;
    }
}
