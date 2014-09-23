/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

<<<<<<< HEAD
/**
 * @author ptinius
 */
public class CreateCloneSnapshot
        extends OMRestBase {
  private static final Logger LOG =
          Logger.getLogger(CreateCloneSnapshot.class);

  /**
   * @param config the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  public CreateCloneSnapshot(final ConfigurationServiceCache config) {
    super(config);
  }

  /**
   * @param request         the {@link Request}
   * @param routeParameters the {@link Map} of route parameters
   * @return Returns the {@link Resource}
   * @throws Exception any unhandled error
   */
  @Override
  public Resource handle(final Request request,
                         final Map<String, String> routeParameters)
          throws Exception {
    final ObjectMapper mapper = new ObjectMapper();
    final Status status = ObjectFactory.createStatus();
    status.setStatus(HttpResponseStatus.NOT_IMPLEMENTED.reasonPhrase());
    status.setCode(HttpResponseStatus.NOT_IMPLEMENTED.code());

    return new JsonResource(new JSONObject(mapper.writeValueAsString(status)));
  }
=======
public class CreateCloneSnapshot implements RequestHandler {

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
            throws Exception {
        return new JsonResource(new JSONObject().put("status", "NOT_IMPLEMENTED"));
    }
>>>>>>> dev
}
