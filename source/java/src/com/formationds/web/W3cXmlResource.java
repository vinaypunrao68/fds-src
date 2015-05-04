/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.web;

import com.formationds.web.toolkit.Resource;

import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import java.io.IOException;
import java.io.OutputStream;

public class W3cXmlResource implements Resource {
    org.w3c.dom.Document document;

    public W3cXmlResource( org.w3c.dom.Document document ) {
        this.document = document;
    }
    @Override
    public String getContentType() {
        return "application/xml; charset=utf-8";
    }

    @Override
    public void render(OutputStream outputStream) throws IOException {
        try {
            // Use a Transformer for output
            TransformerFactory tFactory =
                TransformerFactory.newInstance();
            Transformer transformer =
                tFactory.newTransformer();

            DOMSource source = new DOMSource(document);
            StreamResult result = new StreamResult(outputStream);
            transformer.setOutputProperty( OutputKeys.INDENT, "yes" );
            transformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "2");
            transformer.transform(source, result);

            return;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
