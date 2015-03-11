package com.formationds.smoketest;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.Action;

import java.util.concurrent.ExecutionException;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

public abstract class BaseAmTest {

    public void assertFdsError(ErrorCode errorCode, Action action) throws Exception {
        try {
            action.execute();
        } catch (ApiException e) {
            assertEquals(errorCode, e.getErrorCode());
            return;
        } catch (ExecutionException e) {
            Throwable cause = e.getCause();
            if (!(cause instanceof ApiException)) {
                fail("Should have gotten an ApiException");
            }
            ApiException ex = (ApiException) cause;
            assertEquals(errorCode, ex.getErrorCode());
            return;
        }

        fail("Should have gotten an ApiException");
    }
}
